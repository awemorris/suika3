/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Trident TGUI96xx display driver for NEC PC-98 (DOS/4G).
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
 * ===========================================================================
 * PC-98 built-in Trident graphics: analysis summary
 * ===========================================================================
 *
 * Target machines (desktops with the on-board Trident):
 *
 *   ValueStar V13/V16 (M7 etc.)   Trident TGUI9680XGi, VRAM 2MB
 *   MATE R Ra43/Ra33/Ra266/Ra300  Trident TGUI9682XGi, VRAM 2MB
 *
 * The single most important discovery: XFree86 3.3.x shipped a
 * dedicated PC-98 glue driver for exactly these built-ins,
 *   xc/programs/Xserver/hw/xfree98/vga256/drivers/trident/pc98_tgui.c
 * ("NEC Trident TGUi96xx(PCI Bus Type)"), written by people with the
 * real V13/V16.  Everything below that is not from the generic
 * Trident literature comes from that file.
 *
 * ---------------------------------------------------------------------------
 * A. Bus attachment - this is a PCI chip, not a WAB one
 * ---------------------------------------------------------------------------
 *  - The on-board TGUI96xx is a PCI device: vendor 1023h, device
 *    9660h.  The chip generation is in the PCI revision ID, mirrored
 *    in SR09:  00h = TGUI9660, 01h = TGUI9680, 10h = ProVidia 9682,
 *    21h = ProVidia 9685.  (V16 -> 01h expected, Ra43 -> 10h.)
 *  - BAR0 is the 4MB(+) memory range; NEC's ITF assigns 20000000h
 *    or 21000000h (both candidates are hardcoded in pc98_tgui.c; we
 *    read the live value from configuration space instead).
 *    ** The linear framebuffer is at BAR0 + 0 ** (XF98 sets
 *    ChipLinearBase = vgaPCIInfo->MemBase), the memory-mapped
 *    graphics-engine block is at BAR0 + 400000h (unused here).
 *  - VGA registers decode at the NATIVE 3C0h/3C4h/3CEh/3D4h I/O
 *    block, NOT at the PC-98 WAB relocation 0CA0h/0DA4h.  This
 *    finally explains the observation recorded in the Cirrus driver:
 *    the V16 answers the WAB ID probe at 0FAAh with 5Bh (inside the
 *    supposed Cirrus range) and its relay registers work, but the
 *    VGA file at 0CA0h floats.  The Trident was at 3C0h all along.
 *  - Trident also decodes "extended" I/O aliases above the VGA
 *    block, all confirmed in use by pc98_tgui.c on the real
 *    machines:
 *        43C8h/43C9h  VCLK PLL     (n / m,k; see below)
 *        43C6h/43C7h  MCLK PLL
 *        83C8h/83C6h  "SYNCDAC" index/data (NEC sync/output glue,
 *                     register 04h holds the sync output enables)
 *
 * ---------------------------------------------------------------------------
 * B. Wakeup (PC-98 has no VGA BIOS; the chip has never been POSTed)
 * ---------------------------------------------------------------------------
 * Per pc98_tgui.c VideoEnable(), operating in the OLD register mode:
 *  - SR0E |= 20h selects "Configuration Port 1" at SR0C; SR0C bit4
 *    then tells which wakeup scheme the board straps:
 *      bit4 = 1:  outp(94h, 00h); outp(102h, 01h); outp(94h, 20h);
 *                 3C3h |= 01h            (the NEC/PC-98 wiring)
 *      bit4 = 0:  outp(46E8h, 10h); outp(102h, 01h); outp(46E8h, 08h)
 *                 (the AT-style setup ports)
 *    Note these are the un-relocated 94h/102h - the Trident world's
 *    "POS" ports, decoded by the chip itself, unrelated to the
 *    0904h/FF82h pair the Cirrus WAB machines use.
 *  - MISC is OR'd with C3h beforehand (RAM enable, color I/O).
 *
 * ---------------------------------------------------------------------------
 * C. Video output relay (per pc98_tgui.c crtswNEC96xx/crtswTGUiGen)
 * ---------------------------------------------------------------------------
 * Switching to the accelerator is an ordered dance:
 *   1. GDC side off:  outp(68h,0Eh); outp(6Ah,07h); outp(6Ah,8Fh);
 *      outp(6Ah,06h); and 9A8h=01h (force 31kHz) if it was 24kHz.
 *   2. Trident sync path on: CR23 &= ~20h; CR29 |= 04h;
 *      SYNCDAC[04h] |= 06h; wait 1ms; |= 08h; GR23 &= ~03h;
 *      SYNCDAC[04h] |= 01h; SR01 &= ~10h.
 *   3. The relay latch: outp(0FACh, 02h).
 * Back to the GDC runs the exact mirror (0FACh=00h first, then the
 * sync teardown, then the GDC side on with 6Ah=8Eh, 68h=0Fh).
 * The two-stage WAB interface at 0FAAh/0FABh is NOT used on the
 * 96xx machines (XF98 only uses it for the Cyber9320 one); on the
 * V16 it merely answers the ID 5Bh.  We read it for the log only.
 *
 * ---------------------------------------------------------------------------
 * D. Chip programming (generic TGUI96xx knowledge)
 * ---------------------------------------------------------------------------
 *  - Old/new register modes: READING SR0B selects the new mode and
 *    returns the chip ID (D3h = 9660 family); WRITING SR0B selects
 *    the old mode.  SR09 = revision (see A.).
 *  - SR0E in the new mode: bit7 = enable extension registers; low
 *    bits = 64KB bank.  ** Writes to new-mode SR0E invert bit1 on
 *    the way in ** (the classic Trident detection quirk: write 00h,
 *    read back 02h), so to store value V one must write V ^ 02h.
 *  - VCLK PLL at 43C8h/43C9h (TGUI9660/9680/9682 use the OLD clock
 *    layout; only 9685+ has the new one):
 *        f = 14.31818MHz * (N + 8) / ((M + 2) * 2^K)
 *        43C8h = N[6:0] | (M[0] << 7)
 *        43C9h = M[4:1] | (K << 4)
 *    (Verified in three independent code bases: Linux tridentfb,
 *    XFree86/Xorg trident, PCem vid_tgui9440.c.)  MCLK at 43C6h/
 *    43C7h uses the layout of pc98_tgui.c GetMCLK():
 *        43C6h = N[4:0] << 3 | M[2:0],  43C7h = K << 1 | N[5].
 *  - Depth selection: CR38 (Pixel Bus) 00h/05h/29h for 8/16/24bpp,
 *    plus the hidden DAC register (read 3C8h once, 3C6h four times,
 *    the next 3C6h access is the hidden register): 00h / 30h (565) /
 *    D0h.  No clock doubling/tripling on 96xx at 16/24bpp.
 *  - CR21 bit5 enables the linear aperture; on PCI parts the base
 *    comes from BAR0, no address bits needed in CR21.
 *  - CR1E = 80h (extended memory access), CR2A |= 40h (32-bit bus),
 *    GR0F = (old & F0h) | 12h, CR29 bits5:4 = pitch bits 9:8.
 *  - VRAM size in CR1F low nibble: 1=512KB 3=1MB 7=2MB Fh=4MB.
 *  - Board tuning pc98_tgui.c ChipInit() performs on these exact
 *    machines (DRAM/FIFO/latency values, MCLK=80MHz nominal,
 *    SYNCDAC[0]=01h, GR2Fh=20h, GR5Eh=88h, GR5Fh=48h):
 *    replayed here 1:1, but guarded by STRATO_TRIDENT_NOTUNE=1 for
 *    bring-up in case the ITF already programmed saner values.
 *    (The MCLK encodings in XF98 and Xorg disagree about the
 *    resulting frequency; the register VALUE 53h/00h is what XF98
 *    proved on the V13/V16, so the value is replayed verbatim
 *    rather than recomputed.)
 *
 * ---------------------------------------------------------------------------
 * E. Open items (to verify on real hardware - the driver logs all of it)
 * ---------------------------------------------------------------------------
 *  - Whether the Ra43 (TGUI9682XGi, 1999) keeps the V16's wakeup and
 *    relay wiring.  Same family and same NEC design lineage, but XF98
 *    predates it.  The 0FACh/SR0C/WAB-ID readbacks in the log will
 *    tell.
 *  - The 24bpp VRAM byte order is assumed B,G,R (little-endian
 *    convention, like the Cirrus).  If red/blue come out swapped on
 *    the real DAC, conv_row24() is the one place to touch.
 *  - Whether MCLK/DRAM tuning is required or the power-on defaults
 *    suffice (STRATO_TRIDENT_NOTUNE=1 to compare).
 *  - 800x600x24 would sit exactly at the 40MHz limit XF98 imposes on
 *    24bpp (Bpp_Clocks[3] = 40000); NEC never shipped that mode, so
 *    it is rejected here.
 *
 * ---------------------------------------------------------------------------
 * F. Ra43 real-hardware findings (log of 2026-07, first probe build)
 * ---------------------------------------------------------------------------
 * The first build assumed the V13/V16 model: legacy VGA I/O at the
 * native 3C0h block.  The Ra43 log disproved that:
 *   - PCI 0:8.0 = 1023:9660, BAR0 = 20000000h (4MB decode) - fine;
 *   - the PCI command register was 0002h as shipped: memory decode
 *     ON, ** I/O decode OFF **;
 *   - every 3C0h-block read returned FFh even after enabling I/O
 *     decode and running the wakeup.
 * Conclusion: on the Ra generation NEC drives the chip without
 * legacy VGA I/O at all.  The supported way in is the one Linux
 * tridentfb and Xorg trident use on such boards:
 *   ** BAR1 is a 64KB MMIO block in which the ENTIRE register file
 *   appears memory-mapped at its port offsets ** - SR at +3C4h/3C5h,
 *   CRTC at +3D4h/3D5h, the DAC at +3C6h..3C9h, and even the clock
 *   PLL at +43C8h/43C9h (tridentfb writes it through t_outb) - gated
 *   by CR39 bit0 (which such boards strap on at power-up; Xorg calls
 *   the configuration "MMIOonly").
 * This driver therefore probes in this order: PIO at 3C0h first
 * (keeps the V13/V16 path alive), then BAR1 MMIO, then - only if
 * both are dead - the blind wakeup sequences followed by a PIO
 * retest.  The AT-style 46E8h wakeup is tried before the 94h one:
 * on a machine whose Trident does not claim port 94h, writing it
 * hits the PC-98 FDC mode register, so 94h is the last resort.
 * Note the Ra43 also reports the odd PCI revision D3h (the SR0B
 * chip-ID value); the chip name is therefore decided from SR09
 * once register access works, not from the PCI revision.
 *
 * ---------------------------------------------------------------------------
 * G. NEC's own NT 4.0 miniport (trident.sys, linked 1996-10-15)
 * ---------------------------------------------------------------------------
 * Disassembling NEC's PC-98 NT4 trident.sys settled the remaining
 * questions with first-party answers:
 *  - It imports only VideoPort PORT-I/O helpers, yet after boot it
 *    touches VGA registers exclusively through `mov [base+port]`
 *    memory accesses: at init it reads SR0B, sets SR0E |= 80h and
 *    ** CR39 |= 01h via PIO, then VideoPortGetDeviceBase()-maps a
 *    64KB memory range and stores it as the register base ** -
 *    exactly the BAR1 MMIO model this driver fell back to for the
 *    Ra43.  NEC used MMIO for everything from day one; the offsets
 *    it adds are the raw port numbers (3C4h..3DAh, 43C6h..43C9h,
 *    83C6h/83C8h and even 3C3h/46E8h).
 *  - Every SR0E write in the binary pre-XORs bit1 (`or 80h, xor 02h`)
 *    - third independent confirmation of the invert quirk.
 *  - The NEC sync glue lives in Trident extension registers:
 *    GR20h-2Ah (board init), GR2Ch (relay sync path), GR30h/GR33h
 *    and GR24h (DAC/sync power sequencing, stepped one bit at a
 *    time with WHOLE-VSYNC-FRAME delays), GR40h-46h/50h-53h (sync
 *    mode per display mode), GR42h bit7 = monitor sense, GR5Ah/5Bh
 *    used as scratch, and a SHADOW CRTC bank at I/O 3A4h/3A5h
 *    (gated by GR30h bit6) that carries GDC-like 640x480 timings.
 *    XF98's "SYNCDAC" (83C8h) is also written (regs 00h/04h/08h/
 *    09h/37h/38h) but only during board init.
 *  - The relay proper: machines are typed; the 96xx desktops write
 *    ** 0FACh = 03h ** to switch to the accelerator (not the 02h
 *    XF98 used) and 00h to switch back; TGUI9440-generation WAB
 *    machines use the two-stage 0FAAh/0FABh (reg 03h = 03h/00h)
 *    instead; and unless the monitor code is 93h the driver also
 *    flips ** 0FAAh reg 84h |= / &= ~11h **.  The dance is
 *    bracketed by writes to an indexed 16-bit interface at
 *    ** 8F0h/8F2h ** (index 52h bit7, index 60h bit4) that neither
 *    XF98 nor any public source mentions.
 *  - NEC's own state save covers CRTC 00h-50h and GR 00h-5Fh plus
 *    the clocks - this driver's save ranges were widened to match.
 *  - Its access-range table holds the framebuffer at the FIXED
 *    physical address 73000000h (4MB) - it never reads BAR0.  The
 *    Ra43 ITF's CR21 value C7h decodes to exactly that address as
 *    (bits3:0 << 28) | (bits7:6 << 24), which identifies CR21 on
 *    this wiring as the linear-window placement register (bit5 =
 *    enable).  Field-confirmed: BAR0 reads junk and drops writes,
 *    while the CR21-decoded window is where VRAM answers.
 * The relay/glue tables and sequences extracted from the binary are
 * replayed below (tg_nt_* and the tg_glue_* tables).  The XF98
 * variant is kept selectable via STRATO_TRIDENT_RELAY=xf98.
 *
 * References:
 *  - XFree86 3.3.6 xfree98 pc98_tgui.c / pc98_tgui.h (primary:
 *    PCI base, native VGA I/O, wakeup, relay dance, SYNCDAC, board
 *    tuning, per-depth clock limits)
 *  - XFree86 3.3.6 tvga8900 t89_driver.c (PC98_TGUI ifdefs: linear
 *    base = PCI MemBase, PixelBusReg/CommandReg per depth)
 *  - Linux tridentfb.c (BIOS-less mode-set order, CRTC overflow
 *    layout CR27/CR2B, VCLK search, hidden DAC access)
 *  - Xorg xf86-video-trident trident_dac.c/trident_pll.c (restore
 *    ordering, SR0E XOR quirk, old/new clock layouts)
 *  - PCem vid_tgui9440.c via NP2kai wab/tgui9680.c (SR0E XOR
 *    emulation, 43C8h/43C9h decode, hidden DAC state machine)
 *  - VGADOC TRIDENT.TXT (register names)
 *  - NEC PC-9821V16/M7 official spec sheet (TGUI9680XGi, VRAM 2MB,
 *    640x480 16.77M / 800x600 65K / 1024x768 65K / 1280x1024 256)
 *
 * ===========================================================================
 * Driver design
 * ===========================================================================
 *  - trident_init_disp(mode, bpp) scans PCI for vendor 1023h.  The
 *    9660-family desktops are driven; Cyber laptops read out but are
 *    declined (PCI-BAR 98NOTE machines are out of scope for now).
 *    No PCI Trident -> false, so the main code can try other chips.
 *  - Register access is abstracted (tg_inb/tg_outb): legacy PIO at
 *    3C0h where it answers (V13/V16), BAR1 MMIO where it doesn't
 *    (Ra43) - see section F.
 *    The probe is non-destructive until the chip is positively
 *    fingerprinted (SR0B chip ID D3h + the SR0E write-0-reads-2
 *    signature); state is saved before anything is programmed and
 *    restored on cleanup.
 *  - Aperture-only: frames are written straight into the linear
 *    framebuffer at BAR0.  The graphics engine is never enabled.
 *  - Modes: 640x480 (8/16/24bpp) and 800x600 (8/16bpp).  bpp == -1
 *    picks 24bpp when it fits the VRAM.
 *  - STRATO_TRIDENT_RELAY=min|nt|xf98 selects the relay sequence.
 *    Default is "min": flip only the 0FACh output mux and the GDC
 *    display element, leaving every ITF-configured NEC glue
 *    register alone (the Ra43 field test showed the full 1996-era
 *    NT sequence kills the sync there, while the ITF state works).
 *  - STRATO_TRIDENT_FAC=n overrides the 0FACh accel value
 *    (default 3, the NT driver's choice; XF98 used 2).
 *  - STRATO_TRIDENT_TUNE=1 opts in to the XF98 DRAM/MCLK tuning +
 *    NEC board glue (default off: the ITF already tuned the board).
 *  - STRATO_TRIDENT_NOTUNE=1 forces the tuning off (compat).
 *  - Verbose by design, like the Cirrus driver: this driver only
 *    runs when the user passes the -24 option, and the register
 *    dumps have repeatedly been the only way to debug real hardware.
 */

/* HAL */
#include <strato/strato.h>	/* Public Interface */
#include "98disp.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DOS */
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include <stdint.h>

/*
 * Requested geometry per DISP_* selector.
 */
static const struct disp_geo {
	int w, h;
} disp_geo[] = {
	{  640,  480 },		/* DISP_640X480 */
	{  800,  600 },		/* DISP_800X600 */
	{ 1024,  768 },		/* DISP_1024X768 */
	{ 1280, 1024 }		/* DISP_1280X1024 */
};

static struct trident_disp {
	bool active;
	const char *chip_name;

	/* Screen geometry. */
	int scr_w;		/* 640 or 800 */
	int scr_h;		/* 480 or 600 */
	int bpp;		/* 24, 16 or 8 */
	uint32_t pitch;		/* bytes per scanline */

	/*
	 * VGA register file.  Expected at the native block on these
	 * machines; kept relocatable anyway so a surprise on real
	 * hardware only needs a table entry.  io_3d4/io_3da follow
	 * MISC bit0 between the color and mono blocks.
	 */
	uint16_t io_3c0;
	uint16_t io_3d4;
	uint16_t io_3da;
	uint16_t io_3d4_col, io_3da_col;
	uint16_t io_3d4_mono, io_3da_mono;
	uint16_t io_vclk;	/* 43C8h: VCLK PLL (43C6h = MCLK) */
	uint16_t io_sdac;	/* 83C8h: SYNCDAC index (83C6h = data) */

	/*
	 * Register access path.  When use_mmio is set, every VGA
	 * register access goes through the BAR1 MMIO block (the port
	 * number doubles as the offset inside the block); the PC-98
	 * platform ports (0FACh, 68h, 6Ah, 9A8h, 5Fh, the wakeup
	 * ports) are always real I/O.
	 */
	bool use_mmio;
	int aper_width;		/* 4 = dwords OK, 1 = byte-only lane */
	volatile uint8_t *mmio;	/* mapped BAR1, 64KB */
	uint32_t mmio_phys;

	/* VRAM aperture (always linear on this driver). */
	uint8_t *fb;
	uint32_t fb_phys;
	uint32_t vram_size;

	/* Chip information. */
	uint8_t wab_id;		/* raw 0FAAh register 00h readout (info) */
	uint8_t chip_id;	/* SR0B readback (D3h = 9660 family) */
	uint8_t chip_rev;	/* SR09 (00=9660 01=9680 10h=9682) */

	/* 9A8h horizontal sync rate as found (1 = 31kHz). */
	int hsync31;
} tdisp;

/* Blit placement (centering + clip against the screen). */
static int ofs_x, ofs_y;
static int draw_w, draw_h;

extern struct hal_image *back_image;
extern int game_width;
extern int game_height;

/* Frame presentation. */
static void trident_flip_vram(void);
static void conv_row24(uint8_t *dst, const uint32_t *src, int n);
static void conv_row16(uint8_t *dst, const uint32_t *src, int n);
static void conv_row8(uint8_t *dst, const uint32_t *src, int n);

/* Low-level register access. */
static int tg_inb(unsigned port);
static void tg_outb(unsigned port, int val);
static void tg_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
			  uint16_t d4m, uint16_t dam);
