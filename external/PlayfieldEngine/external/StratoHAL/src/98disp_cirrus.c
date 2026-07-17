/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * CIRRUS CL-GD54xx/75xx display driver for NEC PC-98 (DOS/4G).
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
 * PC-98 built-in Cirrus graphics: analysis summary
 * ===========================================================================
 *
 * Two distinct families of Cirrus built-ins exist on PC-9821:
 *
 *  (A) The C-bus "window accelerator" (WAB) built-ins: CL-GD5428/
 *      5430/5440.  Detected by the machine ID at the two-stage I/O
 *      0FAAh/0FABh; VGA registers at the relocated PC-98 addresses;
 *      VRAM through a banked 32KB window at F20000h.
 *  (B) The PCI built-ins: CL-GD7548 (98NOTE Lavie Nb10/Na13...),
 *      CL-GD7555/7556 (La/Aile) and the desktop Alpine parts
 *      (GD5430...5480).  Detected via PCI configuration space; VGA
 *      registers usually at the native 3C0h block; VRAM through the
 *      linear PCI aperture.
 *
 * ---------------------------------------------------------------------------
 * A. WAB machines (GD5428/5430/5440)
 * ---------------------------------------------------------------------------
 * Machines: PC-9821 Bp/Bs/Be/Bf/Ts/Cs2/Np/Ne2/Nd/Cb/Cx/Cf/Nf/Xe/Cb2/
 * Cx2/Xe10/Xa7e, PC-9801 BX4, SV-98 model1/2/3.
 *
 *  - Two-stage indexed I/O at 0FAAh (index) / 0FABh (data):
 *      reg 00h: machine ID (RO).  50h-5Dh and 70h = Cirrus models;
 *               other values are S3/Matrox/Trident WABs; 00h/FFh =
 *               no two-stage accelerator.  The PCI models (Nb10 and
 *               friends) read FFh here.
 *               ** The ID is only a HINT.  The ValueStar V16
 *               (onboard Trident TGUI9680XGi) reads 5Bh - inside
 *               the supposed Cirrus range - and its relay/window
 *               registers work, but the VGA file at 0CA0h floats
 *               (CR27 reads FFh).  Switching the relay anyway gives
 *               a white, torn screen.  So the chip itself must be
 *               fingerprinted (SR06 unlock readback = 12h and a
 *               plausible CR27) before anything is programmed. **
 *      reg 01h: VRAM window placement.  Values 80h/A0h/C0h/E0h put
 *               the 32KB window at F20000/F00000/F60000/F40000.
 *               (The NT miniport also honors reg 04h bits2:0:
 *               0-3 -> F00000/F20000/F40000/F60000.)
 *      reg 03h: bit1 = video output relay (1: accelerator, 0: the
 *               98 GDC), bit0 = register/MMIO access enable.
 *               Semantics per NP21/W cirrusvga_ofab(); an earlier
 *               revision of this driver had the two bits swapped, so
 *               the relay was never switched back on cleanup.
 *  - Wakeup: 0904h bit5 enables access to POS102 (FF82h); POS102
 *    bit0 and the Sleep Address register (native 3C3h -> relocated
 *    0CA3h) bit0 enable the video subsystem.
 *  - VGA register relocation: 3C0h-3CFh -> 0CA0h-0CAFh, 3D4h ->
 *    0DA4h, 3DAh -> 0DAAh; the mono block 3B0h-3BFh -> 0BA0h-0BAFh
 *    (the NT miniport's access ranges list all three 0BA0h/0CA0h/
 *    0DA0h blocks).  B-MATE would use 0C50h/0D54h/0D5Ah instead
 *    (not needed so far).
 *  - VRAM: 1MB, banked through the 32KB window.  GR09/GR0A are the
 *    bank offset registers, GR0B bit5 selects 16KB granularity.
 *    The mainboard VRAM is shared with the 98 GDC: port 6Ah, 8Eh =
 *    GDC owns it, 8Fh = the accelerator owns it.
 *  - The PCI-era WAB models (Xa7e etc.) add a second relay latch at
 *    0FACh; writing it is harmless on the others.
 *  - The 98NOTE WAB models cannot drive their panels at 24bpp; the
 *    useful depth per machine ID (nec_clgd.txt):
 *        53h Ns / 54h Ts / 56h Ne2: TFT, 4096 colors -> 16bpp
 *        55h Np,Es / 70h Nf:        TFT, full color  -> 16bpp
 *        57h Nd:                    DSTN, 512 colors ->  8bpp
 *        desktops:                                      24bpp
 *
 * Chip programming notes (PC-98 has no VGA BIOS, so nothing has ever
 * POSTed the chip; every register must be set by hand and leftover
 * state from previous drivers must be cleaned):
 *  - CR27 >= A0h identifies the Alpine (GD5430/5440) register
 *    semantics vs. the plain GD5428.
 *  - On Alpine, SR1E bit7 MUST be set (6-bit VCLK denominator
 *    select; cirrusfb: "ONLY 5434!!! (bugged me 10 days)").  Without
 *    it the pixel clock and sync frequencies come out wrong on real
 *    silicon.  Emulators ignore the clock registers entirely, which
 *    is why this is invisible on NP21/W.
 *  - SR1F bit6 = "derive VCLK from MCLK".  A previous driver (e.g.
 *    the Windows one) may have left it set, silently overriding
 *    SR0E/SR1E.  Clear it but preserve the MCLK frequency bits NEC
 *    programmed for the board's DRAM.
 *  - Horizontal Blanking End is an 8-bit compare on Cirrus
 *    (CR03[4:0] + CR05[7] + CR1A[5:4]); program the full horizontal
 *    total or the blanking pulse never terminates - a torn or blank
 *    picture on real hardware that emulators (which ignore blanking
 *    timing) will never show.
 *  - The Hidden DAC register is reached by reading 3C6h four times;
 *    the fifth access hits it.
 *
 * ---------------------------------------------------------------------------
 * B. PCI built-ins, in particular the CL-GD7548 (PC-9821 Nb10)
 * ---------------------------------------------------------------------------
 * PCI topology of the Nb10 (verified on hardware):
 *
 *    0:0.0  8086:1235  class 06h  Intel 430MX host bridge
 *    0:2.0  1033:0009  class 03h  NEC graphics bus bridge.
 *                      BAR0/BAR1 = 0, cmd = 0003h; cfg[40h] =
 *                      00000002h, cfg[44h] = 00000001h - purpose
 *                      unknown, sweeping the values changes nothing.
 *    0:3.0  1013:0038  class 03h  CL-GD7548.  BAR0 = F0000000h
 *                      (size mask FF000000h = a genuine 16MB BAR),
 *                      BAR1 = 0, cmd = 0003h, cfg[40h-FFh] all zero.
 *
 *  - Part ID CR27 = 38h, continuing VGADOC's 0Bh=7542 / 0Ch=7543 /
 *    0Dh=7541 lineage; the NT driver's internal chip tag is 0Eh.
 *  - VGA registers respond at the NATIVE 3C0h/3D4h block (no
 *    relocation).
 *  - Panel: 640x480 TFT (CR2C = C0h).  VRAM: 1MB, dedicated.
 *  - Video output relay: port 0FACh.  Writing 00h -> 02h latches and
 *    actually switches the panel to the accelerator (verified).
 *  - The ITF firmware POSTs the chip; observed defaults:
 *        SR07=C0 SR09=00 SR0F=11 SR14=00 SR16=F0 SR17=01 SR1F=18
 *        GR0B=00 SR2D=00 CR20=02 CR2C=C0 CR2D=00
 *    SR1F=18h means MCLK is programmed - the "dead because nothing
 *    initialized it" theory was ruled out.
 *
 * Symptom that stalled this port: the chip, its VRAM, the BitBLT
 * engine, the scanout and the relay are all alive (register-driven
 * BLT fills reach the panel - white screen / stripe pattern tests),
 * yet reads at BAR0+0 return 00h (decode exists, data does not flow)
 * and writes go nowhere, for every SR07 upper-nibble candidate
 * (F0/C0/00/10/A0), every enable combination ({none, 0FACh=02,
 * 6Ah=8Fh, both, 0904h+FF82h}) and every 4MB quadrant of the BAR;
 * CPU-source BLT FIFO pushes starve everywhere too.  Only the host
 * path looked closed.
 *
 * False positives to beware of on this machine:
 *  - F00000h / C00000h / F20000h are plain system RAM.  Reads and
 *    writes "succeed" but it is not VRAM.
 *  - The BIOS work area 0401h (extended memory amount) is rewritten
 *    by HIMEM (allocations are subtracted), so it cannot be used as
 *    a RAM-amount guard; it reads 1024KB on the test machine.
 *  - The reliable oracle is the BLT engine: fill VRAM with a BLT and
 *    check whether the candidate window follows.  Plain RAM cannot
 *    follow a BLT, so it can never misidentify.
 *
 * The breakthrough came from disassembling NEC's own PC-98 NT 4.0
 * miniport CIRRUS.SYS (PE32 native i386, 50,832 bytes, 1996-10-13),
 * which drives this exact chip.  It does not import VideoPortInt10 -
 * it carries a complete VGA-BIOS-less initialization.  Findings (VAs
 * refer to that binary):
 *
 *  - VA 192EDh, the branch taken when [esi+40h] == 40h (= the 7548
 *    path): reads BAR0 from PCI config space (CF8h/CFCh), then ADDS
 *    0C00000h.  ** The 7548 framebuffer is at BAR0 + 12MB, linear,
 *    1MB long - NOT at BAR0 + 0. **  This is the root cause of the
 *    long stall.
 *  - VA 19475h: [esi+40h] == 4 -> banked (window length 20000h =
 *    128KB, VideoPortMapBankedMemory, bank switch writes bank*2 to
 *    GR09/GR0A = 16KB units); anything else -> linear 1MB.  The
 *    7548 is 40h -> linear.
 *  - VA 1A212h, machine dispatch on the Part ID:
 *        38h (7548): tag 0Eh, panel flag 0, [esi+40h]=40h, linear
 *                    1MB, register base swapped to native
 *                    3B0h/3C0h/3D0h.
 *        3Eh / 47h : read port 4B8Eh bits1:0 -> tag 0Fh / 10h.
 *        41h       : write 60h to 8F0h, read 8F2h, bit0 -> tag
 *                    12h / 13h.
 *        others    : bail out with error 37h.
 *  - Access ranges (.data VA 113A0h, defaults; swapped per machine):
 *        I/O: 0BA0h (len 210h), 0CA0h, 0DA0h  (relocated VGA blocks)
 *             5Fh, 68h, 6Ah, A2h
 *             0FAAh/0FABh (variant machines: 0FA2h/0FA3h)
 *             0904h, 0FF82h (variant: 0902h), 09A8h
 *             0FACh (the relay), 0C8Eh, 08F0h, 08F2h
 *        Mem: 00F00000h len 20000h - replaced by BAR0+12MB on the
 *             7548 path, where the port offset table (VA 114B0h+)
 *             also becomes native (+04h=3B4h, +14h=3C4h, +24h=3D4h).
 *  - VA 1B033h: linear machines ([esi+40h] == 8 or 40h) get
 *    SR17 |= 44h.  VGADOC: SR17 bit2 = Enable Memory-Mapped I/O,
 *    bit6 = place the MMIO block in the LAST 256 BYTES of the linear
 *    memory block.  NP21/W's cirrus_linear_writeb uses
 *    (SR17 & 44h) == 44h as its MMIO predicate.
 *  - Mode table at VA 17AE0h, stride 90h, 27 modes.  Layout:
 *    +00h AttributeFlags, +02h planes, +04h bpp, +0Ah/+0Ch/+0Eh
 *    width/height/stride, +14h/+18h X/YMillimeter, +1Ch Frequency,
 *    +34h valid flag, +38h + tag*4 = per-chip-tag command stream
 *    pointer array, +8Ch extra stream.  The interpreter at VA 1AC74h
 *    executes NT4 DDK CMDCNST.H command streams ([ebp-1] selects
 *    MMIO wrappers vs. port I/O).
 *  - Streams valid for tag 0Eh (7548):
 *        mode  8: 640x480   8bpp, stride  640, VA 14080h  (ported)
 *        mode 16: 640x480  16bpp, stride 1280, VA 141F0h  (ported)
 *        mode 23: 640x480  24bpp, stride 2048, VA 14360h  (ported)
 *        mode 11: 800x600   8bpp,             VA 144D0h  (unported)
 *        mode 18: 800x600  16bpp,             VA 14640h  (unported)
 *        mode 13: 1024x768  8bpp,             VA 147B0h  (unported)
 *    (VgaSetMode swaps tag 0Ch for 0Dh when CR2C[7:6]==11b (TFT);
 *    the 7548's tag 0Eh is not subject to that swap.)
 *  - The captured mode set drives the CRTC at the MONO block
 *    3B4h/3B5h because it programs MISC bit0 = 0; it includes the
 *    754x LCD shadow-bank dance (CR2D=80h ... shadow CRTC values ...
 *    CR2D=11h) in the middle of the CRTC list; and it uses fixed
 *    per-depth values:
 *          8bpp : SR07=C1 SR0E=6E SR16=F3 SR1E=2A MISC=E2 HDAC=00
 *          16bpp: SR07=C3 SR0E=66 SR16=F7 SR1E=3A MISC=EE HDAC=E1
 *          24bpp: SR07=C5 SR0E=6E SR16=FE SR1E=2A MISC=EE HDAC=E5
 *    The SR07 upper nibble KEEPS the firmware's Ch - a blind sweep
 *    that tried F1/01/11/A1 could never land on the one correct C3.
 *    The low nibble is the per-depth clocking.  Strides are 640/
 *    1280/2048: note the 24bpp pitch is 2048, not 640*3=1920
 *    (CR13=00h with CR1B bit4 = offset bit8 -> offset 100h chars).
 *    CRTC deltas per depth: CR04/05 = 55/9F (8bpp) vs 54/9E
 *    (16/24), CR13 = 50/A0/00, CR1B = 02/02/12, CR40 = C0/BF/BF,
 *    CR43 = 02/01/00.
 *  - 754x shadow register semantics (confirmed against the NT4 DDK
 *    sample SR754X.C):
 *        CR2C bit3   = 0: access the vertical shadow registers
 *                      (CR6,7,10,11,15,16)
 *        CR2C bit5:4 = 0: X shadow set (with CR2D bit7), 2: Y, 3: Z
 *        CR2D bit7   = access the LCD timing registers
 *                      (CR19h-30h, CR40h-4Fh)
 *
 * Ecosystem status (why nobody else's code helps here):
 *  - XFree86/X.Org supports only the CLGD755x generation on PC-98
 *    (La series / Aile); the only delta La13 needed was VRAM size
 *    detection, i.e. THAT generation's BAR opens plainly.  Nobody
 *    ever ported the 7548 generation (Nb10 / Na13).
 *  - Windows 3.1 on the Nb10 ships EGCN4.DRV, the standard NEC EGC
 *    driver: it never touches the 7548 at all (GDC passthrough), so
 *    "Windows displays fine" proves nothing about the host path.
 *  - NP21/W: the PCI CL-GD5446 model works with plain BAR access
 *    (24bpp, 2MB VRAM, relay 0FACh FCh -> FEh), and its AUTO WAB
 *    type morphs into an Xe10 the moment 0FAAh is touched - pin the
 *    emulator to a fixed PCI type when testing this path.
 *
 * Open items:
 *  - Real-hardware confirmation that the +12MB aperture works.
 *  - The 800x600 / 1024x768 754x streams are unported (needed for
 *    the larger Lavie panels, e.g. Na13).
 *  - Ports 0FA2h/0FA3h (used by NT's variant machines instead of
 *    0FAAh/0FABh) are worth cross-checking against the WAB path.
 *  - Ports 4B8Eh, 8F0h/8F2h, 0C8Eh, 09A8h, 0902h/0904h, 0FF82h:
 *    detailed purpose unknown (unused on the 7548 path).
 *  - CR29 bit6 flutters run-to-run (08h<->48h): suspected V-Port
 *    live status.
 *  - The meaning of BAR1's low bits (& E0h) read by the
 *    [esi+40h]==80h branch (BAR1=0 on the 7548, irrelevant there).
 *
 * References:
 *  - NEC PC-98 NT4 miniport CIRRUS.SYS disassembly (primary source:
 *    FB address, mode streams, SR17, access ranges)
 *  - NT4 DDK video/miniport/cirrus (CMDCNST.H stream format,
 *    SR754X.C, structure layouts)
 *  - VGADOC CIRRUS.TXT (SR07[7:4] map field, SR09[3:0] VRAM size,
 *    CR2C/CR2D)
 *  - Linux cirrusfb (Alpine init, chip-side mode values)
 *  - XFree86 alp_driver.c (754x VRAM sizing, LCD probe)
 *  - NP21/W wab/wab.c, wab/cirrus_vga.c (WAB relay semantics, PCI
 *    model behavior)
 *  - UNDOCUMENTED 9801/9821 Vol.2 io_wab.txt / nec_clgd.txt
 *  - CL-GD7548 Product Bulletin (dual aperture, programmable linear
 *    addressing)
 *
 * ===========================================================================
 * Driver design
 * ===========================================================================
 *  - cirrus_init_disp(mode, bpp) probes the WAB interface first
 *    (desktop machines take priority; a machine without the WAB
 *    ports is assumed to be a PCI 98NOTE), then PCI.  Each probe is
 *    non-destructive until a chip is positively identified; if
 *    neither finds a Cirrus, false is returned so the main code can
 *    try other display chips.
 *  - Aperture-only: frames are written straight into VRAM - through
 *    the banked 32KB window on WAB machines, through the linear PCI
 *    aperture on the PCI machines.  The BLT engine is not used.
 *  - Modes: the WAB and desktop-PCI paths support 640x480 (8/16/24
 *    bpp) and 800x600 (8/16 bpp; 24bpp does not fit 1MB VRAM).  The
 *    754x laptop path supports 640x480 only (NEC's streams for the
 *    larger modes exist but are unported).  1024x768/1280x1024 are
 *    rejected.
 *  - bpp == -1 picks the highest depth the machine supports: 24 on
 *    desktops, the panel cap on WAB 98NOTEs, and NEC's panel default
 *    of 16bpp on the 754x laptops (24bpp remains selectable
 *    explicitly; the NT miniport offers it too).
 *  - STRATO_CIRRUS_FORCE=54|75 restricts the probe to one family
 *    for real-hardware bring-up.
 *  - Verbose by design: this driver only runs when the user passes
 *    the -24 option, so the detailed WAB/PCI/register dumps below
 *    are expected output, and they have repeatedly been the only
 *    way to debug real hardware.
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

/* Cirrus bank granularity for the 32KB window: 16KB (GR0B bit5 = 1) */
#define CL_BANK_SHIFT	14
#define CL_BANK_MASK	0x3fff

enum cirrus_path {
	CIRRUS_PATH_NONE = 0,
	CIRRUS_PATH_54,		/* C-bus WAB built-in */
	CIRRUS_PATH_75		/* PCI built-in */
};

static struct cirrus_disp {
	enum cirrus_path path;
	const char *chip_name;

	/* Screen geometry. */
	int scr_w;		/* 640 or 800 */
	int scr_h;		/* 480 or 600 */
	int bpp;		/* 24, 16 or 8 */
	uint32_t pitch;		/* bytes per scanline (may exceed w*bpp/8) */

	/*
	 * Relocatable VGA register file.  io_3d4/io_3da follow MISC
	 * bit0 between the color (3D4h-style) and mono (3B4h-style)
	 * blocks; see cl_select_crtc().
	 */
	uint16_t io_3c0;	/* base of the 3C0h-3CFh block */
	uint16_t io_3d4;	/* currently selected CRTC index */
	uint16_t io_3da;	/* currently selected Input Status 1 */
	uint16_t io_3d4_col, io_3da_col;
	uint16_t io_3d4_mono, io_3da_mono;

	/* VRAM aperture. */
	uint8_t *fb;
	uint32_t fb_phys;
	bool linear;		/* true: linear; false: 32KB banked window */
	uint32_t vram_size;
	int cur_bank;

	/* Chip information (for logging / decisions). */
	uint8_t wab_id;		/* raw 0FAAh register 00h readout */
	uint8_t crt27;		/* Cirrus chip ID register */
	bool alpine;		/* GD5430/5440-or-later register semantics */

	/* LCD panel (GD754x laptops). */
	bool lcd;
	int lcd_w;
	int lcd_h;
} cdisp;

/* Blit placement (centering + clip against the screen). */
static int ofs_x, ofs_y;
static int draw_w, draw_h;

extern struct hal_image *back_image;
extern int game_width;
extern int game_height;

/* Frame presentation. */
static void cirrus_flip_vram(void);
static void conv_row24(uint8_t *dst, const uint32_t *src, int n);
static void conv_row16(uint8_t *dst, const uint32_t *src, int n);
static void conv_row8(uint8_t *dst, const uint32_t *src, int n);

/* Low-level register access. */
static void cl_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
			  uint16_t d4m, uint16_t dam);
static void cl_select_crtc(int misc);
static void cl_seq_write(int reg, int val);
static int cl_seq_read(int reg);
static void cl_gfx_write(int reg, int val);
static int cl_gfx_read(int reg);
static void cl_crtc_write(int reg, int val);
static int cl_crtc_read(int reg);
static void cl_attr_write(int reg, int val);
static int cl_attr_read(int reg);
static void cl_misc_write(int val);
static int cl_misc_read(void);
static void cl_hidden_dac_write(int val);
static int cl_hidden_dac_read(void);
static void cl_set_bank(int bank);

/* Shared mode-set building blocks. */
static int cl_hdr_value(void);
static void cl_program_vclk3(void);
static void cl_program_crtc(void);
static void cl_program_gc_ac(void);
static void cl_load_palette(void);
static void cl_modeset_generic(bool banked);
static int cl_resolve_bpp(int req, int cap, int w, int h,
			  uint32_t vram, const char *tag);

/* Misc. */
static void *cl_map_physical(uint32_t phys, uint32_t size);

/* WAB (C-bus GD54xx) module. */
static bool cirrus54_init(int mode, int req_bpp);
static void cirrus54_cleanup(void);

/* PCI (GD75xx / PCI GD54xx) module. */
static bool cirrus75_init(int mode, int req_bpp);
static void cirrus75_cleanup(void);

/*****************************************************************************/
/* Public interface                                                          */
/*****************************************************************************/

bool
cirrus_init_disp(int mode, int bpp)
{
	const char *force;
	bool ok;

	if (mode < DISP_640X480 || mode > DISP_1280X1024) {
		hal_log_info("CIRRUS: invalid mode selector %d.", mode);
		return false;
	}
	if (bpp != -1 && bpp != 8 && bpp != 16 && bpp != 24) {
		hal_log_info("CIRRUS: invalid depth %d (8/16/24 or -1).", bpp);
		return false;
	}

	memset(&cdisp, 0, sizeof(cdisp));

	hal_log_info("CIRRUS: probing; requested %dx%d, depth %d (-1 = auto).",
		     disp_geo[mode].w, disp_geo[mode].h, bpp);

	force = getenv("STRATO_CIRRUS_FORCE");
	if (force != NULL)
		hal_log_info("CIRRUS: STRATO_CIRRUS_FORCE=%s.", force);

	/*
	 * WAB first: the two-stage 0FAAh/0FABh interface identifies
	 * the classic desktop (and pre-PCI 98NOTE) built-ins by a
	 * single harmless read.  Machines without it (the PCI 98NOTE
	 * Lavie line, PCI desktops) read FFh there and fall through
	 * to the PCI configuration space scan.
	 */
	ok = false;
	if (force == NULL || strcmp(force, "75") != 0)
		ok = cirrus54_init(mode, bpp);
	if (!ok && (force == NULL || strcmp(force, "54") != 0))
		ok = cirrus75_init(mode, bpp);
	if (!ok) {
		hal_log_info("CIRRUS: no usable Cirrus built-in; "
			     "yielding to other drivers.");
		return false;
	}

	/* Center the game image; clip if the screen is smaller. */
	ofs_x = (cdisp.scr_w - game_width) / 2;
	ofs_y = (cdisp.scr_h - game_height) / 2;
	if (ofs_x < 0)
		ofs_x = 0;
	if (ofs_y < 0)
		ofs_y = 0;
	draw_w = game_width < cdisp.scr_w ? game_width : cdisp.scr_w;
	draw_h = game_height < cdisp.scr_h ? game_height : cdisp.scr_h;
	draw_w &= ~3;	/* the row converters work 4 pixels at a time */

	hal_log_info("CIRRUS: === configuration summary ===");
	hal_log_info("CIRRUS: path     : %s.",
		     cdisp.path == CIRRUS_PATH_54 ?
		     "WAB (C-bus built-in, 0FAAh/0FABh)" :
		     "PCI (configuration space)");
	hal_log_info("CIRRUS: chip     : %s, CR27=%02Xh, WAB ID=%02Xh.",
		     cdisp.chip_name, cdisp.crt27, cdisp.wab_id);
	hal_log_info("CIRRUS: mode     : %dx%d, %d bpp, pitch %lu bytes.",
		     cdisp.scr_w, cdisp.scr_h, cdisp.bpp,
		     (unsigned long)cdisp.pitch);
	if (cdisp.linear)
		hal_log_info("CIRRUS: aperture : linear, %luKB at %08lXh.",
			     (unsigned long)(cdisp.vram_size >> 10),
			     (unsigned long)cdisp.fb_phys);
	else
		hal_log_info("CIRRUS: aperture : banked 32KB window at "
			     "%08lXh, 16KB granularity, %luKB VRAM.",
			     (unsigned long)cdisp.fb_phys,
			     (unsigned long)(cdisp.vram_size >> 10));
	hal_log_info("CIRRUS: blitter  : unused (aperture-only driver).");
	hal_log_info("CIRRUS: blit     : game %dx%d -> +%d,+%d "
		     "(draw %dx%d).",
		     game_width, game_height, ofs_x, ofs_y, draw_w, draw_h);

	return true;
}

void
cirrus_cleanup_disp(void)
{
	switch (cdisp.path) {
	case CIRRUS_PATH_54:
		cirrus54_cleanup();
		break;
	case CIRRUS_PATH_75:
		cirrus75_cleanup();
		break;
	default:
		break;
	}
	cdisp.path = CIRRUS_PATH_NONE;
	hal_log_info("CIRRUS: cleanup done, output back on the 98 GDC.");
}

void
cirrus_flip(void)
{
	if (cdisp.path == CIRRUS_PATH_NONE)
		return;
	cirrus_flip_vram();
}

/*****************************************************************************/
/* Frame presentation - direct writes through the VRAM aperture              */
/*****************************************************************************/

/*
 * Blit the back image to VRAM.
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R.
 *
 * Banked-window note: one bank switch per row is enough.  The bank
 * granularity is 16KB but the window is 32KB, so a row starting
 * anywhere within the first 16KB extends at most pitch (<= 1920
 * bytes at 640x480x24, 1600 at 800x600x16) into the second half and
 * never leaves the window.
 */
static void
cirrus_flip_vram(void)
{
	const uint32_t *pixels;
	int y, bytespp;

	pixels = back_image->pixels;
	bytespp = cdisp.bpp / 8;

	for (y = 0; y < draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;
		uint32_t off = (uint32_t)(y + ofs_y) * cdisp.pitch +
			       (uint32_t)ofs_x * (uint32_t)bytespp;
		uint8_t *dst;

		if (cdisp.linear) {
			dst = cdisp.fb + off;
		} else {
			cl_set_bank((int)(off >> CL_BANK_SHIFT));
			dst = cdisp.fb + (off & CL_BANK_MASK);
		}

		switch (cdisp.bpp) {
		case 24:
			conv_row24(dst, src, draw_w);
			break;
		case 16:
			conv_row16(dst, src, draw_w);
			break;
		default:
			conv_row8(dst, src, draw_w);
			break;
		}
	}
}

/*
 * Pixel format converters.  n is a multiple of four (enforced by
 * draw_w in cirrus_init_disp()).
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

/* BGRA8888 -> RGB332 (matches the palette set by cl_load_palette()). */
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
 * The register block layout is the native VGA one; only the base
 * moves around: 3C0h/3D4h/3DAh on the PCI machines, 0CA0h/0DA4h/
 * 0DAAh on the classic WAB machines (and 0C50h/0D54h/0D5Ah on
 * B-MATEs, should that ever be needed).
 *
 * The CRTC and Input Status 1 additionally follow MISC bit0 between
 * the color block (3D4h/3DAh style) and the mono block (3B4h/3BAh
 * style; relocated 0BA4h/0BAAh).  The NEC 754x mode streams program
 * MISC bit0 = 0, so every CRTC/ST1 access after that MUST go to the
 * mono block - on real silicon the color block simply stops
 * decoding.  cl_misc_write() keeps the selected base coherent; an
 * earlier revision kept writing the color block and would have lost
 * the whole CRTC stream on hardware (emulators decode both blocks,
 * hiding this).
 */

static void
cl_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
	      uint16_t d4m, uint16_t dam)
{
	cdisp.io_3c0 = b3c0;
	cdisp.io_3d4_col = d4c;
	cdisp.io_3da_col = dac;
	cdisp.io_3d4_mono = d4m;
	cdisp.io_3da_mono = dam;
	/* Default to the color block until MISC says otherwise. */
	cdisp.io_3d4 = d4c;
	cdisp.io_3da = dac;
}

