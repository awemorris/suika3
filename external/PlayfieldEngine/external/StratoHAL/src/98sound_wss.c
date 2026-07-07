/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for PC-9821 V13 built-in WSS / Mate-X PCM
 *
 * Target:
 *   PC-9821 V13 internal WSS-compatible PCM
 *   I/O base: 0F40h
 *   IRQ: IRQ12 (INT5), vector 14h
 *   DMA: ch1
 *
 * Format:
 *   8000Hz, 16-bit signed little-endian, monaural
 *
 * Notes:
 *   - This is a first implementation based on the existing SB16/98 HAL.
 *   - The main design is kept:
 *       auto-init DMA
 *       double buffer
 *       IRQ only flips buffer flags
 *       actual decoding/mixing is done in the main thread
 */

/* Base */
#include <strato/strato.h>

/* Standard C */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

/* DOS / OpenWatcom */
#include <dos.h>
#include <conio.h>
#include <i86.h>

/*
 * Format
 */
#define SAMPLING_RATE	(8000)
#define CHANNELS        (1)
#define FRAME_SIZE      (2)		/* 16-bit mono */
#define HALF_FRAMES     (8192)		/* about 1.024 sec per half at 8kHz */

#define HALF_BYTES      (HALF_FRAMES * FRAME_SIZE)
#define BUF_BYTES       (HALF_BYTES * 2)

/*
 * Playback Base Count register 0Eh/0Fh.
 *
 * The unit is SAMPLES, not bytes: the PI interrupt fires every
 * (count + 1) samples.  NP21W confirms this:
 *
 *   playcount = (playcount regs) * cs4231_playcountshift[fmt] + 4;
 *
 * i.e. the register value is multiplied by bytes-per-sample.
 * For 16-bit mono, 1 sample = 1 frame = 2 bytes, so one half of
 * the buffer is HALF_FRAMES samples -> count = HALF_FRAMES - 1.
 */
#define WSS_BLOCK_COUNT        (HALF_FRAMES - 1)

/*
 * WSS I/O ports
 */
#define WSS_BASE		0x0f40

/* PC-9821-only registers */
#define P_WSS_IRQ_CONFIG        (WSS_BASE + 0)  	/* 0F40h: R/W */
#define P_WSS_SOUND_ID          (WSS_BASE + 3)  	/* 0F43h: Read Only */

/* PC/AT compatible registers */
#define P_WSS_INDEX             (WSS_BASE + 4)		/* 0F44h: R/W (MCE, INDEX) */
#define P_WSS_DATA              (WSS_BASE + 5)		/* 0F45h: R/W (DATA) */
#define P_WSS_STATUS            (WSS_BASE + 6)		/* 0F46h: Read Only (STATUS) */
#define P_WSS_PIO               (WSS_BASE + 7)		/* 0F47h: R/W (PIO DATA) */

/*
 * WSS / CS4231 internal registers
 */
#define WSS_REG_LEFT_INPUT	0x00
#define WSS_REG_RIGHT_INPUT	0x01
#define WSS_REG_LEFT_OUTPUT	0x06
#define WSS_REG_RIGHT_OUTPUT	0x07
#define WSS_REG_FORMAT		0x08
#define WSS_REG_IFACE		0x09
#define WSS_REG_PIN		0x0a
#define WSS_REG_TEST_INIT	0x0b
#define WSS_REG_MISC		0x0c
#define WSS_REG_LOOPBACK	0x0d
#define WSS_REG_PCNT_H		0x0e
#define WSS_REG_PCNT_L		0x0f
#define WSS_REG_ALTSTAT		0x18	/* Alternate Feature Status (I24) */

#define WSS_IFACE_PEN		0x01	/* Interface Control register 09h */
#define WSS_IFACE_CEN           0x02	/* Capture Enable */
#define WSS_IFACE_SDC           0x04	/* Single DMA channel */
#define WSS_IFACE_PPIO		0x40	/* Playback PIO Enable */
#define WSS_IFACE_CPIO		0x80	/* Capture PIO Enable */

/* Pin Control register 0Ah */
#define WSS_PIN_IEN             0x02	/* Interrupt Enable */

/* Error Status and Initialization register 0Bh */
#define WSS_INIT_ACI		0x20	/* Auto-calibrate In-Progress */

