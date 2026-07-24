/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for Sound Blaster 16 on PC98 (CT2720)
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * Design:
 *   - 44.1kHz 16-bit signed stereo (SB16_S16_44K_STEREO) or
 *     8kHz 8-bit signed monaural (SB16_S8_8K_MONO) PCM
 *   - Auto-initialize DMA
 *   - Double buffer
 *   - SB16 raises an IRQ each time it finishes one half of the DMA
 *     buffer.
 *   - The ISR only flips flags and acknowledges the interrupt.
 *   - Actual decoding/mixing (hal_get_wave_samples) is done in the main
 *     thread context by sb16_sound_poll(), which must be called once per
 *     frame from the main loop.
 *   - This avoids calling non-reentrant DOS services (file I/O in the Ogg
 *   - Vorbis decoder etc.) from interrupt context.
 *   - If the main loop misses a refill deadline, the ISR fills the stale
 *     half with silence so that auto-init DMA repeats silence, not noise.
 *
 * Hardware notes (Sound Blaster 16 for PC-9800, CT2720):
 *   - I/O mapping differs from PC/AT: register offset goes to the HIGH
 *     byte, the board base (D2h/D4h/.../DEh, jumper JP9) goes to the LOW
 *     byte.  e.g. AT 2x6h (DSP reset) -> 98 26D2h.
 *   - Board defaults: I/O base D2h, DMA ch3, IRQ5 (PC-98 INT1).
 *   - The current jumper setting can be read back from mixer registers
 *     80h (IRQ) and 81h (DMA), which this driver uses for auto-detection.
 *   - PC-98 has no 16-bit DMA channels; 16-bit PCM is transferred over
 *     the byte-wide DMA channel (ch0 or ch3) of the on-board uPD71037
 *     (i8237A compatible) DMA controller.  The board pairs the bytes.
 */

/* Base */
#include <strato/strato.h>

/* Standard C */
#include <math.h>
#include <string.h>
#include <assert.h>

/* DOS (OpenWatcom) */
#include <dos.h>
#include <conio.h>
#include <i86.h>

/*
 * Config Select
 */

#define SB16_S16_8K_MONO	/* Here! */
#undef  SB16_S16_44K_STEREO
#undef  SB16_S8_8K_MONO

/*
 * Format
 */

#ifdef SB16_S16_44K_STEREO
#define SAMPLING_RATE	(44100)		/* 44100Hz */
#define CHANNELS	(2)		/* 2ch */
#define FRAME_SIZE	(4)		/* 16-bit x 2ch */
#define HALF_FRAMES	(2048)		/* 46ms per half */
#endif

#ifdef SB16_S16_8K_MONO
#define SAMPLING_RATE	(8000)		/* 8000Hz */
#define CHANNELS	(1)		/* 1ch */
#define FRAME_SIZE	(2)		/* 16-bit x 1ch */
#define HALF_FRAMES	(8192)		/* 184ms per half */
#endif

#ifdef SB16_S8_8K_MONO
#define SAMPLING_RATE	(8000)		/* 8000Hz */
#define CHANNELS	(1)		/* 1ch */
#define FRAME_SIZE	(1)		/* 8-bit x 1ch */
#define HALF_FRAMES	(4096)		/* 512ms per half */
#endif

/*
 * Sound Buffer Config
 *
 * The DMA buffer is split into two halves.  The DSP interrupts at the
 * end of each half (block size = half).  HALF_FRAMES is defined per
 * format above so that one half is roughly 50ms; sb16_sound_poll()
 * must be called at least that often.  Increase HALF_FRAMES if the
 * game loop can be slower than that.
 */

#define HALF_BYTES	(HALF_FRAMES * FRAME_SIZE)
#define BUF_BYTES	(HALF_BYTES * 2)

/*
 * The DSP block length parameter (minus one) follows the SB16
 * convention: 16-bit commands (0xB6) count 16-bit samples, 8-bit
 * commands (0xC6) count 8-bit samples, i.e. bytes.
 */