/* Point the CRTC/ST1 accessors at the block MISC bit0 selects. */
static void
cl_select_crtc(int misc)
{
	if (misc & 0x01) {
		cdisp.io_3d4 = cdisp.io_3d4_col;
		cdisp.io_3da = cdisp.io_3da_col;
	} else {
		cdisp.io_3d4 = cdisp.io_3d4_mono;
		cdisp.io_3da = cdisp.io_3da_mono;
	}
}

static void
cl_seq_write(int reg, int val)
{
	outp(cdisp.io_3c0 + 0x04, reg);
	outp(cdisp.io_3c0 + 0x05, val);
}

static int
cl_seq_read(int reg)
{
	outp(cdisp.io_3c0 + 0x04, reg);
	return inp(cdisp.io_3c0 + 0x05);
}

static void
cl_gfx_write(int reg, int val)
{
	outp(cdisp.io_3c0 + 0x0e, reg);
	outp(cdisp.io_3c0 + 0x0f, val);
}

static int
cl_gfx_read(int reg)
{
	outp(cdisp.io_3c0 + 0x0e, reg);
	return inp(cdisp.io_3c0 + 0x0f);
}

static void
cl_crtc_write(int reg, int val)
{
	outp(cdisp.io_3d4, reg);
	outp(cdisp.io_3d4 + 1, val);
}