static void tg_select_crtc(int misc);
static void tg_seq_write(int reg, int val);
static int tg_seq_read(int reg);
static void tg_gfx_write(int reg, int val);
static int tg_gfx_read(int reg);
static void tg_crtc_write(int reg, int val);
static int tg_crtc_read(int reg);
static void tg_attr_write(int reg, int val);
static int tg_attr_read(int reg);
static void tg_misc_write(int val);
static int tg_misc_read(void);
static void tg_hidden_dac_write(int val);
static int tg_hidden_dac_read(void);
static void tg_sdac_write(int reg, int val);
static int tg_sdac_read(int reg);
static void tg_sw_old(void);
static void tg_sw_new(void);
static void tg_wait_ms(int ms);

/* Detection / bring-up. */
static bool tg_pci_find(void);
static void tg_pci_dump_config(void);
static void tg_pci_dump_header(int bus, int dev, int fn);
static bool tg_regs_alive(void);
static bool tg_probe_access_path(void);
static void tg_wakeup_strapped(void);
static void tg_wakeup_blind_at(void);
static void tg_wakeup_blind_pc98(void);
static bool tg_fingerprint(void);
static uint32_t tg_vram_size(void);
static uint32_t tg_vram_probe(void);
static bool tg_aperture_fix(void);
static void tg_board_tune(void);
static void tg_dump_regs(const char *tag);

/* Mode set. */
static void tg_modeset(void);
static void tg_load_palette(void);
static int tg_resolve_bpp(int req, int w, int h, uint32_t vram);

/* Relay. */
static void tg_relay_to_accel(void);
static void tg_relay_to_gdc(void);
static void tg_relay_to_accel_xf98(void);
static void tg_relay_to_gdc_xf98(void);
static void tg_nt_relay_to_accel(void);
static void tg_nt_relay_to_gdc(void);
static void tg_nt_sync_on(void);
static void tg_nt_sync_off(void);
static int tg_nt_monitor_sense(void);
static void tg_8f0_rmw(int idx, int and_mask, int or_bits);
static void tg_wait_frames(int n);
static void tg_apply_triplets(const uint8_t *t, int n, const char *tag);
static bool tg_use_xf98_relay(void);
static int tg_relay_policy(void);
static int tg_fac_value(void);
static void tg_min_relay_to_accel(void);
static void tg_min_relay_to_gdc(void);

#define TG_RELAY_MIN	0
#define TG_RELAY_NT	1
#define TG_RELAY_XF98	2

/* Board init (applied once at bring-up; NT table @13ab8h). */
static const uint8_t tg_glue_board[] = {
	0x94, 0xe0, 0xe0,	/* GR24 |= E0h		*/
	0x95, 0xff, 0x00,	/* GR25  = 00h		*/
	0x94, 0xff, 0x00,	/* GR24  = 00h		*/
	0x92, 0xff, 0x26,	/* GR22  = 26h		*/
	0x96, 0xff, 0x00,	/* GR26  = 00h		*/
	0x9a, 0xff, 0x01,	/* GR2A  = 01h		*/
	0x91, 0xff, 0x00,	/* GR21  = 00h		*/
	0xf2, 0x04, 0x0f,	/* SDAC[04h] = 0Fh	*/
	0x90, 0xff, 0x05,	/* GR20  = 05h		*/
	0xf2, 0x37, 0x33,	/* SDAC[37h] = 33h	*/
	0xf2, 0x38, 0x04,	/* SDAC[38h] = 04h	*/
	0x93, 0x08, 0x08,	/* GR23 |= 08h		*/
	0xf2, 0x08, 0x73,	/* SDAC[08h] = 73h	*/
	0xf2, 0x09, 0x86	/* SDAC[09h] = 86h	*/
};
/*
 * 640x480 sync-glue for the 96xx desktops (NT table @13c05h, the
 * machine-type-27h variant): sync mode regs GR30/40/42/43/50-53,
 * the 3A4h shadow CRTC loaded with 640x480 timings, CR11 protect,
 * CR29 relay bits.  800x600 has no glue table in the NT driver.
 */
static const uint8_t tg_glue_640x480[] = {
	0xa0, 0x30, 0x30,	/* GR30 bits5:4 = 11b	*/
	0xb0, 0xff, 0x12,	/* GR40  = 12h		*/
	0xb2, 0xff, 0xb4,	/* GR42  = B4h		*/
	0xb3, 0xff, 0x88,	/* GR43  = 88h		*/
	0xc0, 0xff, 0x59,	/* GR50  = 59h		*/
	0xc1, 0xff, 0xa7,	/* GR51  = A7h		*/
	0xc2, 0xcf, 0x93,	/* GR52  = 93h (mask CFh) */
	0xc3, 0xff, 0xa3,	/* GR53  = A3h		*/
	0x61, 0x80, 0x00,	/* CRB11h bit7 = 0 (unlock) */
	0x50, 0xff, 0x5f,	/* CRB00h = 5Fh		*/
	0x53, 0xff, 0x82,	/* CRB03h = 82h		*/
	0x54, 0xff, 0x53,	/* CRB04h = 53h		*/
	0x55, 0x3f, 0x9f,	/* CRB05h = 9Fh (mask 3Fh) */
	0x56, 0xff, 0x0b,	/* CRB06h = 0Bh		*/
	0x57, 0xa5, 0x3e,	/* CRB07h = 3Eh (mask A5h) */
	0x60, 0xff, 0xe5,	/* CRB10h = E5h		*/
	0x61, 0xff, 0xa7,	/* CRB11h = A7h		*/
	0x66, 0xff, 0x04,	/* CRB16h = 04h		*/
	0x11, 0x80, 0xa7,	/* CR11 bit7 = 1	*/
	0x29, 0xe0, 0xa0	/* CR29 bits7:5 = 101b	*/
};
/* Sync power-up, one bit per frame (NT setD @13b1bh). */
static const uint8_t tg_glue_sync_on[] = {
	0x94, 0x04, 0xef,	/* GR24 |= 04h		*/
	0xf0, 0x00, 0x01,	/* wait 1 frame		*/
	0x94, 0x02, 0xef,	/* GR24 |= 02h		*/
	0xf0, 0x00, 0x03,	/* wait 3 frames	*/
	0x94, 0x01, 0xef,	/* GR24 |= 01h		*/
	0xf0, 0x00, 0x01,	/* wait 1 frame		*/
	0x94, 0x08, 0xef,	/* GR24 |= 08h		*/
	0xa3, 0x10, 0x10,	/* GR33 |= 10h		*/
	0xa0, 0x40, 0x00	/* GR30 &= ~40h		*/
};
/* Sync power-down (NT setC @13b36h). */
static const uint8_t tg_glue_sync_off[] = {
	0x94, 0xff, 0xef,	/* GR24  = EFh		*/
	0x95, 0xff, 0xef,	/* GR25  = EFh		*/
	0x94, 0x08, 0xe0,	/* GR24 &= ~08h		*/
	0xf0, 0x00, 0x01,	/* wait 1 frame		*/
	0x94, 0x01, 0xe0,	/* GR24 &= ~01h		*/
	0xf0, 0x00, 0x03,	/* wait 3 frames	*/
	0x94, 0x02, 0xe0,	/* GR24 &= ~02h		*/
	0xf0, 0x00, 0x01,	/* wait 1 frame		*/
	0x94, 0x04, 0xe0,	/* GR24 &= ~04h		*/
	0xa3, 0x10, 0x00	/* GR33 &= ~10h		*/
};


/* State save/restore. */
static void tg_save_state(void);
static void tg_restore_state(void);

/*
 * CPU cache control (CR0.CD), for the STRATO_TRIDENT_NOCACHE=1
 * experiment: rules the CPU cache/write-buffer path in or out when
 * the aperture readback misbehaves.  DOS/4GW runs the client in
 * ring 0, so CR0 is directly writable.
 */
static void tg_cache_disable(void);
static void tg_cache_enable(void);
static bool tg_cache_disabled = false;

#ifdef __WATCOMC__
/*
 * Raw opcode bytes so the inline assembler's CPU level doesn't
 * matter (wbinvd is 486+):
 *   0F 09           wbinvd
 *   0F 20 C0        mov eax, cr0
 *   0D 00 00 00 40  or  eax, 40000000h   (CD)
 *   25 FF FF FF 9F  and eax, 9FFFFFFFh   (~CD & ~NW)
 *   0F 22 C0        mov cr0, eax
 */
void tg_cache_disable_asm(void);
#pragma aux tg_cache_disable_asm =		\
	0x0f 0x09				\
	0x0f 0x20 0xc0				\
	0x0d 0x00 0x00 0x00 0x40		\
	0x0f 0x22 0xc0				\
	0x0f 0x09				\
	modify [eax];
void tg_cache_enable_asm(void);
#pragma aux tg_cache_enable_asm =		\
	0x0f 0x20 0xc0				\
	0x25 0xff 0xff 0xff 0x9f		\
	0x0f 0x22 0xc0				\
	modify [eax];
#else
static void tg_cache_disable_asm(void) { }
static void tg_cache_enable_asm(void) { }
#endif

static void
tg_cache_disable(void)
{
	tg_cache_disable_asm();
	tg_cache_disabled = true;
	hal_log_info("TRIDENT: CPU cache disabled (CR0.CD=1 + "
		     "WBINVD) for this session.");
}

static void
tg_cache_enable(void)
{
	if (!tg_cache_disabled)
		return;
	tg_cache_enable_asm();
	tg_cache_disabled = false;
	hal_log_info("TRIDENT: CPU cache re-enabled.");
}

/* PCI access (defined in the PCI detection section). */
static uint32_t pci_read32(int bus, int dev, int fn, int reg);
static void pci_write32(int bus, int dev, int fn, int reg,
			uint32_t val);