#ifdef SB16_S16_44K_STEREO
#define DSP_BLOCK_LEN	(HALF_FRAMES * 2 - 1)	/* 16-bit samples - 1 */
#endif

#ifdef SB16_S16_8K_MONO
#define DSP_BLOCK_LEN	(HALF_FRAMES - 1)	/* 16-bit samples - 1 */
#endif

#ifdef SB16_S8_8K_MONO
#define DSP_BLOCK_LEN	(HALF_BYTES - 1)	/* bytes - 1 */
#endif

/*
 * SB16/98 I/O Ports
 *
 * The base (low byte) is D2h by default and can be changed with JP9.
 * Define SB16_BASE at compile time to override (D2h/D4h/D6h/.../DEh).
 */
#if !defined(SB16_BASE)
#define SB16_BASE	0xd2
#endif
#define SB_PORT(ofs)	(((ofs) << 8) | SB16_BASE)
#define P_MIXER_ADDR	SB_PORT(0x24)	/* AT 2x4h: Mixer index */
#define P_MIXER_DATA	SB_PORT(0x25)	/* AT 2x5h: Mixer data */
#define P_DSP_RESET	SB_PORT(0x26)	/* AT 2x6h: DSP reset */
#define P_DSP_READ	SB_PORT(0x2a)	/* AT 2xAh: DSP read data */
#define P_DSP_WRITE	SB_PORT(0x2c)	/* AT 2xCh: DSP write cmd/data, read: bit7=busy */
#define P_DSP_RSTAT	SB_PORT(0x2e)	/* AT 2xEh: read status (bit7), 8-bit IRQ ack */
#define P_DSP_ACK16	SB_PORT(0x2f)	/* AT 2xFh: 16-bit IRQ ack */

/* DSP commands */
#define DSP_SET_OUTPUT_RATE	0x41
#define DSP_SPEAKER_ON		0xd1
#define DSP_PLAY_16BIT_AUTO	0xb6	/* 16-bit output, auto-init, FIFO */
#define DSP_PLAY_8BIT_AUTO	0xc6	/* 8-bit output, auto-init, FIFO */
#define DSP_MODE_U_MONO		0x00	/* unsigned, mono */
#define DSP_MODE_S_MONO		0x10	/* signed, mono */
#define DSP_MODE_U_STEREO	0x20	/* unsigned, stereo */
#define DSP_MODE_S_STEREO	0x30	/* signed, stereo */
#define DSP_EXIT_16BIT_AUTO	0xd9
#define DSP_EXIT_8BIT_AUTO	0xda
#define DSP_PAUSE_16BIT		0xd5
#define DSP_PAUSE_8BIT		0xd0

/* Mixer (CT1745) registers */
#define MIX_RESET		0x00
#define MIX_MASTER_L		0x30
#define MIX_MASTER_R		0x31
#define MIX_VOICE_L		0x32
#define MIX_VOICE_R		0x33
#define MIX_IRQ_SELECT		0x80
#define MIX_DMA_SELECT		0x81
#define MIX_IRQ_STATUS		0x82

/*
 * PC-98 DMA Controller (uPD71037/i8237A compatible)
 *
 * Per-channel registers (odd port addresses):
 *   ch:      0     1     2     3
 *   addr:   01h   05h   09h   0Dh   (low/high via internal flip-flop)
 *   count:  03h   07h   0Bh   0Fh   (byte count minus one)
 *   bank:   27h   21h   23h   25h   (address bits A16-A23, PC-98 specific)
 */
#define DMA_PORT_SMASK	0x15		/* single mask */
#define DMA_PORT_MODE	0x17		/* mode */
#define DMA_PORT_CLRFF	0x19		/* clear byte pointer flip-flop */
static const int dma_port_addr[4]  = { 0x01, 0x05, 0x09, 0x0d };
static const int dma_port_count[4] = { 0x03, 0x07, 0x0b, 0x0f };
static const int dma_port_bank[4]  = { 0x27, 0x21, 0x23, 0x25 };