static int
cl_crtc_read(int reg)
{
	outp(cdisp.io_3d4, reg);
	return inp(cdisp.io_3d4 + 1);
}

static void
cl_attr_write(int reg, int val)
{
	(void)inp(cdisp.io_3da);	/* reset the index/data flip-flop */
	outp(cdisp.io_3c0 + 0x00, reg);
	outp(cdisp.io_3c0 + 0x00, val);
}

static int
cl_attr_read(int reg)
{
	int val;

	(void)inp(cdisp.io_3da);
	outp(cdisp.io_3c0 + 0x00, reg);
	val = inp(cdisp.io_3c0 + 0x01);
	(void)inp(cdisp.io_3da);	/* leave the flip-flop reset */
	return val;
}

static void
cl_misc_write(int val)
{
	outp(cdisp.io_3c0 + 0x02, val);		/* 3C2h: write */
	cl_select_crtc(val);			/* keep CRTC base coherent */
}

static int
cl_misc_read(void)
{
	return inp(cdisp.io_3c0 + 0x0c);	/* 3CCh: read */
}

/*
 * The Cirrus Hidden DAC Register is accessed by reading the Pixel
 * Mask register (3C6h) four times; the fifth access hits the HDR.
 */
static void
cl_hidden_dac_write(int val)
{
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	outp(cdisp.io_3c0 + 0x06, val);
}

static int
cl_hidden_dac_read(void)
{
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	return inp(cdisp.io_3c0 + 0x06);
}

/* Select a 16KB VRAM bank via GR09 (Offset Register 0). */
static void
cl_set_bank(int bank)
{
	if (bank != cdisp.cur_bank) {
		cl_gfx_write(0x09, bank);
		cdisp.cur_bank = bank;
	}
}

/*****************************************************************************/
/* Shared mode-set building blocks (WAB and desktop-PCI paths)               */
/*****************************************************************************/

/* Hidden DAC value for the current pixel depth (54xx/Alpine family). */
static int
cl_hdr_value(void)
{
	switch (cdisp.bpp) {
	case 24:
		return 0xc5;	/* 8-8-8 truecolor */
	case 16:
		return 0xc1;	/* 5-6-5 */
	default:
		return 0x00;	/* palette mode */
	}
}

/*
 * VCLK3 (selected by MISC clock select = 11b).
 * VClk = 14.31818MHz * N / (D * (1 + P)), SR0E = N, SR1E = (D<<1)|P.
 *
 * Only 24bpp needs VCLK = dot clock x3, and only on the Alpine
 * family; 16bpp and 8bpp use the plain dot clock on every chip (per
 * cirrusfb).
 *
 * On the Alpine family (GD5430/34/40) SR1E bit7 MUST also be set
 * (cirrusfb: "6 bit denom; ONLY 5434!!! (bugged me 10 days)");
 * without it the denominator is misinterpreted and the pixel clock,
 * and therefore the H/V sync frequencies, come out wrong on real
 * silicon.  Emulators ignore the clock registers completely, which
 * is why this was invisible on NP21/W.
 */
static void
cl_program_vclk3(void)
{
	int alp = cdisp.alpine ? 0x80 : 0x00;

	if (cdisp.scr_w == 800) {
		/* 40.000MHz -> N=81, D=29, P=0 (39.99MHz). */
		cl_seq_write(0x0e, 0x51);
		cl_seq_write(0x1e, alp | 0x3a);
	} else if (cdisp.alpine && cdisp.bpp == 24) {
		/* 3 x 25.175 = 75.525MHz -> N=95, D=18, P=0 (75.57MHz) */
		cl_seq_write(0x0e, 0x5f);
		cl_seq_write(0x1e, 0x80 | 0x24);
	} else {
		/* 25.175MHz -> N=74, D=21, P=1 */
		cl_seq_write(0x0e, 0x4a);
		cl_seq_write(0x1e, alp | 0x2b);
	}

	/*
	 * SR1F bit6 = "derive VCLK from MCLK".  If a previous driver
	 * (e.g. the Windows one) left it set, the SR0E/SR1E values
	 * above would simply be ignored.  Clear it, but preserve the
	 * MCLK frequency bits NEC programmed for this board's DRAM.
	 */
	cl_seq_write(0x1f, cl_seq_read(0x1f) & ~0x40);
}

/*
 * CRTC values for 640x480@60Hz (25.175MHz dot clock).
 *
 * These are NOT the plain IBM VGA table values: they are the exact
 * values the Linux cirrusfb driver computes and writes on real
 * Alpine (GD5430/5434/5440) hardware.  The important difference is
 * horizontal blanking: on the Cirrus chips the Horizontal Blanking
 * End compare is extended to 8 bits with CR1A[5:4] as bits <7:6>, so
 * blanking end is programmed as the full horizontal total (100
 * characters = 0110 0100b):
 *   CR03[4:0] = 00100b, CR05[7] = 1, CR1A[5:4] = 01b.
 * The stock VGA table (CR03=82h/CR1A=00h) leaves the 8-bit compare
 * value at 34, so on the real chip the blanking pulse never
 * terminates where it should - one cause of a torn or blank picture
 * that an emulator (which ignores blanking timing entirely) will
 * never show.
 */