/* Alternate Feature Status register 18h (I24) */
#define WSS_ALTSTAT_PI		0x10	/* Playback Interrupt pending */
#define WSS_ALTSTAT_CI		0x20	/* Capture Interrupt pending */
#define WSS_ALTSTAT_TI		0x40	/* Timer Interrupt pending */

/*
 * Index register bits
 */
#define WSS_INDEX_INIT		0x80
#define WSS_INDEX_MCE		0x40
#define WSS_INDEX_TRD		0x20
#define WSS_INDEX_MASK		0x1f

/*
 * CS4231 playback format register 08h.
 *
 * Upper nibble of I8 = (FMT << 1) | S/M:
 *   FMT (bits 7:5): 000 = 8bit unsigned
 *                   001 = u-Law
 *                   010 = 16bit signed little-endian
 *                   011 = A-Law
 *                   110 = 16bit signed big-endian
 *   S/M (bit 4):    0 = mono, 1 = stereo
 *
 * Lower nibble: C2SL = 0 (24.576MHz), CFS = 000 -> 8000Hz.
 */
#define WSS_FMT_8K_S16_MONO	0x40

/*
 * PC-98 DMA Controller
 *
 * Same uPD71037/i8237A-compatible programming model as existing SB16/98 code.
 */

#define DMA_PORT_SMASK		0x15        /* single mask */
#define DMA_PORT_MODE		0x17        /* mode */
#define DMA_PORT_CLRFF		0x19        /* clear byte pointer flip-flop */

/* 8237 DMA mode: */
#define DMA_MODE_SINGLE         0x40	/* single transfer mode */
#define DMA_MODE_AUTO_INIT      0x10	/* auto initialize */
#define DMA_MODE_READ           0x08	/* memory -> device */

/* PC-98 DMA ports per channel (0,1,2,3) */
static const int dma_port_addr[4]  = { 0x01, 0x05, 0x09, 0x0d };
static const int dma_port_count[4] = { 0x03, 0x07, 0x0b, 0x0f };
static const int dma_port_bank[4]  = { 0x27, 0x21, 0x23, 0x25 };

/*
 * PC-98 PIC
 */
#define PIC0_CMD		0x00
#define PIC0_IMR		0x02
#define PIC1_CMD		0x08
#define PIC1_IMR		0x0a
#define PIC_EOI			0x20

/*
 * PC-98 0.6us wait port.
 */
#define WAIT_PORT		0x5f

/*
 * Driver State
 */
static bool wss_ok;

static int wss_irq = 12;
static int wss_vector = 0x14;
static int wss_dma_ch = 1;

/* DMA buffer below 1MB, not crossing a 64KB boundary. */
static uint8_t *dma_buf;
static uint32_t dma_phys;
static uint16_t dos_selector;

/* Double buffer bookkeeping, shared with ISR. */
static volatile int cur_half;
static volatile int fill_half;
static volatile int fill_pending;

/* Old interrupt vector and old PIC mask bit. */
#if defined(__WATCOMC__)
static void (__interrupt __far *old_isr)(void);
#endif
static int old_imr_masked;

/* Input Streams */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Volume Values, Q15 fixed point, 0..32767 */
static int volume_q15[HAL_SOUND_TRACKS];

/* Finish Flags */
static volatile bool finish[HAL_SOUND_TRACKS];

/* Mixing Buffers */
static int32_t mix_buf[HALF_FRAMES];
static uint32_t pull_buf[HALF_FRAMES];

/*
 * Forward Declarations
 */
static bool wss_detect(void);
static void wss_init_chip(void);
static void wss_wait_ready(void);
static void wss_write(int reg, int val);
static int wss_read(int reg);
static void wss_stop_codec(void);
static void wss_ack_irq(void);

static bool alloc_dma_buffer(void);
static void free_dma_buffer(void);
static void setup_dma(void);
static void stop_dma(void);

static void hook_irq(void);
static void unhook_irq(void);

static void fill_half_buffer(int half);
static void dpmi_lock_region(void *p, uint32_t size);

#if defined(__WATCOMC__)
static void __interrupt __far wss_isr(void);
#endif

/*
 * Initialize the WSS sound driver.
 */
