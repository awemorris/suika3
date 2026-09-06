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
 * PC-9821 Graphics Architecture
 * ===========================================================================
 *
 * PC-9821 accelerators are classified by BOTH the graphics chip and
 * the motherboard-side mechanism. The useful architectural classes
 * are:
 *
 * - WAB S3
 *     - Firstly, NEC chose S3 86C928 for the first PC-9821 SVGA, and named
 *       it WAB.
 *     - A WAB is just a S3 chip connected via "Local-Bus Slot."
 *     - NEC used S3 86C928 and Vision864 for WAB.
 *     - Developers can map "Linear Frame Buffer" (LFB) by accessing WAB I/O
 *       ports, and that is, they are just the S3 I/O ports.
 *     - S3 WAB machines are desktop only.
 * - WAB Cirrus
 *     - Secondly, NEC chose affordable Cirrus GD54xx chips.
 *     - But WAB protocol is equal to S3 registers, so NEC added the emulation
 *       LSI between CPU and GD54xx.
 *     - SVGA chips are Cirrus Logic GD5428 and GD5430.
 *     - Cirrus WAB machines are desktop only.
 *     - Desktop only.
 * - WAB Emulation Cirrus + LCD
 *     - Same as WAB Emulation, but has extra LCD controller.
 *     - Laptop only.
 * - Core-Graph Bridge + Cirrus
 *     - Core-Graph is a PCI PnP device.
 *     - SVGA chip is not connected to PCI, but connected to Core-Graph's internal bus.
 *     - SVGA chip is not a PnP device.
 *     - SVGA chips are Cirrus Logic GD5440 and 5446.
 *     - Desktop only.
 * - PCI Cirrus + LCD
 *     - PCI Cirrus Logic GD7543, GD7548, and GD7555.
 *     - Extra LCD controller is integrated with GDC.
 *     - Laptop only.
 * - PCI Matrox
 *     - PCI PnP device. Matrox MGA-2064W and MGA-1064SG.
 *     - Desktop only.
 * - PCI Trident
 *     - PCI PnP device, Trident TGUI9685 and TGUI9680XGi.
 *     - Desktop only.
 * - PCI Trident + LCD
 *     - PCI PnP device, Trident Cyber9320, Cyber9382, and Cyber9385.
 *     - Laptop only.
 *
 * | Y/M     | Series    | Models   | Video Chipset       | Connection    | Driver           | Confirmed?          |
 * |---------|-----------|----------|---------------------|---------------|------------------|---------------------|
 * | 1993/02 | MATE A    | Ae/M2    | S3 86C928           | WAB           | WAB              |                     |
 * | 1993/02 | MATE A    | Ae/M7    | S3 86C928           | WAB           | WAB              |                     |
 * | 1993/02 | MATE A    | Ap/U2    | S3 86C928           | WAB           | WAB              |                     |
 * | 1993/02 | MATE A    | As/U2    | S3 86C928           | WAB           | WAB              |                     |
 * | 1993/08 | MATE A    | Af/U9W   | S3 86C928           | WAB           | WAB              |                     |
 * | 1993    | MATE A    | Ap2      | S3 86C928           | WAB           | WAB              |                     |
 * | 1993    | MATE A    | As2      | S3 86C928           | WAB           | WAB              |                     |
 * | 1994/05 | MATE A    | An/U2    | S3 Vision864        | WAB           | WAB              |                     |
 * | 1994    | MATE A    | Ap3      | S3 Vision864        | WAB           | WAB              |                     |
 * | 1994    | MATE A    | As3      | S3 Vision864        | WAB           | WAB              |                     |
 * | 1994    | MATE X    | Xn       | S3 Vision864        | WAB           | WAB              |                     |
 * | 1994    | MATE X    | Xs       | S3 Vision864        | WAB           | WAB              |                     |
 * | 1994    | MATE X    | Xp       | S3 Vision864        | WAB           | WAB              |                     |
 * | 1993    | MATE B    | Bp       | Cirrus Logic GD5428 | WAB Emulation | WAB              | NP21/W              |
 * | 1993    | MATE B    | Bs       | Cirrus Logic GD5428 | WAB Emulation | WAB              | NP21/W              |
 * | 1993    | MATE B    | Be       | Cirrus Logic GD5428 | WAB Emulation | WAB              | NP21/W              |
 * | 1993    | MATE B    | Bf       | Cirrus Logic GD5428 | WAB Emulation | WAB              | NP21/W              |
 * | 1994    | CanBe     | Cb       | Cirrus Logic GD5430 | WAB Emulation | WAB              |                     |
 * | 1994    | CanBe     | Cx       | Cirrus Logic GD5430 | WAB Emulation | WAB              |                     |
 * | 1995    | CanBe     | Cx2      | Cirrus Logic GD5430 | WAB Emulation | WAB              | NP21/W              |
 * | 1995    | CanBe     | Cb2      | Cirrus Logic GD5430 | WAB Emulation | WAB              | NP21/W              |
 * | 1995    | MATE X    | Xe10     | Cirrus Logic GD5430 | WAB Emulation | WAB              | NP21/W              |
 * | 1994    | 98NOTE    | Ne       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Nf       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Np       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Ns       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Nx       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Nd       | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1994    | 98NOTE    | Ne2      | Cirrus Logic GD5428 | WAB Emulation | WAB + LCD        |                     |
 * | 1995/08 | MATE X    | Xa7e     | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1995/11 | MATE X    | Xb10     | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1995/11 | CanBe     | Cb3      | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1995/11 | CanBe     | Cx3      | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1995/11 | ValueStar | V7       | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1996    | ValueStar | V13/S5   | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       | OK                  |
 * | 1996/10 | ValueStar | V20/S5   | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1996/10 | ValueStar | V20/S7   | Cirrus Logic GD5440 | Core-Graph    | Core-Graph       |                     |
 * | 1995    | CanBe     | Cu10     | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1995    | CanBe     | Ct16     | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1995    | ValueStar | V10      | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1996    | ValueStar | V12      | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1997/05 | ValueStar | V16/S5P  | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1997/02 | MATE X    | Xc13/S5  | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1997/02 | MATE X    | Xc16/M7  | Cirrus Logic GD5446 | Core-Graph    | Core-Graph       |                     |
 * | 1995    | 98NOTE    | Nb7      | Cirrus Logic GD7543 | PCI           | PCI Cirrus + LCD |                     |
 * | 1996    | 98NOTE    | Nb10     | Cirrus Logic GD7548 | PCI           | PCI Cirrus + LCD |                     |
 * | 1996    | 98NOTE    | Na13     | Cirrus Logic GD7548 | PCI           | PCI Cirrus + LCD |                     |
 * | 1996    | 98NOTE    | Ls12     | Cirrus Logic GD7555 | PCI           | PCI Cirrus + LCD |                     |
 * | 1997    | 98NOTE    | Nr12     | Cirrus Logic GD7555 | PCI           | PCI Cirrus + LCD |                     |
 * | 1997    | 98NOTE    | Nr13     | Cirrus Logic GD7555 | PCI           | PCI Cirrus + LCD |                     |
 * | 1997    | 98NOTE    | Ls13     | Cirrus Logic GD7555 | PCI           | PCI Cirrus + LCD |                     |
 * | 1997    | 98NOTE    | Ls150    | Cirrus Logic GD7555 | PCI           | PCI Cirrus + LCD |                     |
 * | 1994    | MATE X    | Xf       | Matrox MGA-II       | Local-Bus     | Matrox (VL)      |                     |
 * | 1995    | MATE X    | Xt13     | Matrox MGA-2064W    | PCI           | Matrox           |                     |
 * | 1996    | MATE X    | Xv13     | Matrox MGA-2064W    | PCI           | Matrox           |                     |
 * | 1996    | MATE X    | Xt16     | Matrox MGA-2064W    | PCI           | Matrox           |                     |
 * | 1996    | MATE X    | Xv20     | Matrox MGA-2064W    | PCI           | Matrox           |                     |
 * | 1997    | MATE R    | Rv20     | Matrox MGA-2064W    | PCI           | Matrox           |                     |
 * | 1997    | ValueStar | V166     | Matrox MGA-1064SG   | PCI           | Matrox           |                     |
 * | 1997    | ValueStar | V200     | Matrox MGA-1064SG   | PCI           | Matrox           |                     |
 * | 1997    | ValueStar | V233     | Matrox MGA-1064SG   | PCI           | Matrox           |                     |
 * | 1995    | 98NOTE    | Nx       | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1995    | 98NOTE    | Nd2      | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1995    | 98NOTE    | Lt2      | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1995    | 98NOTE    | Ne3      | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1995    | 98NOTE    | Na7      | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1995    | 98NOTE    | Na9      | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1996    | 98NOTE    | Na12     | Trident Cyber9320   | PCI           | Trident + LCD    |                     |
 * | 1996    | 98NOTE    | Na15     | Trident Cyber9382   | PCI           | Trident + LCD    |                     |
 * | 1997    | 98NOTE    | Nr15     | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1997    | 98NOTE    | Nr150    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1997    | 98NOTE    | Nr166    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1997    | 98NOTE    | Nw133    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1997    | 98NOTE    | Nw150    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1998    | 98NOTE    | Nr233    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1998    | 98NOTE    | Nr266    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1999    | 98NOTE    | Nr300    | Trident Cyber9385   | PCI           | Trident + LCD    |                     |
 * | 1995    | CanBe     | Cu13     | Trident TGUI9685    | PCI           | Trident          |                     |
 * | 1995    | CanBe     | Ct20     | Trident TGUI9685    | PCI           | Trident          |                     |
 * | 1995    | MATE X    | Xa7      | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1995    | MATE X    | Xa10     | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1996    | MATE X    | Xa12     | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1996    | MATE X    | Xa13     | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1996    | MATE X    | Xa16     | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1997/07 | MATE X    | Xc13/M7  | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1996/06 | ValueStar | V13/M7   | Trident TGUI9682XGi | PCI           | Trident          |                     |
 * | 1996/10 | ValueStar | V16/M7   | Trident TGUI9682XGi | PCI           | Trident          |                     |
 * | 1996/10 | ValueStar | V20/M7   | Trident TGUI9680XGi | PCI           | Trident          |                     |
 * | 1997    | MATE R    | Ra266    | Trident TGUI9682XGi | PCI           | Trident          |                     |
 * | 1998    | MATE R    | Ra300    | Trident TGUI9682XGi | PCI           | Trident          |                     |
 * | 1998    | MATE R    | Ra333    | Trident TGUI9682XGi | PCI           | Trident          |                     |
 * | 2000    | MATE R    | Ra43     | Trident TGUI9682XGi | PCI           | Trident          |                     |
 *
 * ---------------------------------------------------------------------------
 * Chip & Model Identification Table
 * ---------------------------------------------------------------------------
 *
 * | ChipFamily | Connectivity / Probe | Target Chip / Board | ModelCode                                | IoVariant             |
 * |------------|----------------------|---------------------|------------------------------------------|-----------------------|
 * | 0x04       | I/O Port (Native)    | GD5428 Series       | 0x00, 0x01, 0x02, 0x04, 0x09, 0x0A, 0x0B | 1 (WAB Native)        |
 * | 0x04       | I/O Port (WAB Probe) | WAB (NEC WAB B3)    | 0x02                                     | 2 (WAB PCI)           |
 * | 0x08       | I/O Port (Native)    | GD5430              | 0x03, 0x05, 0x06, 0x07, 0x08             | 1 (WAB Native)        |
 * | 0x08       | I/O Port (Native)    | GD5440              | 0x14                                     | 3 (Core-Graph Bridge) |
 * | 0x40       | PCI (DevID 0x1202)   | GD7543              | 0x0C, 0x0D                               | 4 (PCI)               |
 * | 0x40       | PCI (DevID 0x0038)   | GD7548              | 0x0E, 0x0F, 0x10, 0x12, 0x13             | 4 (PCI)               |
 * | 0x80       | PCI (DevID 0x00B8)   | GD5446              | 0x11                                     | 4 (PCI)               |
 *
 * ---------------------------------------------------------------------------
 * Cirrus access-path verdicts
 * ---------------------------------------------------------------------------
 *
 * Access-path verdicts per class:
 *
 *   WAB Emulation: accessed through the WAB interface itself; no
 *   special problem arises on this path.
 *
 *   WAB Emulation:
 *   Core-Graph Bridge + GD5430/5440:
 *
 *      The CRT signal is presumed to come from the Cirrus itself.
 *      Once the Core-Graph Bridge is instructed to switch its CRT
 *      signal source, the ordinary generic Cirrus LFB is usable --
 *      and so is the CPU-source FIFO.
 *
 *   PCI GD75xx laptops: -- role split.
 *
 *     GD75xx:  external Cirrus VRAM, VRAM scanout, pixel formatter,
 *              and pixel-bus master.
 *     NEC LSI: pixel-bus slave, external NEC VRAM, retimings and signal
 *              generation for both LCD and CRT, and the final source
 *              selection (GDC vs Cirrus), 
 *
 *     Our analysis shows GD75xx doesn't have a first-class LFB.  Instead,
 *     CPU-source FIFO is required.  This is due to lack of a nice arbitrator
 *     for the Cirrus VRAM.
 */

/* HAL */
#include <strato/strato.h>	/* Public Interface */
#include "98disp.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
	CIRRUS_PATH_54_BANKED,	/* classic banked onboard/WAB interface */
	CIRRUS_PATH_54_COREGRAPH,	/* V13-class Core-Graph linear GD5440 */
	CIRRUS_PATH_75		/* independently enumerated PCI Cirrus */
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

	/* Host-visible aperture; the same writes become FIFO data during MEMSYSSRC. */
	uint8_t *fb;
	uint32_t fb_phys;
	bool linear;		/* true: linear; false: 32KB banked window */
	uint32_t vram_size;
	int cur_bank;
	bool fifo_only;	/* true: CPU-source BLT FIFO; false: direct aperture */
	bool fifo_capable;	/* GD54xx host path can use retained FIFO code */

	/* Chip information (for logging / decisions). */
	uint8_t wab_id;		/* raw 0FAAh register 00h readout */
	uint8_t crt27;		/* Cirrus chip ID register */
	bool alpine;		/* GD5430/5440-or-later register semantics */

	/* LCD panel (GD754x laptops). */
	bool lcd;
	int lcd_w;
	int lcd_h;
} cdisp;

/*
 * Requested GD54xx host path.  Direct aperture is the default; the proven
 * CPU-source FIFO path remains selectable for diagnosis and fallback.
 */
static bool gd54_fifo_requested;

/* Set before native GD7548 register programming. */
static bool pci_nb10_active;

/*
 * Nb10 FAA/FAB indexed register 03h experiment.
 *
 * Environment variable T selects exactly one condition:
 *
 *   T=0  never read or write FAA/FAB reg03, including cleanup.
 *        This is the strict NT4 family-40h/model-0Eh condition.
 *   T=1  write reg03=01h at the historical early point.
 *        bit0 only; cleanup writes 00h and therefore creates bit0 falling edge.
 *   T=2  write reg03=02h at the historical early point.
 *        bit1 only; bit0 is never raised, so cleanup has no bit0 falling edge.
 *   T=3  write reg03=03h at the historical early point.
 *        Current known-output baseline; cleanup creates bit0 falling edge.
 *
 * Port 0FACh is a separate, canonical Nb10 relay and is not part of this
 * experiment.  FAC=02h on enter and FAC=00h on leave remain enabled for
 * every T value.
 */
static int nb10_reg03_test;
static bool nb10_reg03_touched;