static int pci_bus, pci_dev, pci_fn;
static uint32_t sv_cfg14 = 0xffffffffUL;	/* PCI cfg 14h as found */

/* Misc. */
static void *tg_map_physical(uint32_t phys, uint32_t size);

/*****************************************************************************/
/* Public interface                                                          */
/*****************************************************************************/

bool
trident_init_disp(int mode, int bpp)
{
	int w, h;
	uint32_t need;

	if (mode < DISP_640X480 || mode > DISP_1280X1024) {
		hal_log_info("TRIDENT: invalid mode selector %d.", mode);
		return false;
	}
	if (bpp != -1 && bpp != 8 && bpp != 16 && bpp != 24) {
		hal_log_info("TRIDENT: invalid depth %d (8/16/24 or -1).",
			     bpp);
		return false;
	}

	memset(&tdisp, 0, sizeof(tdisp));

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	hal_log_info("TRIDENT: probing; requested %dx%d, depth %d "
		     "(-1 = auto).", w, h, bpp);

	/*
	 * The WAB machine ID, for the log only.  The 96xx machines do
	 * not use the two-stage interface (the V16 answers 5Bh here
	 * while its VGA file lives at the native block); the value is
	 * still the quickest machine fingerprint we have.
	 */
	outp(0x0faa, 0x00);
	tdisp.wab_id = (uint8_t)inp(0x0fab);
	hal_log_info("TRIDENT: WAB ID (0FAAh reg 00h) reads %02Xh "
		     "(informational only).", tdisp.wab_id);

	/*
	 * The on-board TGUI96xx is a PCI device on every known
	 * machine (V13/V16, Ra series).  No PCI Trident, no driver.
	 */
	if (!tg_pci_find())
		return false;

	if (mode != DISP_640X480 && mode != DISP_800X600) {
		hal_log_info("TRIDENT: %dx%d not supported "
			     "(640x480 / 800x600 only).", w, h);
		return false;
	}

	/*
	 * Find a working register access path (PIO on the V13/V16
	 * generation, BAR1 MMIO on the Ra generation), then prove the
	 * chip's identity before programming anything.
	 */
	tg_set_iobase(0x03c0, 0x03d4, 0x03da, 0x03b4, 0x03ba);
	if (!tg_probe_access_path()) {
		tg_pci_dump_config();
		return false;
	}
	if (!tg_fingerprint())
		return false;

	/* Optional cache experiment before any aperture traffic. */
	if (getenv("STRATO_TRIDENT_NOCACHE") != NULL)
		tg_cache_disable();

	/* Save the horizontal sync rate of the GDC side (9A8h). */
	tdisp.hsync31 = inp(0x09a8) & 0x01;
	hal_log_info("TRIDENT: GDC horizontal sync is %skHz (9A8h).",
		     tdisp.hsync31 ? "31.5" : "24.8");

	/* VRAM size from CR1F, then resolve the depth. */
	tdisp.vram_size = tg_vram_size();
	tdisp.bpp = tg_resolve_bpp(bpp, w, h, tdisp.vram_size);
	if (tdisp.bpp < 0)
		return false;
	if (mode == DISP_800X600 && tdisp.bpp == 24) {
		hal_log_info("TRIDENT: 800x600 at 24bpp would need the "
			     "full 40MHz dot clock at 24bpp - beyond "
			     "what NEC ever shipped; refusing.");
		return false;
	}

	tdisp.scr_w = w;
	tdisp.scr_h = h;
	tdisp.aper_width = 4;
	tdisp.pitch = (uint32_t)w * (uint32_t)(tdisp.bpp / 8);
	need = (uint32_t)h * tdisp.pitch;
	if (need > tdisp.vram_size) {
		hal_log_info("TRIDENT: %dx%d %dbpp needs %luKB but only "
			     "%luKB VRAM.", w, h, tdisp.bpp,
			     (unsigned long)(need >> 10),
			     (unsigned long)(tdisp.vram_size >> 10));
		return false;
	}

	/* Map the linear framebuffer (BAR0 + 0). */
	tdisp.fb = (uint8_t *)tg_map_physical(tdisp.fb_phys,
					      tdisp.vram_size);
	if (tdisp.fb == NULL) {
		hal_log_info("TRIDENT: can't map the framebuffer at "
			     "%08lXh.", (unsigned long)tdisp.fb_phys);
		return false;
	}

	/* From here on we modify the chip: keep an exact undo image. */
	tg_save_state();
	tg_dump_regs("as found");

	/*
	 * Board bring-up (XF98 ChipInit + NEC board glue).  The Ra43
	 * ITF already leaves the board tuned (its own MCLK, DRAM
	 * timings and glue state), and overwriting that config with
	 * values from older machines proved harmful, so this now
	 * defaults OFF.  STRATO_TRIDENT_TUNE=1 opts in;
	 * STRATO_TRIDENT_NOTUNE=1 still forces it off.
	 */
	if (getenv("STRATO_TRIDENT_TUNE") != NULL &&
	    getenv("STRATO_TRIDENT_NOTUNE") == NULL)
		tg_board_tune();
	else
		hal_log_info("TRIDENT: keeping the ITF board tuning "
			     "(MCLK/DRAM/glue untouched; "
			     "STRATO_TRIDENT_TUNE=1 to override).");

	/* Full mode set (leaves the screen blanked). */
	tg_modeset();
	tg_dump_regs("after mode set");

	/*
	 * NEC sync glue (sync mode regs + the 3A4h shadow CRTC
	 * timings): only with the full NT relay.  On the Ra43 the
	 * ITF-provided glue state is already correct and loading
	 * the 1996 tables broke the sync, so the minimal path
	 * leaves all of it alone.  The NT driver carries no glue
	 * table for 800x600 either way.
	 */
	if (tg_relay_policy() == TG_RELAY_NT && tdisp.scr_w == 640)
		tg_apply_triplets(tg_glue_640x480,
				  sizeof(tg_glue_640x480) / 3,
				  "640x480 NEC sync glue");

	/*
	 * Switch the video output relay to the accelerator with the
	 * screen still blanked (SR01 bit5 is set by the mode set):
	 * sync runs, video is dark, and the aperture check plus the
	 * VRAM clear below happen out of sight.
	 */
	tg_relay_to_accel();

	/*
	 * Verify the linear aperture actually reaches VRAM - the
	 * Ra43 field test showed decode problems here - trying CR21
	 * variants if the first attempt fails.  Continue either way
	 * (a garbage picture with working sync still tells us more
	 * than a dead screen).
	 */
	if (tg_aperture_fix()) {
		uint32_t real;

		/* With the aperture live, measure the real VRAM. */
		real = tg_vram_probe();
		if (real != 0 && real != tdisp.vram_size) {
			hal_log_info("TRIDENT: real VRAM measures "
				     "%luKB (was sized %luKB from "
				     "CR1F).",
				     (unsigned long)(real >> 10),
				     (unsigned long)
				     (tdisp.vram_size >> 10));
			if (real > tdisp.vram_size) {
				uint8_t *bigger;

				bigger = (uint8_t *)
					tg_map_physical(tdisp.fb_phys,
							real);
				if (bigger != NULL) {
					tdisp.fb = bigger;
					tdisp.vram_size = real;
				}
			}
		}
	}

	/* Clear VRAM through the linear aperture (width-aware). */
	if (tdisp.aper_width == 1) {
		volatile uint8_t *out = tdisp.fb;
		uint32_t i;

		for (i = 0; i < tdisp.vram_size; i++)
			out[i] = 0;
	} else {
		memset(tdisp.fb, 0, tdisp.vram_size);
	}

	/* Screen on (unblank: SR01 bit5 off). */
	tg_seq_write(0x01, 0x01);

	tdisp.active = true;

	/* Center the game image; clip if the screen is smaller. */
	ofs_x = (tdisp.scr_w - game_width) / 2;
	ofs_y = (tdisp.scr_h - game_height) / 2;
	if (ofs_x < 0)
		ofs_x = 0;
	if (ofs_y < 0)
		ofs_y = 0;
	draw_w = game_width < tdisp.scr_w ? game_width : tdisp.scr_w;
	draw_h = game_height < tdisp.scr_h ? game_height : tdisp.scr_h;
	draw_w &= ~3;	/* the row converters work 4 pixels at a time */

	hal_log_info("TRIDENT: === configuration summary ===");
	hal_log_info("TRIDENT: chip     : %s (SR0B=%02Xh SR09=%02Xh, "
		     "WAB ID=%02Xh).",
		     tdisp.chip_name, tdisp.chip_id, tdisp.chip_rev,
		     tdisp.wab_id);
	hal_log_info("TRIDENT: mode     : %dx%d, %d bpp, pitch %lu bytes.",
		     tdisp.scr_w, tdisp.scr_h, tdisp.bpp,
		     (unsigned long)tdisp.pitch);
	hal_log_info("TRIDENT: aperture : linear, %luKB at %08lXh%s.",
		     (unsigned long)(tdisp.vram_size >> 10),
		     (unsigned long)tdisp.fb_phys,
		     tdisp.fb_phys == 0x73000000UL
		     ? " (the NEC fixed window, not BAR0)"
		     : " (BAR0 + 0)");
	if (tdisp.use_mmio)
		hal_log_info("TRIDENT: registers: BAR1 MMIO at %08lXh "
			     "(legacy VGA I/O is dead on this board).",
			     (unsigned long)tdisp.mmio_phys);
	else
		hal_log_info("TRIDENT: registers: legacy VGA I/O at "
			     "%03Xh.", tdisp.io_3c0);
	hal_log_info("TRIDENT: blitter  : unused (aperture-only driver).");
	hal_log_info("TRIDENT: blit     : game %dx%d -> +%d,+%d "
		     "(draw %dx%d).",
		     game_width, game_height, ofs_x, ofs_y, draw_w, draw_h);

	return true;
}

void
trident_cleanup_disp(void)
{
	if (!tdisp.active)
		return;

	/* Blank while we unwind. */
	tg_seq_write(0x01, tg_seq_read(0x01) | 0x20);

	/* Output back to the 98 GDC (the XF98 mirror dance). */
	tg_relay_to_gdc();

	/* Put every register back the way we found it. */
	tg_restore_state();

	/* PCI cfg 14h back as found (the NT hidden decode enable). */
	if (sv_cfg14 != 0xffffffffUL) {
		pci_write32(pci_bus, pci_dev, pci_fn, 0x14, sv_cfg14);
		sv_cfg14 = 0xffffffffUL;
	}

	tg_cache_enable();

	tdisp.active = false;
	hal_log_info("TRIDENT: cleanup done, output back on the 98 GDC.");
}

void
trident_flip(void)
{
	if (!tdisp.active)
		return;
	trident_flip_vram();
}

/*****************************************************************************/
/* Frame presentation - direct writes through the linear aperture            */
/*****************************************************************************/

/*
 * Blit the back image to VRAM.
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R.
 * The Trident 24bpp framebuffer is assumed to be the same B,G,R
 * order (see the "open items" note at the top).
 */
/* Scratch row for the byte-lane fallback (800 * 3 bytes max). */
static uint8_t tg_rowbuf[2400];

static void
trident_flip_vram(void)
{
	const uint32_t *pixels;
	int y, bytespp, rowlen;

	pixels = back_image->pixels;
	bytespp = tdisp.bpp / 8;
	rowlen = draw_w * bytespp;

	for (y = 0; y < draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;
		uint32_t off = (uint32_t)(y + ofs_y) * tdisp.pitch +
			       (uint32_t)ofs_x * (uint32_t)bytespp;
		uint8_t *dst = tdisp.fb + off;
		uint8_t *conv = (tdisp.aper_width == 1)
				? tg_rowbuf : dst;

		switch (tdisp.bpp) {
		case 24:
			conv_row24(conv, src, draw_w);
			break;
		case 16:
			conv_row16(conv, src, draw_w);
			break;
		default:
			conv_row8(conv, src, draw_w);
			break;
		}

		/*
		 * Byte-lane-only aperture: the converters built the
		 * row in RAM; push it out one byte at a time.
		 */
		if (tdisp.aper_width == 1) {
			volatile uint8_t *out = dst;
			int i;

			for (i = 0; i < rowlen; i++)
				out[i] = tg_rowbuf[i];
		}
	}
}

/*
 * Pixel format converters.  n is a multiple of four (enforced by
 * draw_w in trident_init_disp()).
 */

/* BGRA8888 -> packed BGR888.  VRAM layout is also B, G, R. */
static void
conv_row24(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 4) {
		uint32_t pix0 = src[x];
		uint32_t pix1 = src[x + 1];
		uint32_t pix2 = src[x + 2];
		uint32_t pix3 = src[x + 3];

		dst[0] = (pix0 & 0xffffff) | (pix1 << 24);
		dst[1] = ((pix1 & 0xffff00) >> 8) | ((pix2 & 0xffff) << 16);
		dst[2] = ((pix2 & 0xff0000) >> 16) | ((pix3 & 0xffffff) << 8);

		dst += 3;
	}
}

/* BGRA8888 -> RGB565 (little-endian words, R in bits 15:11). */
#define PACK565(p) \
	((((p) >> 8) & 0xf800) | (((p) >> 5) & 0x07e0) | (((p) >> 3) & 0x001f))

static void
conv_row16(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 2) {
		uint32_t p0 = src[x];
		uint32_t p1 = src[x + 1];

		*dst++ = PACK565(p0) | (PACK565(p1) << 16);
	}
}

/* BGRA8888 -> RGB332 (matches the palette set by tg_load_palette()). */
#define PACK332(p) \
	((((p) >> 16) & 0xe0) | (((p) >> 11) & 0x1c) | (((p) >> 6) & 0x03))

static void
conv_row8(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 4) {
		*dst++ = PACK332(src[x]) |
			 (PACK332(src[x + 1]) << 8) |
			 (PACK332(src[x + 2]) << 16) |
			 (PACK332(src[x + 3]) << 24);
	}
}

/*****************************************************************************/
/* Register file access                                                      */
/*****************************************************************************/

/*
 * The register block is the native VGA layout at 3C0h on all known
 * machines (see the analysis).  A base variable is kept anyway; the
 * extended DAC aliases (VCLK at 43C8h, SYNCDAC at 83C8h) are derived
 * from the base so a relocated surprise stays a one-line fix.
 *
 * The CRTC and Input Status 1 follow MISC bit0 between the color
 * (3D4h) and mono (3B4h) blocks; tg_misc_write() keeps the selected
 * base coherent (our modes always program bit0 = 1).
 */

/*
 * The access-path switch.  MMIO uses the port number as the offset
 * into the BAR1 block, exactly the layout Linux tridentfb and Xorg
 * use ("MMIOonly" boards); volatile keeps Watcom from caching.
 */
static int
tg_inb(unsigned port)
{
	if (tdisp.use_mmio)
		return tdisp.mmio[port];
	return inp(port);
}