bool
wss_init_sound(void)
{
	int n;
	int cfg;
	const int irq_tbl[8] = { -1, 3, 5, 10, 12, -1, -1, -1 };
	const int dma_tbl[8] = { -1, 0, 1, 3, -1, -1, -1, -1 };

	wss_ok = false;

	cur_half = 0;
	fill_half = 0;
	fill_pending = 0;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		wave[n] = NULL;
		volume_q15[n] = 32767;
		finish[n] = false;
	}

	/* Detect a chip. */
	if (!wss_detect())
		return false;

	hal_log_info("WSS: found a card.");

	/* Read the config register. */
	cfg = inp(P_WSS_IRQ_CONFIG);
	hal_log_info("WSS: 0F40h = %02Xh", cfg & 0xff);

	/* Check for IRQ. */
	wss_irq = irq_tbl[(cfg >> 3) & 7];
	if (wss_irq < 0) {
		hal_log_info("WSS: IRQ field is wrong, setting IRQ12 (INT5)...");

		/*
		 * FIX: actually program the config register.
		 * The previous code only overwrote the variable; the
		 * board kept routing the codec IRQ nowhere.
		 * Field value 4 = IRQ12 (see irq_tbl).
		 */
		wss_irq = 12;
		cfg = (cfg & ~(7 << 3)) | (4 << 3);
		outp(P_WSS_IRQ_CONFIG, cfg);
		outp(WAIT_PORT, 0);
		cfg = inp(P_WSS_IRQ_CONFIG);
		if (irq_tbl[(cfg >> 3) & 7] != 12) {
			hal_log_info("WSS: Failed to set IRQ.");
			return false;
		}
	}
	hal_log_info("WSS: IRQ %d\n", wss_irq);

	/* Check for DMA. */
	wss_dma_ch = dma_tbl[cfg & 7];
	if (wss_dma_ch < 0) {
		hal_log_info("WSS: DMA channel is not set, setting DMA1...");

		wss_dma_ch = 1;

		/* Write the config. */
		cfg = (cfg & ~7) | 2;
		outp(P_WSS_IRQ_CONFIG, cfg);
		outp(WAIT_PORT, 0);

		/* Check for DMA again. */
		cfg = inp(P_WSS_IRQ_CONFIG);
		wss_dma_ch = dma_tbl[cfg & 7];
		if (wss_dma_ch < 0) {
			hal_log_info("WSS: Failed. WSS is not available.");
			return false;
		}
	}
	hal_log_info("WSS: DMA %d\n", wss_dma_ch);

	/* Allocate a DMA buffer. */
	if (!alloc_dma_buffer()) {
		hal_log_info("WSS: failed to allocate DMA buffer.");
		return false;
	}
	memset(dma_buf, 0, BUF_BYTES);

	/*
	 * Lock ISR-touched memory.
	 * Failure is acceptable when no VMM is active.
	 */
	dpmi_lock_region((void *)dma_buf, BUF_BYTES);
	dpmi_lock_region((void *)&cur_half, 4096);

	/* Install interrupt handler before enabling playback IRQ. */
	hook_irq();

	/* Program DMA controller for auto-init transfer over the full buffer. */
	setup_dma();

	/*
	 * Initialize the WSS chip.
	 * This also starts playback (PEN) as its very last step.
	 */
	wss_init_chip();

	wss_ok = true;

	hal_log_info("Sound enabled.\n");

	return true;
}

/*
 * Simple WSS presence check.
 *
 * This intentionally touches only the playback attenuator register.
 */
static bool
wss_detect(void)
{
        int id_val;
        int oldv;
        int v;

        /*
	 * Step 1: Check for PC-9821-only Sound ID port.
         * (0xff if not implemented)
	 */
        id_val = inp(P_WSS_SOUND_ID);

        /*
	 * On PC-9821 with internal WSS, the lower bits are set to 000100b (0x04).
	 * Upper bits varies by chip revisions.
	 */
        if ((id_val & 0x3F) != 0x04)
                return false;

        /*
	 * Step 2: Check for CS4231 response.
	 */

        wss_wait_ready();

        oldv = wss_read(WSS_REG_LEFT_OUTPUT);

        /* Write 0x80 mute bit and check the difference. */
        wss_write(WSS_REG_LEFT_OUTPUT, 0x80);
        v = wss_read(WSS_REG_LEFT_OUTPUT);

        /* Set the old value back. */
        wss_write(WSS_REG_LEFT_OUTPUT, oldv);

        if ((v & 0x80) != 0x80)
                return false;

        return true;
}