/*
 * Nb10 SR12 bit6 experiment, selected with environment variable S.
 *
 *   S=0  normal operation.
 *   S=1  after the first completed game-frame FIFO BLT, allow four GD7548
 *        frames, then set SR12 bit6 while the initial pixel-perfect state
 *        should still be active ("freeze-before").
 *   S=2  after the first completed game-frame FIFO BLT, wait for the user to
 *        observe the stable 1+1 attractor and press Enter, then set SR12 bit6
 *        ("freeze-after").
 *
 * In S=1 and S=2, no further frame BLTs occur while the experiment prompts
 * are active.  The program first closes the gate, waits for Enter, reopens it,
 * then waits for a second Enter so the restart transition can be observed.
 */
static int nb10_sr12_test;
static bool nb10_sr12_test_done;

/*
 * Nb10 gate-order experiment selected by environment variable C.
 *
 *   C=0  Current baseline:
 *        pattern/clear -> SR12 open -> FACh=02h -> AC enable.
 *
 *   C=1  Stream-first, output hidden:
 *        preload a static color-bar pattern;
 *        SR12 open while FACh remains 00h and AC remains disabled;
 *        wait at least three seconds with no CPU-source BLT;
 *        then select FACh=02h and enable AC.
 *
 *   C=2  Output-first, stream hidden:
 *        preload the same static pattern;
 *        close SR12, select FACh=02h and enable AC;
 *        wait at least three seconds with no CPU-source BLT;
 *        then reopen SR12.
 *
 * C=1 and C=2 pause once before the final edge and once afterwards.  The
 * second pause occurs before the game is allowed to submit another BLT, so
 * the first visible transition belongs to the gate order, not frame drawing.
 */
static int nb10_gate_order_test;

/*
 * Nb10 FACh/AC split experiment selected by environment variable D.
 *
 * The C experiment showed that reopening SR12 alone can re-trigger the
 * X calibration.  D separates the two remaining final output edges - the
 * 0FACh relay and the VGA attribute-controller enable - to determine which
 * of them starts the calibration FSM.
 *
 *   D=0  Current baseline: after SR12 open, FACh=02h and AC enable are
 *        issued back to back (identical to C=0).
 *
 *   D=1  FACh first, AC later:
 *        preload the static color-bar pattern;
 *        SR12 open, FACh=02h, AC left disabled;
 *        wait at least three seconds with no CPU-source BLT;
 *        key -> AC enable.
 *
 *   D=2  AC first, FACh later:
 *        preload the same static pattern;
 *        SR12 open, FACh left at its idle value (not written), AC enabled;
 *        wait at least three seconds with no CPU-source BLT;
 *        key -> FACh=02h.
 *
 * Interpretation:
 *
 *   D=1 image already in the attractor at the AC edge
 *        -> calibration progressed during the FACh-only dwell
 *        -> FACh is the trigger.
 *   D=2 image already in the attractor at the FACh edge
 *        -> calibration progressed during the AC-only dwell
 *        -> AC enable is the trigger.
 *   Either case starting pixel-perfect and only then calibrating
 *        -> calibration begins only once both gates are up.
 *
 * D=1/2 requires C=0 and S=0; a nonzero D overrides both so exactly one
 * experiment manipulates the final gates.  During the D=2 dwell the FACh
 * value is read and logged but deliberately never written: an idempotent
 * 00h write would itself be an extra edge on the port under test.
 */
static int nb10_fach_ac_test;

/*
 * Nb10 gate-cycle experiment selected by environment variable E.
 *
 * The D experiment established that the X calibration runs whenever
 * SR12 is open AND at least one of the two output gates (FACh relay,
 * AC enable) is up; with both gates down the calibration does not
 * progress.  E asks whether the both-down state merely PAUSES the
 * calibration FSM or RESETS the (mis)trained gear lock.
 *
 * All variants start normally (baseline enter) and let the game run.
 * The experiment arms after the first frame (draining stale keystrokes)
 * and triggers on ANY key pressed during gameplay: press it once the
 * attractor is stable on real game content.  The frame on screen at the
 * keypress stays put - no further CPU-source BLT runs until the final
 * prompt is acknowledged.  (v300.8 blocked at the first game frame,
 * which is typically black and therefore unobservable.)
 *
 *   E=0  normal operation.
 *   E=1  close BOTH gates (AC disable, FACh=00h), dwell, reopen both.
 *   E=2  cycle FACh alone (00h then 02h); AC stays enabled.
 *   E=3  cycle AC alone (disable then enable); FACh stays 02h.
 *
 * Interpretation on reopen:
 *
 *   immediately in the 1+1 attractor
 *        -> the trained gear survives the closed dwell (pause only).
 *   pixel-perfect followed by the horizontal-noise transition
 *        -> the both-down state resets the trained gear.
 *
 * Under the OR model E=2/3 keep one gate up, so calibration continues
 * during their dwell and reopen should show the attractor immediately;
 * they serve as controls for E=1.
 *
 * These FACh writes happen after nb10_nec_lock(), unlike enter/leave.
 * Whether the NEC lock gates port 0FACh is unknown, so every write is
 * followed by a logged readback; a mismatch means the lock blocks FACh
 * and the experiment must be reissued with an unlock bracket.
 */
static int nb10_gate_cycle_test;
static bool nb10_gate_cycle_done;
static bool nb10_gate_cycle_armed;

/*
 * Nb10 NT4 initialization-order experiment selected by environment variable F.
 *
 * Static analysis of the matching NT4 cirrus.dll/CIRRUS.SYS pair showed that
 * the first surface enable performs TWO successful full SET_CURRENT_MODE
 * operations.  The second SET starts immediately after the first one has
 * finished, with this inherited state:
 *
 *   SR12 open, FACh=02h, AC enabled, NEC registers locked.
 *
 * Its prelude unlocks NEC and disables AC, but deliberately leaves FACh at
 * 02h.  The GD7548 stream is then synchronously reset/restarted by the same
 * mode stream (SR00=01h -> 03h) while the NEC receiver is exposed through
 * FACh.  This ordering has not existed in the DOS path before v300.10.
 *
 *   F=0  Existing single successful mode-set baseline.
 *   F=1  After the baseline pass, immediately execute the complete NT4-style
 *        second pass while carrying FACh=02h across the stream reset.  The
 *        second pass never reads or writes FAA/FAB reg03 and contains no
 *        user wait between passes.
 *
 * F=1 overrides S/C/D/E so the initialization-order change is the only
 * active experiment.  T remains independent: T=2 tests the new ordering on
 * the established DOS visibility baseline; T=0 additionally tests the strict
 * NT4 reg03-no-touch condition.
 */
static int nb10_nt4_init_order_test;

/*
 * Nb10 successful-SET checkpoint experiment selected by G and W.
 *
 * The NT4 DLL issues three SET_CURRENT_MODE IOCTL requests during first
 * enable, but the first request (mode | 40000000h) is rejected by the
 * miniport before hardware access.  There are therefore only TWO successful
 * hardware mode sets:
 *
 *   IOCTL request #2 = successful hardware SET #1
 *   IOCTL request #3 = successful hardware SET #2 (after memory mapping)
 *
 * G selects which successful SET is allowed to evolve on an all-black
 * framebuffer before a single static diagnostic pattern reveals the current
 * NEC/Cirrus phase state:
 *
 *   G=0  no checkpoint.
 *   G=1  stop after successful hardware SET #1; do not execute SET #2.
 *   G=2  execute the NT4-style SET #2, then stop after it.
 *
 * W is the automatic black dwell in seconds (0..60, default 10).  During the
 * dwell SR12 is open, FACh=02h and AC is enabled exactly as at the end of the
 * selected SET.  The screen is black because VRAM was cleared, not because a
 * gate is changed.  No CPU-source BLT or display-register polling occurs.
 * After W seconds one static FIFO pattern is written, then all further BLTs
 * are blocked at a key prompt so both the immediate state and any autonomous
 * later transition can be observed.
 *
 * G overrides F to select the requested successful SET and overrides S/C/D/E
 * to keep the checkpoint as the only experiment.  T remains independent.
 */
static int nb10_modeset_checkpoint_test;
static int nb10_modeset_black_dwell_seconds;

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
static void cl_seq_write8(int reg, int val);
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
static void cl_modeset_coregraph_necdrv(void);
static int cl_resolve_bpp(int req, int cap, int w, int h,
			  uint32_t vram, const char *tag);

/* Misc. */
static void nb10_gate_cycle_poll(void);
static void *cl_map_physical(uint32_t phys, uint32_t size);
static bool cl_unmap_physical(void *linear);
static void cl_release_fb_mapping(void);
static bool cl_aperture_clear_visible(void);
static bool cl_blt_fifo_clear_visible(void);
static bool cl_blt_fifo_pattern_visible(void);
static void cirrus_flip_fifo(void);

/* Fixed 0FAA/0FAB GD54xx interface (Core-Graph or banked WAB). */
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
	const char *force, *host_env, *yoff_env;
	char *endp;
	long yoff_value;
	int max_y;
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
	hal_log_info("CIRRUS-BUILD: v300.11 Nb10 SET-stage black-dwell (G/W) experiment.");

	gd54_fifo_requested = false;
	host_env = getenv("STRATO_CIRRUS_HOST");
	if (host_env != NULL) {
		if (strcmp(host_env, "fifo") == 0)
			gd54_fifo_requested = true;
		else if (strcmp(host_env, "aperture") != 0)
			hal_log_info("CIRRUS: unknown STRATO_CIRRUS_HOST=%s; "
			             "using aperture.", host_env);
	}
	hal_log_info("CIRRUS: requested GD54xx host path: %s.",
	             gd54_fifo_requested ? "CPU-source BLT FIFO" :
	                                     "direct VRAM aperture");

	force = getenv("STRATO_CIRRUS_FORCE");
	if (force != NULL)
		hal_log_info("CIRRUS: STRATO_CIRRUS_FORCE=%s.", force);

	/*
	 * Automatic probing is PCI first.  Some PCI-on-board machines
	 * (notably the V13 family) return a WAB-looking ID such as 5Bh
	 * even though no WAB exists.  Touching the WAB path first would
	 * therefore select a bogus banked aperture and may corrupt normal
	 * memory.  Explicit FORCE=54/75 keeps the requested single path.
	 */
	ok = false;
	if (force != NULL && strcmp(force, "54") == 0) {
		hal_log_info("CIRRUS: probe order: forced fixed 0FAA/0FAB GD54xx path only.");
		ok = cirrus54_init(mode, bpp);
	} else if (force != NULL && strcmp(force, "75") == 0) {
		hal_log_info("CIRRUS: probe order: forced PCI path only.");
		ok = cirrus75_init(mode, bpp);
	} else {
		hal_log_info("CIRRUS: probe order: PCI first, then fixed 0FAA/0FAB interface.");
		ok = cirrus75_init(mode, bpp);
		if (!ok)
			ok = cirrus54_init(mode, bpp);
	}
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

	/* Optional vertical placement override for 640x360-on-640x480 games. */
	yoff_env = getenv("STRATO_CIRRUS_YOFF");
	max_y = cdisp.scr_h - draw_h;
	if (max_y < 0)
		max_y = 0;
	if (yoff_env != NULL) {
		endp = NULL;
		yoff_value = strtol(yoff_env, &endp, 0);
		if (endp != yoff_env && *endp == '\0' &&
		    yoff_value >= 0 && yoff_value <= max_y) {
			ofs_y = (int)yoff_value;
			hal_log_info("CIRRUS: STRATO_CIRRUS_YOFF=%d applied.", ofs_y);
		} else {
			hal_log_info("CIRRUS: invalid STRATO_CIRRUS_YOFF=%s "
			             "(valid range 0..%d); using centered y=%d.",
			             yoff_env, max_y, ofs_y);
		}
	}
	hal_log_info("CIRRUS: viewport : y=%d..%d; top/bottom borders %d/%d.",
	             ofs_y, ofs_y + draw_h - 1, ofs_y,
	             cdisp.scr_h - (ofs_y + draw_h));

	hal_log_info("CIRRUS: === configuration summary ===");
	hal_log_info("CIRRUS: path     : %s.",
		     cdisp.path == CIRRUS_PATH_54_BANKED ?
		     "GD54xx fixed-interface / WAB" :
		     cdisp.path == CIRRUS_PATH_54_COREGRAPH ?
		     "NEC Core-Graph integrated GD54xx" :
		     "PCI (configuration space)");
	hal_log_info("CIRRUS: chip     : %s, CR27=%02Xh, fixed ID=%02Xh.",
		     cdisp.chip_name, cdisp.crt27, cdisp.wab_id);
	hal_log_info("CIRRUS: mode     : %dx%d, %d bpp, pitch %lu bytes.",
		     cdisp.scr_w, cdisp.scr_h, cdisp.bpp,
		     (unsigned long)cdisp.pitch);
	if (cdisp.fifo_only)
		hal_log_info("CIRRUS: host path: CPU-source BLT FIFO dword writes at %08lXh; VRAM is never read directly.",
		             (unsigned long)cdisp.fb_phys);
	else if (cdisp.linear)
		hal_log_info("CIRRUS: aperture : linear, %luKB at %08lXh.",
			     (unsigned long)(cdisp.vram_size >> 10),
			     (unsigned long)cdisp.fb_phys);
	else
		hal_log_info("CIRRUS: aperture : banked 32KB window at "
			     "%08lXh, 16KB granularity, %luKB VRAM.",
			     (unsigned long)cdisp.fb_phys,
			     (unsigned long)(cdisp.vram_size >> 10));
	if (cdisp.fifo_only)
		hal_log_info("CIRRUS: blitter  : CPU-source FIFO is the active host path.");
	else if (cdisp.fifo_capable)
		hal_log_info("CIRRUS: blitter  : idle; CPU-source FIFO code retained as fallback.");
	else
		hal_log_info("CIRRUS: blitter  : unused on this chip path.");
	hal_log_info("CIRRUS: blit     : game %dx%d -> +%d,+%d "
		     "(draw %dx%d).",
		     game_width, game_height, ofs_x, ofs_y, draw_w, draw_h);

	return true;
}

void
cirrus_cleanup_disp(void)
{
	switch (cdisp.path) {
	case CIRRUS_PATH_54_BANKED:
	case CIRRUS_PATH_54_COREGRAPH:
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

	if (cdisp.fifo_only) {
		cirrus_flip_fifo();
		return;
	}

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
	if (pci_nb10_active)
		outpw(cdisp.io_3c0 + 0x04,
		      (uint16_t)(((uint16_t)(val & 0xff) << 8) |
		                 (uint16_t)(reg & 0xff)));
	else {
		outp(cdisp.io_3c0 + 0x04, reg);
		outp(cdisp.io_3c0 + 0x05, val);
	}
}

static void
cl_seq_write8(int reg, int val)
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
	if (pci_nb10_active)
		outpw(cdisp.io_3c0 + 0x0e,
		      (uint16_t)(((uint16_t)(val & 0xff) << 8) |
		                 (uint16_t)(reg & 0xff)));
	else {
		outp(cdisp.io_3c0 + 0x0e, reg);
		outp(cdisp.io_3c0 + 0x0f, val);
	}
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
	if (pci_nb10_active)
		outpw(cdisp.io_3d4,
		      (uint16_t)(((uint16_t)(val & 0xff) << 8) |
		                 (uint16_t)(reg & 0xff)));
	else {
		outp(cdisp.io_3d4, reg);
		outp(cdisp.io_3d4 + 1, val);
	}
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
	/* Reset the hidden-DAC access counter deterministically. */
	(void)inp(cdisp.io_3c0 + 0x08);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	outp(cdisp.io_3c0 + 0x06, val);
}