/*
 * PC-98 Interrupt Controller (i8259 x2)
 *
 * Master: 00h/02h, Slave: 08h/0Ah, slave cascades into master IR7.
 * IRQ0-7 -> vector 08h-0Fh, IRQ8-15 -> vector 10h-17h.
 */
#define PIC0_CMD	0x00
#define PIC0_IMR	0x02
#define PIC1_CMD	0x08
#define PIC1_IMR	0x0a
#define PIC_EOI		0x20

/* PC-98 0.6us wait port. */
#define WAIT_PORT	0x5f

/*
 * Driver State
 */

/* True if the board was detected and initialized. */
static bool sb16_ok;

/* Detected configuration. */
static int sb_irq;		/* PC-98 IRQ number (3/5/10/12) */
static int sb_vec;		/* CPU vector */
static int sb_dma;		/* PC-98 DMA channel (0 or 3) */
static uint8_t sb_dma_sel;	/* raw mixer 81h value (for 82h ack decision) */

/* DMA buffer (DOS memory below 1MB, does not cross a 64KB boundary). */
static uint8_t *dma_buf;	/* linear == physical under DOS/4GW */
static uint32_t dma_phys;
static uint16_t dos_selector;	/* for freeing the DOS block */

/* Double buffer bookkeeping (shared with the ISR). */
static volatile int cur_half;		/* half the DSP is playing now */
static volatile int fill_half;		/* half waiting to be refilled */
static volatile int fill_pending;	/* 1 if fill_half needs a refill */

/* Old interrupt vector and old PIC mask bit. */
#if defined(__WATCOMC__)
static void (__interrupt __far *old_isr)(void);
#endif
static int old_imr_masked;

/* Input Streams */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Volume Values (Q15 fixed point, 0..32767) */
static int volume_q15[HAL_SOUND_TRACKS];

/* Finish Flags */
static volatile bool finish[HAL_SOUND_TRACKS];

/* Mixing Buffers */
static int32_t mix_buf[HALF_FRAMES * 2];
static uint32_t pull_buf[HALF_FRAMES];

/*
 * Forward Declarations
 */
static bool dsp_reset(void);
static bool dsp_write(int val);
static int dsp_read(void);
static void mixer_write(int reg, int val);
static int mixer_read(int reg);
static bool detect_config(void);
static bool alloc_dma_buffer(void);
static void free_dma_buffer(void);
static void setup_dma(void);
static void stop_dma(void);
static void start_playback(void);
static void hook_irq(void);
static void unhook_irq(void);
static void fill_half_buffer(int half);
static void dpmi_lock_region(void *p, uint32_t size);
#if defined(__WATCOMC__)
static void __interrupt __far sb16_isr(void);
#endif

/*
 * Initialize the Sound Blaster 16/98.
 */
bool
sb16_init_sound(void)
{
	int n;

	sb16_ok = false;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		wave[n] = NULL;
		volume_q15[n] = 32767;
		finish[n] = false;
	}

	/* Reset and detect the DSP. */
	if (!dsp_reset())
		return false;

	hal_log_info("SB16: found a card.");

	/* Read the IRQ/DMA jumper settings back from the mixer. */
	if (!detect_config()) {
		hal_log_info("SB16: unsupported IRQ/DMA configuration.");
		return false;
	}

	/* Allocate a DMA buffer in DOS memory. */
	if (!alloc_dma_buffer()) {
		hal_log_info("SB16: failed to allocate a DMA buffer.");
		return false;
	}
	memset(dma_buf, 0, BUF_BYTES);

	/* Lock the ISR-touched memory so it is never paged out. */
	dpmi_lock_region((void *)dma_buf, BUF_BYTES);
	dpmi_lock_region((void *)&cur_half, 4096);

	/* Unmute and set the mixer volumes. */
	mixer_write(MIX_RESET, 0);
	mixer_write(MIX_MASTER_L, 0xf8);
	mixer_write(MIX_MASTER_R, 0xf8);
	mixer_write(MIX_VOICE_L, 0xf8);
	mixer_write(MIX_VOICE_R, 0xf8);

	/* Install the interrupt handler. */
	hook_irq();

	/* Program the DMA controller (auto-init, whole buffer). */
	setup_dma();

	/* Program the DSP and start the transfer. */
	start_playback();

	sb16_ok = true;
	return true;
}