static const uint8_t crtc_640x480[] = {
	0x5f,	/* 00: Horizontal Total (800/8 - 5) */
	0x4f,	/* 01: Horizontal Display End (640/8 - 1) */
	0x50,	/* 02: Horizontal Blanking Start (640/8) */
	0x84,	/* 03: Horizontal Blanking End (=100, low 5 bits) */
	0x53,	/* 04: Horizontal Sync Start (656/8 + 1) */
	0x9f,	/* 05: Hsync End (752/8+1)%32, bit7=HBE bit5 */
	0x0b,	/* 06: Vertical Total (525 - 2, low byte) */
	0x3e,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x40,	/* 09: Max Scan Line (bit6 = line compare bit9) */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0xe9,	/* 10: Vertical Sync Start (489, low byte) */
	0x6b,	/* 11: Vsync End (491%16), no V-int, unprotected */
	0xdf,	/* 12: Vertical Display End (479, low byte) */
	0x00,	/* 13: Offset (patched from cdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0xe0,	/* 15: Vertical Blanking Start (480, low byte) */
	0x0b,	/* 16: Vertical Blanking End (523, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * CRTC values for 800x600@60Hz (40.000MHz dot clock, VESA timing,
 * positive H/V sync).  Same construction rules as the 640x480 table;
 * horizontal total is 132 characters (1000 0100b), so CR03[4:0] =
 * 00100b, CR05[7] = 0 and CR1A[5:4] = 10b.
 */
static const uint8_t crtc_800x600[] = {
	0x7f,	/* 00: Horizontal Total (1056/8 - 5) */
	0x63,	/* 01: Horizontal Display End (800/8 - 1) */
	0x64,	/* 02: Horizontal Blanking Start (800/8) */
	0x84,	/* 03: Horizontal Blanking End (=132, low 5 bits) */
	0x6a,	/* 04: Horizontal Sync Start (840/8 + 1) */
	0x1a,	/* 05: Hsync End (968/8+1)%32, bit7=HBE bit5=0 */
	0x72,	/* 06: Vertical Total (628 - 2, low byte) */
	0xf0,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x60,	/* 09: Max Scan Line (bit5=VBS bit9, bit6=LC bit9) */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0x59,	/* 10: Vertical Sync Start (601, low byte) */
	0x6d,	/* 11: Vsync End (605%16), no V-int, unprotected */
	0x57,	/* 12: Vertical Display End (599, low byte) */
	0x00,	/* 13: Offset (patched from cdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0x58,	/* 15: Vertical Blanking Start (600, low byte) */
	0x72,	/* 16: Vertical Blanking End (626, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * Program the CRTC for cdisp.scr_w x cdisp.scr_h with the pitch
 * derived from cdisp.pitch.  CR0-7 are unprotected first; the CR11
 * table values keep bit7 clear afterwards (cirrusfb leaves the CRTC
 * unprotected too).
 */
static void
cl_program_crtc(void)
{
	const uint8_t *tab;
	uint8_t cr1a;
	uint32_t offset;
	int i;

	if (cdisp.scr_w == 800) {
		tab = crtc_800x600;
		cr1a = 0x20;	/* HBE<7:6> = 10b (Htotal 132 chars) */
	} else {
		tab = crtc_640x480;
		cr1a = 0x10;	/* HBE<7:6> = 01b (Htotal 100 chars) */
	}

	offset = cdisp.pitch / 8;

	cl_crtc_write(0x11, cl_crtc_read(0x11) & 0x7f);
	for (i = 0; i < 0x19; i++) {
		if (i == 0x13)
			cl_crtc_write(i, (int)(offset & 0xff));
		else
			cl_crtc_write(i, tab[i]);
	}

	/* CR1A: no interlace; bits5:4 = Horiz. Blanking End <7:6>. */
	cl_crtc_write(0x1a, cr1a);
	/* CR1B: ext display: 16bit wrap; bit4 = offset bit8. */
	cl_crtc_write(0x1b, 0x22 | (int)((offset >> 4) & 0x10));
	/* CR1D: ext overflow: start address bit19 = 0. */
	cl_crtc_write(0x1d, 0x00);
}

/*
 * Program the Graphics Controller for packed-pixel graphics and the
 * Attribute Controller with an identity palette.  Video output is
 * re-enabled at the end (AC index bit5).
 *
 * GR09/GR0A/GR0B (banking) are left to the callers: the WAB path
 * programs the 16KB bank granularity, the linear path zeroes the
 * offsets.
 */
static void
cl_program_gc_ac(void)
{
	int i;

	cl_gfx_write(0x00, 0x00);
	cl_gfx_write(0x01, 0x00);
	cl_gfx_write(0x02, 0x00);
	cl_gfx_write(0x03, 0x00);
	cl_gfx_write(0x04, 0x00);
	cl_gfx_write(0x05, 0x40);	/* mode: 256-color shift (packed) */
	cl_gfx_write(0x06, 0x05);	/* misc: graphics, A0000 64KB map */
	cl_gfx_write(0x07, 0x0f);
	cl_gfx_write(0x08, 0xff);

	for (i = 0; i < 16; i++)
		cl_attr_write(i, i);
	cl_attr_write(0x10, 0x01);	/* mode: graphics */
	cl_attr_write(0x11, 0x00);	/* overscan */
	cl_attr_write(0x12, 0x0f);	/* plane enable */
	cl_attr_write(0x13, 0x00);	/* pixel panning */
	cl_attr_write(0x14, 0x00);	/* color select */
	(void)inp(cdisp.io_3da);
	outp(cdisp.io_3c0 + 0x00, 0x20); /* re-enable video output */
}

/*
 * Load the DAC.  In the direct-color modes (24/16bpp) the DAC is
 * bypassed, so a grayscale ramp is loaded just in case.  In 8bpp the
 * palette implements RGB332, matching conv_row8().
 */
static void
cl_load_palette(void)
{
	int i;

	outp(cdisp.io_3c0 + 0x06, 0xff);	/* no pixel mask */
	outp(cdisp.io_3c0 + 0x08, 0x00);	/* write index 0 */

	if (cdisp.bpp == 8) {
		for (i = 0; i < 256; i++) {
			int r = (i >> 5) & 7;
			int g = (i >> 2) & 7;
			int b = i & 3;

			/* 6-bit DAC entries. */
			outp(cdisp.io_3c0 + 0x09, r * 63 / 7);
			outp(cdisp.io_3c0 + 0x09, g * 63 / 7);
			outp(cdisp.io_3c0 + 0x09, b * 63 / 3);
		}
	} else {
		for (i = 0; i < 256; i++) {
			outp(cdisp.io_3c0 + 0x09, i >> 2);
			outp(cdisp.io_3c0 + 0x09, i >> 2);
			outp(cdisp.io_3c0 + 0x09, i >> 2);
		}
	}
}

/*
 * The full generic mode set for the GD5428/Alpine register model,
 * used by the WAB path (banked = true) and by the desktop PCI chips
 * (banked = false; the 754x laptops use NEC's verbatim streams
 * instead).  There is no VGA BIOS on PC-98, so the full VGA register
 * set is programmed by hand; values follow the Linux cirrusfb driver
 * and the standard 640x480@60 / 800x600@60 timings.
 *
 * Expects cdisp.{scr_w,scr_h,bpp,pitch} to be set and the register
 * base selected.  Leaves the screen blanked (SR1 bit5); the caller
 * unblanks after clearing VRAM.
 */
static void
cl_modeset_generic(bool banked)
{
	int sr07;

	/* Blank the screen during the mode set (SR1 bit5). */
	cl_seq_write(0x00, 0x03);	/* sequencer: run */
	cl_seq_write(0x01, 0x21);	/* 8-dot clock, screen off */

	/* Unlock all Cirrus extension registers. */
	cl_seq_write(0x06, 0x12);

	/*
	 * Identify the chip generation from CR27:
	 * 0xA0 and above = GD5430/5440 (Alpine family).
	 */
	cdisp.crt27 = (uint8_t)cl_crtc_read(0x27);
	cdisp.alpine = (cdisp.crt27 >= 0xa0);
	hal_log_info("CIRRUS: CR27=%02Xh (%s semantics), %d bpp mode set.",
		     cdisp.crt27,
		     cdisp.alpine ? "Alpine GD5430/5440" : "GD5428",
		     cdisp.bpp);

	/*
	 * On PC98, no VGA BIOS has ever run, so extended registers
	 * may hold whatever the NEC firmware / a previous OS driver
	 * left in them.  Explicitly clear the state that can corrupt
	 * the display (per cirrusfb's init_vgachip):
	 */
	if (cdisp.alpine)
		cl_gfx_write(0x33, 0x00);	/* BLT: back to 542x-compatible */
	cl_gfx_write(0x31, 0x04);	/* BitBLT reset... */
	cl_gfx_write(0x31, 0x00);	/* ...end of reset */
	cl_seq_write(0x10, 0x00);	/* HW cursor X */
	cl_seq_write(0x11, 0x00);	/* HW cursor Y */
	cl_seq_write(0x12, 0x00);	/* HW cursor attributes: OFF */
	cl_seq_write(0x13, 0x00);	/* HW cursor pattern address */

	if (!cdisp.alpine) {
		/* GD5428: performance/DRAM control (per cirrusfb) */
		cl_seq_write(0x16, 0x0f);
		cl_seq_write(0x0f, 0xb0);
	}

	/* Sequencer basics */
	cl_seq_write(0x02, 0xff);	/* plane write mask */
	cl_seq_write(0x03, 0x00);	/* character map */
	cl_seq_write(0x04, 0x0a);	/* memory mode: ext memory, chain4 */
	if (banked)
		cl_seq_write(0x17, 0x00);	/* ext control: MMIO off */
	/* (linear PCI: leave the firmware's SR17 alone) */

	/*
	 * Extended Sequencer Mode (SR07), packed pixel:
	 *              24bpp  16bpp  8bpp
	 *   GD5428:    0x25   0x27   0x21
	 *   Alpine:    0xA5   0xA7   0xA1
	 * (Values per cirrusfb; low nibble selects the depth, high
	 * nibble the family's memory wiring.)
	 */
	switch (cdisp.bpp) {
	case 24:
		sr07 = cdisp.alpine ? 0xa5 : 0x25;
		break;
	case 16:
		sr07 = cdisp.alpine ? 0xa7 : 0x27;
		break;
	default:
		sr07 = cdisp.alpine ? 0xa1 : 0x21;
		break;
	}
	cl_seq_write(0x07, sr07);

	/* VCLK3 + SR1F hygiene. */
	cl_program_vclk3();

	/*
	 * Miscellaneous Output: display memory enabled, color I/O,
	 * clock select = VCLK3; negative H/V sync for the 480-line
	 * mode, positive for VESA 800x600.
	 */
	cl_misc_write(cdisp.scr_h == 480 ? 0xcf : 0x0f);

	/* CRTC: timing + pitch for the current mode/depth. */
	cl_program_crtc();

	/* Graphics Controller + banking */
	cl_program_gc_ac();
	cl_gfx_write(0x09, 0x00);	/* Offset Register 0 (bank) */
	cl_gfx_write(0x0a, 0x00);	/* Offset Register 1 */
	if (banked)
		cl_gfx_write(0x0b, cdisp.alpine ? 0x20 : 0x28); /* 16KB gran. */
	else
		cl_gfx_write(0x0b, 0x20);
	cdisp.cur_bank = 0;

	/* DAC: identity ramp (24/16bpp) or the RGB332 palette (8bpp). */
	cl_load_palette();

	/* Hidden DAC: pixel format for the DAC pipeline. */
	cl_hidden_dac_write(cl_hdr_value());
}

/*
 * Resolve the depth for a request.  req == -1: pick the highest
 * depth that both the machine cap (panel/DAC) and VRAM allow.
 * Explicit requests are honored if they fit in VRAM; a request above
 * the machine cap is allowed with a warning (useful for probing what
 * a panel really does).  Returns -1 on failure.
 */
static int
cl_resolve_bpp(int req, int cap, int w, int h, uint32_t vram,
	       const char *tag)
{
	int b;

	if (req == -1) {
		b = cap;
		while (b > 8 &&
		       (uint32_t)w * (uint32_t)h * (uint32_t)(b / 8) > vram)
			b = (b == 24) ? 16 : 8;
		hal_log_info("CIRRUS: %s: auto depth -> %d bpp "
			     "(machine cap %d, VRAM %luKB).",
			     tag, b, cap, (unsigned long)(vram >> 10));
		return b;
	}

	if ((uint32_t)w * (uint32_t)h * (uint32_t)(req / 8) > vram) {
		hal_log_info("CIRRUS: %s: %dx%d at %d bpp does not fit "
			     "%luKB VRAM.",
			     tag, w, h, req, (unsigned long)(vram >> 10));
		return -1;
	}
	if (req > cap)
		hal_log_info("CIRRUS: %s: requested %d bpp exceeds the "
			     "machine default cap of %d; forcing anyway.",
			     tag, req, cap);
	return req;
}

/*
 * DPMI 0x0800: Map a physical address into linear address space.
 * (DOS/4GW uses a zero-based flat address space, so the returned
 * linear address is directly usable as a pointer.)
 */
static void *
cl_map_physical(uint32_t phys, uint32_t size)
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

/*****************************************************************************/
/* WAB module: the C-bus GD5428/5430/5440 built-ins                          */
/*****************************************************************************/

/* Two-stage indexed I/O of the window accelerator interface. */
#define WAB_INDEX	0x0faa
#define WAB_DATA	0x0fab

/* WAB registers */
#define WAB_REG_ID	0x00	/* machine ID (read only) */
#define WAB_REG_WINDOW	0x01	/* VRAM window address */
#define WAB_REG_RELAY	0x03	/* video output relay */

/* WAB_REG_WINDOW value: window at 0xF20000 (32KB) */
#define WAB_WINDOW_F2	0x80
#define WAB_WINDOW_ADDR	0x00F20000UL
#define WAB_WINDOW_SIZE	0x8000

/*
 * WAB_REG_RELAY: bit1 = output relay (1: accelerator / 0: 98 GDC),
 * bit0 = register/MMIO access enable (per NP21/W cirrusvga_ofab()).
 */
#define WAB_RELAY_GDC	0x00	/* GDC output, access off (exit state) */
#define WAB_RELAY_SETUP	0x01	/* GDC output, register access on */
#define WAB_RELAY_WAB	0x03	/* accelerator output, access on */

/* Second relay latch on the PCI-era WAB models (Xa7e etc.). */
#define WAB_RELAY2_PORT	0x0fac

/* Shared-VRAM ownership switch. */
#define VRAM_SW_PORT	0x6a
#define VRAM_SW_GDC	0x8e
#define VRAM_SW_WAB	0x8f

/* Wakeup ports. */
#define WAB_P904	0x0904	/* 102 Access Control (bit5) */
#define WAB_PFF82	0xff82	/* POS102 (bit0 = video subsystem enable) */
/*
 * (The Sleep Address - native 3C3h - and the relocated VGA register
 * bases live at base+offset of whichever block answers; see the
 * table inside wab_probe_cirrus_regs().)
 */

static void
wab_write(int reg, int val)
{
	outp(WAB_INDEX, reg);
	outp(WAB_DATA, val);
}

static int
wab_read(int reg)
{
	outp(WAB_INDEX, reg);
	return inp(WAB_DATA);
}

/*
 * Detect the built-in window accelerator interface.
 *
 * WAB register 00h returns the machine ID; the CIRRUS models are
 * said to use 50h-5Dh and 70h.  (Other values are S3/Matrox/Trident
 * models or 00h/FFh when no two-stage I/O accelerator is present.
 * Note the PCI-connected models - Nb10 and friends - read FFh here;
 * those are handled by the PCI module below.  NP21/W's AUTO WAB
 * type morphs into an Xe10 the moment this port is touched - pin
 * the emulator to a fixed type when testing.)
 *
 * The ID is treated as a HINT only: the ValueStar V16 reads 5Bh
 * here yet carries a Trident TGUI9680XGi, so a positive ID must be
 * followed by wab_probe_cirrus_regs() before any programming.
 */
static bool
wab_detect(void)
{
	cdisp.wab_id = (uint8_t)wab_read(WAB_REG_ID);

	if (!((cdisp.wab_id >= 0x50 && cdisp.wab_id <= 0x5d) ||
	      cdisp.wab_id == 0x70)) {
		hal_log_info("CIRRUS: WAB ID reads %02Xh, "
			     "not a GD54xx built-in.", cdisp.wab_id);
		return false;
	}

	return true;
}

/* Dump the WAB interface registers (called once a Cirrus ID is seen). */
static void
wab_dump(void)
{
	hal_log_info("CIRRUS: WAB regs: 00=%02Xh 01=%02Xh 02=%02Xh "
		     "03=%02Xh 04=%02Xh; relay2 0FACh=%02Xh.",
		     wab_read(0x00), wab_read(0x01), wab_read(0x02),
		     wab_read(0x03), wab_read(0x04),
		     inp(WAB_RELAY2_PORT));
}

/*
 * Default (= highest sensible) pixel depth for a WAB machine ID
 * (nec_clgd.txt): desktops take the full 24bpp; the 98NOTE panels
 * do not.
 */
static int
wab_default_bpp(uint8_t id)
{
	switch (id) {
	case 0x53:	/* Ns:  TFT, 4096 colors */
	case 0x54:	/* Ts:  TFT, 4096 colors */
	case 0x56:	/* Ne2: TFT, 4096 colors */
	case 0x55:	/* Np/Es: TFT, full color */
	case 0x70:	/* Nf:  TFT, full color */
		return 16;
	case 0x57:	/* Nd:  DSTN, 512 colors */
		return 8;
	default:	/* desktops */
		return 24;
	}
}

/*
 * Fingerprint the chip behind the WAB interface.  The machine ID
 * alone is unreliable (the V16 reads 5Bh with a Trident TGUI9680XGi
 * behind it; its relay/window registers work, the VGA file does
 * not, and switching the relay to an unprogrammed foreign chip
 * yields a white, torn screen).
 *
 * The signature used is the Cirrus SR06 extension lock, which is
 * not a storage register: writing 0Fh (lock) reads back FFh, and
 * writing the 12h key reads back 12h.  A scratch register echoes
 * the 0Fh, a floating bus reads FFh for both - either way the pair
 * of checks fails.  CR27 (aligned to the block MISC bit0 selects)
 * must additionally look like a GD542x/543x/544x ID (80h-BFh).
 *
 * The register file is probed at the standard relocation, the
 * B-MATE relocation and the native block, in that order; the sleep
 * address (native 3C3h) lives at base+3 in each block and is
 * enabled per attempt.  On success the I/O base is left selected
 * for the rest of the init.
 */
static bool
wab_probe_cirrus_regs(void)
{
	static const struct {
		uint16_t b3c0, d4c, dac, d4m, dam;
		const char *name;
	} bases[] = {
		{ 0x0ca0, 0x0da4, 0x0daa, 0x0ba4, 0x0baa, "relocated" },
		{ 0x0c50, 0x0d54, 0x0d5a, 0x0b54, 0x0b5a, "B-MATE" },
		{ 0x03c0, 0x03d4, 0x03da, 0x03b4, 0x03ba, "native" }
	};
	int i, lock, key, misc, cr27;

	for (i = 0; i < (int)(sizeof(bases) / sizeof(bases[0])); i++) {
		cl_set_iobase(bases[i].b3c0, bases[i].d4c, bases[i].dac,
			      bases[i].d4m, bases[i].dam);

		/* Sleep Address (3C3h equivalent): enable the chip. */
		outp(cdisp.io_3c0 + 0x03, 0x01);

		cl_seq_write(0x06, 0x0f);	/* lock: must read FFh */
		lock = cl_seq_read(0x06);
		cl_seq_write(0x06, 0x12);	/* unlock: must echo 12h */
		key = cl_seq_read(0x06);
		if (lock != 0xff || key != 0x12) {
			hal_log_info("CIRRUS: no Cirrus SR06 at the %s "
				     "base (lock=%02Xh key=%02Xh).",
				     bases[i].name, lock, key);
			continue;
		}

		misc = cl_misc_read();
		cl_select_crtc(misc);
		cr27 = cl_crtc_read(0x27);
		if (cr27 < 0x80 || cr27 > 0xbf) {
			hal_log_info("CIRRUS: SR06 answers at the %s base "
				     "but CR27=%02Xh is not a GD54xx.",
				     bases[i].name, cr27);
			continue;
		}

		hal_log_info("CIRRUS: chip fingerprint OK at the %s base "
			     "(3C0h=%03Xh): SR06 lock/key=FFh/12h, "
			     "MISC=%02Xh, CR27=%02Xh.",
			     bases[i].name, cdisp.io_3c0, misc, cr27);
		return true;
	}

	hal_log_info("CIRRUS: WAB ID %02Xh, but no Cirrus register file "
		     "answers - likely an S3/Trident/Matrox accelerator; "
		     "leaving it alone.", cdisp.wab_id);
	return false;
}

/* Undo the wakeup/relay side effects when aborting mid-init. */
static void
wab_abort(void)
{
	wab_write(WAB_REG_RELAY, WAB_RELAY_GDC);
	outp(WAB_PFF82, 0x00);
}

static bool
cirrus54_init(int mode, int req_bpp)
{
	int i, w, h, bpp;

	if (!wab_detect())
		return false;

	hal_log_info("CIRRUS: CL-GD54xx built-in found (WAB ID %02Xh).",
		     cdisp.wab_id);
	wab_dump();

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	if (mode != DISP_640X480 && mode != DISP_800X600) {
		hal_log_info("CIRRUS: WAB: %dx%d not supported "
			     "(640x480 / 800x600 only).", w, h);
		return false;
	}
	if (mode == DISP_800X600)
		hal_log_info("CIRRUS: WAB: 800x600 is untested on real "
			     "hardware (40MHz VCLK).");

	cdisp.vram_size = 1024UL * 1024UL;
	bpp = cl_resolve_bpp(req_bpp, wab_default_bpp(cdisp.wab_id),
			     w, h, cdisp.vram_size, "WAB");
	if (bpp < 0)
		return false;

	cdisp.scr_w = w;
	cdisp.scr_h = h;
	cdisp.bpp = bpp;
	cdisp.pitch = (uint32_t)w * (uint32_t)(bpp / 8);
	cdisp.linear = false;

	/*
	 * Wake up the video subsystem.
	 * 0904h bit5 enables access to POS102 (FF82h); POS102 bit0
	 * and the Sleep Address (3C3h equivalent, base+3) bit0 -
	 * written per candidate base inside the probe below - enable
	 * the chip.
	 */
	outp(WAB_P904, 0x20);
	outp(WAB_PFF82, 0x01);
	hal_log_info("CIRRUS: WAB: video subsystem awake "
		     "(0904h=20h FF82h=01h).");

	/* Enable WAB register access, keep the 98 GDC on screen for now. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_SETUP);

	/*
	 * The machine ID said Cirrus; now make the chip prove it
	 * before we program a single register (see the V16/Trident
	 * story above).  This also selects the register base.
	 */
	if (!wab_probe_cirrus_regs()) {
		wab_abort();
		return false;
	}

	/* Place the VRAM window at 0xF20000. */
	wab_write(WAB_REG_WINDOW, WAB_WINDOW_F2);
	hal_log_info("CIRRUS: WAB: 32KB VRAM window at %08lXh.",
		     (unsigned long)WAB_WINDOW_ADDR);

	/* Map the window into our address space. */
	cdisp.fb_phys = WAB_WINDOW_ADDR;
	cdisp.fb = (uint8_t *)cl_map_physical(WAB_WINDOW_ADDR,
					      WAB_WINDOW_SIZE);
	if (cdisp.fb == NULL) {
		hal_log_info("CIRRUS: can't map the VRAM window.");
		wab_abort();
		return false;
	}

	/* Hand the shared VRAM over to the accelerator. */
	outp(VRAM_SW_PORT, VRAM_SW_WAB);
	hal_log_info("CIRRUS: WAB: shared VRAM switched to the "
		     "accelerator (6Ah=8Fh).");

	/* Full mode set (leaves the screen blanked). */
	cl_modeset_generic(true);

	/* Clear the whole 1MB VRAM through the banked window. */
	for (i = 0; i < (int)(cdisp.vram_size >> CL_BANK_SHIFT); i++) {
		cl_set_bank(i);
		memset(cdisp.fb, 0, 1 << CL_BANK_SHIFT);
	}
	cl_set_bank(0);

	/* Screen back on. */
	cl_seq_write(0x01, 0x01);

	/* Switch the video output relay to the accelerator. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_WAB);
	outp(WAB_RELAY2_PORT, 0x02);	/* PCI models (harmless elsewhere) */
	hal_log_info("CIRRUS: WAB: relay switched to the accelerator "
		     "(reg 03h=%02Xh, 0FACh=%02Xh).",
		     wab_read(WAB_REG_RELAY), inp(WAB_RELAY2_PORT));

	cdisp.chip_name = cdisp.alpine ? "CL-GD5430/5440 (WAB)" :
					 "CL-GD5428 (WAB)";
	cdisp.path = CIRRUS_PATH_54;

	return true;
}

static void
cirrus54_cleanup(void)
{
	/* Blank the accelerator output first (SR1 bit5). */
	cl_seq_write(0x01, 0x21);

	/*
	 * Relay back to the 98 GDC output: clear bit1 of the relay
	 * register.  Keep bit0 (register access) for the moment so
	 * the write above and the ones below still reach the chip,
	 * then drop it as the final WAB access.
	 */
	wab_write(WAB_REG_RELAY, WAB_RELAY_SETUP);
	outp(WAB_RELAY2_PORT, 0x00);	/* PCI models (harmless elsewhere) */

	/* Return the shared VRAM to the GDC. */
	outp(VRAM_SW_PORT, VRAM_SW_GDC);

	/* Disable register access; leave the relay on the GDC side. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_GDC);

	/* Put the video subsystem back to sleep (mirror of init). */
	outp(WAB_PFF82, 0x00);
}

/*****************************************************************************/
/* PCI module: GD754x/755x laptops and the PCI desktop Alpines               */
/*****************************************************************************/

#define PCI_CONFIG_ADDR		0x0cf8
#define PCI_CONFIG_DATA		0x0cfc
#define PCI_VENDOR_CIRRUS	0x1013

/* The linear framebuffer aperture we map and use. */
#define PCI_FB_LENGTH		0x00100000UL	/* 1MB */

/* Video output relay of the PCI models (verified on the Nb10). */
#define PCI_RELAY_PORT		0x0fac

/*
 * Known PCI device IDs.
 *
 * fb_offset is added to BAR0 to find the framebuffer: the NT4
 * miniport adds 0C00000h (12MB) on its 7548 path, and the same is
 * assumed for the rest of the 754x line.  The 755x (La) generation
 * and the desktop chips open their BAR plainly (XFree86 drives the
 * 755x that way; NP21/W's 5446 model likewise).
 */
static const struct pci_model {
	uint16_t dev;
	const char *name;
	bool laptop;		/* 754x/755x LCD chip (NEC stream path) */
	int def_bpp;
	uint32_t fb_offset;
} pci_models[] = {
	{ 0x0038, "CL-GD7548",      true,  16, 0x00C00000UL },
	{ 0x1200, "CL-GD7542",      true,  16, 0x00C00000UL },
	{ 0x1202, "CL-GD7543",      true,  16, 0x00C00000UL },
	{ 0x1204, "CL-GD7541",      true,  16, 0x00C00000UL },
	{ 0x0040, "CL-GD7555",      true,  16, 0x00000000UL },
	{ 0x004c, "CL-GD7556",      true,  16, 0x00000000UL },
	{ 0x00a0, "CL-GD5430/5440", false, 24, 0x00000000UL },
	{ 0x00a2, "CL-GD5432",      false, 24, 0x00000000UL },
	{ 0x00a4, "CL-GD5434",      false, 24, 0x00000000UL },
	{ 0x00a8, "CL-GD5434",      false, 24, 0x00000000UL },
	{ 0x00ac, "CL-GD5436",      false, 24, 0x00000000UL },
	{ 0x00b8, "CL-GD5446",      false, 24, 0x00000000UL },
	{ 0x00bc, "CL-GD5480",      false, 24, 0x00000000UL }
};

static const struct pci_model *chip;
static int pci_bus, pci_dev, pci_fn;
static bool ext_was_locked;

/* Saved chip state (restored on cleanup). */
static uint8_t sv_crtc[0x19];
static uint8_t sv_cr1a, sv_cr1b, sv_cr1d, sv_cr2c, sv_cr2d;
static uint8_t sv_crlcd[0x4e - 0x40 + 1];
static uint8_t sv_sr[0x30];
static uint8_t sv_gr[0x20];
static uint8_t sv_attr[0x15];
static uint8_t sv_misc, sv_hdr, sv_dac_mask;
static uint8_t sv_dac[256 * 3];

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

/* Scan the first buses, logging everything we pass by. */
static bool
pci_find_cirrus(int *obus, int *odev, int *ofn)
{
	int bus, dev, fn, nfn, ndev;
	uint32_t id, classcode;
	size_t i;

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
				classcode = pci_read32(bus, dev, fn, 0x08);
				hal_log_info("CIRRUS: PCI %d:%d.%d = "
					     "%04lX:%04lX class %02lXh.",
					     bus, dev, fn,
					     (unsigned long)(id & 0xffff),
					     (unsigned long)(id >> 16),
					     (unsigned long)(classcode >> 24));
				ndev++;
				if ((id & 0xffff) != PCI_VENDOR_CIRRUS)
					continue;
				for (i = 0;
				     i < sizeof(pci_models) /
					 sizeof(pci_models[0]);
				     i++) {
					if (pci_models[i].dev !=
					    (uint16_t)(id >> 16))
						continue;
					chip = &pci_models[i];
					*obus = bus;
					*odev = dev;
					*ofn = fn;
					return true;
				}
			}
		}
	}

	if (ndev == 0)
		hal_log_info("CIRRUS: PCI config space is silent.");
	else
		hal_log_info("CIRRUS: no supported Cirrus chip on PCI "
			     "(%d devices seen).", ndev);
	return false;
}