static int
cl_hidden_dac_read(void)
{
	int val;

	(void)inp(cdisp.io_3c0 + 0x08);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	(void)inp(cdisp.io_3c0 + 0x06);
	val = inp(cdisp.io_3c0 + 0x06);
	/* GD7548 read does not reliably disarm the counter. */
	(void)inp(cdisp.io_3c0 + 0x08);
	return val;
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
 * NEC-driver "path 08h" mode set for the linear onboard GD54xx
 * family (machine IDs 58h-5Dh, including the V13 ID 5Bh).
 *
 * IMPORTANT FOR OTHER CORE-GRAPH PORTS:
 * This is only the vendor-chip half of mode setting.  The caller must first
 * execute coregraph_necdrv_gate_enter(), which configures the surrounding NEC
 * clock/MUX/VRAM-routing LSI.  S3, Trident or Matrox Core-Graph ports should
 * expect the same two-layer design, but must recover their own board sequence
 * and their own vendor register stream rather than copying these Cirrus values.
 *
 * These values are intentionally not folded into cl_modeset_generic().
 * The board wiring is different from both a classic banked WAB and a normally
 * enumerated PCI Alpine:
 *
 *   SR07  high nibble selects the Core-Graph memory wiring; low bits select
 *         packed-pixel depth.  V13 24bpp is 15h, not the PCI-Alpine A5h.
 *   SR0E/SR1E form VCLK3; SR1F must not override it from MCLK.
 *   SR17  enables the linear host path and places MMIO in its final 256 bytes.
 *   SR18  bit6 is cleared by the NEC postlude after the command stream.
 *   CR13/CR1B together encode the scanline offset; 24bpp is 2048 bytes.
 *   MISC  E3h selects the clock/sync and keeps the relocated color CRTC block.
 *   HDR   selects the RAMDAC packed-pixel interpretation (E5h for 24bpp).
 *
 * Only the three 640x480 streams have been transcribed so far.  The function
 * leaves the screen blanked; the caller resets BLT state, clears VRAM through
 * the selected host method and finally writes SR01=01h.
 */
static void
cl_modeset_coregraph_necdrv(void)
{
	static const uint8_t seq_idx[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x08,
		0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x16, 0x18,
		0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	static const uint8_t seq8[] = {
		0x01, 0x01, 0x0f, 0x00, 0x0e, 0x11, 0x00,
		0x66, 0x48, 0x56, 0x60, 0x30, 0x58, 0x40,
		0x3b, 0x23, 0x3d, 0x3b, 0x20
	};
	static const uint8_t seq16[] = {
		0x01, 0x01, 0x0f, 0x00, 0x0e, 0x13, 0x00,
		0x6d, 0x48, 0x56, 0x60, 0x30, 0x58, 0x40,
		0x3e, 0x23, 0x3d, 0x3b, 0x20
	};
	static const uint8_t seq24[] = {
		0x01, 0x01, 0x0f, 0x00, 0x0e, 0x15, 0x00,
		0x3a, 0x48, 0x56, 0x60, 0x30, 0x58, 0x40,
		0x16, 0x23, 0x3d, 0x3b, 0x20
	};
	static const uint8_t crtc8[0x1c] = {
		0x5f,0x4f,0x50,0x84,0x54,0x80,0x0b,0x3e,
		0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
		0xe5,0x87,0xdf,0x50,0x00,0xe7,0x04,0xe3,
		0xff,0x00,0x90,0x22
	};
	static const uint8_t crtc16[0x1c] = {
		0x5f,0x4f,0x50,0x84,0x53,0x9f,0x0b,0x3e,
		0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
		0xe5,0x87,0xdf,0xa0,0x00,0xe7,0x04,0xe3,
		0xff,0x00,0x90,0x22
	};
	static const uint8_t crtc24[0x1c] = {
		0x5f,0x4f,0x50,0x84,0x53,0x9f,0x0b,0x3e,
		0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
		0xe5,0x87,0xdf,0x00,0x00,0xe7,0x04,0xe3,
		0xff,0x00,0x90,0x32
	};
	static const uint8_t gfx[9] = {
		0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff
	};
	static const uint8_t attr[21] = {
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
		0x41,0x00,0x0f,0x00,0x00
	};
	const uint8_t *seq, *crtc;
	int hdr, i;

	if (cdisp.bpp == 8) {
		seq = seq8;
		crtc = crtc8;
		hdr = 0x20;
	} else if (cdisp.bpp == 16) {
		seq = seq16;
		crtc = crtc16;
		hdr = 0xe1;
	} else {
		seq = seq24;
		crtc = crtc24;
		hdr = 0xe5;
	}

	/*
	 * A previous driver or interrupted BLT may have left the engine waiting
	 * for CPU-source data.  In that state, ordinary linear-aperture writes
	 * are consumed by the BLT FIFO instead of reaching VRAM.  Log the stale
	 * state, then reset exactly as the Cirrus reset edge requires (04h->00h).
	 */
	hal_log_info("CIRRUS-CORE: pre-mode BLT: GR30=%02Xh GR31=%02Xh "
	             "GR32=%02Xh GR33=%02Xh; SR18=%02Xh.",
	             cl_gfx_read(0x30), cl_gfx_read(0x31),
	             cl_gfx_read(0x32), cl_gfx_read(0x33),
	             cl_seq_read(0x18));
	cl_gfx_write(0x33, 0x00);
	cl_gfx_write(0x31, 0x04);
	cl_gfx_write(0x31, 0x00);

	/* The stream starts by unlocking extensions and disabling the cursor. */
	cl_seq_write(0x06, 0x12);
	cl_seq_write(0x12, 0x00);
	for (i = 0; i < (int)sizeof(seq_idx); i++)
		cl_seq_write(seq_idx[i], seq[i]);

	/* Preserve board-specific DRAM bits exactly as NEC's interpreter does. */
	cl_seq_write8(0x0f, (cl_seq_read(0x0f) & 0xdf) | 0x20);

	/* E3h keeps the relocated CRTC on the color block (0DA4h). */
	cl_misc_write(0xe3);
	cl_gfx_write(0x06, 0x05);
	cl_seq_write(0x00, 0x03);

	/* Unlock CR0-7, then replay CR00-CR1B verbatim. */
	cl_crtc_write(0x11, 0x20);
	for (i = 0; i < 0x1c; i++)
		cl_crtc_write(i, crtc[i]);

	for (i = 0; i < 9; i++)
		cl_gfx_write(i, gfx[i]);

	/* Attribute stream: one flip-flop reset, then index/data pairs. */
	(void)inp(cdisp.io_3da);
	for (i = 0; i < 21; i++) {
		outp(cdisp.io_3c0, i);
		outp(cdisp.io_3c0, attr[i]);
	}
	(void)inp(cdisp.io_3da);
	outp(cdisp.io_3c0, 0x20);

	cl_hidden_dac_write(hdr);
	outp(cdisp.io_3c0 + 0x06, 0xff);

	/* Linear aperture, 16KB bank units, no active bank offset. */
	cl_gfx_write(0x09, 0x00);
	cl_gfx_write(0x0a, 0x00);
	cl_gfx_write(0x0b, 0x21);
	cdisp.cur_bank = 0;

	/* Path 8 is a linear machine: MMIO occupies the final 256 bytes. */
	cl_seq_write(0x17, (uint8_t)(cl_seq_read(0x17) | 0x44));

	/*
	 * Exact NEC-driver postlude for chip tag 07h (V13 ID 5Bh): after the mode
	 * command stream and SR17 update, CIRRUS.SYS clears SR18 bit6.
	 * The previous diagnostic transcription missed this operation and left
	 * the Signature Generator Control register at 40h.
	 */
	cl_seq_write(0x18, (uint8_t)(cl_seq_read(0x18) & 0xbf));

	/* Reset once more after the full stream, before any aperture write. */
	cl_gfx_write(0x31, 0x04);
	cl_gfx_write(0x31, 0x00);

	cl_load_palette();

	cdisp.crt27 = (uint8_t)cl_crtc_read(0x27);
	cdisp.alpine = (cdisp.crt27 >= 0xa0);

	/* Keep scanout blank until the framebuffer has been cleared. */
	cl_seq_write(0x01, 0x21);

	hal_log_info("CIRRUS-CORE: NEC-driver path-08h mode: SR07=%02Xh "
		     "SR0E=%02Xh SR1E=%02Xh SR17=%02Xh SR18=%02Xh "
		     "MISC=%02Xh CR13=%02Xh CR1B=%02Xh GR0B=%02Xh "
		     "GR31=%02Xh HDR=%02Xh.",
		     cl_seq_read(0x07), cl_seq_read(0x0e),
		     cl_seq_read(0x1e), cl_seq_read(0x17),
		     cl_seq_read(0x18), cl_misc_read(),
		     cl_crtc_read(0x13), cl_crtc_read(0x1b),
		     cl_gfx_read(0x0b), cl_gfx_read(0x31),
		     cl_hidden_dac_read());
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

/* DPMI 0x0801: release a mapping returned by function 0x0800. */
static bool
cl_unmap_physical(void *linear)
{
	union REGS r;
	uint32_t addr = (uint32_t)linear;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0801;
	r.w.bx = (uint16_t)(addr >> 16);
	r.w.cx = (uint16_t)(addr & 0xffff);
	int386(0x31, &r, &r);
	return !r.w.cflag;
}

static void
cl_release_fb_mapping(void)
{
	if (cdisp.fb == NULL)
		return;
	if (cdisp.fb_phys >= 0x00100000UL &&
	    !cl_unmap_physical(cdisp.fb))
		hal_log_info("CIRRUS: warning: DPMI could not release host window mapping at %08lXh.",
		             (unsigned long)cdisp.fb_phys);
	cdisp.fb = NULL;
}

/*
 * Clear only the scanout-visible portion through the normal VRAM aperture.
 *
 * On linear machines SR17 may place MMIO in the final 256 bytes of the 1MB
 * aperture.  The supported visible modes fit below that region, and the
 * bounds check below prevents a future mode from accidentally memset()ing
 * BLT registers.  A banked row is at most 2048 bytes; with 16KB bank units
 * and a 32KB window one bank selection per row remains sufficient.
 */
static bool
cl_aperture_clear_visible(void)
{
	uint32_t visible, limit;
	int y;

	if (cdisp.fb == NULL)
		return false;

	visible = cdisp.pitch * (uint32_t)cdisp.scr_h;
	limit = cdisp.vram_size;
	if (cdisp.linear && (cl_seq_read(0x17) & 0x44) == 0x44 &&
	    limit >= 0x100)
		limit -= 0x100;
	if (visible > limit) {
		hal_log_info("CIRRUS: visible aperture clear %lu bytes exceeds "
		             "safe host range %lu bytes.",
		             (unsigned long)visible, (unsigned long)limit);
		return false;
	}

	if (cdisp.linear) {
		memset(cdisp.fb, 0, visible);
	} else {
		for (y = 0; y < cdisp.scr_h; y++) {
			uint32_t off = (uint32_t)y * cdisp.pitch;
			cl_set_bank((int)(off >> CL_BANK_SHIFT));
			memset(cdisp.fb + (off & CL_BANK_MASK), 0, cdisp.pitch);
		}
		cl_set_bank(0);
	}
	return true;
}

/* Cirrus BitBLT registers retained for GD54xx FIFO diagnosis/fallback. */
#define CL_BLT_MODE_MEMSYS_SRC   0x04
#define CL_BLT_MODE_PIX8         0x00
#define CL_BLT_MODE_PIX16        0x10
#define CL_BLT_MODE_PIX24        0x20
#define CL_BLT_STATUS_BUSY       0x01
#define CL_BLT_STATUS_START      0x02
#define CL_BLT_STATUS_RESET      0x04
#define CL_BLT_ROP_SRC           0x0d

static void
cl_blt_write16(int lo_reg, uint32_t value)
{
	cl_gfx_write(lo_reg, (int)(value & 0xff));
	cl_gfx_write(lo_reg + 1, (int)((value >> 8) & 0xff));
}

static void
cl_blt_write24(int lo_reg, uint32_t value)
{
	cl_gfx_write(lo_reg, (int)(value & 0xff));
	cl_gfx_write(lo_reg + 1, (int)((value >> 8) & 0xff));
	cl_gfx_write(lo_reg + 2, (int)((value >> 16) & 0x3f));
}

static bool
cl_blt_wait_idle(unsigned long limit, const char *where)
{
	unsigned long i;
	int status;

	for (i = 0; i < limit; i++) {
		status = cl_gfx_read(0x31);
		if ((status & CL_BLT_STATUS_BUSY) == 0)
			return true;
	}
	hal_log_info("CIRRUS-BLT: timeout %s; GR31=%02Xh.",
	             where, cl_gfx_read(0x31));
	return false;
}

static void
cl_blt_reset(void)
{
	cl_gfx_write(0x31, CL_BLT_STATUS_RESET);
	cl_gfx_write(0x31, 0x00);
}

static int
cl_blt_pixel_mode(void)
{
	switch (cdisp.bpp) {
	case 24:
		return CL_BLT_MODE_PIX24;
	case 16:
		return CL_BLT_MODE_PIX16;
	default:
		return CL_BLT_MODE_PIX8;
	}
}

/*
 * Start a host-to-video BitBLT.  width_bytes is the number of source bytes
 * consumed per scanline, not the destination pitch.  The source pitch is
 * programmed to the same value; all supported row widths are dword-aligned.
 */
static bool
cl_blt_fifo_start(uint32_t dst, uint32_t width_bytes, uint32_t height)
{
	int status;

	if (cdisp.fb == NULL || width_bytes == 0 || height == 0)
		return false;
	if (!cl_blt_wait_idle(2000000UL, "before CPU-source start"))
		return false;

	cl_blt_reset();
	cl_blt_write16(0x20, width_bytes - 1);       /* width in bytes minus one */
	cl_blt_write16(0x22, height - 1);            /* height minus one */
	cl_blt_write16(0x24, cdisp.pitch);           /* destination pitch */
	cl_blt_write16(0x26, width_bytes);           /* source pitch */
	cl_blt_write24(0x28, dst);                   /* destination VRAM address */
	cl_blt_write24(0x2c, 0x000000UL);            /* unused for MEMSYSSRC */
	cl_gfx_write(0x2f, 0x00);                    /* no left-edge skip */
	/* NP21/W/QEMU also uses GR30[5:4] as the BLT pixel width. */
	cl_gfx_write(0x30, pci_nb10_active ? CL_BLT_MODE_MEMSYS_SRC :
	             (CL_BLT_MODE_MEMSYS_SRC | cl_blt_pixel_mode()));
	cl_gfx_write(0x32, CL_BLT_ROP_SRC);          /* source copy */
	cl_gfx_write(0x33, 0x00);
	cl_gfx_write(0x31, CL_BLT_STATUS_START);

	/* START is asynchronous on the real GD7548: wait for BUSY to rise. */
	{
		unsigned long t;

		status = 0;
		for (t = 0; t < 65536UL; t++) {
			status = cl_gfx_read(0x31);
			if (status & CL_BLT_STATUS_BUSY)
				return true;
		}
	}
	hal_log_info("CIRRUS-BLT: engine did not enter BUSY; "
	             "GR30=%02Xh GR31=%02Xh.",
	             cl_gfx_read(0x30), status);
	cl_blt_reset();
	return false;
}

/*
 * Push one scanline into the active CPU-source FIFO.  Cirrus requires host
 * source data to be supplied as dword writes.  While MEMSYSSRC is active and
 * GR31.PAUSE is clear, the chip ignores the memory address and consumes the
 * data cycle, so repeatedly writing the first dword of either the PCI BAR or
 * the fixed reg01 window is intentional.
 *
 * The final partial dword is zero-padded.  For non-color-expanded transfers
 * the programmed byte width makes the chip ignore up to three padding bytes
 * at the end of each scanline.
 */
static void
cl_blt_fifo_feed_row(const uint8_t *src, uint32_t count)
{
	volatile uint32_t *fifo = (volatile uint32_t *)cdisp.fb;
	uint32_t value;

	while (count >= 4) {
		value = (uint32_t)src[0] |
		        ((uint32_t)src[1] << 8) |
		        ((uint32_t)src[2] << 16) |
		        ((uint32_t)src[3] << 24);
		fifo[0] = value;
		src += 4;
		count -= 4;
	}
	if (count != 0) {
		value = (uint32_t)src[0];
		if (count > 1)
			value |= (uint32_t)src[1] << 8;
		if (count > 2)
			value |= (uint32_t)src[2] << 16;
		fifo[0] = value;
	}
}

/* Clear the visible display through exactly the same FIFO used for frames. */
static bool
cl_blt_fifo_clear_visible(void)
{
	static uint32_t zeros32[512];       /* 2048 zero bytes */
	const uint8_t *zeros = (const uint8_t *)zeros32;
	uint32_t row_bytes;
	int y;

	row_bytes = (uint32_t)cdisp.scr_w * (uint32_t)(cdisp.bpp / 8);
	if (row_bytes > sizeof(zeros32)) {
		hal_log_info("CIRRUS-BLT: clear row %lu exceeds FIFO staging buffer.",
		             (unsigned long)row_bytes);
		return false;
	}
	if (!cl_blt_fifo_start(0, row_bytes, (uint32_t)cdisp.scr_h))
		return false;

	for (y = 0; y < cdisp.scr_h; y++)
		cl_blt_fifo_feed_row(zeros, row_bytes);

	if (!cl_blt_wait_idle(4000000UL, "after FIFO clear")) {
		cl_blt_reset();
		return false;
	}
	return true;
}


/*
 * Fill the full visible screen with eight vertical color bars through the
 * CPU-source FIFO.  This is a self-contained hardware diagnostic: it does
 * not use back_image and never reads VRAM.
 */
static bool
cl_blt_fifo_pattern_visible(void)
{
	static uint32_t row32[512];       /* 2048-byte aligned staging row */
	uint8_t *row = (uint8_t *)row32;
	static const uint8_t rgb[8][3] = {
		{255,255,255}, {255,255,  0}, {  0,255,255}, {  0,255,  0},
		{255,  0,255}, {255,  0,  0}, {  0,  0,255}, {  0,  0,  0}
	};
	uint32_t row_bytes;
	int x, y, bar;

	row_bytes = (uint32_t)cdisp.scr_w * (uint32_t)(cdisp.bpp / 8);
	if (row_bytes > sizeof(row32)) {
		hal_log_info("CIRRUS-BLT: pattern row %lu exceeds FIFO staging buffer.",
		             (unsigned long)row_bytes);
		return false;
	}
	if (!cl_blt_fifo_start(0, row_bytes, (uint32_t)cdisp.scr_h))
		return false;

	for (y = 0; y < cdisp.scr_h; y++) {
		for (x = 0; x < cdisp.scr_w; x++) {
			uint8_t r, g, b;
			bar = (x * 8) / cdisp.scr_w;
			if (bar > 7)
				bar = 7;
			r = rgb[bar][0];
			g = rgb[bar][1];
			b = rgb[bar][2];

			/* Alternate brightness every 16 lines to expose pitch errors. */
			if (y & 0x10) {
				r >>= 1;
				g >>= 1;
				b >>= 1;
			}

			if (cdisp.bpp == 24) {
				row[x * 3 + 0] = b;
				row[x * 3 + 1] = g;
				row[x * 3 + 2] = r;
			} else if (cdisp.bpp == 16) {
				uint16_t p = (uint16_t)(((uint16_t)(r & 0xf8) << 8) |
				                        ((uint16_t)(g & 0xfc) << 3) |
				                        ((uint16_t)b >> 3));
				row[x * 2 + 0] = (uint8_t)p;
				row[x * 2 + 1] = (uint8_t)(p >> 8);
			} else {
				row[x] = (uint8_t)((r & 0xe0) |
				                   ((g >> 3) & 0x1c) |
				                   ((b >> 6) & 0x03));
			}
		}
		cl_blt_fifo_feed_row(row, row_bytes);
	}

	if (!cl_blt_wait_idle(4000000UL, "after FIFO pattern")) {
		cl_blt_reset();
		return false;
	}
	return true;
}


/*
 * Wait on the GD7548 Input Status 1 vertical-retrace bit.
 *
 * This is used only to place the S=1 gate edge a few complete GD7548 frames
 * after the first BLT.  It does not claim that this status bit describes the
 * final NEC-side LCD/CRT blanking interval.
 */
static bool
nb10_wait_status1(int want_set, unsigned long limit)
{
	unsigned long i;

	for (i = 0; i < limit; i++) {
		int set = (inp(cdisp.io_3da) & 0x08) != 0;
		if (set == want_set)
			return true;
	}
	return false;
}

static bool
nb10_wait_complete_frames(int frames)
{
	int i;

	for (i = 0; i < frames; i++) {
		/* Finish any current retrace, then observe one complete next retrace. */
		if (!nb10_wait_status1(0, 1000000UL) ||
		    !nb10_wait_status1(1, 1000000UL) ||
		    !nb10_wait_status1(0, 1000000UL))
			return false;
	}
	return true;
}

static void
nb10_sr12_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_sr12_test = 0;
	nb10_sr12_test_done = false;

	env = getenv("S");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 2) {
			nb10_sr12_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid S=%s; using S=0.",
			             env);
		}
	}

	hal_log_info("CIRRUS-NB10: SR12 experiment S=%d: %s.",
	             nb10_sr12_test,
	             nb10_sr12_test == 0 ? "normal operation" :
	             nb10_sr12_test == 1 ?
	                 "freeze-before after four complete GD7548 frames" :
	                 "freeze-after on user key at the stable attractor");
}