/*
 * Cleanup the Sound Blaster 16/98.
 */
void
sb16_cleanup_sound(void)
{
	int n;

	if (!sb16_ok)
		return;

	sb16_ok = false;

	for (n = 0; n < HAL_SOUND_TRACKS; n++)
		wave[n] = NULL;

	/* Stop the DSP transfer. */
#ifdef SB16_S16_44K_STEREO
	dsp_write(DSP_EXIT_16BIT_AUTO);
	dsp_write(DSP_PAUSE_16BIT);
#endif
#ifdef SB16_S16_8K_MONO
	dsp_write(DSP_EXIT_16BIT_AUTO);
	dsp_write(DSP_PAUSE_16BIT);
#endif
#ifdef SB16_S8_8K_MONO
	dsp_write(DSP_EXIT_8BIT_AUTO);
	dsp_write(DSP_PAUSE_8BIT);
#endif
	dsp_reset();

	/* Stop the DMA channel. */
	stop_dma();

	/* Restore the interrupt vector and the PIC mask. */
	unhook_irq();

	/* Free the DMA buffer. */
	free_dma_buffer();
}

/*
 * Pump the sound: decode and mix into the DMA buffer.
 *
 * [IMPORTANT]
 *  - Call this once per frame from the main loop.
 *  - hal_get_wave_samples() is called here, in the main thread context,
 *    because it may perform DOS file I/O which must never happen inside
 *    an interrupt handler.
 */
void
sb16_sound_poll(void)
{
	int half;

	if (!sb16_ok)
		return;

	if (!fill_pending)
		return;

	_disable();
	half = fill_half;
	fill_pending = 0;
	_enable();

	fill_half_buffer(half);
}

/*
 * Start sound playback on a stream.
 */
bool
sb16_play_sound(
	int n,
	struct hal_wave *w)
{
	assert(n < HAL_SOUND_TRACKS);
	assert(w != NULL);

	if (!sb16_ok)
		return true;

	_disable();
	{
		wave[n] = w;
		finish[n] = false;
	}
	_enable();

	/* Refill as early as possible. */
	sb16_sound_poll();

	return true;
}

/*
 * Stop sound playback on a stream.
 */
bool
sb16_stop_sound(
	int n)
{
	assert(n < HAL_SOUND_TRACKS);

	if (!sb16_ok)
		return true;

	_disable();
	{
		wave[n] = NULL;
	}
	_enable();

	return true;
}

/*
 * Set a sound volume for a stream.
 */
bool
sb16_set_sound_volume(
	int n,
	float vol)
{
	double scale;

	assert(n < HAL_SOUND_TRACKS);
	assert(vol >= 0 && vol <= 1.0f);

	/* Convert a scale factor to an exponential value. (Same curve as the ALSA HAL.) */
	scale = (pow(10.0, (double)vol) - 1.0) / (10.0 - 1.0);

	volume_q15[n] = (int)(scale * 32767.0);
	if (volume_q15[n] > 32767)
		volume_q15[n] = 32767;
	if (volume_q15[n] < 0)
		volume_q15[n] = 0;

	return true;
}

/*
 * Check if a sound stream is finished.
 */
bool
sb16_is_sound_finished(
	int n)
{
	if (!sb16_ok)
		return true;

	if (!finish[n])
		return false;

	return true;
}

/*
 * DSP Access
 */