static void
tg_outb(unsigned port, int val)
{
	if (tdisp.use_mmio)
		tdisp.mmio[port] = (uint8_t)val;
	else
		outp(port, val);
}

static void
tg_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
	      uint16_t d4m, uint16_t dam)
{
	tdisp.io_3c0 = b3c0;
	tdisp.io_3d4_col = d4c;
	tdisp.io_3da_col = dac;
	tdisp.io_3d4_mono = d4m;
	tdisp.io_3da_mono = dam;
	tdisp.io_3d4 = d4c;
	tdisp.io_3da = dac;
	/* 3C8h + 4000h and 3C8h + 8000h, in base-relative terms. */
	tdisp.io_vclk = (uint16_t)((b3c0 + 0x08) | 0x4000);
	tdisp.io_sdac = (uint16_t)((b3c0 + 0x08) | 0x8000);
}

/* Point the CRTC/ST1 accessors at the block MISC bit0 selects. */
static void
tg_select_crtc(int misc)
{
	if (misc & 0x01) {
		tdisp.io_3d4 = tdisp.io_3d4_col;
		tdisp.io_3da = tdisp.io_3da_col;
	} else {
		tdisp.io_3d4 = tdisp.io_3d4_mono;
		tdisp.io_3da = tdisp.io_3da_mono;
	}
}

static void
tg_seq_write(int reg, int val)
{
	tg_outb(tdisp.io_3c0 + 0x04, reg);
	tg_outb(tdisp.io_3c0 + 0x05, val);
}

static int
tg_seq_read(int reg)
{
	tg_outb(tdisp.io_3c0 + 0x04, reg);
	return tg_inb(tdisp.io_3c0 + 0x05);
}

static void
tg_gfx_write(int reg, int val)
{
	tg_outb(tdisp.io_3c0 + 0x0e, reg);
	tg_outb(tdisp.io_3c0 + 0x0f, val);
}

static int
tg_gfx_read(int reg)
{
	tg_outb(tdisp.io_3c0 + 0x0e, reg);
	return tg_inb(tdisp.io_3c0 + 0x0f);
}

static void
tg_crtc_write(int reg, int val)
{
	tg_outb(tdisp.io_3d4, reg);
	tg_outb(tdisp.io_3d4 + 1, val);
}

static int
tg_crtc_read(int reg)
{
	tg_outb(tdisp.io_3d4, reg);
	return tg_inb(tdisp.io_3d4 + 1);
}

static void
tg_attr_write(int reg, int val)
{
	(void)tg_inb(tdisp.io_3da);	/* reset the index/data flip-flop */
	tg_outb(tdisp.io_3c0 + 0x00, reg);
	tg_outb(tdisp.io_3c0 + 0x00, val);
}

static int
tg_attr_read(int reg)
{
	int val;

	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0 + 0x00, reg);
	val = tg_inb(tdisp.io_3c0 + 0x01);
	(void)tg_inb(tdisp.io_3da);	/* leave the flip-flop reset */
	return val;
}

static void
tg_misc_write(int val)
{
	tg_outb(tdisp.io_3c0 + 0x02, val);	/* 3C2h: write */
	tg_select_crtc(val);			/* keep CRTC base coherent */
}

static int
tg_misc_read(void)
{
	return tg_inb(tdisp.io_3c0 + 0x0c);	/* 3CCh: read */
}

/*
 * The Trident hidden DAC register: read the DAC Write Index (3C8h)
 * once to reset the state machine, read the Pixel Mask (3C6h) four
 * times, and the next 3C6h access hits the hidden register.  A final
 * 3C8h read resets the state again.
 */
static void
tg_hidden_dac_write(int val)
{
	(void)tg_inb(tdisp.io_3c0 + 0x08);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	tg_outb(tdisp.io_3c0 + 0x06, val);
	(void)tg_inb(tdisp.io_3c0 + 0x08);
}

static int
tg_hidden_dac_read(void)
{
	int val;

	(void)tg_inb(tdisp.io_3c0 + 0x08);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	val = tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x08);
	return val;
}

/* The NEC SYNCDAC glue: 83C8h = index, 83C6h = data (pc98_tgui.c). */
static void
tg_sdac_write(int reg, int val)
{
	tg_outb(tdisp.io_sdac, reg);
	tg_outb((uint16_t)(tdisp.io_sdac - 2), val);
}

static int
tg_sdac_read(int reg)
{
	tg_outb(tdisp.io_sdac, reg);
	return tg_inb((uint16_t)(tdisp.io_sdac - 2));
}

/*
 * Old/new register modes.  WRITING SR0B selects the old mode;
 * READING it selects the new mode and returns the chip ID.
 */
static void
tg_sw_old(void)
{
	int v;

	v = tg_seq_read(0x0b);		/* (also: new mode) */
	tg_seq_write(0x0b, v);		/* write -> old mode */
}

static void
tg_sw_new(void)
{
	(void)tg_seq_read(0x0b);	/* read -> new mode */
}

/*
 * Millisecond-ish delay via the PC-98 wait port (5Fh, ~0.6us per
 * access) - no timers touched, works at any CPU speed.
 */
static void
tg_wait_ms(int ms)
{
	long i;

	for (i = 0; i < (long)ms * 1700L; i++)
		(void)inp(0x5f);
}

/*****************************************************************************/
/* PCI detection                                                             */
/*****************************************************************************/

#define PCI_CONFIG_ADDR		0x0cf8
#define PCI_CONFIG_DATA		0x0cfc
#define PCI_VENDOR_TRIDENT	0x1023

static uint32_t
pci_read32(int bus, int dev, int fn, int reg)
{
	outpd(PCI_CONFIG_ADDR,
	      0x80000000UL |
	      ((uint32_t)bus << 16) |
	      ((uint32_t)dev << 11) |
	      ((uint32_t)fn << 8) |
	      ((uint32_t)reg & 0xfc));

	return (uint32_t)inpd(PCI_CONFIG_DATA);
}

static void
pci_write32(int bus, int dev, int fn, int reg, uint32_t val)
{
	outpd(PCI_CONFIG_ADDR,
	      0x80000000UL |
	      ((uint32_t)bus << 16) |
	      ((uint32_t)dev << 11) |
	      ((uint32_t)fn << 8) |
	      ((uint32_t)reg & 0xfc));
	outpd(PCI_CONFIG_DATA, val);
}

/*
 * Scan the first buses for a Trident, logging everything we pass by.
 * The 9660 family is what the target desktops carry; the revision
 * (config 08h low byte, mirrored in SR09) splits 9660/9680/9682/9685.
 */
static bool
tg_pci_find(void)
{
	int bus, dev, fn, nfn, ndev;
	uint32_t id, classrev, bar0, cmd, mask, orig;
	uint16_t device;
	uint8_t rev;

	ndev = 0;
	for (bus = 0; bus < 4; bus++) {
		for (dev = 0; dev < 32; dev++) {
			id = pci_read32(bus, dev, 0, 0x00);
			if ((id & 0xffff) == 0xffff || (id & 0xffff) == 0)
				continue;
			nfn = (pci_read32(bus, dev, 0, 0x0c) &
			       0x00800000UL) ? 8 : 1;
			for (fn = 0; fn < nfn; fn++) {
				id = pci_read32(bus, dev, fn, 0x00);
				if ((id & 0xffff) == 0xffff ||
				    (id & 0xffff) == 0)
					continue;
				classrev = pci_read32(bus, dev, fn, 0x08);
				hal_log_info("TRIDENT: PCI %d:%d.%d = "
					     "%04lX:%04lX class %02lXh "
					     "rev %02lXh.",
					     bus, dev, fn,
					     (unsigned long)(id & 0xffff),
					     (unsigned long)(id >> 16),
					     (unsigned long)(classrev >> 24),
					     (unsigned long)(classrev & 0xff));
				ndev++;
				if ((id & 0xffff) != PCI_VENDOR_TRIDENT)
					continue;

				device = (uint16_t)(id >> 16);
				rev = (uint8_t)(classrev & 0xff);
				if (device != 0x9660) {
					hal_log_info("TRIDENT: device "
						     "%04Xh is not the "
						     "9660 desktop family "
						     "(Cyber laptop or "
						     "unknown); leaving "
						     "it alone.", device);
					continue;
				}

				pci_bus = bus;
				pci_dev = dev;
				pci_fn = fn;

				/*
				 * The Ra43 reports the odd revision
				 * D3h (the SR0B chip-ID value), so
				 * the PCI revision is informational;
				 * the name is refined from SR09 once
				 * register access works.
				 */
				tdisp.chip_name = "TGUI96xx family";

				/* BAR0 + decode size + enable. */
				cmd = pci_read32(bus, dev, fn, 0x04);
				bar0 = pci_read32(bus, dev, fn, 0x10);
				pci_write32(bus, dev, fn, 0x04,
					    cmd & ~0x2UL);
				orig = bar0;
				pci_write32(bus, dev, fn, 0x10,
					    0xffffffffUL);
				mask = pci_read32(bus, dev, fn, 0x10) &
				       ~0xfUL;
				pci_write32(bus, dev, fn, 0x10, orig);
				pci_write32(bus, dev, fn, 0x04,
					    cmd | 0x03);

				bar0 &= ~0xfUL;
				hal_log_info("TRIDENT: %s (PCI rev "
					     "%02Xh) at %d:%d.%d, "
					     "BAR0=%08lXh (decode %luMB), "
					     "cmd=%04lXh -> readback "
					     "%04lXh.",
					     tdisp.chip_name, rev,
					     bus, dev, fn,
					     (unsigned long)bar0,
					     (unsigned long)
					     ((mask ? (~mask + 1) : 0)
					      >> 20),
					     (unsigned long)(cmd & 0xffff),
					     (unsigned long)
					     (pci_read32(bus, dev, fn,
							 0x04) & 0xffff));
				if (bar0 == 0) {
					hal_log_info("TRIDENT: BAR0 is "
						     "unassigned, giving "
						     "up.");
					return false;
				}

				/*
				 * The linear framebuffer is at BAR0 + 0
				 * (XF98: ChipLinearBase = MemBase; the
				 * GE MMIO block would be at +400000h).
				 */
				tdisp.fb_phys = bar0;

				/*
				 * BAR1 is the 64KB register MMIO
				 * block (tridentfb/Xorg).  If the ITF
				 * left it unassigned, park it just
				 * past BAR0's 4MB decode - the very
				 * address XF98 used for its GE
				 * window, known-free on these boards.
				 */
				{
					uint32_t bar1;

					bar1 = pci_read32(bus, dev, fn,
							  0x14);
					bar1 &= ~0xfUL;
					if (bar1 == 0) {
						bar1 = bar0 + 0x400000UL;
						pci_write32(bus, dev, fn,
							    0x14, bar1);
						bar1 = pci_read32(bus, dev,
								  fn, 0x14)
						       & ~0xfUL;
						hal_log_info("TRIDENT: "
							     "BAR1 was "
							     "unassigned; "
							     "parked at "
							     "%08lXh.",
							     (unsigned long)
							     bar1);
					} else {
						hal_log_info("TRIDENT: "
							     "BAR1 (register "
							     "MMIO) at "
							     "%08lXh.",
							     (unsigned long)
							     bar1);
					}
					tdisp.mmio_phys = bar1;
				}
				return true;
			}
		}
	}

	if (ndev == 0)
		hal_log_info("TRIDENT: PCI config space is silent.");
	else
		hal_log_info("TRIDENT: no Trident on PCI (%d devices "
			     "seen); the 96xx built-ins are PCI, so "
			     "yielding to other drivers.", ndev);
	return false;
}

/*
 * Dump the Trident's PCI configuration header to the log - the raw
 * material for diagnosing a machine whose wiring we don't know yet.
 */
static void
tg_pci_dump_header(int bus, int dev, int fn)
{
	int reg;

	hal_log_info("TRIDENT: PCI config header of %d:%d.%d:",
		     bus, dev, fn);
	for (reg = 0; reg < 0x40; reg += 0x10) {
		hal_log_info("TRIDENT:   %02Xh: %08lXh %08lXh "
			     "%08lXh %08lXh.", reg,
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 4),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 8),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 12));
	}
}

static void
tg_pci_dump_config(void)
{
	int reg;

	hal_log_info("TRIDENT: PCI config header of %d:%d.%d:",
		     pci_bus, pci_dev, pci_fn);
	for (reg = 0; reg < 0x40; reg += 0x10) {
		hal_log_info("TRIDENT:   %02Xh: %08lXh %08lXh "
			     "%08lXh %08lXh.", reg,
			     (unsigned long)pci_read32(pci_bus, pci_dev,
						       pci_fn, reg),
			     (unsigned long)pci_read32(pci_bus, pci_dev,
						       pci_fn, reg + 4),
			     (unsigned long)pci_read32(pci_bus, pci_dev,
						       pci_fn, reg + 8),
			     (unsigned long)pci_read32(pci_bus, pci_dev,
						       pci_fn, reg + 12));
	}
}

/*****************************************************************************/
/* Wakeup and fingerprint                                                    */
/*****************************************************************************/

/*
 * A cheap liveness test for the current register access path: the
 * pattern of an existing chip is SR0B == D3h; a dead path reads FFh
 * everywhere (also probe SR00/SR01 so a floating D3h can't fool us).
 */
static bool
tg_regs_alive(void)
{
	int id, s0, s1;

	tg_sw_old();
	id = tg_seq_read(0x0b);		/* also selects the new mode */
	s0 = tg_seq_read(0x00);
	s1 = tg_seq_read(0x01);

	if (id == 0xff && s0 == 0xff && s1 == 0xff)
		return false;
	return id == 0xd3;
}

/*
 * The strap-guided wakeup (pc98_tgui.c VideoEnable(), verbatim).
 * Runs in the OLD register mode: SR0E bit5 there selects the
 * Configuration Port at SR0C, whose bit4 straps the wakeup flavor.
 * Only meaningful when the register path already answers.
 */
static void
tg_wakeup_strapped(void)
{
	int tmp, cfg;

	/* MISC: RAM enable, color I/O (ChipInit does this first). */
	tg_misc_write(tg_misc_read() | 0xc3);

	tg_sw_old();

	tmp = tg_seq_read(0x0e);
	tg_seq_write(0x0e, tmp | 0x20);	/* select Configuration Port 1 */
	cfg = tg_seq_read(0x0c);
	tg_seq_write(0x0e, tmp);

	hal_log_info("TRIDENT: wakeup: old-mode SR0E=%02Xh, SR0C=%02Xh "
		     "-> %s scheme.", tmp, cfg,
		     (cfg & 0x10) ? "PC-98 (94h/102h/3C3h)"
				  : "AT (46E8h/102h)");

	if ((cfg & 0x10) == 0x10)
		tg_wakeup_blind_pc98();
	else
		tg_wakeup_blind_at();
}

/* The AT-style setup-port wakeup.  Safe to fire blind. */
static void
tg_wakeup_blind_at(void)
{
	hal_log_info("TRIDENT: wakeup: firing the AT scheme "
		     "(46E8h/102h).");
	outp(0x46e8, 0x10);
	outp(0x102, 0x01);
	outp(0x46e8, 0x08);
}