static void
nb10_gate_order_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_gate_order_test = 0;

	env = getenv("C");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 2) {
			nb10_gate_order_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid C=%s; using C=0.",
			             env);
		}
	}

	/*
	 * S manipulates SR12 after the first game frame.  Combining it with C
	 * would add a second independent SR12 experiment and obscure the result.
	 */
	if (nb10_gate_order_test != 0 && nb10_sr12_test != 0) {
		hal_log_info("CIRRUS-NB10: C=%d requires S=0; overriding S=%d.",
		             nb10_gate_order_test, nb10_sr12_test);
		nb10_sr12_test = 0;
		nb10_sr12_test_done = false;
	}

	hal_log_info("CIRRUS-NB10: gate-order experiment C=%d: %s.",
	             nb10_gate_order_test,
	             nb10_gate_order_test == 0 ?
	                 "baseline SR12 -> FACh -> AC order" :
	             nb10_gate_order_test == 1 ?
	                 "SR12 stream first; FACh/AC hidden for manual 3-second dwell" :
	                 "FACh/AC first; SR12 closed for manual 3-second dwell");
}

static void
nb10_fach_ac_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_fach_ac_test = 0;

	env = getenv("D");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 2) {
			nb10_fach_ac_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid D=%s; using D=0.",
			             env);
		}
	}

	/*
	 * D splits the same final edges that C reorders and that S retests
	 * after the first frame.  Exactly one of them may be active.
	 */
	if (nb10_fach_ac_test != 0 && nb10_gate_order_test != 0) {
		hal_log_info("CIRRUS-NB10: D=%d requires C=0; overriding C=%d.",
		             nb10_fach_ac_test, nb10_gate_order_test);
		nb10_gate_order_test = 0;
	}
	if (nb10_fach_ac_test != 0 && nb10_sr12_test != 0) {
		hal_log_info("CIRRUS-NB10: D=%d requires S=0; overriding S=%d.",
		             nb10_fach_ac_test, nb10_sr12_test);
		nb10_sr12_test = 0;
		nb10_sr12_test_done = false;
	}

	hal_log_info("CIRRUS-NB10: FACh/AC split experiment D=%d: %s.",
	             nb10_fach_ac_test,
	             nb10_fach_ac_test == 0 ?
	                 "baseline back-to-back FACh + AC enable" :
	             nb10_fach_ac_test == 1 ?
	                 "FACh=02h first; AC held disabled for manual 3-second dwell" :
	                 "AC enabled first; FACh untouched for manual 3-second dwell");
}

static void
nb10_gate_cycle_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_gate_cycle_test = 0;
	nb10_gate_cycle_done = false;
	nb10_gate_cycle_armed = false;

	env = getenv("E");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 3) {
			nb10_gate_cycle_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid E=%s; using E=0.",
			             env);
		}
	}

	/* E cycles the same gates the other experiments manipulate. */
	if (nb10_gate_cycle_test != 0 && nb10_sr12_test != 0) {
		hal_log_info("CIRRUS-NB10: E=%d requires S=0; overriding S=%d.",
		             nb10_gate_cycle_test, nb10_sr12_test);
		nb10_sr12_test = 0;
		nb10_sr12_test_done = false;
	}
	if (nb10_gate_cycle_test != 0 && nb10_gate_order_test != 0) {
		hal_log_info("CIRRUS-NB10: E=%d requires C=0; overriding C=%d.",
		             nb10_gate_cycle_test, nb10_gate_order_test);
		nb10_gate_order_test = 0;
	}
	if (nb10_gate_cycle_test != 0 && nb10_fach_ac_test != 0) {
		hal_log_info("CIRRUS-NB10: E=%d requires D=0; overriding D=%d.",
		             nb10_gate_cycle_test, nb10_fach_ac_test);
		nb10_fach_ac_test = 0;
	}

	hal_log_info("CIRRUS-NB10: gate-cycle experiment E=%d: %s.",
	             nb10_gate_cycle_test,
	             nb10_gate_cycle_test == 0 ?
	                 "normal operation" :
	             nb10_gate_cycle_test == 1 ?
	                 "close both FACh and AC at the attractor, dwell, reopen" :
	             nb10_gate_cycle_test == 2 ?
	                 "cycle FACh alone at the attractor; AC stays enabled" :
	                 "cycle AC alone at the attractor; FACh stays 02h");
}

static void
nb10_nt4_init_order_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_nt4_init_order_test = 0;

	env = getenv("F");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 1) {
			nb10_nt4_init_order_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid F=%s; using F=0.",
			             env);
		}
	}

	/*
	 * F changes the cold initialization sequence itself.  Combining it with
	 * a later SR12/gate experiment would make the initial state ambiguous, so
	 * keep F=1 as a clean one-variable comparison.  T intentionally remains
	 * independent because T=0 versus T=2 is part of the NT4/DOS difference.
	 */
	if (nb10_nt4_init_order_test != 0 && nb10_sr12_test != 0) {
		hal_log_info("CIRRUS-NB10: F=%d requires S=0; overriding S=%d.",
		             nb10_nt4_init_order_test, nb10_sr12_test);
		nb10_sr12_test = 0;
		nb10_sr12_test_done = false;
	}
	if (nb10_nt4_init_order_test != 0 && nb10_gate_order_test != 0) {
		hal_log_info("CIRRUS-NB10: F=%d requires C=0; overriding C=%d.",
		             nb10_nt4_init_order_test, nb10_gate_order_test);
		nb10_gate_order_test = 0;
	}
	if (nb10_nt4_init_order_test != 0 && nb10_fach_ac_test != 0) {
		hal_log_info("CIRRUS-NB10: F=%d requires D=0; overriding D=%d.",
		             nb10_nt4_init_order_test, nb10_fach_ac_test);
		nb10_fach_ac_test = 0;
	}
	if (nb10_nt4_init_order_test != 0 && nb10_gate_cycle_test != 0) {
		hal_log_info("CIRRUS-NB10: F=%d requires E=0; overriding E=%d.",
		             nb10_nt4_init_order_test, nb10_gate_cycle_test);
		nb10_gate_cycle_test = 0;
		nb10_gate_cycle_done = false;
		nb10_gate_cycle_armed = false;
	}

	hal_log_info("CIRRUS-NB10: NT4 initialization-order experiment F=%d: %s.",
	             nb10_nt4_init_order_test,
	             nb10_nt4_init_order_test == 0 ?
	                 "single successful SET_CURRENT_MODE baseline" :
	                 "two immediate full mode sets; pass 2 carries FACh=02h across SR00 reset and does not touch reg03");
}

static void
nb10_modeset_checkpoint_configure(void)
{
	const char *env;
	char *endp;
	long value;

	nb10_modeset_checkpoint_test = 0;
	nb10_modeset_black_dwell_seconds = 10;

	env = getenv("G");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 2) {
			nb10_modeset_checkpoint_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid G=%s; using G=0.",
			             env);
		}
	}

	env = getenv("W");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 60) {
			nb10_modeset_black_dwell_seconds = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid W=%s; using W=10 seconds.",
			             env);
		}
	}

	if (nb10_modeset_checkpoint_test == 1 &&
	    nb10_nt4_init_order_test != 0) {
		hal_log_info("CIRRUS-NB10: G=1 observes hardware SET #1 only; "
		             "overriding F=%d to F=0.",
		             nb10_nt4_init_order_test);
		nb10_nt4_init_order_test = 0;
	} else if (nb10_modeset_checkpoint_test == 2 &&
	           nb10_nt4_init_order_test != 1) {
		hal_log_info("CIRRUS-NB10: G=2 requires the NT4-style hardware "
		             "SET #2; overriding F=%d to F=1.",
		             nb10_nt4_init_order_test);
		nb10_nt4_init_order_test = 1;
	}

	/* G owns initialization and the first visible diagnostic frame. */
	if (nb10_modeset_checkpoint_test != 0 && nb10_sr12_test != 0) {
		hal_log_info("CIRRUS-NB10: G=%d requires S=0; overriding S=%d.",
		             nb10_modeset_checkpoint_test, nb10_sr12_test);
		nb10_sr12_test = 0;
		nb10_sr12_test_done = false;
	}
	if (nb10_modeset_checkpoint_test != 0 && nb10_gate_order_test != 0) {
		hal_log_info("CIRRUS-NB10: G=%d requires C=0; overriding C=%d.",
		             nb10_modeset_checkpoint_test, nb10_gate_order_test);
		nb10_gate_order_test = 0;
	}
	if (nb10_modeset_checkpoint_test != 0 && nb10_fach_ac_test != 0) {
		hal_log_info("CIRRUS-NB10: G=%d requires D=0; overriding D=%d.",
		             nb10_modeset_checkpoint_test, nb10_fach_ac_test);
		nb10_fach_ac_test = 0;
	}
	if (nb10_modeset_checkpoint_test != 0 && nb10_gate_cycle_test != 0) {
		hal_log_info("CIRRUS-NB10: G=%d requires E=0; overriding E=%d.",
		             nb10_modeset_checkpoint_test, nb10_gate_cycle_test);
		nb10_gate_cycle_test = 0;
		nb10_gate_cycle_done = false;
		nb10_gate_cycle_armed = false;
	}

	hal_log_info("CIRRUS-NB10: successful-SET checkpoint G=%d, W=%d sec: %s.",
	             nb10_modeset_checkpoint_test,
	             nb10_modeset_black_dwell_seconds,
	             nb10_modeset_checkpoint_test == 0 ?
	                 "disabled" :
	             nb10_modeset_checkpoint_test == 1 ?
	                 "black dwell after IOCTL request #2 / hardware SET #1" :
	                 "black dwell after IOCTL request #3 / hardware SET #2");
}

static void
nb10_wait_key(const char *message)
{
	/* Discard stale keystrokes so one Enter advances exactly one stage. */
	while (kbhit())
		(void)getch();

	hal_log_info("%s", message);
	printf("%s\n", message);
	fflush(stdout);
	(void)getch();
}