/* Reset the DSP and check for the 0xAA response. */
static bool
dsp_reset(void)
{
	int i;

	outp(P_DSP_RESET, 1);
	for (i = 0; i < 8; i++)
		outp(WAIT_PORT, 0);	/* > 3us */
	outp(P_DSP_RESET, 0);

	/* Wait for data available (up to ~60ms). */
	for (i = 0; i < 100000; i++) {
		if (inp(P_DSP_RSTAT) & 0x80) {
			if (inp(P_DSP_READ) == 0xaa)
				return true;
		}
		outp(WAIT_PORT, 0);
	}

	return false;
}

/* Write a command/data byte to the DSP. */
static bool
dsp_write(int val)
{
	long i;

	for (i = 0; i < 1000000L; i++) {
		if (!(inp(P_DSP_WRITE) & 0x80)) {
			outp(P_DSP_WRITE, val);
			return true;
		}
	}
	return false;
}

#if 0
/* Read a data byte from the DSP. (-1 on timeout) */
static int
dsp_read(void)
{
	long i;

	for (i = 0; i < 1000000L; i++) {
		if (inp(P_DSP_RSTAT) & 0x80)
			return inp(P_DSP_READ);
	}
	return -1;
}
#endif

/*
 * Mixer Access
 */

static void
mixer_write(int reg, int val)
{
	outp(P_MIXER_ADDR, reg);
	outp(WAIT_PORT, 0);
	outp(P_MIXER_DATA, val);
	outp(WAIT_PORT, 0);
}

static int
mixer_read(int reg)
{
	outp(P_MIXER_ADDR, reg);
	outp(WAIT_PORT, 0);
	return inp(P_MIXER_DATA);
}

/*
 * Configuration Detection
 *
 * Mixer register 80h reflects the IRQ jumper:
 *   01h -> IRQ3 (INT0), 08h -> IRQ5 (INT1),
 *   02h -> IRQ10 (INT41), 04h -> IRQ12 (INT5)
 * Mixer register 81h reflects the DMA jumper:
 *   01h/20h -> DMA ch0, 02h/40h -> DMA ch3
 */
static bool
detect_config(void)
{
	int v;

	v = mixer_read(MIX_IRQ_SELECT) & 0x0f;
	switch (v) {
	case 0x01: sb_irq = 3;  break;
	case 0x08: sb_irq = 5;  break;
	case 0x02: sb_irq = 10; break;
	case 0x04: sb_irq = 12; break;
	default:   sb_irq = 5;  break;	/* Assume the board default. */
	}
	sb_vec = (sb_irq < 8) ? (0x08 + sb_irq) : (0x10 + (sb_irq - 8));

	v = mixer_read(MIX_DMA_SELECT);
	sb_dma_sel = (uint8_t)v;
	if (v & (0x02 | 0x40))
		sb_dma = 3;
	else if (v & (0x01 | 0x20))
		sb_dma = 0;
	else {
		sb_dma = 3;		/* Assume the board default. */
		sb_dma_sel = 0x02;
	}

	/*
	 * Select the 8-bit style DMA programming (low channel bit) so that
	 * the block runs over the single byte-wide PC-98 DMA channel.
	 */
	mixer_write(MIX_DMA_SELECT, (sb_dma == 3) ? 0x02 : 0x01);
	sb_dma_sel = (uint8_t)mixer_read(MIX_DMA_SELECT);

	return true;
}

/*
 * DMA Buffer Allocation
 *
 * DPMI 0100h allocates DOS (conventional) memory, which is below 1MB
 * and identity-mapped under DOS/4GW, so linear == physical.  We
 * allocate twice the needed size and align so the buffer never
 * crosses a 64KB physical boundary (the PC-98 DMAC bank register
 * does not auto-increment across it).
 */