/*
 * The PC-98-style setup-port wakeup.  NOT safe to fire blind on an
 * arbitrary machine: if the Trident does not claim port 94h, the
 * write lands on the PC-98 FDC mode register.  Fired last, and only
 * when everything else has already failed.
 */
static void
tg_wakeup_blind_pc98(void)
{
	int v;

	hal_log_info("TRIDENT: wakeup: firing the PC-98 scheme "
		     "(94h/102h/3C3h) - last resort.");
	outp(0x94, 0x00);
	outp(0x102, 0x01);
	outp(0x94, 0x20);
	v = inp(tdisp.io_3c0 + 0x03);
	outp(tdisp.io_3c0 + 0x03, v == 0xff ? 0x01 : (v | 0x01));
}

/*
 * Establish a working register access path.
 *
 *  1. Legacy PIO at the native 3C0h block (the V13/V16 wiring; XF98
 *     proved it there).
 *  2. BAR1 MMIO (the Ra wiring: NEC ships those with PCI I/O decode
 *     off and the legacy block dead; the full register file appears
 *     inside the 64KB BAR1 window at its port offsets, gated by
 *     CR39 bit0 which such boards have set out of reset).
 *  3. Blind wakeups (AT scheme first, the FDC-hazardous 94h scheme
 *     last), then a PIO retest.
 */
static bool
tg_probe_access_path(void)
{
	tdisp.use_mmio = false;

	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: legacy VGA I/O answers at "
			     "%03Xh; using PIO.", tdisp.io_3c0);
		tg_wakeup_strapped();
		return true;
	}
	hal_log_info("TRIDENT: legacy VGA I/O at %03Xh is dead "
		     "(reads FFh); trying BAR1 MMIO.", tdisp.io_3c0);

	if (tdisp.mmio_phys != 0) {
		tdisp.mmio = (volatile uint8_t *)
			tg_map_physical(tdisp.mmio_phys, 0x10000);
		if (tdisp.mmio != NULL) {
			tdisp.use_mmio = true;
			if (tg_regs_alive()) {
				hal_log_info("TRIDENT: BAR1 MMIO at "
					     "%08lXh answers (SR0B via "
					     "memory); using MMIO for "
					     "all registers.",
					     (unsigned long)
					     tdisp.mmio_phys);
				return true;
			}
			tdisp.use_mmio = false;
			hal_log_info("TRIDENT: BAR1 MMIO is mapped but "
				     "the register file does not "
				     "answer there.");
		} else {
			hal_log_info("TRIDENT: can't map BAR1 at "
				     "%08lXh.",
				     (unsigned long)tdisp.mmio_phys);
		}
	} else {
		hal_log_info("TRIDENT: no BAR1 to try.");
	}

	/* Both blind wakeups, least dangerous first, then retest PIO. */
	tg_wakeup_blind_at();
	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: PIO came alive after the AT "
			     "wakeup.");
		return true;
	}
	tg_wakeup_blind_pc98();
	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: PIO came alive after the PC-98 "
			     "wakeup.");
		return true;
	}

	hal_log_info("TRIDENT: no register access path works (PIO "
		     "dead, MMIO dead, wakeups didn't help).  Config "
		     "dump follows for diagnosis.");
	return false;
}

/*
 * Prove there is a Trident TGUI96xx behind the VGA block before a
 * single register is programmed:
 *  - reading SR0B must return the 9660-family ID D3h;
 *  - the classic signature: writing 00h to new-mode SR0E must read
 *    back 02h (the hardware inverts bit1 on the way in).
 */
static bool
tg_fingerprint(void)
{
	int id, rev, old0e, sig;

	tg_sw_old();
	id = tg_seq_read(0x0b);		/* read: chip ID, now new mode */
	rev = tg_seq_read(0x09);

	old0e = tg_seq_read(0x0e);
	tg_seq_write(0x0e, 0x00);
	sig = tg_seq_read(0x0e);
	tg_seq_write(0x0e, old0e ^ 0x02);	/* re-store the old value */

	hal_log_info("TRIDENT: fingerprint: SR0B=%02Xh SR09=%02Xh, "
		     "SR0E write-00h reads %02Xh (expect xxx2h).",
		     id, rev, sig);

	if (id != 0xd3) {
		hal_log_info("TRIDENT: SR0B is not the TGUI9660-family "
			     "ID (D3h); refusing to program the chip.");
		return false;
	}
	if ((sig & 0x0f) != 0x02) {
		hal_log_info("TRIDENT: the SR0E bit1-invert signature "
			     "failed; refusing to program the chip.");
		return false;
	}

	tdisp.chip_id = (uint8_t)id;
	tdisp.chip_rev = (uint8_t)rev;

	/* SR09 is the authoritative revision. */
	switch (rev) {
	case 0x00:
		tdisp.chip_name = "TGUI9660";
		break;
	case 0x01:
		tdisp.chip_name = "TGUI9680";
		break;
	case 0x10:
		tdisp.chip_name = "ProVidia TGUI9682";
		break;
	case 0x21:
		tdisp.chip_name = "ProVidia TGUI9685";
		break;
	default:
		tdisp.chip_name = "TGUI96xx (unrecognized SR09)";
		break;
	}
	hal_log_info("TRIDENT: chip fingerprint OK: %s via %s.",
		     tdisp.chip_name,
		     tdisp.use_mmio ? "BAR1 MMIO" : "legacy PIO");
	return true;
}

/*
 * One write/read cycle against the mapped framebuffer, fully
 * logged: what the first dwords read as found, what they read
 * after a test pattern.  The Ra43 aperture came up dead, so the
 * raw values matter for diagnosis.
 */
static bool
tg_aperture_test(const char *tag)
{
	volatile uint32_t *p = (volatile uint32_t *)tdisp.fb;
	volatile uint8_t *b = (volatile uint8_t *)tdisp.fb;
	uint32_t r0, r1, w0, w1;
	int b0, b1, b2, b3;
	bool dw_ok, by_ok;

	/* Dword cycles at offsets 0/4. */
	r0 = p[0];
	r1 = p[1];
	p[0] = 0x55aa1234UL;
	p[1] = 0xc3a5960fUL;
	w0 = p[0];
	w1 = p[1];
	dw_ok = (w0 == 0x55aa1234UL && w1 == 0xc3a5960fUL);

	/*
	 * Byte cycles at offsets 8..0Bh: the register block only
	 * proves byte access works on this wiring, so the aperture
	 * may be byte-lane-limited too.
	 */
	b[8] = 0xa5;
	b[9] = 0x5a;
	b[10] = 0xc3;
	b[11] = 0x3c;
	b0 = b[8];
	b1 = b[9];
	b2 = b[10];
	b3 = b[11];
	by_ok = (b0 == 0xa5 && b1 == 0x5a && b2 == 0xc3 && b3 == 0x3c);

	hal_log_info("TRIDENT: aperture test (%s): dwords found "
		     "%08lXh %08lXh, after write %08lXh %08lXh -> "
		     "%s; bytes read %02Xh %02Xh %02Xh %02Xh -> %s.",
		     tag,
		     (unsigned long)r0, (unsigned long)r1,
		     (unsigned long)w0, (unsigned long)w1,
		     dw_ok ? "OK" : "dead",
		     b0, b1, b2, b3,
		     by_ok ? "OK" : "dead");

	if (dw_ok) {
		tdisp.aper_width = 4;
		return true;
	}
	if (by_ok) {
		tdisp.aper_width = 1;
		hal_log_info("TRIDENT: aperture is byte-lane only; "
			     "blits will fall back to byte copies.");
		return true;
	}
	return false;
}

/*
 * Get the linear aperture reaching VRAM.
 *
 * NEC's NT4 trident.sys accesses the framebuffer at the FIXED
 * physical address 73000000h (its access-range table; it never
 * reads BAR0).  The Ra43 ITF leaves CR21 = C7h, and C7h decodes to
 * exactly that address under the hypothesis
 *
 *     linear base = (CR21 bits3:0 << 28) | (CR21 bits7:6 << 24)
 *
 * (7 -> 70000000h, plus 11b -> 03000000h), with bit5 the enable.
 * So on this wiring CR21 is a window-placement register and the
 * framebuffer lives at 73000000h - NOT behind BAR0.  When the
 * BAR0 test fails, remap the framebuffer at the CR21-decoded
 * address (and at the literal NT address as a backstop) and test
 * there.
 */
static bool
tg_aperture_fix(void)
{
	uint32_t cand[3];
	const char *cname[3];
	uint8_t *newfb;
	int cr21, i;

	if (tg_aperture_test("BAR0"))
		return true;

	/*
	 * The NT4 driver's enable path sets bit1 of PCI config reg
	 * 14h (nominally BAR1, read-only low bits on compliant
	 * hardware) before touching the framebuffer - i.e. it is a
	 * vendor memory-decode enable on this chip.  Set it and
	 * retest.
	 */
	{
		uint32_t v;

		v = pci_read32(pci_bus, pci_dev, pci_fn, 0x14);
		sv_cfg14 = v;
		pci_write32(pci_bus, pci_dev, pci_fn, 0x14,
			    v | 0x02UL);
		hal_log_info("TRIDENT: PCI cfg14h (NT hidden decode "
			     "enable): %08lXh -> %08lXh.",
			     (unsigned long)v,
			     (unsigned long)pci_read32(pci_bus,
						       pci_dev,
						       pci_fn,
						       0x14));
	}
	if (tg_aperture_test("BAR0 + cfg14h.1"))
		return true;

	cr21 = tg_crtc_read(0x21);

	/* BAR2 (config 18h): the third window NEC assigns. */
	cand[0] = pci_read32(pci_bus, pci_dev, pci_fn, 0x18) & ~0xfUL;
	cname[0] = "BAR2";
	cand[1] = ((uint32_t)(cr21 & 0x0f) << 28) |
		  ((uint32_t)((cr21 >> 6) & 0x03) << 24);
	cname[1] = "CR21 window";
	cand[2] = 0x73000000UL;	/* NEC NT4 fixed range, backstop */
	cname[2] = "NT4 fixed window";

	hal_log_info("TRIDENT: BAR0 aperture is dead; candidates: "
		     "BAR2=%08lXh, CR21(%02Xh)-decoded=%08lXh, "
		     "NT4 fixed=73000000h.",
		     (unsigned long)cand[0], cr21,
		     (unsigned long)cand[1]);

	for (i = 0; i < 3; i++) {
		int j, dup;

		if (cand[i] == 0 || cand[i] == tdisp.fb_phys)
			continue;
		dup = 0;
		for (j = 0; j < i; j++)
			if (cand[j] == cand[i])
				dup = 1;
		if (dup)
			continue;
		newfb = (uint8_t *)tg_map_physical(cand[i],
						   tdisp.vram_size);
		if (newfb == NULL) {
			hal_log_info("TRIDENT: can't map %08lXh.",
				     (unsigned long)cand[i]);
			continue;
		}
		tdisp.fb = newfb;
		tdisp.fb_phys = cand[i];
		if (tg_aperture_test(cname[i])) {
			hal_log_info("TRIDENT: framebuffer adopted "
				     "at %08lXh (%s).",
				     (unsigned long)cand[i],
				     cname[i]);
			return true;
		}
	}

	hal_log_info("TRIDENT: no candidate window reaches VRAM; "
		     "the picture will show stale VRAM.  NEC "
		     "companion device headers for diagnosis:");
	tg_pci_dump_header(0, 6, 0);	/* 1033:002C bridge */
	tg_pci_dump_header(0, 7, 0);	/* 1033:0009 display */
	tg_pci_dump_header(pci_bus, pci_dev, pci_fn);
	return false;
}

/*
 * Measure the VRAM through the linear aperture: plant distinct tags
 * just under each candidate size, largest first; on a smaller chip
 * the higher writes wrap and get overwritten, so the largest intact
 * tag is the true size.  Used when CR1F carries a code we can't
 * decode (the Ra43 reads F5h).
 */
static uint32_t
tg_vram_probe(void)
{
	volatile uint32_t *p;
	uint32_t sizes[3];
	uint32_t s;
	int i, cr21;

	sizes[0] = 4096UL * 1024UL;
	sizes[1] = 2048UL * 1024UL;
	sizes[2] = 1024UL * 1024UL;

	/*
	 * One-shot 4MB scratch mapping; DOS4GW address space is
	 * plentiful and there is no DPMI unmap on the exit path
	 * anyway.
	 */
	p = (volatile uint32_t *)tg_map_physical(tdisp.fb_phys,
						 sizes[0]);
	if (p == NULL)
		return 0;

	cr21 = tg_crtc_read(0x21);
	tg_crtc_write(0x21, cr21 | 0x20);	/* aperture on */

	p[0] = 0x55aa1234UL;
	for (i = 0; i < 3; i++)
		p[(sizes[i] - 16) / 4] = sizes[i] ^ 0x0badf00dUL;

	hal_log_info("TRIDENT: VRAM probe: base dword reads %08lXh "
		     "(wrote 55AA1234h).", (unsigned long)p[0]);
	s = 0;
	if (p[0] == 0x55aa1234UL) {
		for (i = 0; i < 3; i++) {
			if (p[(sizes[i] - 16) / 4] ==
			    (sizes[i] ^ 0x0badf00dUL)) {
				s = sizes[i];
				break;
			}
		}
		if (s == 0)
			s = 512UL * 1024UL;
	}

	tg_crtc_write(0x21, cr21);		/* as found */
	return s;
}

/* VRAM size from CR1F (SPR) low nibble. */
static uint32_t
tg_vram_size(void)
{
	int spr;
	uint32_t k;

	tg_sw_new();
	tg_select_crtc(tg_misc_read());
	spr = tg_crtc_read(0x1f);

	switch (spr & 0x0f) {
	case 0x01:
		k = 512UL * 1024UL;
		break;
	case 0x03:
		k = 1024UL * 1024UL;
		break;
	case 0x07:
		k = 2048UL * 1024UL;
		break;
	case 0x0f:
		k = 4096UL * 1024UL;
		break;
	default:
		hal_log_info("TRIDENT: CR1F=%02Xh is not a known VRAM "
			     "code; probing through the aperture.",
			     spr);
		k = tg_vram_probe();
		if (k != 0) {
			hal_log_info("TRIDENT: aperture probe -> "
				     "%luKB VRAM.",
				     (unsigned long)(k >> 10));
			return k;
		}
		k = 1024UL * 1024UL;
		hal_log_info("TRIDENT: aperture probe failed too; "
			     "assuming 1MB.");
		break;
	}
	hal_log_info("TRIDENT: CR1F=%02Xh -> %luKB VRAM.",
		     spr, (unsigned long)(k >> 10));
	return k;
}