static void
nb10_sr12_close(void)
{
	uint8_t before, after;

	before = (uint8_t)cl_seq_read(0x12);
	cl_seq_write8(0x12, before | 0x40);
	after = (uint8_t)cl_seq_read(0x12);
	hal_log_info("CIRRUS-NB10: SR12 gate closed: %02Xh -> %02Xh.",
	             before, after);
}

static void
nb10_sr12_open(void)
{
	uint8_t before, after;

	before = (uint8_t)cl_seq_read(0x12);
	cl_seq_write8(0x12, before & 0xbf);
	after = (uint8_t)cl_seq_read(0x12);
	hal_log_info("CIRRUS-NB10: SR12 gate reopened: %02Xh -> %02Xh.",
	             before, after);
}

/*
 * Run once, immediately after the first game-frame FIFO BLT has completed.
 * Blocking here deliberately prevents all subsequent CPU-source BLTs while
 * the static image and SR12 gate behavior are observed.
 */
static void
nb10_sr12_after_first_frame(void)
{
	if (!pci_nb10_active || nb10_sr12_test == 0 ||
	    nb10_sr12_test_done)
		return;

	nb10_sr12_test_done = true;

	if (nb10_sr12_test == 1) {
		if (!nb10_wait_complete_frames(4))
			hal_log_info("CIRRUS-NB10: S=1 vertical-status wait "
			             "timed out; closing SR12 anyway.");
	} else {
		nb10_wait_key(
		    "CIRRUS-NB10 S=2: wait for the stable 1+1 attractor, "
		    "then press Enter to close SR12 bit6.");
	}

	nb10_sr12_close();
	nb10_wait_key(
	    "CIRRUS-NB10: SR12 is closed. Observe whether the current "
	    "image remains pixel-perfect/1+1, changes, or goes black; "
	    "then press Enter to reopen.");
	nb10_sr12_open();
	nb10_wait_key(
	    "CIRRUS-NB10: SR12 is open again. Observe whether restart is "
	    "immediately pixel-perfect, immediately 1+1, or repeats the "
	    "horizontal-noise calibration sequence; press Enter to continue.");
}

/* Present one game frame using CPU-source BitBLT only. */
static void
cirrus_flip_fifo(void)
{
	uint32_t row32[512];        /* 2048-byte naturally aligned staging row */
	uint8_t *row = (uint8_t *)row32;
	const uint32_t *pixels;
	uint32_t row_bytes, dst;
	int y, bytespp;

	if (back_image == NULL || back_image->pixels == NULL ||
	    draw_w <= 0 || draw_h <= 0)
		return;

	pixels = back_image->pixels;
	bytespp = cdisp.bpp / 8;
	row_bytes = (uint32_t)draw_w * (uint32_t)bytespp;
	if (row_bytes > sizeof(row32)) {
		hal_log_info("CIRRUS-BLT: frame row %lu exceeds FIFO staging buffer.",
		             (unsigned long)row_bytes);
		return;
	}

	dst = (uint32_t)ofs_y * cdisp.pitch +
	      (uint32_t)ofs_x * (uint32_t)bytespp;
	if (!cl_blt_fifo_start(dst, row_bytes, (uint32_t)draw_h))
		return;

	for (y = 0; y < draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;

		switch (cdisp.bpp) {
		case 24:
			conv_row24(row, src, draw_w);
			break;
		case 16:
			conv_row16(row, src, draw_w);
			break;
		default:
			conv_row8(row, src, draw_w);
			break;
		}
		cl_blt_fifo_feed_row(row, row_bytes);
	}

	if (!cl_blt_wait_idle(4000000UL, "after frame FIFO feed")) {
		cl_blt_reset();
		return;
	}

	nb10_sr12_after_first_frame();
	nb10_gate_cycle_poll();
}

/*****************************************************************************/
/* Fixed 0FAA/0FAB module: Core-Graph GD54xx and physical WABs             */
/*****************************************************************************/

/* Two-stage indexed I/O of the window accelerator interface. */
#define WAB_INDEX	0x0faa
#define WAB_DATA	0x0fab
#define WAB2_INDEX	0x0fa2
#define WAB2_DATA	0x0fa3

/* WAB registers */
#define WAB_REG_ID	0x00	/* machine ID (read only) */
#define WAB_REG_WINDOW	0x01	/* banked VRAM window address */
#define WAB_REG_LINEAR	0x02	/* linear aperture base, value << 24 */
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
#define WAB_RELAY_VIDEO 0x02    /* GDC output, accelerator video output */
#define WAB_RELAY_WAB	0x03	/* accelerator output, access on */

/* Second relay latch on the PCI-era WAB models (Xa7e etc.). */
#define WAB_RELAY2_PORT	0x0fac

/* Shared-VRAM ownership switch. */
#define VRAM_SW_PORT	0x6a
#define VRAM_SW_GDC	0x8e
#define VRAM_SW_WAB	0x8f

/* PC-98 Core-Graph/GDC routing ports used by NEC CIRRUS.SYS path 8. */
#define PC98_WAIT_PORT	0x5f
#define PC98_GDC_MODE_PORT	0x68


/* Wakeup ports. */
#define WAB_P904	0x0904	/* 102 Access Control (bit5) */
#define WAB_P902	0x0902	/* WAB IoVariant 2 enable port */
#define WAB_PFF82	0xff82	/* POS102 (bit0 = video subsystem enable) */
#define P54_SLEEP	0x0ca3	/* 3C3: Sleep Address (bit0 = enable) */

/* Relocated VGA register bases on the WAB machines. */
#define IO54_3C0	0x0ca0
#define IO54_3D4	0x0da4
#define IO54_3DA	0x0daa
#define IO54_3B4	0x0ba4	/* mono block, per the NT access ranges */
#define IO54_3BA	0x0baa

/* IoVariant 2 / PC-9801-96 WAB relocation. */
#define IO54B_3C0	0x0c50
#define IO54B_3D4	0x0d54
#define IO54B_3DA	0x0d5a
#define IO54B_3B4	0x0b54
#define IO54B_3BA	0x0b5a
#define P54B_SLEEP	0x0c53

/* Saved motherboard control state for the onboard/legacy GD54xx path. */
static uint8_t gd54_saved_p904, gd54_saved_pff82, gd54_saved_sleep;
static uint8_t gd54_saved_relay, gd54_saved_window, gd54_saved_linear;
static uint8_t gd54_saved_vram_sw;
static bool gd54_saved_valid;
static bool gd54_used_vram_switch;
static bool gd54_coregraph;

/*
 * WAB has two incompatible software-visible layouts:
 *   IoVariant 1: FAA/FAB + CA0/DA4/DAA + FF82 + CA3
 *   IoVariant 2: FA2/FA3 + C50/D54/D5A + 0902 + C53
 */
static int gd54_io_variant = 1;
static uint16_t gd54_wab_index = WAB_INDEX;
static uint16_t gd54_wab_data = WAB_DATA;
static uint16_t gd54_sleep_port = P54_SLEEP;
static uint16_t gd54_enable_port = WAB_PFF82;

/* Seen during the PCI pre-scan: NEC 1033:0009 Core-Graph bridge. */
static bool nec_coregraph_seen;
static int nec_coregraph_bus, nec_coregraph_dev, nec_coregraph_fn;

static bool
gd54_necdrv_path8_id(uint8_t id)
{
	return id >= 0x58 && id <= 0x5d;
}

static void
gd54_select_io_variant(int variant)
{
	gd54_io_variant = variant;
	if (variant == 2) {
		gd54_wab_index = WAB2_INDEX;
		gd54_wab_data = WAB2_DATA;
		gd54_sleep_port = P54B_SLEEP;
		gd54_enable_port = WAB_P902;
		cl_set_iobase(IO54B_3C0, IO54B_3D4, IO54B_3DA,
		              IO54B_3B4, IO54B_3BA);
	} else {
		gd54_wab_index = WAB_INDEX;
		gd54_wab_data = WAB_DATA;
		gd54_sleep_port = P54_SLEEP;
		gd54_enable_port = WAB_PFF82;
		cl_set_iobase(IO54_3C0, IO54_3D4, IO54_3DA,
		              IO54_3B4, IO54_3BA);
	}
}

static void
wab_write(int reg, int val)
{
	outp(gd54_wab_index, reg);
	outp(gd54_wab_data, val);
}

static int
wab_read(int reg)
{
	outp(gd54_wab_index, reg);
	return inp(gd54_wab_data);
}

/* Decode the firmware-selected 32KB host window from fixed register 01h. */
static uint32_t
gd54_window_phys(uint8_t value)
{
	switch (value & 0xe0) {
	case 0x80:
		return 0x00f20000UL;
	case 0xa0:
		return 0x00f00000UL;
	case 0xc0:
		return 0x00f60000UL;
	case 0xe0:
		return 0x00f40000UL;
	default:
		return 0;
	}
}

/*
 * Detect the built-in CIRRUS accelerator.
 *
 * WAB register 00h returns the machine ID; CIRRUS models use
 * 50h-5Dh and 70h.  (Other values are S3/Matrox/Trident models or
 * 00h/FFh when no two-stage I/O accelerator is present.  Note the
 * PCI-connected models - Nb10 and friends - read FFh here; those
 * are handled by the PCI module below.  NP21/W's AUTO WAB type
 * morphs into an Xe10 the moment this port is touched - pin the
 * emulator to a fixed type when testing.)
 */
/*
 * Confirm that a WAB-ID-looking value really belongs to a relocated
 * Cirrus VGA block.  Some PCI-on-board machines (notably the V13
 * family) return 5Bh through 0FAAh/0FABh even though they have no WAB.
 * Accepting that value alone selects the banked path, reads CR27=FFh,
 * and can overwrite ordinary memory at the supposed F20000h VRAM
 * window.
 *
 * This probe is deliberately reversible and runs before the VRAM
 * window is mapped or port 6Ah changes ownership.
 */
static bool
gd54_identity_at_stage(const char *stage)
{
	uint8_t sr06, cr27;

	cl_select_crtc(cl_misc_read());
	cl_seq_write(0x06, 0x12);
	sr06 = (uint8_t)cl_seq_read(0x06);
	cr27 = sr06 == 0x12 ? (uint8_t)cl_crtc_read(0x27) : 0xff;
	hal_log_info("CIRRUS-V13: %s: SR06=%02Xh CR27=%02Xh "
		     "(0904=%02Xh FF82=%02Xh sleep=%02Xh reg03=%02Xh "
		     "reg01=%02Xh reg02=%02Xh 6A=%02Xh).",
		     stage, sr06, cr27, inp(WAB_P904), inp(gd54_enable_port),
		     inp(gd54_sleep_port), wab_read(WAB_REG_RELAY),
		     wab_read(WAB_REG_WINDOW), wab_read(WAB_REG_LINEAR),
		     inp(VRAM_SW_PORT));
	return sr06 == 0x12 && cr27 != 0x00 && cr27 != 0xff;
}

static void
gd54_restore_board_state(void)
{
	if (!gd54_saved_valid)
		return;

	if (!gd54_coregraph)
		outp(VRAM_SW_PORT, gd54_saved_vram_sw);
	wab_write(WAB_REG_LINEAR, gd54_saved_linear);
	wab_write(WAB_REG_WINDOW, gd54_saved_window);
	wab_write(WAB_REG_RELAY, gd54_saved_relay);
	outp(gd54_sleep_port, gd54_saved_sleep);
	if (!gd54_coregraph) {
		outp(gd54_enable_port, gd54_saved_pff82);
		outp(WAB_P904, gd54_saved_p904);
	}
	gd54_saved_valid = false;
}

static bool
wab_validate_cirrus(void)
{
	uint8_t old_relay, old_sr06, sr06, cr27;
	uint8_t old_p904, old_pff82, old_sleep;

	/* wab_detect() has already selected the matching I/O layout. */
	gd54_select_io_variant(gd54_io_variant);

	old_p904 = (uint8_t)inp(WAB_P904);
	old_pff82 = (uint8_t)inp(gd54_enable_port);
	old_sleep = (uint8_t)inp(gd54_sleep_port);
	old_relay = (uint8_t)wab_read(WAB_REG_RELAY);

	/*
	 * Core-Graph machines expose the relocated Cirrus block directly;
	 * 0904h/FF82h are WAB-era controls and read FFh on the V13.
	 */
	if (!gd54_necdrv_path8_id(cdisp.wab_id)) {
		if (gd54_io_variant == 2) {
			/* Exact NEC IoVariant-2 wake sequence. */
			outp(WAB_P904, 0x00);
			outp(gd54_enable_port, 0x01); /* 0902h */
			outp(WAB_P904, 0x20);
		} else {
			outp(WAB_P904, old_p904 | 0x20);
			outp(gd54_enable_port, old_pff82 | 0x01);
		}
	}
	outp(gd54_sleep_port, old_sleep | 0x01);
	wab_write(WAB_REG_RELAY, old_relay | WAB_RELAY_SETUP);

	cl_select_crtc(cl_misc_read());
	old_sr06 = (uint8_t)cl_seq_read(0x06);
	cl_seq_write(0x06, 0x12);
	sr06 = (uint8_t)cl_seq_read(0x06);
	cr27 = sr06 == 0x12 ? (uint8_t)cl_crtc_read(0x27) : 0xff;

	/* Restore every temporary enable before returning. */
	cl_seq_write(0x06, old_sr06);
	wab_write(WAB_REG_RELAY, old_relay);
	outp(gd54_sleep_port, old_sleep);
	if (!gd54_necdrv_path8_id(cdisp.wab_id)) {
		outp(gd54_enable_port, old_pff82);
		outp(WAB_P904, old_p904);
	}

	if (sr06 != 0x12 || cr27 == 0x00 || cr27 == 0xff) {
		hal_log_info("CIRRUS: WAB ID %02Xh is a false positive: "
			     "relocated VGA validation SR06=%02Xh CR27=%02Xh; "
			     "continuing with PCI probe.",
			     cdisp.wab_id, sr06, cr27);
		return false;
	}

	hal_log_info("CIRRUS: WAB/Core-Graph ID %02Xh validated: "
		     "IoVariant=%d index/data=%03Xh/%03Xh VGA=%03Xh "
		     "SR06=%02Xh CR27=%02Xh.",
		     cdisp.wab_id, gd54_io_variant,
		     gd54_wab_index, gd54_wab_data, cdisp.io_3c0,
		     sr06, cr27);
	return true;
}

static bool
wab_detect(void)
{
	uint8_t probe;

	/*
	 * NEC's first probe is NOT the FAA/FAB fixed-interface ID.
	 * PC-9801-96 / WAB IoVariant 2 is identified by this destructive-looking
	 * but reference-driver-exact handshake:
	 *
	 *   FA2 <- 00h
	 *   FA3 <- FFh
	 *   read FA3 == 60h
	 *
	 * A match changes both the indexed-control pair and the relocated VGA
	 * block.  Treating 60h as a normal FAA/FAB ID makes validation hit the
	 * wrong ports and report SR06/CR27 as FFh.
	 */
	outp(WAB2_INDEX, 0x00);
	outp(WAB2_DATA, 0xff);
	probe = (uint8_t)inp(WAB2_DATA);
	if (probe == 0x60) {
		gd54_select_io_variant(2);
		cdisp.wab_id = 0x60;
		hal_log_info("CIRRUS: WAB IoVariant 2 detected: "
		             "FA2/FA3 probe returned 60h; VGA block=C50h/D54h.");
		return wab_validate_cirrus();
	}

	/* Fixed-interface / Core-Graph layout. */
	gd54_select_io_variant(1);
	cdisp.wab_id = (uint8_t)wab_read(WAB_REG_ID);
	if (!((cdisp.wab_id >= 0x50 && cdisp.wab_id <= 0x5d) ||
	      cdisp.wab_id == 0x70)) {
		hal_log_info("CIRRUS: fixed-interface ID reads %02Xh, "
		             "not a supported GD54xx built-in.",
		             cdisp.wab_id);
		return false;
	}

	return wab_validate_cirrus();
}