static bool
alloc_dma_buffer(void)
{
	union REGS r;
	uint32_t base, start;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0100;
	r.w.bx = (BUF_BYTES * 2 + 15) / 16;	/* paragraphs */
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return false;

	dos_selector = r.w.dx;
	base = (uint32_t)r.w.ax << 4;

	start = base;
	if ((start & 0xffff) + BUF_BYTES > 0x10000)
		start = (start + 0xffff) & 0xffff0000UL;

	dma_phys = start;
	dma_buf = (uint8_t *)start;	/* zero-based flat address space */

	return true;
}

static void
free_dma_buffer(void)
{
	union REGS r;

	if (dma_buf == NULL)
		return;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0101;
	r.w.dx = dos_selector;
	int386(0x31, &r, &r);

	dma_buf = NULL;
}

/* DMA Controller Setup (auto-initialize, memory -> device) */
static void
setup_dma(void)
{
	int ch = sb_dma;

	_disable();

	/* Mask the channel. */
	outp(DMA_PORT_SMASK, 0x04 | ch);

	/* Mode: single transfer, address increment, auto-init, read (mem->dev). */
	outp(DMA_PORT_MODE, 0x58 | ch);

	/* Clear the byte pointer flip-flop. */
	outp(DMA_PORT_CLRFF, 0);

	/* Address (A0-A15). */
	outp(dma_port_addr[ch], (int)(dma_phys & 0xff));
	outp(dma_port_addr[ch], (int)((dma_phys >> 8) & 0xff));

	/* Bank (A16-A23). */
	outp(dma_port_bank[ch], (int)((dma_phys >> 16) & 0xff));

	/* Count (bytes - 1). */
	outp(DMA_PORT_CLRFF, 0);
	outp(dma_port_count[ch], (BUF_BYTES - 1) & 0xff);
	outp(dma_port_count[ch], ((BUF_BYTES - 1) >> 8) & 0xff);

	/* Unmask the channel. */
	outp(DMA_PORT_SMASK, ch);

	_enable();
}

static void
stop_dma(void)
{
	outp(DMA_PORT_SMASK, 0x04 | sb_dma);
}

/* DSP Playback Start */
static void
start_playback(void)
{
	cur_half = 0;
	fill_half = 0;
	fill_pending = 0;

	/* Set the output sample rate. */
	dsp_write(DSP_SET_OUTPUT_RATE);
	dsp_write((SAMPLING_RATE >> 8) & 0xff);
	dsp_write(SAMPLING_RATE & 0xff);

	/* Turn the speaker on. (No-op for 16-bit DACs, but harmless.) */
	dsp_write(DSP_SPEAKER_ON);

	/*
	 * Start an auto-init transfer.
	 * The block length is one half of the DMA buffer, so we get an
	 * interrupt each time a half finishes.
	 */
#ifdef SB16_S16_44K_STEREO
	dsp_write(DSP_PLAY_16BIT_AUTO);
	dsp_write(DSP_MODE_S_STEREO);
	dsp_write(DSP_BLOCK_LEN & 0xff);
	dsp_write((DSP_BLOCK_LEN >> 8) & 0xff);
#endif
#ifdef SB16_S16_8K_MONO
	dsp_write(DSP_PLAY_16BIT_AUTO);
	dsp_write(DSP_MODE_S_MONO);
	dsp_write(DSP_BLOCK_LEN & 0xff);
	dsp_write((DSP_BLOCK_LEN >> 8) & 0xff);
#endif
#ifdef SB16_S8_8K_MONO
	dsp_write(DSP_PLAY_8BIT_AUTO);
	dsp_write(DSP_MODE_S_MONO);
	dsp_write(DSP_BLOCK_LEN & 0xff);
	dsp_write((DSP_BLOCK_LEN >> 8) & 0xff);
#endif
}

/*
 * Interrupt Handling
 */