/*
 * Initialize the WSS chip.
 *
 * The correct canonical sequence is:
 *
 *   1. MCE on: write format (I8) and interface mode bits (I9,
 *      SDC/PPIO/CPIO, but PEN=0).  These are the only registers
 *      that actually require MCE.
 *   2. MCE off, wait for INIT to clear and autocalibration (ACI
 *      in I11) to finish.
 *   3. In normal mode: program the base count (I14/I15), unmute
 *      the DAC (I6/I7), clear pending interrupts, then enable IEN
 *      (I10).
 *   4. Finally set PEN.  PEN is documented to be set and reset
 *      WITHOUT MCE, and setting it is what starts DMA playback
 *      (in NP21W this is the write that triggers DMAEXT_START).
 */
static void
wss_init_chip(void)
{
	unsigned int count;
	unsigned int iface;
	unsigned int pin;
	long i;

	count = WSS_BLOCK_COUNT;

	wss_wait_ready();

	/*
	 * --- MCE phase -------------------------------------------
	 *
	 * Playback format (I8):
	 *   0x40 = 8000Hz / 16-bit signed little-endian / mono
	 */
	outp(P_WSS_INDEX, WSS_INDEX_MCE | WSS_REG_FORMAT);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, WSS_FMT_8K_S16_MONO);
	outp(WAIT_PORT, 0);
	wss_wait_ready();

	/*
	 * Interface register (I9), mode bits only:
	 *   PEN  = 0 (started later, without MCE)
	 *   CEN  = 0
	 *   SDC  = 1 (single DMA channel, matches the board wiring)
	 *   PPIO = 0, CPIO = 0 (DMA, not PIO)
	 */
	outp(P_WSS_INDEX, WSS_INDEX_MCE | WSS_REG_IFACE);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, WSS_IFACE_SDC);
	outp(WAIT_PORT, 0);

	wss_wait_ready();

	/*
	 * --- leave MCE -------------------------------------------
	 */
	outp(P_WSS_INDEX, 0);
	outp(WAIT_PORT, 0);

	/* Wait for INIT to clear (bounded, unlike the old loop). */
	wss_wait_ready();

	/* Wait for autocalibration to finish (bounded). */
	for (i = 0; i < 100000L; i++) {
		if (!(wss_read(WSS_REG_TEST_INIT) & WSS_INIT_ACI))
			break;
		outp(WAIT_PORT, 0);
	}

	wss_wait_ready();

	/*
	 * --- normal mode programming -----------------------------
	 *
	 * Playback base count (I14 = upper, I15 = lower).
	 * Unit is samples - 1; PI fires every (count + 1) samples,
	 * i.e. once per half buffer.  The base registers do not
	 * require MCE and are loaded into the current count when
	 * PEN is set.
	 */
	wss_write(WSS_REG_PCNT_H, (count >> 8) & 0xff);
	wss_write(WSS_REG_PCNT_L, count & 0xff);

	/*
	 * Unmute the DAC and set full volume (I6/I7):
	 *   bit7 = mute, lower bits = attenuation, 00h = max.
	 */
	wss_write(WSS_REG_LEFT_OUTPUT, 0x00);
	wss_write(WSS_REG_RIGHT_OUTPUT, 0x00);

	/*
	 * Clear any pending interrupt state (Status register and
	 * I24) before enabling the interrupt output.
	 */
	wss_ack_irq();

	/*
	 * Pin Control register (I10), bit1 = IEN.
	 * This enables the codec interrupt output.  Do this before
	 * PEN so the very first half-buffer interrupt is not lost.
	 */
	pin = wss_read(WSS_REG_PIN);
	wss_write(WSS_REG_PIN, pin | WSS_PIN_IEN);

	/*
	 * --- start playback --------------------------------------
	 */
	iface = wss_read(WSS_REG_IFACE);
	iface |= WSS_IFACE_PEN;
	iface &= ~(WSS_IFACE_CEN | WSS_IFACE_PPIO | WSS_IFACE_CPIO);
	wss_write(WSS_REG_IFACE, iface);
}