/*
 * Size BAR0 the standard way (all-ones write, read back the mask).
 * Memory decode is disabled around the probe; everything is
 * restored.  A genuine 16MB BAR (mask FF000000h) is the expected
 * answer on the 754x machines.
 */
static uint32_t
pci_size_bar0(void)
{
	uint32_t cmd, orig, mask;

	cmd = pci_read32(pci_bus, pci_dev, pci_fn, 0x04);
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, cmd & ~0x2UL);

	orig = pci_read32(pci_bus, pci_dev, pci_fn, 0x10);
	pci_write32(pci_bus, pci_dev, pci_fn, 0x10, 0xffffffffUL);
	mask = pci_read32(pci_bus, pci_dev, pci_fn, 0x10) & ~0xfUL;
	pci_write32(pci_bus, pci_dev, pci_fn, 0x10, orig);

	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, cmd);

	return mask ? (~mask + 1) : 0;
}

/*
 * Find the VGA register base: the PCI machines are expected at the
 * native 3C0h block (the Nb10 is), but the WAB-style relocation is
 * probed as a fallback.  The probe criterion is the SR06 extension
 * lock accepting the 12h key.
 *
 * The CRTC/ST1 base is then aligned with the CURRENT MISC bit0, so
 * that the firmware state can be read/saved from wherever the chip
 * is decoding right now.  (MISC is not modified here; the mode set
 * reprograms it and cl_misc_write() re-selects the base.)
 */