/* Dump the WAB interface registers (called once a Cirrus ID is seen). */
static void
wab_dump(void)
{
	hal_log_info("CIRRUS: fixed-interface regs: 00=%02Xh 01=%02Xh 02=%02Xh "
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
 * NEC-driver board-side entry/exit sequence for internal path 08h.
 *
 * This is outside the VGA command stream.  It configures the PC-98
 * Core-Graph/GDC routing logic before the GD5440 timing registers are
 * programmed.  Omitting it leaves the accelerator relay switched but the
 * surrounding clock/mux state in the 98-GDC configuration, which produces a
 * continuously drifting, periodically blanked picture even with static VRAM.
 *
 * Enter order observed in NEC CIRRUS.SYS:
 *   68h <- 0Eh
 *   6Ah <- 07h, 8Fh, 06h
 *   indexed reg03 <- 03h
 *   two writes to wait port 5Fh
 *   relocated Sleep Address <- 01h (path 08h)
 *
 * Exit uses the complementary 8Eh selection and 68h <- 0Fh.
 *
 * io_disp.txt documents 07h/06h as permission/inhibition of protected
 * mode-F/F modification.  It does NOT document 8Eh/8Fh as protected on this
 * GD5440 generation.  The likely routing/ownership interpretation therefore
 * remains a hypothesis; the exact sequence itself is a NEC-driver observation
 * and a V13 hardware requirement.  Preserve it verbatim for porting reference.
 */
static void
coregraph_necdrv_gate_enter(void)
{
	outp(PC98_GDC_MODE_PORT, 0x0e);
	outp(VRAM_SW_PORT, 0x07);
	outp(VRAM_SW_PORT, 0x8f);
	outp(VRAM_SW_PORT, 0x06);
	wab_write(WAB_REG_RELAY, WAB_RELAY_WAB);
	outp(PC98_WAIT_PORT, 0x00);
	outp(PC98_WAIT_PORT, 0x00);
	outp(gd54_sleep_port, 0x01);

	hal_log_info("CIRRUS-CORE: NEC-driver gate enter: 68h=0Eh; "
	             "6Ah sequence 07h,8Fh,06h; reg03=%02Xh; sleep=%02Xh.",
	             wab_read(WAB_REG_RELAY), inp(gd54_sleep_port));
}

static void
coregraph_necdrv_gate_leave(void)
{
	unsigned long i;

	/* Exact path-08h unwind order from NEC's miniport. */
	outp(gd54_sleep_port, 0x00);
	wab_write(WAB_REG_RELAY, WAB_RELAY_GDC);
	outp(PC98_WAIT_PORT, 0x00);
	outp(VRAM_SW_PORT, 0x07);
	outp(VRAM_SW_PORT, 0x8e);
	outp(VRAM_SW_PORT, 0x06);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0x00);
	outp(PC98_GDC_MODE_PORT, 0x0f);

	hal_log_info("CIRRUS-CORE: NEC-driver gate leave: reg03=%02Xh; "
	             "6Ah sequence 07h,8Eh,06h; 68h=0Fh.",
	             wab_read(WAB_REG_RELAY));
}

static bool
cirrus54_init(int mode, int req_bpp)
{
	int w, h, bpp;
	uint8_t relay_setup;
	bool coregraph;

	if (!wab_detect())
		return false;

	coregraph = gd54_necdrv_path8_id(cdisp.wab_id);
	gd54_coregraph = coregraph;

	hal_log_info("CIRRUS: onboard/legacy GD54xx control interface found "
	             "(ID %02Xh; not assumed to be a physical WAB).",
	             cdisp.wab_id);
	wab_dump();
	if (coregraph && nec_coregraph_seen)
		hal_log_info("CIRRUS-CORE: ID %02Xh selects NEC-driver path 08h; "
		             "NEC 1033:0009 marker is at PCI %d:%d.%d.",
		             cdisp.wab_id, nec_coregraph_bus, nec_coregraph_dev,
		             nec_coregraph_fn);
	else if (coregraph)
		hal_log_info("CIRRUS-CORE: ID %02Xh selects NEC-driver path 08h "
		             "(no 1033:0009 marker was observed).", cdisp.wab_id);

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	if (coregraph && mode != DISP_640X480) {
		hal_log_info("CIRRUS-CORE: only the recovered NEC-driver 640x480 streams "
		             "are currently enabled.");
		return false;
	}
	if (mode != DISP_640X480 && mode != DISP_800X600) {
		hal_log_info("CIRRUS: GD54xx control path: %dx%d not supported "
		             "(640x480 / 800x600 only).", w, h);
		return false;
	}

	cdisp.vram_size = 1024UL * 1024UL;
	bpp = cl_resolve_bpp(req_bpp, wab_default_bpp(cdisp.wab_id),
	                     w, h, cdisp.vram_size, "GD54xx onboard/legacy");
	if (bpp < 0)
		return false;

	cdisp.scr_w = w;
	cdisp.scr_h = h;
	cdisp.bpp = bpp;
	if (coregraph) {
		/* Fixed by NEC's path-8 command streams. */
		cdisp.pitch = bpp == 8 ? 640UL :
		              (bpp == 16 ? 1280UL : 2048UL);
	} else {
		cdisp.pitch = (uint32_t)w * (uint32_t)(bpp / 8);
	}
	cdisp.linear = coregraph;
	cdisp.fifo_only = gd54_fifo_requested;
	cdisp.fifo_capable = true;

	/* wab_detect() selected the matching WAB/Core-Graph layout. */
	gd54_select_io_variant(gd54_io_variant);

	/* Preserve all board-side registers before changing any routing. */
	gd54_saved_p904 = (uint8_t)inp(WAB_P904);
	gd54_saved_pff82 = (uint8_t)inp(gd54_enable_port);
	gd54_saved_sleep = (uint8_t)inp(gd54_sleep_port);
	gd54_saved_relay = (uint8_t)wab_read(WAB_REG_RELAY);
	gd54_saved_window = (uint8_t)wab_read(WAB_REG_WINDOW);
	gd54_saved_linear = (uint8_t)wab_read(WAB_REG_LINEAR);
	gd54_saved_vram_sw = (uint8_t)inp(VRAM_SW_PORT);
	gd54_saved_valid = true;
	gd54_used_vram_switch = false;

	/*
	 * 0904h/FF82h belong to the older WAB wake path.  Core-Graph path 8
	 * instead needs NEC's external 68h/6Ah gate sequence; that is issued
	 * later, immediately before the VGA mode stream.
	 */
	if (!coregraph) {
		if (gd54_io_variant == 2) {
			/* Exact NEC IoVariant-2 wake sequence. */
			outp(WAB_P904, 0x00);
			outp(gd54_enable_port, 0x01); /* 0902h */
			outp(WAB_P904, 0x20);
		} else {
			outp(WAB_P904, gd54_saved_p904 | 0x20);
			outp(gd54_enable_port, gd54_saved_pff82 | 0x01);
		}
	}
	outp(gd54_sleep_port, gd54_saved_sleep | 0x01);
	relay_setup = (uint8_t)((gd54_saved_relay & ~WAB_RELAY_VIDEO) |
	                        WAB_RELAY_SETUP);
	wab_write(WAB_REG_RELAY, relay_setup);

	if (!gd54_identity_at_stage("after wake + register enable")) {
		hal_log_info("CIRRUS: register identity vanished during wake; "
		             "aborting before any host-memory access.");
		gd54_restore_board_state();
		return false;
	}

	/*
	 * Select the host aperture independently from the transfer method.
	 *
	 * Core-Graph path 08h: reg02=F0h opens a 1MB linear window at
	 * F0000000h.  With the BLT idle it is ordinary VRAM; during a
	 * MEMSYSSRC BLT the same write cycles become FIFO source dwords.
	 * Direct mode maps the full 1MB, while FIFO mode needs only 64KB.
	 *
	 * Classic path-04h/WAB machines use the reg01 32KB bank window for
	 * direct VRAM access.  The retained FIFO path uses the same mapped
	 * window, though its behavior still needs confirmation on every WAB.
	 */
	if (coregraph) {
		wab_write(WAB_REG_LINEAR, 0xf0);
		if ((uint8_t)wab_read(WAB_REG_LINEAR) != 0xf0) {
			hal_log_info("CIRRUS-BLT: reg02 did not retain F0h "
			             "(reads %02Xh); cannot open Core-Graph host aperture.",
			             wab_read(WAB_REG_LINEAR));
			gd54_restore_board_state();
			return false;
		}
		cdisp.fb_phys = 0xf0000000UL;
		cdisp.fb = (uint8_t *)cl_map_physical(cdisp.fb_phys,
		                                      cdisp.fifo_only ? 0x10000UL :
		                                                        cdisp.vram_size);
		if (cdisp.fb == NULL) {
			hal_log_info("CIRRUS: cannot map Core-Graph linear host "
			             "aperture at %08lXh.",
			             (unsigned long)cdisp.fb_phys);
			gd54_restore_board_state();
			return false;
		}
		hal_log_info("CIRRUS-CORE: reg02=%02Xh opens %s host aperture "
		             "at %08lXh (%luKB mapped).",
		             wab_read(WAB_REG_LINEAR),
		             cdisp.fifo_only ? "CPU-source FIFO" : "linear VRAM",
		             (unsigned long)cdisp.fb_phys,
		             (unsigned long)((cdisp.fifo_only ? 0x10000UL :
		                                               cdisp.vram_size) >> 10));
	} else {
		if (gd54_io_variant == 2) {
			uint8_t strap, ack;

			/*
			 * IoVariant 2 uses reg04&7 as the board-selected aperture.
			 * Encode the same physical choice back through reg01.
			 */
			strap = (uint8_t)(wab_read(0x04) & 0x07);
			switch (strap) {
			case 0: ack = 0xa0; cdisp.fb_phys = 0x00f00000UL; break;
			case 1: ack = 0x80; cdisp.fb_phys = 0x00f20000UL; break;
			case 2: ack = 0xe0; cdisp.fb_phys = 0x00f40000UL; break;
			case 3: ack = 0xc0; cdisp.fb_phys = 0x00f60000UL; break;
			default:
				hal_log_info("CIRRUS: WAB IoVariant 2 reg04 strap "
				             "%02Xh is unsupported.", strap);
				gd54_restore_board_state();
				return false;
			}
			wab_write(WAB_REG_WINDOW, ack);
			hal_log_info("CIRRUS: WAB IoVariant 2 aperture strap=%u, "
			             "reg01=%02Xh -> %08lXh.",
			             strap, ack, (unsigned long)cdisp.fb_phys);
		} else {
			wab_write(WAB_REG_WINDOW, WAB_WINDOW_F2);
			cdisp.fb_phys =
			    gd54_window_phys((uint8_t)wab_read(WAB_REG_WINDOW));
		}

		if (!gd54_identity_at_stage("after selecting reg01 host window")) {
			hal_log_info("CIRRUS: reg01 window selection disabled the chip.");
			gd54_restore_board_state();
			return false;
		}
		if (cdisp.fb_phys == 0) {
			hal_log_info("CIRRUS-BLT: reg01=%02Xh has no known "
			             "32KB-window decoding.",
			             wab_read(WAB_REG_WINDOW));
			gd54_restore_board_state();
			return false;
		}
		cdisp.fb = (uint8_t *)cl_map_physical(cdisp.fb_phys, WAB_WINDOW_SIZE);
		if (cdisp.fb == NULL) {
			hal_log_info("CIRRUS: cannot map reg01 host window at %08lXh.",
			             (unsigned long)cdisp.fb_phys);
			gd54_restore_board_state();
			return false;
		}
		hal_log_info("CIRRUS: physical-WAB %s window is "
		             "reg01=%02Xh -> %08lXh; reg02 left at %02Xh.",
		             cdisp.fifo_only ? "FIFO host" : "banked VRAM",
		             wab_read(WAB_REG_WINDOW),
		             (unsigned long)cdisp.fb_phys, gd54_saved_linear);
	}

	if (!coregraph) {
		outp(VRAM_SW_PORT, VRAM_SW_WAB);
		if ((uint8_t)inp(VRAM_SW_PORT) == VRAM_SW_WAB &&
		    gd54_identity_at_stage("after 6Ah=8Fh")) {
			gd54_used_vram_switch = true;
			hal_log_info("CIRRUS: 6Ah=8Fh retained.");
		} else {
			outp(VRAM_SW_PORT, gd54_saved_vram_sw);
			hal_log_info("CIRRUS: 6Ah=8Fh was not retained; original value restored.");
		}
	}

	/*
	 * The recovered NEC-driver Core-Graph board sequence must run before the
	 * path-8 VGA stream, exactly as NEC's miniport does.
	 */
	if (coregraph)
		coregraph_necdrv_gate_enter();

	/* Use the recovered NEC-driver path-08h stream on Core-Graph; generic on path 04h. */
	if (coregraph)
		cl_modeset_coregraph_necdrv();
	else
		cl_modeset_generic(true);
	if (cdisp.crt27 == 0x00 || cdisp.crt27 == 0xff) {
		hal_log_info("CIRRUS: CR27 became %02Xh before host-memory clear; aborting.",
		             cdisp.crt27);
		gd54_restore_board_state();
		cl_release_fb_mapping();
		return false;
	}

	/*
	 * Reset the engine even in aperture mode.  A stale MEMSYSSRC command
	 * would otherwise consume the following framebuffer clear as FIFO data.
	 */
	cl_blt_reset();
	if (cdisp.fifo_only) {
		if (!cl_blt_fifo_clear_visible()) {
			hal_log_info("CIRRUS-BLT: initial visible-screen FIFO clear failed.");
			gd54_restore_board_state();
			cl_release_fb_mapping();
			return false;
		}
	} else {
		if (!cl_aperture_clear_visible()) {
			hal_log_info("CIRRUS: initial visible-screen aperture clear failed.");
			gd54_restore_board_state();
			cl_release_fb_mapping();
			return false;
		}
	}

	/* Screen on.  Core-Graph reg03 was already selected before the mode stream. */
	cl_seq_write(0x01, 0x01);
	if (!coregraph) {
		wab_write(WAB_REG_RELAY,
		          relay_setup | WAB_RELAY_VIDEO | WAB_RELAY_SETUP);
		outp(WAB_RELAY2_PORT, 0x02);
	}

	if (coregraph) {
		hal_log_info("CIRRUS-CORE: output selected via reg03=%02Xh; "
		             "0FACh untouched (reads %02Xh).",
		             wab_read(WAB_REG_RELAY), inp(WAB_RELAY2_PORT));
		hal_log_info("CIRRUS-CORE: visible screen cleared to black via %s.",
		             cdisp.fifo_only ? "CPU-source FIFO" :
		                               "direct linear VRAM aperture");
	} else
		hal_log_info("CIRRUS: GD54xx output selected (reg03=%02Xh, 0FACh=%02Xh).",
		             wab_read(WAB_REG_RELAY), inp(WAB_RELAY2_PORT));

	cdisp.chip_name = cdisp.alpine ? "CL-GD5430/5440 (onboard/legacy)" :
	                                 "CL-GD5428 (onboard/legacy)";
	cdisp.path = coregraph ? CIRRUS_PATH_54_COREGRAPH :
	                         CIRRUS_PATH_54_BANKED;
	return true;
}

static void
cirrus54_cleanup(void)
{
	/* Keep register access enabled while blanking, then unwind board routing. */
	cl_seq_write(0x01, 0x21);
	if (gd54_coregraph)
		coregraph_necdrv_gate_leave();
	else
		outp(WAB_RELAY2_PORT, 0x00);
	gd54_restore_board_state();
	cl_release_fb_mapping();
	gd54_coregraph = false;
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

/*
 * Nb10 is a PCI attachment, but its board-side gate is not the generic
 * PCI-laptop relay.  Keep the last output-producing DOS sequence as a
 * bootstrap: reg03=03h plus a pre-relay FIFO clear.  This is deliberately
 * not claimed to be the final byte-for-byte NT4 model-0Eh sequence; it is
 * the known-good visibility baseline from which the pure path can be
 * reduced again.
 */
static bool pci_nb10_board_active;
static uint32_t pci_saved_command;

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
	nec_coregraph_seen = false;
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
				if ((id & 0xffff) == 0x1033 &&
				    (uint16_t)(id >> 16) == 0x0009) {
					nec_coregraph_seen = true;
					nec_coregraph_bus = bus;
					nec_coregraph_dev = dev;
					nec_coregraph_fn = fn;
					hal_log_info("CIRRUS: NEC Core-Graph bridge marker "
					     "found at PCI %d:%d.%d.", bus, dev, fn);
				}
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

	/* Sequencer stream begins with SR00=01h synchronous reset. */
	seq_stream(seq, nseq);

	/* Exact RMW form used by the miniport. */
	cl_seq_write(0x0f, (cl_seq_read(0x0f) & 0xdf) | 0x20);

	/* Generic PCI laptops keep their historical early SR17 placement. */
	if (!pci_nb10_active)
		cl_seq_write(0x17, (uint8_t)(cl_seq_read(0x17) | 0x44));

	cl_misc_write(misc);
	cl_gfx_write(0x06, 0x05);

	/* Critical: release SR00 synchronous reset before programming CRTC. */
	cl_seq_write(0x00, 0x03);

	crtc_stream(crtc, ncrtc);
	gfx_stream(gpost, ngpost);

	(void)inp(cdisp.io_3da);
	for (i = 0; i < 21; i++) {
		outp(cdisp.io_3c0, (uint8_t)i);
		outp(cdisp.io_3c0, m754x_atc[i]);
	}
	/* Nb10's board postlude owns the final AC-enable edge. */
	if (!pci_nb10_active) {
		(void)inp(cdisp.io_3da);
		outp(cdisp.io_3c0, 0x20);
	}

	cl_hidden_dac_write(hdr);
	outp(cdisp.io_3c0 + 0x06, 0xff);

	cl_gfx_write(0x09, 0x00);
	cl_gfx_write(0x0a, 0x00);
	cl_gfx_write(0x0b, 0x20);
	cl_gfx_write(0x31, 0x00);
	cl_gfx_write(0x0e, 0x00);
	cdisp.cur_bank = 0;

	if (!pci_nb10_active)
		cl_load_palette();
}


/*
 * Nb10 FAA/FAB reg03 test support.
 *
 * Use FAA/FAB explicitly.  wab_write() follows the currently selected WAB
 * IoVariant and must not be used for the independently enumerated Nb10.
 * No function in this block reads reg03.
 */
static void
nb10_reg03_write(const char *stage, uint8_t value)
{
	outp(WAB_INDEX, WAB_REG_RELAY);
	outp(WAB_DATA, value);
	nb10_reg03_touched = true;
	hal_log_info("CIRRUS-NB10: T=%d FAA/FAB reg03 %s write %02Xh.",
	             nb10_reg03_test, stage, value);
}

static void
nb10_reg03_configure(void)
{
	const char *env;
	char *endp;
	long value;

	/* Unset or invalid T defaults to the established bit1-only baseline. */
	nb10_reg03_test = 2;
	nb10_reg03_touched = false;

	env = getenv("T");
	if (env != NULL) {
		endp = NULL;
		value = strtol(env, &endp, 0);
		if (endp != env && *endp == '\0' &&
		    value >= 0 && value <= 3) {
			nb10_reg03_test = (int)value;
		} else {
			hal_log_info("CIRRUS-NB10: invalid T=%s; using T=2.",
			             env);
		}
	}

	hal_log_info("CIRRUS-NB10: reg03 matrix T=%d: %s. "
	             "FACh relay and 6Ah dance remain canonical.",
	             nb10_reg03_test,
	             nb10_reg03_test == 0 ?
	                 "FAA/FAB reg03 read/write count is zero" :
	             nb10_reg03_test == 1 ?
	                 "early reg03=01h (bit0 only)" :
	             nb10_reg03_test == 2 ?
	                 "early reg03=02h (bit1 only)" :
	                 "early reg03=03h (bit0+bit1 baseline)");
}

static void
nb10_reg03_apply_enter(void)
{
	switch (nb10_reg03_test) {
	case 1:
		nb10_reg03_write("enter", 0x01);
		break;
	case 2:
		nb10_reg03_write("enter", 0x02);
		break;
	case 3:
		nb10_reg03_write("enter", 0x03);
		break;
	default:
		/* T=0: strict no-touch. */
		break;
	}
}

static void
nb10_reg03_apply_leave(void)
{
	/*
	 * T=0 must not even perform an idempotent 00h write.  T=1..3 restore
	 * the historical DOS exit value so their edge behavior remains visible.
	 */
	if (nb10_reg03_touched)
		nb10_reg03_write("leave", 0x00);

	nb10_reg03_touched = false;
}

/* ---- Nb10 family-40h/model-0Eh board layer ----------------------------- */

#define NB10_NEC_INDEX_PORT 0x08f0
#define NB10_NEC_DATA_PORT  0x08f2

static void
nb10_nec_unlock(void)
{
	uint16_t v;

	outpw(NB10_NEC_INDEX_PORT, 0x0052);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_WAIT_PORT, 0);
	v = (uint16_t)inpw(NB10_NEC_DATA_PORT);
	outpw(NB10_NEC_DATA_PORT, v | 0x0080);

	outpw(NB10_NEC_INDEX_PORT, 0x0060);
	v = (uint16_t)inpw(NB10_NEC_DATA_PORT);
	outpw(NB10_NEC_DATA_PORT, v & 0xffef);
}