static void
hook_irq(void)
{
#if defined(__WATCOMC__)
	int imr_port, bit;

	old_isr = _dos_getvect(sb_vec);
	_dos_setvect(sb_vec, sb16_isr);

	/* Unmask the IRQ in the PIC. */
	_disable();
	if (sb_irq < 8) {
		imr_port = PIC0_IMR;
		bit = 1 << sb_irq;
	} else {
		imr_port = PIC1_IMR;
		bit = 1 << (sb_irq - 8);
	}
	old_imr_masked = inp(imr_port) & bit;
	outp(imr_port, inp(imr_port) & ~bit);
	if (sb_irq >= 8) {
		/* Make sure the cascade (master IR7 on PC-98) is open. */
		outp(PIC0_IMR, inp(PIC0_IMR) & ~0x80);
	}
	_enable();
#endif
}

static void
unhook_irq(void)
{
#if defined(__WATCOMC__)
	int imr_port, bit;

	_disable();
	if (sb_irq < 8) {
		imr_port = PIC0_IMR;
		bit = 1 << sb_irq;
	} else {
		imr_port = PIC1_IMR;
		bit = 1 << (sb_irq - 8);
	}
	if (old_imr_masked)
		outp(imr_port, inp(imr_port) | bit);
	_enable();

	_dos_setvect(sb_vec, old_isr);
#endif
}

#if defined(__WATCOMC__)
/*
 * The SB16 interrupt handler.
 *
 * [IMPORTANT]
 *  - This runs with interrupts disabled; keep it short.
 *  - Never call hal_get_wave_samples() or any DOS service here.
 */
static void __interrupt __far
sb16_isr(void)
{
	int st;

	old_isr();

	/* Acknowledge the DSP interrupt(s). */
	st = mixer_read(MIX_IRQ_STATUS);
	if (st & 0x01)
		(void)inp(P_DSP_RSTAT);
	if (st & 0x02)
		(void)inp(P_DSP_ACK16);

	/*
	 * The half that just finished playing needs a refill.  If the
	 * previous refill request was not served in time, fill that
	 * half with silence so that auto-init DMA repeats silence.
	 */
	if (fill_pending)
		memset(dma_buf + fill_half * HALF_BYTES, 0, HALF_BYTES);
	fill_half = cur_half;
	cur_half ^= 1;
	fill_pending = 1;

	/* Send EOI to the PIC(s). */
	if (sb_irq >= 8)
		outp(PIC1_CMD, PIC_EOI);
	outp(PIC0_CMD, PIC_EOI);
}
#endif

/*
 * Mixing
 */

/* Decode, mix and write one half of the DMA buffer. */
static void
fill_half_buffer(int half)
{
#ifdef SB16_S16_44K_STEREO
	uint32_t *dst;
	uint32_t frame;
	int32_t il, ir;
	int16_t sl, sr;
	int n, i, got, q15;
	bool eos;

	dst = (uint32_t *)(dma_buf + half * HALF_BYTES);

	memset(mix_buf, 0, sizeof(mix_buf));

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		if (wave[n] == NULL)
			continue;

		/* Get PCM samples. */
		got = hal_get_wave_samples(wave[n], pull_buf, HALF_FRAMES);
		eos = hal_is_wave_eos(wave[n]);

		/* Scale by the volume and accumulate. */
		q15 = volume_q15[n];
		for (i = 0; i < got; i++) {
			frame = pull_buf[i];
			sl = (int16_t)(uint16_t)frame;
			sr = (int16_t)(uint16_t)(frame >> 16);
			mix_buf[i * 2 + 0] += ((int32_t)sl * q15) >> 15;
			mix_buf[i * 2 + 1] += ((int32_t)sr * q15) >> 15;
		}

		/* Handle an end-of-stream. */
		if (got < HALF_FRAMES || eos) {
			_disable();
			if (wave[n] != NULL) {
				wave[n] = NULL;
				finish[n] = true;
			}
			_enable();
		}
	}

	/* Clip and pack. */
	for (i = 0; i < HALF_FRAMES; i++) {
		il = mix_buf[i * 2 + 0];
		ir = mix_buf[i * 2 + 1];

		il = il > 32767 ? 32767 : il;
		il = il < -32768 ? -32768 : il;
		ir = ir > 32767 ? 32767 : ir;
		ir = ir < -32768 ? -32768 : ir;

		dst[i] = ((uint32_t)(uint16_t)(int16_t)il) |
			 (((uint32_t)(uint16_t)(int16_t)ir) << 16);
	}