static bool
probe_regbase(void)
{
	static const struct {
		uint16_t b3c0, d4c, dac, d4m, dam;
	} bases[] = {
		{ 0x03c0, 0x03d4, 0x03da, 0x03b4, 0x03ba },	/* native */
		{ 0x0ca0, 0x0da4, 0x0daa, 0x0ba4, 0x0baa }	/* relocated */
	};
	int i, v, misc;

	for (i = 0; i < 2; i++) {
		cl_set_iobase(bases[i].b3c0, bases[i].d4c, bases[i].dac,
			      bases[i].d4m, bases[i].dam);

		/* Wake the chip if it is asleep (3C3 bit0). */
		v = inp(cdisp.io_3c0 + 0x03);
		if (v != 0xff && (v & 0x01) == 0)
			outp(cdisp.io_3c0 + 0x03, v | 0x01);

		v = cl_seq_read(0x06);
		ext_was_locked = (v != 0x12);
		cl_seq_write(0x06, 0x12);
		if (cl_seq_read(0x06) != 0x12)
			continue;

		misc = cl_misc_read();
		cl_select_crtc(misc);

		hal_log_info("CIRRUS: VGA registers at the %s block "
			     "(3C0h=%03Xh); MISC=%02Xh, CRTC at %03Xh.",
			     i == 0 ? "native" : "relocated",
			     cdisp.io_3c0, misc, cdisp.io_3d4);
		return true;
	}

	return false;
}

/* Dump the firmware-left register state before we touch anything. */
static void
dump_fw_regs(void)
{
	hal_log_info("CIRRUS: fw: SR07=%02Xh SR09=%02Xh SR0F=%02Xh "
		     "SR14=%02Xh SR16=%02Xh SR17=%02Xh SR1F=%02Xh.",
		     cl_seq_read(0x07), cl_seq_read(0x09),
		     cl_seq_read(0x0f), cl_seq_read(0x14),
		     cl_seq_read(0x16), cl_seq_read(0x17),
		     cl_seq_read(0x1f));
	hal_log_info("CIRRUS: fw: GR0B=%02Xh CR20=%02Xh CR27=%02Xh "
		     "CR2C=%02Xh CR2D=%02Xh MISC=%02Xh HDR=%02Xh.",
		     cl_gfx_read(0x0b), cl_crtc_read(0x20),
		     cl_crtc_read(0x27), cl_crtc_read(0x2c),
		     cl_crtc_read(0x2d), cl_misc_read(),
		     cl_hidden_dac_read());
}

/* Query the LCD panel behind the 754x shadow registers. */
static void
probe_lcd(void)
{
	static const char *lcd_types[] = {
		"dual-scan mono", "unknown", "DSTN", "TFT"
	};
	int cr2c, cr2d, size;

	cdisp.lcd = false;
	if (!chip->laptop)
		return;

	cr2c = cl_crtc_read(0x2c);
	cr2d = cl_crtc_read(0x2d);

	cl_crtc_write(0x2d, cr2d | 0x80);	/* LCD bank on */
	size = (cl_crtc_read(0x09) >> 2) & 3;
	cl_crtc_write(0x2d, cr2d);		/* LCD bank off */

	switch (size) {
	case 0:
		cdisp.lcd_w = 640;
		cdisp.lcd_h = 480;
		break;
	case 1:
		cdisp.lcd_w = 800;
		cdisp.lcd_h = 600;
		break;
	case 2:
		cdisp.lcd_w = 1024;
		cdisp.lcd_h = 768;
		break;
	default:
		cdisp.lcd_w = 0;
		cdisp.lcd_h = 0;
		break;
	}
	cdisp.lcd = true;

	hal_log_info("CIRRUS: LCD panel: %dx%d %s (CR2C=%02Xh).",
		     cdisp.lcd_w, cdisp.lcd_h,
		     lcd_types[(cr2c >> 6) & 3], cr2c);
}

/*
 * Save / restore the full register state so cleanup puts the chip
 * back exactly as the firmware left it.  The LCD shadow bank is a
 * 754x feature and is only touched on the laptop chips.
 */
