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
 *
 * WSS playback format register 08h:
 *   40h = 8000Hz / 16-bit signed linear / mono
 */
#define SAMPLING_RATE	(8000)
#define CHANNELS        (1)
#define FRAME_SIZE      (2)		/* 16-bit mono */
#define HALF_FRAMES     (1024)		/* about 1.024 sec per half */

#define HALF_BYTES      (HALF_FRAMES * FRAME_SIZE)
#define BUF_BYTES       (HALF_BYTES * 2)

/*
 * For V13 internal WSS / CS4231-compatible codec:
 *
 *   Playback Base Count register 0Eh/0Fh:
 *     16-bit mode count = transferred bytes / 2 - 1
 *
 * Half-buffer periodic IRQ:
 */
#define WSS_BLOCK_COUNT        ((HALF_BYTES / 2) - 1)

/*
 * WSS I/O ports
 */
#if !defined(WSS_BASE)
#define WSS_BASE		0x0f40
#endif

/* PC-9821-only registers */
#define P_WSS_IRQ_CONFIG        (WSS_BASE + 0)  /* 0F40h: R/W */
#define P_WSS_SOUND_ID          (WSS_BASE + 3)  /* 0F43h: Read Only */

/* PC/AT compatible registers */
#define P_WSS_INDEX             (WSS_BASE + 4)  /* 0F44h: R/W (MCE, INDEX) */
#define P_WSS_DATA              (WSS_BASE + 5)  /* 0F45h: R/W (DATA) */
#define P_WSS_STATUS            (WSS_BASE + 6)  /* 0F46h: Read Only (STATUS) */
#define P_WSS_PIO               (WSS_BASE + 7)  /* 0F47h: R/W (PIO DATA) */

#if 0
#define P_WSS_INDEX		(WSS_BASE + 0)
#define P_WSS_DATA		(WSS_BASE + 1)
#define P_WSS_STATUS		(WSS_BASE + 2)
#define P_WSS_PIO		(WSS_BASE + 3)
#endif

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

#define WSS_IFACE_PEN           0x01
#define WSS_IFACE_CEN           0x02
#define WSS_IFACE_SDC           0x04

#define WSS_PIN_IEN             0x02

/*
 * Index register bits
 */
#define WSS_INDEX_INIT		0x80
#define WSS_INDEX_MCE		0x40
#define WSS_INDEX_TRD		0x20
#define WSS_INDEX_MASK		0x1f

/*
 * Playback format:
 *   8000Hz / 16-bit signed linear / mono
 */
#define WSS_FMT_8K_S16_MONO	0x40

/*
 * Interface Control register 09h
 *
 * Based on the information used for this port:
 *   bit0 = PEN  playback enable
 *   bit1 = PIEN playback interrupt enable
 */
#define WSS_IFACE_PEN		0x01
#define WSS_IFACE_PIEN		0x02

/*
 * PC-98 DMA Controller
 *
 * Same uPD71037/i8237A-compatible programming model as existing SB16/98 code.
 */
#define WSS_DMA_CH		1

#define DMA_PORT_SMASK		0x15        /* single mask */
#define DMA_PORT_MODE		0x17        /* mode */
#define DMA_PORT_CLRFF		0x19        /* clear byte pointer flip-flop */

static const int dma_port_addr[4]  = { 0x01, 0x05, 0x09, 0x0d };
static const int dma_port_count[4] = { 0x03, 0x07, 0x0b, 0x0f };
static const int dma_port_bank[4]  = { 0x27, 0x21, 0x23, 0x25 };

/*
 * PC-98 PIC
 *
 * INT5  -> vector 0Dh.
 */