/* Dump the registers that matter for remote debugging. */
static void
tg_dump_regs(const char *tag)
{
	tg_sw_new();
	tg_select_crtc(tg_misc_read());

	hal_log_info("TRIDENT: regs (%s):", tag);
	hal_log_info("TRIDENT:   MISC=%02Xh HDR=%02Xh SR01=%02Xh "
		     "SR0D=%02Xh SR0E=%02Xh SR0F=%02Xh.",
		     tg_misc_read(), tg_hidden_dac_read(),
		     tg_seq_read(0x01), tg_seq_read(0x0d),
		     tg_seq_read(0x0e), tg_seq_read(0x0f));
	hal_log_info("TRIDENT:   CR1E=%02Xh CR1F=%02Xh CR21=%02Xh "
		     "CR27=%02Xh CR29=%02Xh CR2A=%02Xh CR2B=%02Xh.",
		     tg_crtc_read(0x1e), tg_crtc_read(0x1f),
		     tg_crtc_read(0x21), tg_crtc_read(0x27),
		     tg_crtc_read(0x29), tg_crtc_read(0x2a),
		     tg_crtc_read(0x2b));
	hal_log_info("TRIDENT:   CR20=%02Xh CR23=%02Xh CR25=%02Xh "
		     "CR2F=%02Xh CR30=%02Xh CR36=%02Xh CR38=%02Xh "
		     "CR39=%02Xh.",
		     tg_crtc_read(0x20), tg_crtc_read(0x23),
		     tg_crtc_read(0x25), tg_crtc_read(0x2f),
		     tg_crtc_read(0x30), tg_crtc_read(0x36),
		     tg_crtc_read(0x38), tg_crtc_read(0x39));
	hal_log_info("TRIDENT:   GR0F=%02Xh GR23=%02Xh GR2F=%02Xh; "
		     "VCLK=%02Xh/%02Xh MCLK=%02Xh/%02Xh; "
		     "SDAC[00]=%02Xh SDAC[04]=%02Xh; 0FACh=%02Xh.",
		     tg_gfx_read(0x0f), tg_gfx_read(0x23),
		     tg_gfx_read(0x2f),
		     tg_inb(tdisp.io_vclk), tg_inb(tdisp.io_vclk + 1),
		     tg_inb(tdisp.io_vclk - 2), tg_inb(tdisp.io_vclk - 1),
		     tg_sdac_read(0x00), tg_sdac_read(0x04),
		     inp(0x0fac));

	/* The NEC sync-glue register file (see section G). */
	hal_log_info("TRIDENT:   GR21=%02Xh GR24=%02Xh GR25=%02Xh "
		     "GR2C=%02Xh GR30=%02Xh GR33=%02Xh.",
		     tg_gfx_read(0x21), tg_gfx_read(0x24),
		     tg_gfx_read(0x25), tg_gfx_read(0x2c),
		     tg_gfx_read(0x30), tg_gfx_read(0x33));
	hal_log_info("TRIDENT:   GR40=%02Xh GR42=%02Xh GR43=%02Xh "
		     "GR50=%02Xh GR51=%02Xh GR52=%02Xh GR53=%02Xh "
		     "GR5A=%02Xh GR5B=%02Xh.",
		     tg_gfx_read(0x40), tg_gfx_read(0x42),
		     tg_gfx_read(0x43), tg_gfx_read(0x50),
		     tg_gfx_read(0x51), tg_gfx_read(0x52),
		     tg_gfx_read(0x53), tg_gfx_read(0x5a),
		     tg_gfx_read(0x5b));
	{
		int gr30, crb00, crb06, crb10, crb11;
		unsigned w52, w60;

		gr30 = tg_gfx_read(0x30);
		if ((gr30 & 0x40) == 0)
			tg_gfx_write(0x30, gr30 | 0x40);
		tg_outb(0x03a4, 0x00); crb00 = tg_inb(0x03a5);
		tg_outb(0x03a4, 0x06); crb06 = tg_inb(0x03a5);
		tg_outb(0x03a4, 0x10); crb10 = tg_inb(0x03a5);
		tg_outb(0x03a4, 0x11); crb11 = tg_inb(0x03a5);
		if ((gr30 & 0x40) == 0)
			tg_gfx_write(0x30, gr30);
		outpw(0x08f0, 0x52); w52 = inpw(0x08f2);
		outpw(0x08f0, 0x60); w60 = inpw(0x08f2);
		outp(0x0faa, 0x84);
		hal_log_info("TRIDENT:   CRB00=%02Xh CRB06=%02Xh "
			     "CRB10=%02Xh CRB11=%02Xh; 8F0h[52]=%04Xh "
			     "[60]=%04Xh; FAA[84h]=%02Xh.",
			     crb00, crb06, crb10, crb11, w52, w60,
			     inp(0x0fab));
	}
}

/*****************************************************************************/
/* Board bring-up (pc98_tgui.c ChipInit(), minus the graphics engine)        */
/*****************************************************************************/

static void
tg_board_tune(void)
{
	hal_log_info("TRIDENT: board tune (XF98 ChipInit values): "
		     "old MCLK=%02Xh/%02Xh.",
		     tg_inb(tdisp.io_vclk - 2), tg_inb(tdisp.io_vclk - 1));

	tg_sw_new();
	tg_seq_write(0x0e, tg_seq_read(0x0e) | 0x80);	/* unlock ext */

	tg_seq_write(0x0f, tg_seq_read(0x0f) & 0xef);

	tg_select_crtc(tg_misc_read());

	/* Bus & DRAM setup, values verbatim from XF98. */
	tg_crtc_write(0x2a, tg_crtc_read(0x2a) | 0x40);	/* local bus/DRAM */
	tg_crtc_write(0x20, 0x38);	/* Command FIFO */
	tg_crtc_write(0x23, 0xe8);	/* DRAM Timing Control */
	tg_crtc_write(0x25, 0x0a);	/* RAMDAC R/W Timing */
	tg_crtc_write(0x2f, 0x27);	/* Performance Tuning */
	tg_crtc_write(0x30, 0x0f);	/* Display Queue Latency */
	tg_crtc_write(0x33, 0x01);	/* Read Cache Control */
	tg_crtc_write(0x3b, 0x21);	/* Clock and Tuning */
	tg_crtc_write(0x3c, 0x00);	/* Miscellaneous Control */

	/*
	 * MCLK: the exact register value XF98 programs on the
	 * V13/V16 for its nominal "80MHz" (n=10 m=3 k=0 in its own
	 * encoding).  Replayed as a value, not recomputed - see the
	 * header note about the conflicting MCLK formulas.
	 */
	tg_outb(tdisp.io_vclk - 2, 0x53);	/* 43C6h */
	tg_outb(tdisp.io_vclk - 1, 0x00);	/* 43C7h */

	/* The graphics engine (CR34/35/36) stays disabled. */
	tg_crtc_write(0x36, 0x00);

	/* NEC glue defaults from ChipInit. */
	tg_sdac_write(0x00, 0x01);
	tg_gfx_write(0x2f, 0x20);
	tg_gfx_write(0x5e, 0x88);
	tg_gfx_write(0x5f, 0x48);

	/*
	 * The NEC board-init glue NEC's own NT4 driver applies
	 * (GR20h-2Ah, GR23h, SDAC 04h/08h/09h/37h/38h); byte-exact
	 * from trident.sys, see section G.
	 */
	tg_apply_triplets(tg_glue_board, sizeof(tg_glue_board) / 3,
			  "NEC board glue");
}

/*****************************************************************************/
/* Mode set                                                                  */
/*****************************************************************************/

/*
 * CRTC values built with the tridentfb rules (standard VGA layout;
 * Trident keeps the 6-bit Horizontal Blanking End compare, unlike
 * the Cirrus 8-bit extension).
 *
 * 640x480@60: 25.175MHz, H 640/16/96/48, V 480/10/2/33, -h -v sync.
 */