static void
nb10_nec_lock(void)
{
	uint16_t v;

	outpw(NB10_NEC_INDEX_PORT, 0x0060);
	v = (uint16_t)inpw(NB10_NEC_DATA_PORT);
	outpw(NB10_NEC_DATA_PORT, v | 0x0010);

	outpw(NB10_NEC_INDEX_PORT, 0x0052);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_WAIT_PORT, 0);
	v = (uint16_t)inpw(NB10_NEC_DATA_PORT);
	outpw(NB10_NEC_DATA_PORT, v & 0xff7f);
}

static void
nb10_attr_preamble(void)
{
	int cr24, attr;

	outp(cdisp.io_3d4, 0x24);
	cr24 = inp(cdisp.io_3d4 + 1);
	if (cr24 & 0x80) {
		attr = inp(cdisp.io_3c0 + 1);
		outp(cdisp.io_3c0, attr);
	}
	outp(cdisp.io_3c0, 0x31);
	outp(cdisp.io_3c0, 0x00);
	outp(cdisp.io_3c0, 0x00);
}

static void
nb10_attr_enable(void)
{
	int cr24, attr;

	outp(cdisp.io_3d4, 0x24);
	cr24 = inp(cdisp.io_3d4 + 1);
	if (cr24 & 0x80) {
		attr = inp(cdisp.io_3c0 + 1);
		outp(cdisp.io_3c0, attr);
	}
	outp(cdisp.io_3c0, 0x20);
}

/* ---- Nb10 successful-SET black-dwell checkpoint (G/W) ------------------ */

/*
 * Busy-wait on the C runtime clock.  On the DOS target this advances from the
 * periodic system timer while the single task spins.  The loop deliberately
 * performs no VGA/NEC I/O, no FIFO write and no keyboard polling.
 */
static bool
nb10_passive_wait_seconds(int seconds)
{
	clock_t start, now, span;
	char message[256];

	if (seconds <= 0)
		return true;

	start = clock();
	if (start == (clock_t)-1) {
		sprintf(message,
		        "CIRRUS-NB10 G=%d: clock() is unavailable. Keep the "
		        "screen black for at least %d seconds, then press one key.",
		        nb10_modeset_checkpoint_test, seconds);
		nb10_wait_key(message);
		return true;
	}
	span = (clock_t)seconds * (clock_t)CLOCKS_PER_SEC;
	do {
		now = clock();
		if (now == (clock_t)-1) {
			sprintf(message,
			        "CIRRUS-NB10 G=%d: clock() failed during the dwell. "
			        "Wait until at least %d seconds have elapsed, then "
			        "press one key.",
			        nb10_modeset_checkpoint_test, seconds);
			nb10_wait_key(message);
			return true;
		}
	} while ((clock_t)(now - start) < span);
	return true;
}

static bool
nb10_modeset_black_checkpoint(int hardware_set)
{
	const char *request_name;

	request_name = hardware_set == 1 ?
	    "IOCTL request #2 / hardware SET #1" :
	    "IOCTL request #3 / hardware SET #2";

	/*
	 * Start the timer immediately at the SET checkpoint.  Do not log or print
	 * before the wait: even harmless console/file I/O would add an unmeasured
	 * delay to W and make short phase samples less reproducible.
	 */
	if (!nb10_passive_wait_seconds(nb10_modeset_black_dwell_seconds))
		return false;

	hal_log_info("CIRRUS-NB10 G=%d: %s black dwell completed: %d seconds "
	             "with SR12 open, FACh=02h and AC enabled; no BLT or "
	             "display-register poll occurred during W.",
	             nb10_modeset_checkpoint_test, request_name,
	             nb10_modeset_black_dwell_seconds);

	/* One write reveals the state reached during the black interval. */
	if (cdisp.bpp == 8)
		cl_load_palette();
	cl_blt_reset();
	if (!cl_blt_fifo_pattern_visible()) {
		hal_log_info("CIRRUS-NB10 G=%d: one-shot reveal pattern failed.",
		             nb10_modeset_checkpoint_test);
		return false;
	}

	nb10_wait_key(
	    hardware_set == 1 ?
	    "CIRRUS-NB10 G=1: one static pattern was written after the black "
	    "dwell following hardware SET #1. No further BLTs are running. "
	    "Observe (a) its immediate pixel parity, (b) whether the reveal write "
	    "made horizontal noise, and (c) whether the static image later cycles "
	    "through 1px-missing/noise/pixel-perfect. Press one key to continue." :
	    "CIRRUS-NB10 G=2: one static pattern was written after the black "
	    "dwell following hardware SET #2. No further BLTs are running. "
	    "Observe (a) its immediate pixel parity, (b) whether the reveal write "
	    "made horizontal noise, and (c) whether the static image later cycles "
	    "through 1px-missing/noise/pixel-perfect. Press one key to continue.");
	return true;
}

/* ---- Nb10 NT4 first-enable double-SET (F) experiment -------------------- */

/*
 * Execute only the SECOND successful SET_CURRENT_MODE observed during NT4's
 * first DrvEnableSurface path.  The caller has just completed the normal DOS
 * pass, so FACh is already 02h, AC is enabled and NEC is locked.
 *
 * Keep the critical interval free of diagnostics and user interaction: the
 * first operation is the second miniport prelude, and no log/readback occurs
 * until the complete second postlude has finished.  In particular this pass
 * never calls nb10_reg03_apply_enter(), so FAA/FAB indexed reg03 receives no
 * additional access.
 */
static bool
nb10_nt4_second_set(void)
{
	uint32_t cmd;
	unsigned long i;

	/* family-40h prelude: PCI decode, NEC unlock, routing dance, AC down. */
	cmd = pci_read32(pci_bus, pci_dev, pci_fn, 0x04);
	/* Avoid echoing write-one-to-clear PCI status bits in the upper word. */
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04,
	            (cmd & 0x0000ffffUL) | 0x00000003UL);
	nb10_nec_unlock();
	outp(PC98_GDC_MODE_PORT, 0x0e);
	outp(VRAM_SW_PORT, 0x07);
	outp(VRAM_SW_PORT, 0x8f);
	outp(VRAM_SW_PORT, 0x06);
	nb10_attr_preamble();

	/*
	 * FACh remains 02h while this stream performs SR00=01h -> 03h and
	 * restarts the GD7548 pixel stream.  This inherited state is the point of
	 * F=1; do not lower FACh and do not touch FAA/FAB reg03 here.
	 */
	program_mode_754x();
	cl_select_crtc(cl_misc_read());
	cl_seq_write8(0x17, (uint8_t)(cl_seq_read(0x17) | 0x44));
	cl_seq_write8(0x02, (uint8_t)(cl_seq_read(0x02) | 0x0f));

	/*
	 * NT4 zeroes its mapped 1MB range here.  Retain the already verified DOS
	 * CPU-source FIFO clear for this first ordering experiment, so the only
	 * new variable is the second full mode set and its FACh carry state.
	 */
	cl_blt_reset();
	if (!cl_blt_fifo_clear_visible()) {
		nb10_nec_lock();
		return false;
	}

	/* Exact model-0Eh postlude order, including the idempotent FACh write. */
	cl_seq_write8(0x12, (uint8_t)(cl_seq_read(0x12) & 0xbf));
	outp(PCI_RELAY_PORT, 0x02);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0);
	nb10_attr_enable();
	outp(cdisp.io_3c0 + 0x06, 0xff);
	nb10_nec_lock();

	if (nb10_modeset_checkpoint_test == 2 &&
	    !nb10_modeset_black_checkpoint(2))
		return false;

	hal_log_info("CIRRUS-NB10 F=1: NT4-style second full SET completed; "
	             "FACh carried high across SR00 reset, reg03 untouched in "
	             "pass 2, visible FIFO clear used (not NT4's direct 1MB zero). "
	             "Final SR00=%02Xh SR12=%02Xh FACh=%02Xh GR31=%02Xh.",
	             cl_seq_read(0x00), cl_seq_read(0x12),
	             inp(PCI_RELAY_PORT), cl_gfx_read(0x31));
	return true;
}

/* ---- Nb10 gate-cycle (E) experiment -------------------------------------- */

static void
nb10_fach_write_logged(const char *stage, uint8_t value)
{
	unsigned long i;
	int readback;

	outp(PCI_RELAY_PORT, value);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0);
	readback = inp(PCI_RELAY_PORT);
	hal_log_info("CIRRUS-NB10 E=%d %s: FACh write %02Xh, readback %02Xh%s.",
	             nb10_gate_cycle_test, stage, value, readback,
	             readback == value ? "" : " (MISMATCH: NEC lock may gate FACh)");
}

/*
 * Polled after every completed game-frame FIFO BLT.  The first call only
 * drains stale keystrokes and arms the trigger; any later keypress during
 * gameplay starts the cycle, freezing the current frame on screen (the
 * prompts block, so no further BLT is submitted until the cycle ends).
 */