//#define WSS_IRQ			12		/* IRQ12 (slave 4) */
//#define WSS_VECTOR		0x14		/* Vector for INT5 */

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
static void wss_wait_ready(void);
static void wss_write(int reg, int val);
static int wss_read(int reg);
static void wss_stop_codec(void);
static void wss_set_format_and_count(void);
static void wss_set_volume(void);
static void wss_ack_irq(void);
static void wss_start_codec(void);

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

	wss_ok = false;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		wave[n] = NULL;
		volume_q15[n] = 32767;
		finish[n] = false;
	}

	if (!wss_detect())
		return false;

	hal_log_info("Mate-X PCM: found a card.");

	/* init_sound()内、wss_detect()成功後 */
	{
		int cfg = inp(P_WSS_IRQ_CONFIG);   /* セットアップメニューの設定を反映した値 */
		static const int irq_tbl[8] = { -1, 3, 5, 10, 12, -1, -1, -1 };
		static const int dma_tbl[8] = { -1, 0, 1, 3, -1, -1, -1, -1 };

		hal_log_info("WSS: 0F40h = %02Xh", cfg & 0xff);

		wss_irq    = irq_tbl[(cfg >> 3) & 7];
		wss_dma_ch = dma_tbl[cfg & 7];

		if (wss_irq < 0 || wss_dma_ch < 0)
			return false;    /* 内蔵サウンド切り離し状態など */

		printf("Mate-X PCM: IRQ %d\n", wss_irq);
	}

	if (!alloc_dma_buffer()) {
		hal_log_info("Mate-X PCM: failed to allocate DMA buffer.");
		return false;
	}

	memset(dma_buf, 0, BUF_BYTES);

	/*
	 * Lock ISR-touched memory.
	 * Failure is acceptable when no VMM is active.
	 */
	dpmi_lock_region((void *)dma_buf, BUF_BYTES);
	dpmi_lock_region((void *)&cur_half, 4096);

	/*
	 * Stop codec first, then configure it.
	 */
	wss_stop_codec();
	wss_set_format_and_count();
	wss_set_volume();

	/*
	 * PC-9821 IRQ/DMA routing:
	 *   bit5-3 = 100b (INT5 / IRQ12), bit2-0 = 011b (DMA #3)
	 */
	outp(P_WSS_IRQ_CONFIG, 0x23);
	outp(WAIT_PORT, 0);

	/*
	 * Install interrupt handler before enabling playback IRQ.
	 */
	hook_irq();

	/*
	 * Program DMA controller for auto-init transfer over the full buffer.
	 */
	setup_dma();

	/*
	 * Start WSS playback.
	 */
	cur_half = 0;
	fill_half = 0;
	fill_pending = 0;

	wss_start_codec();

	wss_ok = true;

	printf("Sound enabled.\n");

	return true;
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

printf("FILL1\n");

	if (!fill_pending)
		return;

printf("FILL2\n");

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

printf("PLAY\n");
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
		outp(WAIT_PORT, 0); // バスウェイトを入れる
	}
}

static void
wss_write(
	int reg,
	int val)
{
	wss_wait_ready();
	outp(P_WSS_INDEX, reg & WSS_INDEX_MASK);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, val);
	outp(WAIT_PORT, 0);
}

static int
wss_read(
	int reg)
{
	wss_wait_ready();
	outp(P_WSS_INDEX, reg & WSS_INDEX_MASK);
	outp(WAIT_PORT, 0);
	return inp(P_WSS_DATA);
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
	 * PC-9821内蔵WSSの場合、下位ビットに「000100b (0x04)」が定義されています
	 * チップのマイナーリビジョンによって上位ビットが変わるため、マスクして比較します
	 */
        if ((id_val & 0x3F) != 0x04)
                return false; 

        /* Step 2: CS4231のレジスタが応答するか（提示されたR/Wテストのポート修正版） */
        wss_wait_ready();

        oldv = wss_read(WSS_REG_LEFT_OUTPUT);

        /* 0x80（ミュートビットなど）を書き込んで変化を見る */
        wss_write(WSS_REG_LEFT_OUTPUT, 0x80);
        v = wss_read(WSS_REG_LEFT_OUTPUT);

        /* 元の設定に戻す */
        wss_write(WSS_REG_LEFT_OUTPUT, oldv);

        if ((v & 0x80) != 0x80)
                return false;

        return true;
}

static void
wss_stop_codec(void)
{
    int pin;

    /*
     * Stop playback/capture.
     */
    wss_write(WSS_REG_IFACE, 0x00);

    /*
     * Disable codec IRQ output.
     */
    pin = wss_read(WSS_REG_PIN);
    wss_write(WSS_REG_PIN, pin & ~WSS_PIN_IEN);

    wss_ack_irq();
}

/*
 * Configure:
 *   8000Hz / 16-bit signed linear / mono
 *   playback block count = HALF_BYTES / 2 - 1
 *
 * Format and base count are programmed while MCE is enabled.
 */
static void
wss_set_format_and_count(void)
{
	unsigned count;

	count = WSS_BLOCK_COUNT;

	wss_wait_ready();

	/*
	 * MCE on, select playback format register.
	 */
	outp(P_WSS_INDEX, WSS_INDEX_MCE | WSS_REG_FORMAT);
	outp(WAIT_PORT, 0);

	/*
	 * Playback format:
	 *   20h = 8000Hz / 16-bit signed linear / mono
	 */
	outp(P_WSS_DATA, WSS_FMT_8K_S16_MONO);
	outp(WAIT_PORT, 0);

	/*
	 * Playback base count high.
	 */
	outp(P_WSS_INDEX, WSS_INDEX_MCE | WSS_REG_PCNT_H);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, (count >> 8) & 0xff);
	outp(WAIT_PORT, 0);

	/*
	 * Playback base count low.
	 */
	outp(P_WSS_INDEX, WSS_INDEX_MCE | WSS_REG_PCNT_L);
	outp(WAIT_PORT, 0);
	outp(P_WSS_DATA, count & 0xff);
	outp(WAIT_PORT, 0);

	/*
	 * MCE off.
	 */
	outp(P_WSS_INDEX, WSS_REG_FORMAT);
	outp(WAIT_PORT, 0);

	while (inp(P_WSS_INDEX) & WSS_INDEX_INIT)
		outp(WAIT_PORT, 0);

	while (wss_read(WSS_REG_TEST_INIT) & 0x10)
		;

	wss_wait_ready();
}