/*
 * Cleanup the WSS sound driver.
 */
void
wss_cleanup_sound(void)
{
	int n;

	if (!wss_ok)
		return;

	wss_ok = false;

	for (n = 0; n < HAL_SOUND_TRACKS; n++)
		wave[n] = NULL;

	wss_stop_codec();
	stop_dma();
	unhook_irq();
	free_dma_buffer();
}

/*
 * Pump the sound: decode and mix into the DMA buffer.
 *
 * Call this once per frame from the main loop.
 */
void
wss_sound_poll(void)
{
	int half;

	if (!wss_ok)
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
wss_play_sound(
	int n,
	struct hal_wave *w)
{
	assert(n < HAL_SOUND_TRACKS);
	assert(w != NULL);

	if (!wss_ok)
		return true;

	_disable();
	{
		wave[n] = w;
		finish[n] = false;
	}
	_enable();

	/*
	 * Force an early refill if possible.
	 * If no IRQ has occurred yet, playback begins with silence
	 * until the first half-buffer IRQ.
	 */
	wss_sound_poll();

	return true;
}

/*
 * Stop sound playback on a stream.
 */
bool
wss_stop_sound(
	int n)
{
	assert(n < HAL_SOUND_TRACKS);

	if (!wss_ok)
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
wss_set_sound_volume(
	int n,
	float vol)
{
	double scale;

	assert(n < HAL_SOUND_TRACKS);
	assert(vol >= 0 && vol <= 1.0f);

	/* Same curve as other HALs. */
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
wss_is_sound_finished(
	int n)
{
	if (!wss_ok)
		return true;

	if (!finish[n])
		return false;

	return true;
}

/*
 * WSS low-level access
 */
static void
wss_wait_ready(void)
{
	long i;

	for (i = 0; i < 100000L; i++) {
		if ((inp(P_WSS_INDEX) & WSS_INDEX_INIT) == 0)
			break;
		outp(WAIT_PORT, 0); /* バスウェイトを入れる */
	}
}

static void
wss_write(
	int reg,
	int val)
{
	wss_wait_ready();
	_disable();
	outp(P_WSS_INDEX, reg & WSS_INDEX_MASK);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, val);
	outp(WAIT_PORT, 0);
	_enable();
}

static int
wss_read(
	int reg)
{
	int v;

	wss_wait_ready();
	_disable();
	outp(P_WSS_INDEX, reg & WSS_INDEX_MASK);
	outp(WAIT_PORT, 0);
	v = inp(P_WSS_DATA);
	_enable();
	return v;
}

static void
wss_stop_codec(void)
{
	int pin;
	int iface;

	/* Disable the codec IRQ output first, then stop playback. */
	pin = wss_read(WSS_REG_PIN);
	wss_write(WSS_REG_PIN, pin & ~WSS_PIN_IEN);

	/* PEN off (allowed without MCE). */
	iface = wss_read(WSS_REG_IFACE);
	wss_write(WSS_REG_IFACE, iface & ~(WSS_IFACE_PEN | WSS_IFACE_CEN));

	wss_ack_irq();
}

/* Acknowledge / clear codec interrupt state (mainline version). */
static void
wss_ack_irq(void)
{
	(void)inp(P_WSS_STATUS);
	outp(P_WSS_STATUS, 0x00);
	outp(WAIT_PORT, 0);

	/* Clear PI/TI/CI in I24. */
	wss_write(WSS_REG_ALTSTAT, 0x00);
}

/*
 * DMA Buffer Allocation
 *
 * DPMI 0100h allocates conventional DOS memory below 1MB.
 * We allocate twice the needed size and align so the DMA buffer never
 * crosses a 64KB physical boundary.
 */
static bool
alloc_dma_buffer(void)
{
	union REGS r;
	uint32_t base;
	uint32_t start;

	memset(&r, 0, sizeof(r));

	r.w.ax = 0x0100;
	r.w.bx = (BUF_BYTES * 2 + 15) / 16;    /* paragraphs */
	int386(0x31, &r, &r);

	if (r.w.cflag)
		return false;

	dos_selector = r.w.dx;
	base = (uint32_t)r.w.ax << 4;

	start = base;
	if ((start & 0xffff) + BUF_BYTES > 0x10000)
		start = (start + 0xffff) & 0xffff0000UL;

	dma_phys = start;
	dma_buf = (uint8_t *)start;        /* zero-based flat address space */

	return true;
}

static void
free_dma_buffer(void)
{
	union REGS r;

	if (dos_selector == 0)
		return;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0101;
	r.w.dx = dos_selector;
	int386(0x31, &r, &r);

	dos_selector = 0;
	dma_buf = NULL;
	dma_phys = 0;
}

/*
 * DMA Controller Setup
 *
 * Auto-init, memory -> device, over the full double buffer.
 */
static void
setup_dma(void)
{
	int ch;
	uint16_t count;

	ch = wss_dma_ch;
	count = BUF_BYTES - 1;

	_disable();

	/* Mask channel. */
	outp(DMA_PORT_SMASK, 0x04 | ch);

	/* Program DMA mode for the actual channel. */
	outp(DMA_PORT_MODE, DMA_MODE_SINGLE | DMA_MODE_AUTO_INIT | DMA_MODE_READ | (ch & 3));

	/* Clear byte pointer flip-flop. */
	outp(DMA_PORT_CLRFF, 0);

	/* Address A0-A15. */
	outp(dma_port_addr[ch], (int)(dma_phys & 0xff));
	outp(dma_port_addr[ch], (int)((dma_phys >> 8) & 0xff));

	/* Bank A16-A23. */
	outp(dma_port_bank[ch], (int)((dma_phys >> 16) & 0xff));

	/* Count: full double buffer, bytes - 1. */
	outp(DMA_PORT_CLRFF, 0);
	outp(dma_port_count[ch], count & 0xff);
	outp(dma_port_count[ch], (count >> 8) & 0xff);

	/* Unmask channel. */
	outp(DMA_PORT_SMASK, ch);

	_enable();
}

static void
stop_dma(void)
{
	outp(DMA_PORT_SMASK, 0x04 | wss_dma_ch);
}

/*
 * Interrupt Handling
 */
static void
hook_irq(void)
{
#if defined(__WATCOMC__)
    int imr_port;
    int bit;

    if (wss_irq == 12)
	    wss_vector = 0x14;
    else if (wss_irq == 10)
	    wss_vector = 0x12;
    else if (wss_irq == 5)
	    wss_vector = 0x0d;
    else if (wss_irq == 3)
	    wss_vector = 0x0b;
    else
	    wss_vector = -1;

    if (wss_vector == -1) {
	    hal_log_info("Invalid IRQ number\n");
	    return;
    }

    old_isr = _dos_getvect(wss_vector);
    _dos_setvect(wss_vector, wss_isr);

    _disable();

    if (wss_irq < 8) {
        imr_port = PIC0_IMR;
        bit = 1 << wss_irq;
    } else {
        imr_port = PIC1_IMR;
        bit = 1 << (wss_irq - 8);
    }

    old_imr_masked = inp(imr_port) & bit;

    /* Unmask WSS IRQ. */
    outp(imr_port, inp(imr_port) & ~bit);

    if (wss_irq >= 8) {
        /*
         * PC-98 slave PIC cascades into master IR7.
         * Make sure master IR7 is also unmasked.
         */
        outp(PIC0_IMR, inp(PIC0_IMR) & ~0x80);
    }

    _enable();
#endif
}

static void
unhook_irq(void)
{
#if defined(__WATCOMC__)
    int imr_port;
    int bit;

    _disable();

    if (wss_irq < 8) {
        imr_port = PIC0_IMR;
        bit = 1 << wss_irq;
    } else {
        imr_port = PIC1_IMR;
        bit = 1 << (wss_irq - 8);
    }

    if (old_imr_masked)
        outp(imr_port, inp(imr_port) | bit);

    _enable();

    _dos_setvect(wss_vector, old_isr);
#endif
}


static void
pic_eoi_for_irq(int irq)
{
    if (irq >= 8) {
        outp(PIC1_CMD, PIC_EOI);
        outp(PIC0_CMD, PIC_EOI);
    } else {
        outp(PIC0_CMD, PIC_EOI);
    }
}


#if defined(__WATCOMC__)
/*
 * WSS interrupt handler.
 *
 * Important:
 *   - Keep this short.
 *   - Do not call hal_get_wave_samples() here.
 *   - Do not use DOS services here.
 *   - Raw port I/O only: wss_read/wss_write must not be used here
 *     because they call _enable().
 */
static void __interrupt __far
wss_isr(void)
{
	/*
	 * Acknowledge the codec interrupt source.
	 *
	 * 1. Any write to the Status register (0F46h) clears the
	 *    interrupt on the real chip.
	 * 2. FIX: additionally clear PI/TI/CI in I24.  NP21W only
	 *    calls pic_resetirq() when all three bits have been
	 *    written to 0 (cs4231_control, CS4231REG_IRQSTAT), so
	 *    without this the emulated IRQ line stays asserted and
	 *    no further interrupts arrive.
	 */
	(void)inp(P_WSS_STATUS);
	outp(P_WSS_STATUS, 0x00);
	outp(WAIT_PORT, 0);

	outp(P_WSS_INDEX, WSS_REG_ALTSTAT);	/* no MCE */
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, 0x00);			/* clear PI/TI/CI */
	outp(WAIT_PORT, 0);

	/*
	 * Then acknowledge the PIC (slave first, then master).
	 */
	pic_eoi_for_irq(wss_irq);

	/*
	 * Minimal bookkeeping only.
	 */
	cur_half ^= 1;
	fill_half = cur_half ^ 1;
	fill_pending = 1;
}
#endif

/*
 * Mixing
 *
 * Decode, mix, clip and write one half of the DMA buffer.
 *
 * Output:
 *   8000Hz / signed 16-bit / monaural / little-endian
 */
static void
fill_half_buffer(
	int half)
{
	uint16_t *dst;
	uint32_t frame;
	int32_t mixed;
	int16_t sl;
	int16_t sr;
	int n;
	int i;
	int got;
	int q15;
	bool eos;

	dst = (uint16_t *)(dma_buf + half * HALF_BYTES);

	memset(mix_buf, 0, sizeof(mix_buf));

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		if (wave[n] == NULL)
			continue;

		got = hal_get_wave_samples(wave[n], pull_buf, HALF_FRAMES);
		eos = hal_is_wave_eos(wave[n]);

		q15 = volume_q15[n];

		for (i = 0; i < got; i++) {
			frame = pull_buf[i];

			/*
			 * hal_get_wave_samples() returns:
			 *   low  16 bits = left
			 *   high 16 bits = right
			 *
			 * Mix down to mono.
			 */
			sl = (int16_t)(uint16_t)frame;
			sr = (int16_t)(uint16_t)(frame >> 16);

			mixed = ((int32_t)sl + (int32_t)sr) >> 1;
			mixed = (mixed * q15) >> 15;

			mix_buf[i] += mixed;
		}

		if (got < HALF_FRAMES || eos) {
			_disable();
			if (wave[n] != NULL) {
				wave[n] = NULL;
				finish[n] = true;
			}
			_enable();
		}
	}

	for (i = 0; i < HALF_FRAMES; i++) {
		mixed = mix_buf[i];

		if (mixed > 32767)
			mixed = 32767;
		if (mixed < -32768)
			mixed = -32768;

		/*
		 * WSS 16-bit signed linear is little-endian.
		 * On x86 this stores correctly as low byte, high byte.
		 */
		dst[i] = (uint16_t)(int16_t)mixed;
	}
}

/*
 * DPMI 0600h:
 * Lock a linear address region so that a virtual memory manager never
 * pages it out. Failure is acceptable when no VMM is active.
 */
static void
dpmi_lock_region(
	void *p,
	uint32_t size)
{
	union REGS r;
	uint32_t lin;

	lin = (uint32_t)p;

	memset(&r, 0, sizeof(r));

	r.w.ax = 0x0600;
	r.w.bx = (uint16_t)(lin >> 16);
	r.w.cx = (uint16_t)(lin & 0xffff);
	r.w.si = (uint16_t)(size >> 16);
	r.w.di = (uint16_t)(size & 0xffff);

	int386(0x31, &r, &r);
}