static void
save_state(void)
{
	int i;

	/* Read the CRTC in the CRT bank (CR2D bit7 = 0 on 754x). */
	sv_cr2c = (uint8_t)cl_crtc_read(0x2c);
	sv_cr2d = (uint8_t)cl_crtc_read(0x2d);
	if (chip->laptop && (sv_cr2d & 0x80))
		cl_crtc_write(0x2d, sv_cr2d & 0x7f);

	for (i = 0; i < 0x19; i++)
		sv_crtc[i] = (uint8_t)cl_crtc_read(i);
	sv_cr1a = (uint8_t)cl_crtc_read(0x1a);
	sv_cr1b = (uint8_t)cl_crtc_read(0x1b);
	sv_cr1d = (uint8_t)cl_crtc_read(0x1d);

	/*
	 * The LCD timing bank (CR40-4E behind CR2D bit7) is the
	 * panel's own setup.  Save it so cleanup is exact; the
	 * mode-set rewrites it from the NEC table.
	 */
	if (chip->laptop) {
		cl_crtc_write(0x2d, sv_cr2d | 0x80);
		for (i = 0x40; i <= 0x4e; i++)
			sv_crlcd[i - 0x40] = (uint8_t)cl_crtc_read(i);
		cl_crtc_write(0x2d, sv_cr2d);
	}

	for (i = 0; i < 0x30; i++)
		sv_sr[i] = (uint8_t)cl_seq_read(i);

	for (i = 0; i < 0x20; i++)
		sv_gr[i] = (uint8_t)cl_gfx_read(i);

	for (i = 0; i < 0x15; i++)
		sv_attr[i] = (uint8_t)cl_attr_read(i);

	sv_misc = (uint8_t)cl_misc_read();
	sv_hdr = (uint8_t)cl_hidden_dac_read();

	sv_dac_mask = (uint8_t)inp(cdisp.io_3c0 + 0x06);
	outp(cdisp.io_3c0 + 0x07, 0x00);
	for (i = 0; i < 256 * 3; i++)
		sv_dac[i] = (uint8_t)inp(cdisp.io_3c0 + 0x09);
}

static void
restore_state(void)
{
	int i;

	/* Sequencer (skip SR06 lock; handled by the caller). */
	for (i = 0; i < 0x30; i++) {
		if (i == 0x06)
			continue;
		cl_seq_write(i, sv_sr[i]);
	}

	for (i = 0; i < 0x20; i++)
		cl_gfx_write(i, sv_gr[i]);

	/* MISC first: bit0 re-selects the CRTC base for the writes below. */
	cl_misc_write(sv_misc);

	/* CRTC: unlock CR0-7, restore the CRT bank, then the LCD bank. */
	cl_crtc_write(0x11, sv_crtc[0x11] & 0x7f);
	for (i = 0; i < 0x19; i++) {
		if (i == 0x11)
			continue;
		cl_crtc_write(i, sv_crtc[i]);
	}
	cl_crtc_write(0x1a, sv_cr1a);
	cl_crtc_write(0x1b, sv_cr1b);
	cl_crtc_write(0x1d, sv_cr1d);

	if (chip->laptop) {
		cl_crtc_write(0x2d, sv_cr2d | 0x80);
		for (i = 0x40; i <= 0x4e; i++)
			cl_crtc_write(i, sv_crlcd[i - 0x40]);
	}
	cl_crtc_write(0x2c, sv_cr2c);
	cl_crtc_write(0x2d, sv_cr2d);
	cl_crtc_write(0x11, sv_crtc[0x11]);

	for (i = 0; i < 0x15; i++)
		cl_attr_write(i, sv_attr[i]);
	(void)inp(cdisp.io_3da);
	outp(cdisp.io_3c0 + 0x00, 0x20);

	outp(cdisp.io_3c0 + 0x06, sv_dac_mask);
	outp(cdisp.io_3c0 + 0x08, 0x00);
	for (i = 0; i < 256 * 3; i++)
		outp(cdisp.io_3c0 + 0x09, sv_dac[i]);
	cl_hidden_dac_write(sv_hdr);

	cl_seq_write(0x01, sv_sr[0x01]);
}

static void
relay_to_accel(void)
{
	hal_log_info("CIRRUS: relay 0FACh reads %02Xh.",
		     inp(PCI_RELAY_PORT));
	outp(PCI_RELAY_PORT, 0x02);
	hal_log_info("CIRRUS: 0FACh now reads %02Xh.",
		     inp(PCI_RELAY_PORT));
}

static void
relay_to_gdc(void)
{
	outp(PCI_RELAY_PORT, 0x00);
}

/*
 * CL-GD7548 mode-set register streams, extracted verbatim from the
 * PC-98 NT 4.0 miniport (CIRRUS.SYS, 1996-10-13, chip tag 0xE).
 * The streams program MISC bit0 = 0, so the CRTC is written at the
 * MONO block 3B4h/3B5h (cl_misc_write() switches the base
 * automatically).  The CR2D 80h/11h dance in the middle of the CRTC
 * list switches to the 754x LCD shadow bank and back - it is part
 * of the captured order and must be preserved.
 */

struct cl_regpair { uint8_t idx, val; };

/* ---- 8bpp 640x480  MISC=0XE2  HiddenDAC=0X00  stride 640 ---- */
static const struct cl_regpair m8_seq[] = {
	{0x06,0x12}, {0x12,0x00}, {0x00,0x01}, {0x01,0x01}, {0x02,0x0F}, {0x03,0x00}, {0x04,0x0E}, {0x07,0xC1}, {0x0F,0x31}, {0x16,0xF3}, {0x1F,0x23}, {0x21,0x08}, {0x25,0x04}, {0x2A,0x00}, {0x2B,0x80}, {0x2C,0x00}, {0x2D,0x00}, {0x2E,0x08}, {0x2F,0x02}, {0x0B,0x66}, {0x0C,0x53}, {0x0D,0x5F}, {0x0E,0x6E}, {0x1B,0x3B}, {0x1C,0x30}, {0x1D,0x23}, {0x1E,0x2A}
};
static const struct cl_regpair m8_crtc[] = {
	{0x11,0x20}, {0x00,0x5F}, {0x01,0x4F}, {0x02,0x50}, {0x03,0x82}, {0x04,0x55}, {0x05,0x9F}, {0x06,0x0B}, {0x07,0x3E}, {0x08,0x00}, {0x09,0x40}, {0x0A,0x00}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x0F,0x00}, {0x10,0xE5}, {0x11,0xA7}, {0x12,0xDF}, {0x13,0x50}, {0x14,0x00}, {0x15,0xE7}, {0x16,0x04}, {0x17,0xE3}, {0x18,0xFF}, {0x19,0x00}, {0x1A,0x00}, {0x1B,0x02}, {0x1D,0x10}, {0x1E,0x21}, {0x1F,0x00}, {0x20,0x62}, {0x21,0x00}, {0x23,0x10}, {0x2C,0xC3}, {0x2E,0x00}, {0x30,0x00}, {0x3C,0x00}, {0x40,0xC0}, {0x41,0x00}, {0x42,0x00}, {0x43,0x02}, {0x44,0xA5}, {0x47,0xA2}, {0x48,0x10}, {0x49,0x00}, {0x4A,0xDF}, {0x4B,0x00}, {0x4C,0x00}, {0x4D,0x66}, {0x4E,0x40}, {0x2D,0x80}, {0x02,0x00}, {0x03,0xCC}, {0x04,0xE5}, {0x05,0xEC}, {0x06,0x15}, {0x07,0x8D}, {0x08,0x00}, {0x09,0x02}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x2D,0x11}
};
static const struct cl_regpair m8_gpost[] = {
	{0x00,0x00}, {0x01,0x00}, {0x02,0x00}, {0x03,0x00}, {0x04,0x00}, {0x05,0x40}, {0x06,0x05}, {0x07,0x0F}, {0x08,0xFF}
};
#define M8_MISC		0xE2
#define M8_HDR		0x00
#define M8_PITCH	640

/* ---- 16bpp 640x480  MISC=0XEE  HiddenDAC=0XE1  stride 1280 ---- */
static const struct cl_regpair m16_seq[] = {
	{0x06,0x12}, {0x12,0x00}, {0x00,0x01}, {0x01,0x01}, {0x02,0x0F}, {0x03,0x00}, {0x04,0x0E}, {0x07,0xC3}, {0x0F,0x31}, {0x16,0xF7}, {0x1F,0x23}, {0x21,0x08}, {0x25,0x04}, {0x2A,0x00}, {0x2B,0x80}, {0x2C,0x00}, {0x2D,0x00}, {0x2E,0x08}, {0x2F,0x02}, {0x0B,0x66}, {0x0C,0x53}, {0x0D,0x5F}, {0x0E,0x66}, {0x1B,0x3B}, {0x1C,0x30}, {0x1D,0x23}, {0x1E,0x3A}
};
static const struct cl_regpair m16_crtc[] = {
	{0x11,0x20}, {0x00,0x5F}, {0x01,0x4F}, {0x02,0x50}, {0x03,0x82}, {0x04,0x54}, {0x05,0x9E}, {0x06,0x0B}, {0x07,0x3E}, {0x08,0x00}, {0x09,0x40}, {0x0A,0x00}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x0F,0x00}, {0x10,0xE5}, {0x11,0xA7}, {0x12,0xDF}, {0x13,0xA0}, {0x14,0x00}, {0x15,0xE7}, {0x16,0x04}, {0x17,0xE3}, {0x18,0xFF}, {0x19,0x00}, {0x1A,0x00}, {0x1B,0x02}, {0x1D,0x10}, {0x1E,0x21}, {0x1F,0x00}, {0x20,0x62}, {0x21,0x00}, {0x23,0x10}, {0x2C,0xC3}, {0x2E,0x00}, {0x30,0x00}, {0x3C,0x00}, {0x40,0xBF}, {0x41,0x00}, {0x42,0x00}, {0x43,0x01}, {0x44,0xA5}, {0x47,0xA2}, {0x48,0x10}, {0x49,0x00}, {0x4A,0xDF}, {0x4B,0x00}, {0x4C,0x00}, {0x4D,0x66}, {0x4E,0x40}, {0x2D,0x80}, {0x02,0x00}, {0x03,0xCC}, {0x04,0xE5}, {0x05,0xEC}, {0x06,0x15}, {0x07,0x8D}, {0x08,0x00}, {0x09,0x02}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x2D,0x11}
};
static const struct cl_regpair m16_gpost[] = {
	{0x00,0x00}, {0x01,0x00}, {0x02,0x00}, {0x03,0x00}, {0x04,0x00}, {0x05,0x40}, {0x06,0x05}, {0x07,0x0F}, {0x08,0xFF}
};
#define M16_MISC	0xEE
#define M16_HDR		0xE1
#define M16_PITCH	1280

/* ---- 24bpp 640x480  MISC=0XEE  HiddenDAC=0XE5  stride 2048 ---- */
static const struct cl_regpair m24_seq[] = {
	{0x06,0x12}, {0x12,0x00}, {0x00,0x01}, {0x01,0x01}, {0x02,0x0F}, {0x03,0x00}, {0x04,0x0E}, {0x07,0xC5}, {0x0F,0x31}, {0x16,0xFE}, {0x1F,0x23}, {0x21,0x08}, {0x25,0x04}, {0x2A,0x00}, {0x2B,0x80}, {0x2C,0x00}, {0x2D,0x00}, {0x2E,0x08}, {0x2F,0x02}, {0x0B,0x66}, {0x0C,0x53}, {0x0D,0x5F}, {0x0E,0x6E}, {0x1B,0x3B}, {0x1C,0x30}, {0x1D,0x23}, {0x1E,0x2A}
};
static const struct cl_regpair m24_crtc[] = {
	{0x11,0x20}, {0x00,0x5F}, {0x01,0x4F}, {0x02,0x50}, {0x03,0x82}, {0x04,0x54}, {0x05,0x9E}, {0x06,0x0B}, {0x07,0x3E}, {0x08,0x00}, {0x09,0x40}, {0x0A,0x00}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x0F,0x00}, {0x10,0xE5}, {0x11,0xA7}, {0x12,0xDF}, {0x13,0x00}, {0x14,0x00}, {0x15,0xE7}, {0x16,0x04}, {0x17,0xE3}, {0x18,0xFF}, {0x19,0x00}, {0x1A,0x00}, {0x1B,0x12}, {0x1D,0x10}, {0x1E,0x21}, {0x1F,0x00}, {0x20,0x62}, {0x21,0x00}, {0x23,0x10}, {0x2C,0xC3}, {0x2E,0x00}, {0x30,0x00}, {0x3C,0x00}, {0x40,0xBF}, {0x41,0x00}, {0x42,0x00}, {0x43,0x00}, {0x44,0xA5}, {0x47,0xA2}, {0x48,0x10}, {0x49,0x00}, {0x4A,0xDF}, {0x4B,0x00}, {0x4C,0x00}, {0x4D,0x66}, {0x4E,0x40}, {0x2D,0x80}, {0x02,0x00}, {0x03,0xCC}, {0x04,0xE5}, {0x05,0xEC}, {0x06,0x15}, {0x07,0x8D}, {0x08,0x00}, {0x09,0x02}, {0x0B,0x00}, {0x0C,0x00}, {0x0D,0x00}, {0x0E,0x00}, {0x2D,0x11}
};
static const struct cl_regpair m24_gpost[] = {
	{0x00,0x00}, {0x01,0x00}, {0x02,0x00}, {0x03,0x00}, {0x04,0x00}, {0x05,0x40}, {0x06,0x05}, {0x07,0x0F}, {0x08,0xFF}
};
#define M24_MISC	0xEE
#define M24_HDR		0xE5
#define M24_PITCH	2048