#endif
#ifdef SB16_S16_8K_MONO
	uint16_t *dst;
	uint32_t frame;
	int32_t il, ir;
	int16_t sl;
	int n, i, got, q15;
	bool eos;

	dst = (uint16_t *)(dma_buf + half * HALF_BYTES);

	memset(mix_buf, 0, sizeof(mix_buf));

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		if (wave[n] == NULL)
			continue;

		/* Get PCM samples. */
		got = hal_get_wave_samples(wave[n], pull_buf, HALF_FRAMES);
		eos = hal_is_wave_eos(wave[n]);

		/* Scale by the volume and accumulate. */
		q15 = volume_q15[n];
		for (i = 0; i < got; i++) {
			frame = pull_buf[i];
			sl = (int16_t)(uint16_t)frame;
			mix_buf[i] += ((int32_t)sl * q15) >> 15;
		}

		/* Handle an end-of-stream. */
		if (got < HALF_FRAMES || eos) {
			_disable();
			if (wave[n] != NULL) {
				wave[n] = NULL;
				finish[n] = true;
			}
			_enable();
		}
	}

	/* Clip and pack. */
	for (i = 0; i < HALF_FRAMES; i++) {
		il = mix_buf[i];

		il = il > 32767 ? 32767 : il;
		il = il < -32768 ? -32768 : il;
		ir = ir > 32767 ? 32767 : ir;
		ir = ir < -32768 ? -32768 : ir;

		dst[i] = ((uint32_t)(uint16_t)(int16_t)il);
	}
#endif
#ifdef SB16_S8_8K_MONO
	uint8_t *dst;
	uint32_t frame;
	int32_t il;
	int16_t sl, sr;
	int n, i, got, q15;
	bool eos;

	dst = (uint8_t *)(dma_buf + half * HALF_BYTES);

	memset(mix_buf, 0, sizeof(mix_buf));

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		if (wave[n] == NULL)
			continue;

		/* Get PCM samples. */
		got = hal_get_wave_samples(wave[n], pull_buf, HALF_FRAMES);
		eos = hal_is_wave_eos(wave[n]);

		/* Mix down L/R, scale by the volume, and accumulate. */
		q15 = volume_q15[n];
		for (i = 0; i < got; i++) {
			frame = pull_buf[i];
			sl = (int16_t)(uint16_t)frame;
			sr = (int16_t)(uint16_t)(frame >> 16);
			mix_buf[i] += ((((int32_t)sl + (int32_t)sr) >> 1) * q15) >> 15;
		}

		/* Handle an end-of-stream. */
		if (got < HALF_FRAMES || eos) {
			_disable();
			if (wave[n] != NULL) {
				wave[n] = NULL;
				finish[n] = true;
			}
			_enable();
		}
	}

	/* Clip and pack. */
	for (i = 0; i < HALF_FRAMES; i++) {
		il = mix_buf[i];

		il = il > 32767 ? 32767 : il;
		il = il < -32768 ? -32768 : il;

		dst[i] = (int8_t)(il >> 8);
	}
#endif
}

/*
 * DPMI 0600h: Lock a linear address region so that a virtual memory
 * manager never pages it out. (No-op failure is acceptable when no
 * VMM is active.)
 */
static void
dpmi_lock_region(void *p, uint32_t size)
{
	union REGS r;
	uint32_t lin = (uint32_t)p;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0600;
	r.w.bx = (uint16_t)(lin >> 16);
	r.w.cx = (uint16_t)(lin & 0xffff);
	r.w.si = (uint16_t)(size >> 16);
	r.w.di = (uint16_t)(size & 0xffff);
	int386(0x31, &r, &r);
}