static const uint8_t tg_crtc_640x480[] = {
	0x5f,	/* 00: Horizontal Total (800/8 - 5) */
	0x4f,	/* 01: Horizontal Display End (640/8 - 1) */
	0x50,	/* 02: Horizontal Blanking Start (640/8) */
	0x02,	/* 03: Horizontal Blanking End ((95+3) & 1Fh) */
	0x52,	/* 04: Horizontal Sync Start (656/8) */
	0x9e,	/* 05: Hsync End (752/8 & 1Fh), bit7 = HBE bit5 */
	0x0b,	/* 06: Vertical Total (523, low byte) */
	0x3e,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x40,	/* 09: Max Scan Line */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0xea,	/* 10: Vertical Sync Start (490, low byte) */
	0x0c,	/* 11: Vsync End (492 % 16), unprotected */
	0xdf,	/* 12: Vertical Display End (479, low byte) */
	0x00,	/* 13: Offset (patched from tdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0xe0,	/* 15: Vertical Blanking Start (480, low byte) */
	0x0b,	/* 16: Vertical Blanking End (523, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * 800x600@60 (VESA): 40.000MHz, H 800/40/128/88, V 600/1/4/23,
 * +h +v sync.
 */
static const uint8_t tg_crtc_800x600[] = {
	0x7f,	/* 00: Horizontal Total (1056/8 - 5) */
	0x63,	/* 01: Horizontal Display End (800/8 - 1) */
	0x64,	/* 02: Horizontal Blanking Start (800/8) */
	0x02,	/* 03: Horizontal Blanking End ((127+3) & 1Fh) */
	0x69,	/* 04: Horizontal Sync Start (840/8) */
	0x19,	/* 05: Hsync End (968/8 & 1Fh), bit7 = HBE bit5 = 0 */
	0x72,	/* 06: Vertical Total (626, low byte) */
	0xf0,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x60,	/* 09: Max Scan Line (bit5 = VBS bit9) */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0x59,	/* 10: Vertical Sync Start (601, low byte) */
	0x0d,	/* 11: Vsync End (605 % 16), unprotected */
	0x57,	/* 12: Vertical Display End (599, low byte) */
	0x00,	/* 13: Offset (patched from tdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0x58,	/* 15: Vertical Blanking Start (600, low byte) */
	0x72,	/* 16: Vertical Blanking End (626, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * VCLK PLL pairs (old layout, ports 43C8h/43C9h):
 *   f = 14.31818MHz * (N+8) / ((M+2) * 2^K)
 *   43C8h = N | (M bit0 << 7),  43C9h = (M >> 1) | (K << 4)
 */
#define TG_VCLK_25175_LO	0xd7	/* N=87 M=25 K=1 -> 25.188MHz */
#define TG_VCLK_25175_HI	0x1c
#define TG_VCLK_40000_LO	0xbe	/* N=62 M=23 K=0 -> 40.091MHz */
#define TG_VCLK_40000_HI	0x0b

/* Hidden DAC (Command Register) per depth. */
static int
tg_hdr_value(void)
{
	switch (tdisp.bpp) {
	case 24:
		return 0xd0;	/* 8-8-8 packed truecolor */
	case 16:
		return 0x30;	/* 5-6-5 */
	default:
		return 0x00;	/* palette mode */
	}
}

/* CR38 (Pixel Bus) per depth (TGUI96xx values). */
static int
tg_pixelbus_value(void)
{
	switch (tdisp.bpp) {
	case 24:
		return 0x29;
	case 16:
		return 0x05;	/* 16bpp + 16-bit bus */
	default:
		return 0x00;
	}
}

/*
 * Load the DAC.  In the direct-color modes the DAC is bypassed, so a
 * grayscale ramp is loaded just in case.  In 8bpp the palette
 * implements RGB332, matching conv_row8().
 */
static void
tg_load_palette(void)
{
	int i;

	tg_outb(tdisp.io_3c0 + 0x06, 0xff);	/* no pixel mask */
	tg_outb(tdisp.io_3c0 + 0x08, 0x00);	/* write index 0 */

	if (tdisp.bpp == 8) {
		for (i = 0; i < 256; i++) {
			int r = (i >> 5) & 7;
			int g = (i >> 2) & 7;
			int b = i & 3;

			/* 6-bit DAC entries. */
			tg_outb(tdisp.io_3c0 + 0x09, r * 63 / 7);
			tg_outb(tdisp.io_3c0 + 0x09, g * 63 / 7);
			tg_outb(tdisp.io_3c0 + 0x09, b * 63 / 3);
		}
	} else {
		for (i = 0; i < 256; i++) {
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
		}
	}
}

/*
 * The full mode set, assembled from tridentfb's BIOS-less order and
 * XF98's SetRegisters().  Leaves the screen blanked (SR01 bit5); the
 * caller unblanks after clearing VRAM.
 */
static void
tg_modeset(void)
{
	const uint8_t *tab;
	uint32_t offset;
	int i, misc;

	hal_log_info("TRIDENT: setting %dx%d %d bpp (pitch %lu), "
		     "VCLK %s.",
		     tdisp.scr_w, tdisp.scr_h, tdisp.bpp,
		     (unsigned long)tdisp.pitch,
		     tdisp.scr_w == 800 ? "40.0MHz" : "25.175MHz");

	if (tdisp.scr_w == 800) {
		tab = tg_crtc_800x600;
		misc = 0x2b;	/* +h +v sync, VCLK PLL, color, RAM */
	} else {
		tab = tg_crtc_640x480;
		misc = 0xeb;	/* -h -v sync, VCLK PLL, color, RAM */
	}
	offset = tdisp.pitch / 8;

	tg_sw_new();

	/* Sequencer: run, blank, extensions unlocked (bank 0). */
	tg_seq_write(0x00, 0x03);
	tg_seq_write(0x01, 0x21);	/* 8-dot clock, screen off */
	tg_seq_write(0x0e, 0x82);	/* -> stored 80h: ext on, bank 0 */
	tg_seq_write(0x02, 0x0f);	/* plane write mask */
	tg_seq_write(0x03, 0x00);	/* character map */
	tg_seq_write(0x04, 0x0e);	/* memory mode: ext, chain4 */

	/* XF98 SetRegisters: old-mode SR0D = 20h, new-mode SR0D = 0. */
	tg_sw_old();
	tg_seq_write(0x0d, 0x20);
	tg_sw_new();
	tg_seq_write(0x0d, 0x00);

	/* VCLK before MISC selects it. */
	if (tdisp.scr_w == 800) {
		tg_outb(tdisp.io_vclk, TG_VCLK_40000_LO);
		tg_outb(tdisp.io_vclk + 1, TG_VCLK_40000_HI);
	} else {
		tg_outb(tdisp.io_vclk, TG_VCLK_25175_LO);
		tg_outb(tdisp.io_vclk + 1, TG_VCLK_25175_HI);
	}

	/* MISC: clock select 10b = the programmable VCLK. */
	tg_misc_write(misc);

	/* CRTC: unlock, then the timing table with the pitch patched. */
	tg_crtc_write(0x11, tg_crtc_read(0x11) & 0x7f);
	for (i = 0; i < 0x19; i++) {
		if (i == 0x13)
			tg_crtc_write(i, (int)(offset & 0xff));
		else
			tg_crtc_write(i, tab[i]);
	}

	/* CR27: vertical overflow bits 10 (all zero here) + LC bit10. */
	tg_crtc_write(0x27, (tg_crtc_read(0x27) & 0x07) | 0x08);
	/* CR2B: horizontal overflow bits 8 (all zero for our modes). */
	tg_crtc_write(0x2b, 0x00);
	/* CR1E: enable access to extended memory, no interlace. */
	tg_crtc_write(0x1e, 0x80);
	/*
	 * CR21: linear aperture on (base = PCI BAR0).  The Ra43 ITF
	 * leaves C7h here - bits of unknown meaning on the 9682 -
	 * so set bit5 and preserve the rest instead of overwriting.
	 */
	{
		int cr21 = tg_crtc_read(0x21);

		tg_crtc_write(0x21, cr21 | 0x20);
		hal_log_info("TRIDENT: CR21 %02Xh -> %02Xh "
			     "(linear aperture enabled).",
			     cr21, tg_crtc_read(0x21));
	}
	/* CR29: pitch bits 9:8 (keep bit2 - the relay uses it). */
	tg_crtc_write(0x29, (tg_crtc_read(0x29) & 0xcf) |
			    (int)((offset & 0x300) >> 4));
	/* CR2A: 32-bit bus mode. */
	tg_crtc_write(0x2a, tg_crtc_read(0x2a) | 0x40);
	/* CR2F: performance bit (tridentfb/Xorg both set it). */
	tg_crtc_write(0x2f, tg_crtc_read(0x2f) | 0x10);
	/* CR38: pixel bus = the depth. */
	tg_crtc_write(0x38, tg_pixelbus_value());
	/*
	 * CR39: no PCI bursts.  Bit0 gates the BAR1 register MMIO -
	 * clearing it while running over MMIO would saw off the
	 * branch we sit on, so it is preserved (set) in MMIO mode.
	 */
	if (tdisp.use_mmio)
		tg_crtc_write(0x39, (tg_crtc_read(0x39) & ~0x06) | 0x01);
	else
		tg_crtc_write(0x39, tg_crtc_read(0x39) & ~0x07);
	/* CR50: hardware cursor off. */
	tg_crtc_write(0x50, 0x00);

	/* Graphics controller: packed-pixel graphics at A0000. */
	tg_gfx_write(0x00, 0x00);
	tg_gfx_write(0x01, 0x00);
	tg_gfx_write(0x02, 0x00);
	tg_gfx_write(0x03, 0x00);
	tg_gfx_write(0x04, 0x00);
	tg_gfx_write(0x05, 0x40);	/* mode: 256-color shift */
	tg_gfx_write(0x06, 0x05);	/* misc: graphics, A0000 64KB */
	tg_gfx_write(0x07, 0x0f);
	tg_gfx_write(0x08, 0xff);
	/* GR0F: keep the strap bits, extended memory + tridentfb bits. */
	tg_gfx_write(0x0f, (tg_gfx_read(0x0f) & 0xf0) | 0x12);
	/* GR2F: XF98 value (ChipInit 20h, mode set ORs 04h). */
	tg_gfx_write(0x2f, 0x24);

	/* Attribute controller: identity palette + graphics mode. */
	(void)tg_inb(tdisp.io_3da);		/* reset flip-flop */
	for (i = 0; i < 16; i++) {
		tg_outb(tdisp.io_3c0, (uint8_t)i);
		tg_outb(tdisp.io_3c0, (uint8_t)i);
	}
	tg_outb(tdisp.io_3c0, 0x10);
	tg_outb(tdisp.io_3c0, 0x41);	/* graphics, 8-bit color */
	tg_outb(tdisp.io_3c0, 0x11);
	tg_outb(tdisp.io_3c0, 0x00);	/* overscan */
	tg_outb(tdisp.io_3c0, 0x12);
	tg_outb(tdisp.io_3c0, 0x0f);	/* plane enable */
	tg_outb(tdisp.io_3c0, 0x13);
	tg_outb(tdisp.io_3c0, 0x00);	/* pixel panning */
	tg_outb(tdisp.io_3c0, 0x14);
	tg_outb(tdisp.io_3c0, 0x00);	/* color select */
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);	/* re-enable video output */

	/* Hidden DAC: the depth format. */
	tg_hidden_dac_write(tg_hdr_value());

	/* DAC: the RGB332 palette for 8bpp / a ramp otherwise. */
	tg_load_palette();
}

/*
 * Resolve the depth for a request.  req == -1: the highest depth
 * that fits VRAM (these desktops have full 24bpp DACs).
 */
static int
tg_resolve_bpp(int req, int w, int h, uint32_t vram)
{
	int b;

	if (req == -1) {
		b = 24;
		while (b > 8 &&
		       (uint32_t)w * (uint32_t)h * (uint32_t)(b / 8) > vram)
			b = (b == 24) ? 16 : 8;
		if (w == 800 && b == 24)
			b = 16;		/* see the 40MHz note */
		hal_log_info("TRIDENT: auto depth -> %d bpp "
			     "(VRAM %luKB).",
			     b, (unsigned long)(vram >> 10));
		return b;
	}

	if ((uint32_t)w * (uint32_t)h * (uint32_t)(req / 8) > vram) {
		hal_log_info("TRIDENT: %dx%d at %d bpp does not fit "
			     "%luKB VRAM.",
			     w, h, req, (unsigned long)(vram >> 10));
		return -1;
	}
	return req;
}

/*****************************************************************************/
/* Video output relay                                                        */
/*****************************************************************************/

/*
 * Two relay implementations are carried:
 *  - the NT path (default): the sequence NEC's own NT4 trident.sys
 *    performs, extracted by disassembly (see section G);
 *  - the XF98 path (STRATO_TRIDENT_RELAY=xf98): the pc98_tgui.c
 *    dance, kept as a fallback since it too ran on real V13/V16.
 */

/*
 * Relay policy.  The Ra43 field test showed that the full NT
 * sequence (from a 1996 driver that predates the Ra43) kills the
 * sync outputs on that board, while the ITF leaves the NEC glue in
 * a working state (SDAC[04]=0Fh, CR29 bit2, tuned MCLK).  The
 * default is therefore MINIMAL: switch the 0FACh mux and touch
 * nothing NEC-specific.  STRATO_TRIDENT_RELAY=nt|xf98 escalates to
 * the full dances for comparison.
 */
static int
tg_relay_policy(void)
{
	const char *s = getenv("STRATO_TRIDENT_RELAY");

	if (s == NULL)
		return TG_RELAY_MIN;
	if (s[0] == 'x' || s[0] == 'X')
		return TG_RELAY_XF98;
	if (s[0] == 'n' || s[0] == 'N')
		return TG_RELAY_NT;
	return TG_RELAY_MIN;
}

static bool
tg_use_xf98_relay(void)
{
	return tg_relay_policy() == TG_RELAY_XF98;
}

/*
 * The 0FACh relay value.  Field-tested on the Ra43: 02h (XF98's
 * value) selects the accelerator with working sync; 03h (the NT4
 * driver's value, from older machines) kills the sync outputs
 * there.  Default 02h, overridable for experiments.
 */
static int
tg_fac_value(void)
{
	const char *s = getenv("STRATO_TRIDENT_FAC");

	if (s != NULL && s[0] >= '0' && s[0] <= '9')
		return s[0] - '0';
	return 0x02;
}

/*
 * The minimal relay: leave every NEC glue register exactly as the
 * ITF configured it and only flip what selects the output source -
 * the 98 GDC display element and the 0FACh latch.
 */
static void
tg_min_relay_to_accel(void)
{
	outp(0x68, 0x0e);	/* GDC display element off */
	(void)inp(0x5f);
	(void)inp(0x5f);

	/* NT enable path: GR21 bit5 cleared on the way to the accel. */
	tg_gfx_write(0x21, tg_gfx_read(0x21) & ~0x20);

	outp(0x0fac, tg_fac_value());

	/*
	 * NT strobes SDAC[00h] bit6 right after the relay latch -
	 * it looks like a "load settings" pulse for the DAC glue.
	 */
	tg_sdac_write(0x00, tg_sdac_read(0x00) | 0x40);
	tg_wait_ms(1);
	tg_sdac_write(0x00, tg_sdac_read(0x00) & ~0x40);

	/* Overscan black, ATC output enabled. */
	tg_attr_write(0x31, 0x00);
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);
}

static void
tg_min_relay_to_gdc(void)
{
	outp(0x0fac, 0x00);
	(void)inp(0x5f);
	(void)inp(0x5f);

	/* NT disable path: GR21 bit5 set on the way back to the GDC. */
	tg_gfx_write(0x21, tg_gfx_read(0x21) | 0x20);

	outp(0x68, 0x0f);	/* GDC display element on */
}

static void
tg_relay_to_accel(void)
{
	hal_log_info("TRIDENT: relay: 0FACh reads %02Xh, "
		     "SDAC[04]=%02Xh; switching to the accelerator "
		     "(%s sequence).",
		     inp(0x0fac), tg_sdac_read(0x04),
		     tg_relay_policy() == TG_RELAY_XF98 ? "XF98" :
		     tg_relay_policy() == TG_RELAY_NT ? "NT" :
		     "minimal");

	switch (tg_relay_policy()) {
	case TG_RELAY_XF98:
		tg_relay_to_accel_xf98();
		break;
	case TG_RELAY_NT:
		tg_nt_relay_to_accel();
		break;
	default:
		tg_min_relay_to_accel();
		break;
	}

	hal_log_info("TRIDENT: relay: 0FACh now reads %02Xh, "
		     "GR24=%02Xh GR33=%02Xh SDAC[04]=%02Xh.",
		     inp(0x0fac), tg_gfx_read(0x24),
		     tg_gfx_read(0x33), tg_sdac_read(0x04));
}

static void
tg_relay_to_gdc(void)
{
	switch (tg_relay_policy()) {
	case TG_RELAY_XF98:
		tg_relay_to_gdc_xf98();
		break;
	case TG_RELAY_NT:
		tg_nt_relay_to_gdc();
		break;
	default:
		tg_min_relay_to_gdc();
		break;
	}
}

/*
 * Wait for n vertical sync periods by watching Input Status 1 bit3.
 * NEC's NT driver steps the sync power sequence in units of whole
 * frames, so this is load-bearing, not cosmetic.  Timeouts (~25ms
 * per phase, counted in 5Fh wait-port reads) keep a dead or
 * blank-screen chip from hanging the machine.
 */
static void
tg_wait_frames(int n)
{
	long guard;

	while (n-- > 0) {
		guard = 40000L;
		while ((tg_inb(tdisp.io_3da) & 0x08) != 0 && guard-- > 0)
			(void)inp(0x5f);
		guard = 40000L;
		while ((tg_inb(tdisp.io_3da) & 0x08) == 0 && guard-- > 0)
			(void)inp(0x5f);
	}
}

/*
 * The undocumented indexed 16-bit interface at 8F0h (index) / 8F2h
 * (data) that NEC's NT driver brackets the relay with.  Real port
 * I/O, not a chip register.
 */
static void
tg_8f0_rmw(int idx, int and_mask, int or_bits)
{
	unsigned v;

	outpw(0x08f0, (unsigned)idx);
	v = inpw(0x08f2);
	outpw(0x08f2, (v & (unsigned)and_mask) | (unsigned)or_bits);
}

/*
 * The register-script interpreter matching the NT driver's triplet
 * tables: each entry is (selector, mask-or-index, value).
 *   selector 00h-4Fh : CRTC reg, new = (val & mask) | (old & ~mask)
 *   selector 50h-6Fh : shadow CRTC at 3A4h (reg - 50h), gated by
 *                      GR30 bit6 which is restored afterwards
 *   selector 70h-DFh : GR reg (selector - 70h)
 *   selector F0h     : wait <value> vsync frames
 *   selector F2h     : SYNCDAC[<mask>] = value (direct)
 *   selector F3h/F4h : MCLK / VCLK pair (43C6h,43C7h / 43C8h,43C9h)
 */
static void
tg_apply_triplets(const uint8_t *t, int n, const char *tag)
{
	int sel, mask, val, old, gr30;

	hal_log_info("TRIDENT: applying %s (%d entries).", tag, n);
	for (; n-- > 0; t += 3) {
		sel = t[0];
		mask = t[1];
		val = t[2];

		if (sel < 0x50) {
			if (sel == 0x11) {
				/* respect the CR0-7 protect bit */
				old = tg_crtc_read(0x11);
				tg_crtc_write(0x11, (val & mask) |
						    (old & ~mask));
				continue;
			}
			old = tg_crtc_read(sel);
			tg_crtc_write(sel, (val & mask) | (old & ~mask));
		} else if (sel < 0x70) {
			gr30 = tg_gfx_read(0x30);
			if ((gr30 & 0x40) == 0)
				tg_gfx_write(0x30, gr30 | 0x40);
			tg_outb(0x03a4, sel - 0x50);
			old = tg_inb(0x03a5);
			tg_outb(0x03a5, (val & mask) | (old & ~mask));
			if ((gr30 & 0x40) == 0)
				tg_gfx_write(0x30, gr30);
		} else if (sel < 0xe0) {
			old = tg_gfx_read(sel - 0x70);
			tg_gfx_write(sel - 0x70,
				     (val & mask) | (old & ~mask));
		} else if (sel == 0xf0) {
			tg_wait_frames(val);
		} else if (sel == 0xf2) {
			tg_sdac_write(mask, val);
		} else if (sel == 0xf3) {
			tg_outb(tdisp.io_vclk - 2, mask);
			tg_outb(tdisp.io_vclk - 1, val);
		} else if (sel == 0xf4) {
			tg_outb(tdisp.io_vclk, mask);
			tg_outb(tdisp.io_vclk + 1, val);
		}
	}
}

/*
 * The tables below are byte-exact extractions from NEC's NT4
 * trident.sys (.data), re-encoded for the interpreter above (the
 * binary uses high-nibble dispatch; here CRTC = raw index, shadow
 * CRTC = +50h, GR = +70h).
 */





/*
 * Monitor sense, NT style: GR42 bit7, mapped through the machine
 * type 27h row of the NT code table (93h = one monitor family,
 * 13h = the other).  The code is then folded back into GR42/GR43
 * and SR0F bit2 is cleared, exactly as the NT driver does.
 */
static int
tg_nt_monitor_sense(void)
{
	int code, v;

	code = (tg_gfx_read(0x42) & 0x80) ? 0x13 : 0x93;

	v = tg_gfx_read(0x42);
	tg_gfx_write(0x42, (v & 0x0f) | ((code & 0x0f) << 4));
	v = tg_gfx_read(0x43);
	tg_gfx_write(0x43, (v & 0x07) | (code & 0xf8));
	tg_seq_write(0x0f, tg_seq_read(0x0f) & ~0x04);

	hal_log_info("TRIDENT: monitor sense: GR42 bit7 -> code %02Xh.",
		     code);
	return code;
}