/* The AC stream is identical in all three modes. */
static const uint8_t m754x_atc[21] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};

static void
seq_stream(const struct cl_regpair *p, int n)
{
	int i;

	for (i = 0; i < n; i++)
		cl_seq_write(p[i].idx, p[i].val);
}

static void
crtc_stream(const struct cl_regpair *p, int n)
{
	int i;

	/*
	 * The captured CRTC list includes CR2D 80h (select the LCD
	 * shadow bank) partway through and CR2D 11h at the end.  We
	 * just replay it in order; cl_crtc_write hits the mono block
	 * that cl_misc_write(MISC bit0=0) selected.
	 */
	for (i = 0; i < n; i++)
		cl_crtc_write(p[i].idx, p[i].val);
}

static void
gfx_stream(const struct cl_regpair *p, int n)
{
	int i;

	for (i = 0; i < n; i++)
		cl_gfx_write(p[i].idx, p[i].val);
}

/*
 * Replay the NEC NT4 mode set for the 754x laptops.  cdisp.bpp
 * selects the stream; the pitch is the stream's fixed stride (set
 * by the caller).
 */
static void
program_mode_754x(void)
{
	const struct cl_regpair *seq, *crtc, *gpost;
	int nseq, ncrtc, ngpost;
	uint8_t misc, hdr;
	int i;

	switch (cdisp.bpp) {
	case 8:
		seq = m8_seq;
		nseq = (int)(sizeof(m8_seq) / sizeof(m8_seq[0]));
		crtc = m8_crtc;
		ncrtc = (int)(sizeof(m8_crtc) / sizeof(m8_crtc[0]));
		gpost = m8_gpost;
		ngpost = (int)(sizeof(m8_gpost) / sizeof(m8_gpost[0]));
		misc = M8_MISC;
		hdr = M8_HDR;
		break;
	case 24:
		seq = m24_seq;
		nseq = (int)(sizeof(m24_seq) / sizeof(m24_seq[0]));
		crtc = m24_crtc;
		ncrtc = (int)(sizeof(m24_crtc) / sizeof(m24_crtc[0]));
		gpost = m24_gpost;
		ngpost = (int)(sizeof(m24_gpost) / sizeof(m24_gpost[0]));
		misc = M24_MISC;
		hdr = M24_HDR;
		break;
	default:	/* 16bpp */
		seq = m16_seq;
		nseq = (int)(sizeof(m16_seq) / sizeof(m16_seq[0]));
		crtc = m16_crtc;
		ncrtc = (int)(sizeof(m16_crtc) / sizeof(m16_crtc[0]));
		gpost = m16_gpost;
		ngpost = (int)(sizeof(m16_gpost) / sizeof(m16_gpost[0]));
		misc = M16_MISC;
		hdr = M16_HDR;
		break;
	}

	/* Sequencer: unlock, HW cursor off, then the depth block. */
	seq_stream(seq, nseq);

	/* SR0F: preserve the DRAM-type bits, force extended-mode bit. */
	cl_seq_write(0x0f, (cl_seq_read(0x0f) & 0xdf) | 0x20);

	/*
	 * SR17 |= 44h: enable memory-mapped I/O with the MMIO block
	 * in the last 256 bytes of the linear aperture (what the NT
	 * miniport does for linear machines, VA 1B033h).  The VRAM
	 * clear below therefore stops 256 bytes short of the end.
	 */
	cl_seq_write(0x17, (uint8_t)(cl_seq_read(0x17) | 0x44));

	/*
	 * Miscellaneous Output.  Bit0 = 0 -> the CRTC and Input
	 * Status 1 move to the mono block (3B4h/3BAh);
	 * cl_misc_write() re-points our accessors accordingly, so
	 * the stream below lands where the chip actually decodes.
	 */
	cl_misc_write(misc);

	/* GR06 = 05 (graphics, correct memory map) before the CRTC. */
	cl_gfx_write(0x06, 0x05);

	/* SR03 char map, then the CRTC incl. the LCD shadow dance. */
	cl_seq_write(0x03, 0x00);
	crtc_stream(crtc, ncrtc);

	/* Graphics controller. */
	gfx_stream(gpost, ngpost);

	/* Attribute controller (identity palette + mode bits). */
	(void)inp(cdisp.io_3da);		/* reset flip-flop */
	for (i = 0; i < 21; i++) {
		outp(cdisp.io_3c0, (uint8_t)i);
		outp(cdisp.io_3c0, m754x_atc[i]);
	}
	(void)inp(cdisp.io_3da);
	outp(cdisp.io_3c0, 0x20);		/* enable video output */

	/* Hidden DAC / pixel mask (depth format). */
	cl_hidden_dac_write(hdr);
	outp(cdisp.io_3c0 + 0x06, 0xff);	/* pixel mask */

	/* Tail: banking off, blitter idle, extended write bits. */
	cl_gfx_write(0x09, 0x00);
	cl_gfx_write(0x0a, 0x00);
	cl_gfx_write(0x0b, 0x20);
	cl_gfx_write(0x31, 0x00);
	cl_gfx_write(0x0e, 0x00);
	cdisp.cur_bank = 0;

	/* DAC: the RGB332 palette for 8bpp / a ramp otherwise. */
	cl_load_palette();
}

static bool
cirrus75_init(int mode, int req_bpp)
{
	uint32_t bar0, barsize, cmd;
	int w, h, bpp;

	if (!pci_find_cirrus(&pci_bus, &pci_dev, &pci_fn))
		return false;

	hal_log_info("CIRRUS: %s found at PCI %d:%d.%d (dev ID %04Xh).",
		     chip->name, pci_bus, pci_dev, pci_fn, chip->dev);

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	if (chip->laptop) {
		if (mode != DISP_640X480) {
			hal_log_info("CIRRUS: %s: only 640x480 is "
				     "implemented (NEC streams for "
				     "800x600/1024x768 exist in the NT "
				     "miniport at VA 144D0h/14640h/147B0h "
				     "but are unported).", chip->name);
			return false;
		}
	} else {
		if (mode != DISP_640X480 && mode != DISP_800X600) {
			hal_log_info("CIRRUS: PCI desktop: %dx%d not "
				     "supported (640x480 / 800x600 only).",
				     w, h);
			return false;
		}
	}

	/* BAR0, its decode size, then enable I/O + memory decode. */
	bar0 = pci_read32(pci_bus, pci_dev, pci_fn, 0x10);
	barsize = pci_size_bar0();
	cmd = pci_read32(pci_bus, pci_dev, pci_fn, 0x04);
	hal_log_info("CIRRUS: BAR0=%08lXh (decode %luMB), cmd=%04lXh.",
		     (unsigned long)bar0,
		     (unsigned long)(barsize >> 20),
		     (unsigned long)(cmd & 0xffff));
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, cmd | 0x03);
	bar0 &= ~0xfUL;
	if (bar0 == 0) {
		hal_log_info("CIRRUS: BAR0 is unassigned, giving up.");
		return false;
	}

	/*
	 * The framebuffer aperture.  On the 754x line this is BAR0 +
	 * 12MB (the single most important fact in this file; see the
	 * analysis at the top).  The 755x/desktop chips open their
	 * BAR plainly.
	 */
	cdisp.fb_phys = bar0 + chip->fb_offset;
	cdisp.vram_size = PCI_FB_LENGTH;
	cdisp.linear = true;
	hal_log_info("CIRRUS: framebuffer = BAR0 + %08lXh = %08lXh.",
		     (unsigned long)chip->fb_offset,
		     (unsigned long)cdisp.fb_phys);

	cdisp.fb = (uint8_t *)cl_map_physical(cdisp.fb_phys,
					      cdisp.vram_size);
	if (cdisp.fb == NULL) {
		hal_log_info("CIRRUS: can't map the framebuffer.");
		return false;
	}

	/* Find the VGA register block and dump the firmware state. */
	if (!probe_regbase()) {
		hal_log_info("CIRRUS: VGA registers not responding at "
			     "the native or relocated base.");
		return false;
	}
	dump_fw_regs();

	cdisp.crt27 = (uint8_t)cl_crtc_read(0x27);

	/* Save everything the firmware left, then blank. */
	save_state();
	cl_seq_write(0x01, sv_sr[0x01] | 0x20);

	probe_lcd();

	/* Resolve the depth. */
	if (chip->laptop) {
		if (req_bpp == -1) {
			bpp = chip->def_bpp;
			hal_log_info("CIRRUS: %s: auto depth -> %d bpp "
				     "(NEC's panel default; 24bpp can be "
				     "requested explicitly).",
				     chip->name, bpp);
		} else {
			bpp = req_bpp;
		}
	} else {
		bpp = cl_resolve_bpp(req_bpp, 24, w, h, cdisp.vram_size,
				     "PCI");
		if (bpp < 0)
			return false;
	}

	cdisp.scr_w = w;
	cdisp.scr_h = h;
	cdisp.bpp = bpp;
	if (chip->laptop) {
		/* Pitches are fixed by the NEC streams. */
		switch (bpp) {
		case 8:
			cdisp.pitch = M8_PITCH;
			break;
		case 24:
			cdisp.pitch = M24_PITCH;	/* 2048, not 1920! */
			break;
		default:
			cdisp.pitch = M16_PITCH;
			break;
		}
	} else {
		cdisp.pitch = (uint32_t)w * (uint32_t)(bpp / 8);
	}

	hal_log_info("CIRRUS: setting %dx%d %d bpp (pitch %lu) via %s.",
		     w, h, bpp, (unsigned long)cdisp.pitch,
		     chip->laptop ? "the NEC 754x register streams" :
				    "the generic Alpine mode set");

	/* Program the mode. */
	if (chip->laptop)
		program_mode_754x();
	else
		cl_modeset_generic(false);

	/*
	 * Clear VRAM, but stop 256 bytes short of the end: with
	 * SR17 bit6 set (NEC's linear-machine setting) the last 256
	 * bytes of the linear block decode as memory-mapped BLT
	 * registers, and a memset over them could start a random
	 * blit.
	 */
	memset(cdisp.fb, 0, cdisp.vram_size - 0x100);

	/* Screen on. */
	cl_seq_write(0x01, 0x01);

	/* Switch the panel/output relay to the accelerator. */
	relay_to_accel();

	cdisp.chip_name = chip->name;
	cdisp.path = CIRRUS_PATH_75;

	return true;
}

static void
cirrus75_cleanup(void)
{
	/* Blank while we unwind. */
	cl_seq_write(0x01, 0x21);

	/* Output back to the 98 GDC. */
	relay_to_gdc();

	/* Put every register back the way the firmware left it. */
	restore_state();

	/* Re-lock the extensions if they were locked when we came. */
	if (ext_was_locked)
		cl_seq_write(0x06, 0x0f);
}