static void
nb10_gate_cycle_poll(void)
{
	if (!pci_nb10_active || nb10_gate_cycle_test == 0 ||
	    nb10_gate_cycle_done)
		return;

	if (!nb10_gate_cycle_armed) {
		while (kbhit())
			(void)getch();
		nb10_gate_cycle_armed = true;
		hal_log_info("CIRRUS-NB10 E=%d armed: press any key during "
		             "gameplay once the 1+1 attractor is stable on "
		             "real game content.", nb10_gate_cycle_test);
		return;
	}

	if (!kbhit())
		return;
	(void)getch();

	nb10_gate_cycle_done = true;
	hal_log_info("CIRRUS-NB10 E=%d triggered; the current game frame "
	             "stays on screen for the cycle.", nb10_gate_cycle_test);

	switch (nb10_gate_cycle_test) {
	case 1:
		nb10_attr_preamble();
		nb10_fach_write_logged("close", 0x00);
		nb10_wait_key(
		    "CIRRUS-NB10 E=1 stage 1: AC is disabled and FACh is 00h "
		    "(both gates DOWN; the panel should show the 98-GDC side). "
		    "Wait at least 5 seconds, then press Enter to reopen both.");
		nb10_fach_write_logged("reopen", 0x02);
		nb10_attr_enable();
		nb10_wait_key(
		    "CIRRUS-NB10 E=1 stage 2: both gates are UP again. If the "
		    "image is IMMEDIATELY in the 1+1 attractor, the trained "
		    "gear survived (closed dwell = pause). If it starts "
		    "pixel-perfect and repeats the horizontal-noise "
		    "calibration, the both-down dwell RESET the gear. "
		    "Press Enter to continue.");
		break;
	case 2:
		nb10_fach_write_logged("close", 0x00);
		nb10_wait_key(
		    "CIRRUS-NB10 E=2 stage 1: FACh is 00h, AC stays enabled "
		    "(one gate still UP). Wait at least 5 seconds, then press "
		    "Enter to restore FACh=02h.");
		nb10_fach_write_logged("reopen", 0x02);
		nb10_wait_key(
		    "CIRRUS-NB10 E=2 stage 2: FACh is 02h again. Expected "
		    "under the OR model: immediately in the attractor. A "
		    "pixel-perfect restart here would contradict the OR "
		    "model. Press Enter to continue.");
		break;
	default:
		nb10_attr_preamble();
		nb10_wait_key(
		    "CIRRUS-NB10 E=3 stage 1: AC is disabled, FACh stays 02h "
		    "(one gate still UP). Wait at least 5 seconds, then press "
		    "Enter to re-enable AC.");
		nb10_attr_enable();
		nb10_wait_key(
		    "CIRRUS-NB10 E=3 stage 2: AC is enabled again. Expected "
		    "under the OR model: immediately in the attractor. A "
		    "pixel-perfect restart here would contradict the OR "
		    "model. Press Enter to continue.");
		break;
	}
}

static bool
nb10_known_output_enter(void)
{
	unsigned long i;

	nb10_nec_unlock();
	outp(PC98_GDC_MODE_PORT, 0x0e);
	outp(VRAM_SW_PORT, 0x07);
	outp(VRAM_SW_PORT, 0x8f);
	outp(VRAM_SW_PORT, 0x06);

	/*
	 * Canonical model-0Eh ordering keeps the 6Ah dance, while T controls only
	 * the optional FAA/FAB indexed reg03 write.  T=0 reaches this point with
	 * zero reg03 reads/writes and remains no-touch through leave.
	 */
	nb10_reg03_apply_enter();
	nb10_attr_preamble();
	pci_nb10_board_active = true;

	program_mode_754x();
	cl_select_crtc(cl_misc_read());
	cl_seq_write8(0x17, (uint8_t)(cl_seq_read(0x17) | 0x44));
	cl_seq_write8(0x02, (uint8_t)(cl_seq_read(0x02) | 0x0f));

	/*
	 * Use a static hardware pattern for C=1/2 and D=1/2.  It is fully
	 * resident before either final gate edge, so the visible calibration
	 * transition cannot be blamed on another CPU-source BLT.  The plain
	 * baseline (C=0, D=0) preserves the normal black clear.
	 */
	cl_blt_reset();
	if (nb10_gate_order_test == 0 && nb10_fach_ac_test == 0) {
		if (!cl_blt_fifo_clear_visible()) {
			nb10_nec_lock();
			return false;
		}
	} else {
		if (!cl_blt_fifo_pattern_visible()) {
			nb10_nec_lock();
			return false;
		}
	}

	if (nb10_gate_order_test == 1) {
		/*
		 * C=1: the GD7548 stream is live, but both final output controls
		 * remain hidden.  Waiting is deliberately passive: no status polling
		 * and no further BLT are performed.
		 */
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) & 0xbf));
		nb10_wait_key(
		    "CIRRUS-NB10 C=1 stage 1: SR12 is OPEN, FACh is 00h, "
		    "and AC is disabled. Wait at least 3 seconds, then press "
		    "one key to expose the preloaded pattern.");

		outp(PCI_RELAY_PORT, 0x02);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);
		nb10_attr_enable();
		outp(cdisp.io_3c0 + 0x06, 0xff);
		nb10_nec_lock();

		if (cdisp.bpp == 8)
			cl_load_palette();

		nb10_wait_key(
		    "CIRRUS-NB10 C=1 stage 2: pattern is now visible. "
		    "Observe whether it starts already in the attractor, or "
		    "starts pixel-perfect and then shows horizontal calibration "
		    "noise. Press one key to continue to the game.");
	} else if (nb10_gate_order_test == 2) {
		/*
		 * C=2: select the final relay and AC while the GD7548 stream is
		 * closed.  The screen should remain black until SR12 is reopened.
		 */
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) | 0x40));
		outp(PCI_RELAY_PORT, 0x02);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);
		nb10_attr_enable();
		outp(cdisp.io_3c0 + 0x06, 0xff);
		nb10_nec_lock();

		if (cdisp.bpp == 8)
			cl_load_palette();

		nb10_wait_key(
		    "CIRRUS-NB10 C=2 stage 1: FACh is 02h and AC is enabled, "
		    "but SR12 is CLOSED. Wait at least 3 seconds, then press "
		    "one key to reopen the GD7548 stream.");
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) & 0xbf));
		nb10_wait_key(
		    "CIRRUS-NB10 C=2 stage 2: SR12 is now OPEN. Observe "
		    "whether the pattern starts pixel-perfect and calibrates, "
		    "or appears immediately in the attractor. Press one key "
		    "to continue to the game.");
	} else if (nb10_fach_ac_test == 1) {
		/*
		 * D=1: raise the relay alone.  The AC stays disabled, so the
		 * panel should show blank/border content while any internal
		 * calibration triggered by FACh runs to completion unseen.
		 * The dwell is passive: no polling, no further BLT.
		 */
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) & 0xbf));
		outp(PCI_RELAY_PORT, 0x02);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);
		hal_log_info("CIRRUS-NB10 D=1: FACh edge done (reads %02Xh); "
		             "AC still disabled.", inp(PCI_RELAY_PORT));
		nb10_wait_key(
		    "CIRRUS-NB10 D=1 stage 1: SR12 is OPEN and FACh is 02h, "
		    "but AC is disabled. Wait at least 3 seconds, then press "
		    "one key to enable AC and expose the preloaded pattern.");

		nb10_attr_enable();
		outp(cdisp.io_3c0 + 0x06, 0xff);
		nb10_nec_lock();

		if (cdisp.bpp == 8)
			cl_load_palette();

		nb10_wait_key(
		    "CIRRUS-NB10 D=1 stage 2: AC is now enabled. If the "
		    "pattern is ALREADY in the 1+1 attractor, calibration ran "
		    "during the FACh-only dwell (FACh is the trigger). If it "
		    "starts pixel-perfect and then shows horizontal noise, the "
		    "AC edge (or both gates together) is the trigger. Press "
		    "one key to continue to the game.");
	} else if (nb10_fach_ac_test == 2) {
		/*
		 * D=2: enable the AC alone.  FACh is intentionally never
		 * written during the dwell - even an idempotent 00h write
		 * would add an edge on the port under test - so the panel
		 * keeps showing the 98-GDC side.  Its value is only read.
		 */
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) & 0xbf));
		nb10_attr_enable();
		outp(cdisp.io_3c0 + 0x06, 0xff);

		if (cdisp.bpp == 8)
			cl_load_palette();

		hal_log_info("CIRRUS-NB10 D=2: AC edge done; FACh untouched "
		             "(reads %02Xh).", inp(PCI_RELAY_PORT));
		nb10_wait_key(
		    "CIRRUS-NB10 D=2 stage 1: SR12 is OPEN and AC is enabled, "
		    "but FACh has not been written. Wait at least 3 seconds, "
		    "then press one key to switch FACh to 02h.");

		outp(PCI_RELAY_PORT, 0x02);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);
		nb10_nec_lock();

		nb10_wait_key(
		    "CIRRUS-NB10 D=2 stage 2: FACh is now 02h. If the "
		    "pattern is ALREADY in the 1+1 attractor, calibration ran "
		    "during the AC-only dwell (AC enable is the trigger). If "
		    "it starts pixel-perfect and then shows horizontal noise, "
		    "the FACh edge (or both gates together) is the trigger. "
		    "Press one key to continue to the game.");
	} else {
		/* C=0, D=0: current baseline order. */
		cl_seq_write8(0x12,
		              (uint8_t)(cl_seq_read(0x12) & 0xbf));
		outp(PCI_RELAY_PORT, 0x02);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);
		nb10_attr_enable();
		outp(cdisp.io_3c0 + 0x06, 0xff);
		nb10_nec_lock();

		if (nb10_modeset_checkpoint_test == 1 &&
		    !nb10_modeset_black_checkpoint(1))
			return false;

		/*
		 * F=1 starts the second full SET immediately: no key wait, no
		 * diagnostic read and (for 8bpp) no palette load is inserted between
		 * the first postlude and the second prelude.
		 */
		if (nb10_nt4_init_order_test == 1 &&
		    !nb10_nt4_second_set())
			return false;

		if (cdisp.bpp == 8)
			cl_load_palette();
	}

	return true;
}

static void
nb10_known_output_leave(void)
{
	unsigned long i;

	if (!pci_nb10_board_active)
		return;

	nb10_nec_unlock();
	cl_blt_reset();
	cl_seq_write8(0x12, (uint8_t)(cl_seq_read(0x12) | 0x40));
	outp(PC98_WAIT_PORT, 0);
	outp(VRAM_SW_PORT, 0x07);
	outp(VRAM_SW_PORT, 0x8e);
	outp(VRAM_SW_PORT, 0x06);
	nb10_reg03_apply_leave();
	outp(PCI_RELAY_PORT, 0x00);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0);
	outp(PC98_GDC_MODE_PORT, 0x0f);
	nb10_nec_lock();
	pci_nb10_board_active = false;
}

static bool
cirrus75_init(int mode, int req_bpp)
{
	uint32_t bar0, barsize, cmd;
	int w, h, bpp;

	if (!pci_find_cirrus(&pci_bus, &pci_dev, &pci_fn))
		return false;

	pci_nb10_active = (chip->dev == 0x0038);
	pci_nb10_board_active = false;
	if (pci_nb10_active) {
		nb10_reg03_configure();
		nb10_sr12_configure();
		nb10_gate_order_configure();
		nb10_fach_ac_configure();
		nb10_gate_cycle_configure();
		nb10_nt4_init_order_configure();
		nb10_modeset_checkpoint_configure();
	}

	hal_log_info("CIRRUS: %s found at PCI %d:%d.%d (dev ID %04Xh).",
	             chip->name, pci_bus, pci_dev, pci_fn, chip->dev);

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	if (chip->laptop) {
		if (mode != DISP_640X480) {
			hal_log_info("CIRRUS: %s: only 640x480 is implemented.",
			             chip->name);
			return false;
		}
	} else if (mode != DISP_640X480 && mode != DISP_800X600) {
		hal_log_info("CIRRUS: PCI desktop: %dx%d not supported.", w, h);
		return false;
	}

	bar0 = pci_read32(pci_bus, pci_dev, pci_fn, 0x10);
	barsize = pci_size_bar0();
	cmd = pci_read32(pci_bus, pci_dev, pci_fn, 0x04);
	pci_saved_command = cmd;
	hal_log_info("CIRRUS: BAR0=%08lXh (decode %luMB), cmd=%04lXh.",
	             (unsigned long)bar0,
	             (unsigned long)(barsize >> 20),
	             (unsigned long)(cmd & 0xffff));
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, cmd | 0x03);
	bar0 &= ~0xfUL;
	if (bar0 == 0)
		return false;

	cdisp.fb_phys = bar0 + chip->fb_offset;
	cdisp.vram_size = PCI_FB_LENGTH;
	cdisp.linear = true;
	cdisp.fifo_capable = !chip->laptop || pci_nb10_active;
	cdisp.fifo_only = pci_nb10_active ? true :
	                    (cdisp.fifo_capable && gd54_fifo_requested);

	cdisp.fb = (uint8_t *)cl_map_physical(cdisp.fb_phys,
	                                      cdisp.fifo_only ? 0x10000UL :
	                                                        cdisp.vram_size);
	if (cdisp.fb == NULL)
		return false;

	if (!probe_regbase()) {
		hal_log_info("CIRRUS: VGA registers not responding at native/relocated base.");
		cl_release_fb_mapping();
		return false;
	}

	cdisp.crt27 = (uint8_t)cl_crtc_read(0x27);
	save_state();
	cl_seq_write(0x01, sv_sr[0x01] | 0x20);
	probe_lcd();

	if (chip->laptop) {
		bpp = (req_bpp == -1) ? chip->def_bpp : req_bpp;
		if (bpp != 8 && bpp != 16 && bpp != 24)
			goto fail;
	} else {
		bpp = cl_resolve_bpp(req_bpp, 24, w, h, cdisp.vram_size,
		                     "PCI");
		if (bpp < 0)
			goto fail;
	}

	cdisp.scr_w = w;
	cdisp.scr_h = h;
	cdisp.bpp = bpp;
	if (chip->laptop)
		cdisp.pitch = bpp == 8 ? M8_PITCH :
		              (bpp == 24 ? M24_PITCH : M16_PITCH);
	else
		cdisp.pitch = (uint32_t)w * (uint32_t)(bpp / 8);

	if (pci_nb10_active) {
		hal_log_info("CIRRUS-NB10: canonical 6Ah/FAC route with "
		             "FAA/FAB reg03 selected independently by T.");
		if (!nb10_known_output_enter())
			goto fail;
	} else {
		if (chip->laptop)
			program_mode_754x();
		else
			cl_modeset_generic(false);

		if (cdisp.fifo_capable) {
			cl_blt_reset();
			if (cdisp.fifo_only) {
				if (!cl_blt_fifo_clear_visible())
					goto fail;
			} else if (!cl_aperture_clear_visible()) {
				goto fail;
			}
		} else {
			memset(cdisp.fb, 0, cdisp.vram_size - 0x100);
		}

		cl_seq_write(0x01, 0x01);
		relay_to_accel();
	}

	cdisp.chip_name = chip->name;
	cdisp.path = CIRRUS_PATH_75;
	return true;

fail:
	if (pci_nb10_board_active)
		nb10_known_output_leave();
	restore_state();
	if (ext_was_locked)
		cl_seq_write(0x06, 0x0f);
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, pci_saved_command);
	cl_release_fb_mapping();
	pci_nb10_active = false;
	nb10_sr12_test_done = false;
	nb10_gate_cycle_done = false;
	nb10_gate_cycle_armed = false;
	nb10_modeset_checkpoint_test = 0;
	return false;
}

static void
cirrus75_cleanup(void)
{
	cl_seq_write(0x01, 0x21);

	if (pci_nb10_active)
		nb10_known_output_leave();
	else
		relay_to_gdc();

	restore_state();
	if (ext_was_locked)
		cl_seq_write(0x06, 0x0f);
	pci_write32(pci_bus, pci_dev, pci_fn, 0x04, pci_saved_command);
	cl_release_fb_mapping();
	pci_nb10_active = false;
	pci_nb10_board_active = false;
	nb10_sr12_test_done = false;
	nb10_gate_cycle_done = false;
	nb10_gate_cycle_armed = false;
	nb10_modeset_checkpoint_test = 0;
}