/* Enable the sync outputs (NT display-on path). */
static void
tg_nt_sync_on(void)
{
	int i;

	tg_gfx_write(0x33, tg_gfx_read(0x33) | 0x20);
	tg_apply_triplets(tg_glue_sync_on,
			  sizeof(tg_glue_sync_on) / 3, "sync-on script");

	/* GR5A scratch flags, as the NT driver maintains them. */
	tg_gfx_write(0x5a, (tg_gfx_read(0x5a) & ~0x04) | 0x03);

	/* Wait for the settle flag (GR23 bit4), max ~15 frames. */
	tg_wait_frames(1);
	for (i = 0; i < 15; i++) {
		if ((tg_gfx_read(0x23) & 0x10) == 0)
			break;
		tg_wait_frames(1);
	}
	hal_log_info("TRIDENT: sync-on settled after %d extra "
		     "frame(s), GR23=%02Xh.", i, tg_gfx_read(0x23));
}

/* Disable the sync outputs (NT display-off path). */
static void
tg_nt_sync_off(void)
{
	tg_apply_triplets(tg_glue_sync_off,
			  sizeof(tg_glue_sync_off) / 3, "sync-off script");
	tg_gfx_write(0x5a, (tg_gfx_read(0x5a) & ~0x03) | 0x04);
}

/*
 * The NT relay to the accelerator (trident.sys 10F04h, byte-faithful
 * where possible; the 0x21-byte table copies are pre-baked above).
 */
static void
tg_nt_relay_to_accel(void)
{
	int code;

	/* Bracket open on the undocumented 8F0h interface. */
	tg_8f0_rmw(0x52, 0xffff, 0x0080);

	/* 98 GDC display element off. */
	outp(0x68, 0x0e);
	outp(0x6a, 0x07);
	outp(0x6a, 0x8f);
	outp(0x6a, 0x06);
	(void)inp(0x5f);
	(void)inp(0x5f);
	(void)inp(0x5f);

	/* Re-assert video subsystem enable through the register path. */
	tg_outb(tdisp.io_3c0 + 0x03, 0x01);	/* 3C3h */
	tg_outb(0x46e8, 0x08);
	tg_misc_write(tg_misc_read() | 0x01);
	tg_seq_write(0x0f, tg_seq_read(0x0f) & ~0x10);

	/* Sync path handover. */
	tg_crtc_write(0x23, tg_crtc_read(0x23) & ~0x20);
	tg_gfx_write(0x2c, tg_gfx_read(0x2c) | 0x06);
	tg_gfx_write(0x21, tg_gfx_read(0x21) & ~0x28);
	tg_wait_ms(2);

	code = tg_nt_monitor_sense();

	/* The relay latch: NT writes 03h here, not XF98's 02h. */
	outp(0x0fac, 0x03);
	if (code != 0x93) {
		outp(0x0faa, 0x84);
		outp(0x0fab, inp(0x0fab) | 0x11);
		hal_log_info("TRIDENT: relay: FAA[84h] |= 11h "
			     "(monitor code %02Xh).", code);
	}
	(void)inp(0x5f);
	(void)inp(0x5f);
	(void)inp(0x5f);

	/* Bracket close. */
	tg_8f0_rmw(0x60, 0xffef, 0x0000);

	/* Overscan black; keep the ATC video output enabled. */
	tg_attr_write(0x31, 0x00);
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);

	/* Sync outputs up, one bit per frame. */
	tg_nt_sync_on();
}

/* The NT relay back to the 98 GDC (trident.sys 11109h). */
static void
tg_nt_relay_to_gdc(void)
{
	int code;

	/* Sync outputs down first (NT display-off path). */
	tg_nt_sync_off();

	tg_8f0_rmw(0x60, 0xffff, 0x0010);
	tg_8f0_rmw(0x52, 0xffff, 0x0080);

	/* Overscan black. */
	tg_attr_write(0x11, 0x00);

	tg_sw_new();
	tg_seq_write(0x0e, tg_seq_read(0x0e) | 0x80);

	tg_crtc_write(0x23, tg_crtc_read(0x23) | 0x20);
	(void)inp(0x5f);
	(void)inp(0x5f);
	(void)inp(0x5f);

	/* 98 GDC display element on. */
	outp(0x6a, 0x07);
	outp(0x6a, 0x8e);
	outp(0x6a, 0x06);
	(void)inp(0x5f);
	(void)inp(0x5f);
	(void)inp(0x5f);

	tg_gfx_write(0x2c, tg_gfx_read(0x2c) | 0x06);

	code = (tg_gfx_read(0x42) & 0x80) ? 0x13 : 0x93;
	outp(0x0fac, 0x00);
	if (code != 0x93) {
		outp(0x0faa, 0x84);
		outp(0x0fab, inp(0x0fab) & ~0x11);
	}
	(void)inp(0x5f);
	(void)inp(0x5f);
	(void)inp(0x5f);

	if (code == 0x93)
		tg_wait_ms(16);		/* the NT extra settle */
	tg_wait_ms(4);

	outp(0x68, 0x0f);
	tg_8f0_rmw(0x52, 0xff7f, 0x0000);
}

/*****************************************************************************/
/* Video output relay - XF98 variant (pc98_tgui.c crtswNEC96xx())            */
/*****************************************************************************/

static void
tg_relay_to_accel_xf98(void)
{
	/* 1. The 98 GDC side off (crtswNECGen(1)). */
	outp(0x68, 0x0e);
	outp(0x6a, 0x07);
	outp(0x6a, 0x8f);
	outp(0x6a, 0x06);
	if (tdisp.hsync31 == 0)
		outp(0x09a8, 0x01);	/* 24.8kHz -> 31.5kHz */

	/* 2. The Trident sync path on (crtswTGUiGen(1)). */
	tg_sw_new();
	tg_select_crtc(tg_misc_read());
	tg_crtc_write(0x23, tg_crtc_read(0x23) & 0xdf);
	tg_crtc_write(0x29, tg_crtc_read(0x29) | 0x04);

	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x06);
	tg_wait_ms(1);
	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x08);
	tg_gfx_write(0x23, tg_gfx_read(0x23) & ~0x03);
	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x01);
	tg_seq_write(0x01, tg_seq_read(0x01) & ~0x10);

	/* 3. The relay latch. */
	outp(0x0fac, 0x02);
}

static void
tg_relay_to_gdc_xf98(void)
{
	/* 1. The relay latch back. */
	outp(0x0fac, 0x00);

	/* 2. The Trident sync path off (crtswTGUiGen(0)). */
	tg_sw_new();
	tg_select_crtc(tg_misc_read());
	tg_seq_write(0x01, tg_seq_read(0x01) | 0x10);
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x01);
	tg_gfx_write(0x23, 0x01 | (tg_gfx_read(0x23) & ~0x03));
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x02);
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x30);
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x08);
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x04);
	tg_crtc_write(0x29, tg_crtc_read(0x29) & ~0x04);
	tg_crtc_write(0x23, tg_crtc_read(0x23) | 0x20);

	/* 3. The 98 GDC side on (crtswNECGen(0)). */
	if (tdisp.hsync31 == 0)
		outp(0x09a8, 0x00);	/* back to 24.8kHz */
	outp(0x6a, 0x07);
	outp(0x6a, 0x8e);
	outp(0x6a, 0x06);
	outp(0x68, 0x0f);
}

/*****************************************************************************/
/* State save/restore                                                        */
/*****************************************************************************/

/*
 * Save ranges follow NEC's own NT driver, which snapshots CRTC
 * 00h-50h and GR 00h-5Fh (we take 00h-6Fh: the NEC sync glue also
 * writes GR60h-6Dh) plus the clocks.  The 3A4h shadow CRTC bank is
 * saved too since the sync glue programs it.
 */
static uint8_t sv_crtc[0x51];
static uint8_t sv_sr[0x10];		/* new mode; 0Bh skipped */
static uint8_t sv_sr0d_old, sv_sr0d_new, sv_sr0e_new;
static uint8_t sv_gr[0x70];
static uint8_t sv_crb[0x19];		/* 3A4h shadow CRTC */
static uint8_t sv_gr30;
static uint8_t sv_attr[0x15];
static uint8_t sv_misc, sv_hdr, sv_dac_mask;
static uint8_t sv_vclk_lo, sv_vclk_hi, sv_mclk_lo, sv_mclk_hi;
static uint8_t sv_sdac[5];
static const uint8_t tg_sdac_idx[5] = { 0x00, 0x04, 0x08, 0x09, 0x37 };
static uint8_t sv_sdac38;
static uint8_t sv_dac[256 * 3];

static void
tg_save_state(void)
{
	int i;

	sv_misc = (uint8_t)tg_misc_read();
	tg_select_crtc(sv_misc);

	/* Both flavors of the mode-dependent sequencer registers. */
	tg_sw_new();
	sv_sr0d_new = (uint8_t)tg_seq_read(0x0d);
	sv_sr0e_new = (uint8_t)tg_seq_read(0x0e);
	tg_sw_old();
	sv_sr0d_old = (uint8_t)tg_seq_read(0x0d);
	tg_sw_new();

	for (i = 0; i < 0x10; i++) {
		if (i == 0x0b || i == 0x0d || i == 0x0e) {
			sv_sr[i] = 0;
			continue;	/* handled above / mode switch */
		}
		sv_sr[i] = (uint8_t)tg_seq_read(i);
	}

	for (i = 0; i <= 0x50; i++)
		sv_crtc[i] = (uint8_t)tg_crtc_read(i);

	for (i = 0; i < 0x70; i++)
		sv_gr[i] = (uint8_t)tg_gfx_read(i);

	/* The 3A4h shadow CRTC bank, gated by GR30 bit6. */
	sv_gr30 = sv_gr[0x30];
	if ((sv_gr30 & 0x40) == 0)
		tg_gfx_write(0x30, sv_gr30 | 0x40);
	for (i = 0; i <= 0x18; i++) {
		tg_outb(0x03a4, i);
		sv_crb[i] = (uint8_t)tg_inb(0x03a5);
	}
	if ((sv_gr30 & 0x40) == 0)
		tg_gfx_write(0x30, sv_gr30);

	for (i = 0; i < 0x15; i++)
		sv_attr[i] = (uint8_t)tg_attr_read(i);

	sv_hdr = (uint8_t)tg_hidden_dac_read();
	sv_dac_mask = (uint8_t)tg_inb(tdisp.io_3c0 + 0x06);

	tg_outb(tdisp.io_3c0 + 0x07, 0x00);
	for (i = 0; i < 256 * 3; i++)
		sv_dac[i] = (uint8_t)tg_inb(tdisp.io_3c0 + 0x09);

	sv_vclk_lo = (uint8_t)tg_inb(tdisp.io_vclk);
	sv_vclk_hi = (uint8_t)tg_inb(tdisp.io_vclk + 1);
	sv_mclk_lo = (uint8_t)tg_inb(tdisp.io_vclk - 2);
	sv_mclk_hi = (uint8_t)tg_inb(tdisp.io_vclk - 1);

	for (i = 0; i < 5; i++)
		sv_sdac[i] = (uint8_t)tg_sdac_read(tg_sdac_idx[i]);
	sv_sdac38 = (uint8_t)tg_sdac_read(0x38);
}

static void
tg_restore_state(void)
{
	int i;

	tg_sw_new();

	/* Sequencer basics (skip the mode-dependent ones for now). */
	for (i = 0; i < 0x10; i++) {
		if (i == 0x0b || i == 0x0d || i == 0x0e)
			continue;
		if (i == 0x01)
			continue;	/* unblank last */
		tg_seq_write(i, sv_sr[i]);
	}
	tg_sw_old();
	tg_seq_write(0x0d, sv_sr0d_old);
	tg_sw_new();
	tg_seq_write(0x0d, sv_sr0d_new);

	/*
	 * GR file: restore GR30 last within the block so the shadow
	 * CRTC gate finishes in its saved position, and put the
	 * shadow bank back while the gate is forced open.
	 */
	for (i = 0; i < 0x70; i++) {
		if (i == 0x30)
			continue;
		tg_gfx_write(i, sv_gr[i]);
	}
	tg_gfx_write(0x30, sv_gr30 | 0x40);
	for (i = 0; i <= 0x18; i++) {
		if (i == 0x11)
			continue;	/* handled below */
		tg_outb(0x03a4, i);
		tg_outb(0x03a5, sv_crb[i]);
	}
	tg_outb(0x03a4, 0x11);
	tg_outb(0x03a5, sv_crb[0x11]);
	tg_gfx_write(0x30, sv_gr30);

	/* MISC first: bit0 re-selects the CRTC base for the writes. */
	tg_misc_write(sv_misc);

	/* CRTC 00h-50h: unlock CR0-7 first, CR11 written back last. */
	tg_crtc_write(0x11, sv_crtc[0x11] & 0x7f);
	for (i = 0; i <= 0x50; i++) {
		if (i == 0x11)
			continue;
		tg_crtc_write(i, sv_crtc[i]);
	}
	tg_crtc_write(0x11, sv_crtc[0x11]);

	for (i = 0; i < 0x15; i++)
		tg_attr_write(i, sv_attr[i]);
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);

	/* DAC + hidden DAC + pixel mask. */
	tg_outb(tdisp.io_3c0 + 0x08, 0x00);
	for (i = 0; i < 256 * 3; i++)
		tg_outb(tdisp.io_3c0 + 0x09, sv_dac[i]);
	tg_hidden_dac_write(sv_hdr);
	tg_outb(tdisp.io_3c0 + 0x06, sv_dac_mask);

	/* Clocks. */
	tg_outb(tdisp.io_vclk, sv_vclk_lo);
	tg_outb(tdisp.io_vclk + 1, sv_vclk_hi);
	tg_outb(tdisp.io_vclk - 2, sv_mclk_lo);
	tg_outb(tdisp.io_vclk - 1, sv_mclk_hi);

	/* The NEC glue. */
	tg_sdac_write(0x38, sv_sdac38);
	for (i = 4; i >= 0; i--)
		tg_sdac_write(tg_sdac_idx[i], sv_sdac[i]);

	/*
	 * SR0E last: writes in the new mode invert bit1, so pre-XOR
	 * to land the stored value exactly where it was.  Unblank via
	 * the saved SR01 as the final act.
	 */
	tg_seq_write(0x0e, sv_sr0e_new ^ 0x02);
	tg_seq_write(0x01, sv_sr[0x01]);
}

/*****************************************************************************/
/* Misc                                                                      */
/*****************************************************************************/

/*
 * DPMI 0x0800: Map a physical address into linear address space.
 * (DOS/4GW uses a zero-based flat address space, so the returned
 * linear address is directly usable as a pointer.)
 */
static void *
tg_map_physical(uint32_t phys, uint32_t size)
{
	union REGS r;

	if (phys < 0x100000UL)
		return (void *)phys;	/* first MB is identity-mapped */

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0800;
	r.w.bx = (uint16_t)(phys >> 16);
	r.w.cx = (uint16_t)(phys & 0xffff);
	r.w.si = (uint16_t)(size >> 16);
	r.w.di = (uint16_t)(size & 0xffff);
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return NULL;

	return (void *)(((uint32_t)r.w.bx << 16) | r.w.cx);
}