static void
wss_set_volume(void)
{
	/*
	 * Playback DAC attenuator:
	 *   bit7 = mute
	 *   lower bits = attenuation
	 *
	 * 00h is maximum volume on many CS4231-compatible codecs.
	 */
	wss_write(WSS_REG_LEFT_OUTPUT, 0x00);
	wss_write(WSS_REG_RIGHT_OUTPUT, 0x00);

	/*
	 * Optional: mute line input to avoid noise.
	 * If this causes trouble on V13, remove these two writes.
	 */
	wss_write(WSS_REG_LEFT_INPUT, 0x80);
	wss_write(WSS_REG_RIGHT_INPUT, 0x80);
}

static void
wss_ack_irq(void)
{
	/*
	 * WSS status port IRQ clear.
	 * Some implementations clear by writing 00h; a read before write
	 * is harmless and useful on several compatible devices.
	 */
	(void)inp(P_WSS_STATUS);
	outp(WAIT_PORT, 0);
	outp(P_WSS_STATUS, 0x00);
	outp(WAIT_PORT, 0);
}

static void
wss_start_codec(void)
{
	int pin;
	int iface;

	/*
	 * Enable codec IRQ output.
	 * Pin Control register 0Ah, bit1 = interrupt enable.
	 */
	pin = wss_read(WSS_REG_PIN);
	wss_write(WSS_REG_PIN, pin | WSS_PIN_IEN);

	/*
	 * Start playback.
	 *
	 * Important:
	 *   Do NOT set bit1 here as "playback IRQ enable".
	 *   On AD1848/CS4231-like codecs, bit1 is usually capture enable.
	 */
	iface = wss_read(WSS_REG_IFACE);
	wss_write(WSS_REG_IFACE, iface | WSS_IFACE_PEN);
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
 * ch3, auto-init, memory -> device.
 */
static void
setup_dma(void)
{
	int ch;

	ch = WSS_DMA_CH;

	_disable();

	/*
	 * Mask channel.
	 */
	outp(DMA_PORT_SMASK, 0x04 | ch);

	/*
	 * Mode:
	 *   single transfer
	 *   address increment
	 *   auto-init
	 *   read transfer, memory -> device
	 *
	 * This follows the existing PC-98 SB16 driver style.
	 */
	outp(DMA_PORT_MODE, 0x58 | ch);

	/*
	 * Clear byte pointer flip-flop.
	 */
	outp(DMA_PORT_CLRFF, 0);

	/*
	 * Address A0-A15.
	 */
	outp(dma_port_addr[ch], (int)(dma_phys & 0xff));
	outp(dma_port_addr[ch], (int)((dma_phys >> 8) & 0xff));

	/*
	 * Bank A16-A23.
	 */
	outp(dma_port_bank[ch], (int)((dma_phys >> 16) & 0xff));

	/*
	 * Count: full double buffer, bytes - 1.
	 */
	outp(DMA_PORT_CLRFF, 0);
	outp(dma_port_count[ch], (BUF_BYTES - 1) & 0xff);
	outp(dma_port_count[ch], ((BUF_BYTES - 1) >> 8) & 0xff);

	/*
	 * Unmask channel.
	 */
	outp(DMA_PORT_SMASK, ch);

	_enable();
}

static void
stop_dma(void)
{
	outp(DMA_PORT_SMASK, 0x04 | WSS_DMA_CH);
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
	    printf("Invalid IRQ number\n");
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

#if defined(__WATCOMC__)
/*
 * WSS interrupt handler.
 *
 * Important:
 *   - Keep this short.
 *   - Do not call hal_get_wave_samples() here.
 *   - Do not use DOS services here.
 */
static void __interrupt __far
wss_isr(void)
{
    /*
     * Acknowledge WSS interrupt first.
     */
    wss_ack_irq();

    if (fill_pending)
        memset(dma_buf + fill_half * HALF_BYTES, 0, HALF_BYTES);

    fill_half = cur_half;
    cur_half ^= 1;
    fill_pending = 1;

    /*
     * IRQ12 is on the slave PIC.
     * Send EOI to slave first, then master.
     */
    outp(PIC1_CMD, PIC_EOI);
    outp(PIC0_CMD, PIC_EOI);
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
