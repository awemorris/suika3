/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * NEC PC-9821 Graphics
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
 * PC-9821 Graphics Architecture: WAB vs. PCI/BAR Access
 * ===========================================================================
 *
 * PC-9821 accelerators are classified by BOTH the graphics chip and
 * the motherboard-side mechanism. The useful architectural classes
 * are:
 *
 * - WAB S3
 *     - WAB is the name of a CPU-direct local-bus slot.
 *     - It has a I/O port to setup the Linear Frame Buffer (LFB).
 *     - SVGA chips are S3 86C928 and Vision864.
 *     - Desktop only.
 * - WAB Emulation Cirrus
 *     - WAB-compatible, but actually not an original local-bus.
 *     - WAB emulator translates WAB I/O access to PCI VGA via PCI.
 *     - SVGA chips are Cirrus Logic GD5428 and GD5430.
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
 * | Y/M     | Series         | Models        | Video Chipset       | Connection        | Driver          |
 * |---------|----------------|---------------|---------------------|-------------------|-----------------|
 * | 1993/02 | MATE A         | Ae/M2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | Ae/M7         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | Ap/U2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | As/U2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/08 | MATE A         | Af/U9W        | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993    | MATE A         | Ap2           | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993    | MATE A         | As2           | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1994/05 | MATE A         | An/U2         | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE A         | Ap3           | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE A         | As3           | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xn            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xs            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xp            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xf            | Matrox MGA-II       | WAB Local-Bus     | WAB             |
 * | 1993    | MATE B         | Be            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1993    | MATE B         | Bp            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1993    | MATE B         | Bs            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1994    | CanBe          | Cb            | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1994    | CanBe          | Cx            | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | CanBe          | Cx2           | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | CanBe          | Cb2           | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | MATE X         | Xe10          | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1994    | 98NOTE         | Ne            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nf            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Np            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Ns            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nx            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nd            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Ne2           | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1995/11 | CanBe          | Cb3           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995/11 | CanBe          | Cx3           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | ValueStar      | V7            | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V13           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V20           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | CanBe          | Cu10          | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | CanBe          | Ct16          | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | ValueStar      | V10           | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V12           | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997    | ValueStar      | V16/S5P       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997/02 | MATE X         | Xc13/S5       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997/02 | MATE X         | Xc16/M7       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | 98NOTE         | Nb7           | Cirrus Logic GD7543 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Nb10          | Cirrus Logic GD7548 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Na13          | Cirrus Logic GD7548 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Ls12          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Nr12          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Nr13          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Ls13          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Ls150         | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1995    | MATE X         | Xt13          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xv13          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xt16          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xv20          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1997    | MATE R         | Rv20          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V166          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V200          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V233          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1995    | 98NOTE         | Nx            | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Nd2           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Lt2           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Ne3           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Na7           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Na9           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1996    | 98NOTE         | Na12          | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1996    | 98NOTE         | Na15          | Trident Cyber9382   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr15          | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr150         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr166         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nw133         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nw150         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1998    | 98NOTE         | Nr233         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1998    | 98NOTE         | Nr266         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1999    | 98NOTE         | Nr300         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1995    | CanBe          | Cu13          | Trident TGUI9685    | PCI               | PCI Trident     |
 * | 1995    | CanBe          | Ct20          | Trident TGUI9685    | PCI               | PCI Trident     |
 * | 1995    | MATE X         | Xa7           | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1995    | MATE X         | Xa10          | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1996    | MATE X         | Xa12          | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1996    | MATE X         | Xa13          | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1996    | MATE X         | Xa16          | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1997/07 | MATE X         | Xc13/M7       | Trident TGUI9680XGi | PCI               | PCI Trident     |
 * | 1996    | ValueStar      | V16/M7        | Trident TGUI9682XGi | PCI               | PCI Trident     |
 * | 1997    | MATE R         | Ra266         | Trident TGUI9682XGi | PCI               | PCI Trident     |
 * | 1998    | MATE R         | Ra300         | Trident TGUI9682XGi | PCI               | PCI Trident     |
 * | 1998    | MATE R         | Ra333         | Trident TGUI9682XGi | PCI               | PCI Trident     |
 * | 2000    | MATE R         | Ra43          | Trident TGUI9682XGi | PCI               | PCI Trident     |
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


#include <strato/strato.h>
#include "98disp.h"

#include <stdio.h>
#include <string.h>

#include <dos.h>
#include <conio.h>
#include <i86.h>

/* ------------------------------------------------------------------------- */
/* Public framebuffer source                                                  */
/* ------------------------------------------------------------------------- */

extern struct hal_image *back_image;
extern int game_width;
extern int game_height;

/* ------------------------------------------------------------------------- */
/* Constants                                                                  */
/* ------------------------------------------------------------------------- */

#define PCI_CONFIG_ADDR		0x0cf8
#define PCI_CONFIG_DATA		0x0cfc

#define PCI_VENDOR_NEC		0x1033
#define PCI_DEVICE_CGB		0x0009

#define PCI_VENDOR_CIRRUS	0x1013
#define PCI_DEVICE_GD7548	0x0038

#define WAB_INDEX		0x0faa
#define WAB_DATA		0x0fab
#define WAB_REG_ID		0x00
#define WAB_REG_LINEAR		0x02
#define WAB_REG_RELAY		0x03

#define CORE_IO_3C0		0x0ca0
#define CORE_IO_3D4		0x0da4
#define CORE_IO_3DA		0x0daa
#define CORE_IO_3B4		0x0ba4
#define CORE_IO_3BA		0x0baa
#define CORE_SLEEP		0x0ca3

#define NATIVE_IO_3C0		0x03c0
#define NATIVE_IO_3D4		0x03d4
#define NATIVE_IO_3DA		0x03da
#define NATIVE_IO_3B4		0x03b4
#define NATIVE_IO_3BA		0x03ba
#define NATIVE_SLEEP		0x03c3

#define PC98_WAIT_PORT		0x005f
#define PC98_GDC_MODE_PORT	0x0068
#define PC98_VRAM_SW_PORT	0x006a

#define NB10_NEC_INDEX		0x08f0
#define NB10_NEC_DATA		0x08f2
#define NB10_RELAY		0x0fac

/*
 * Family-40h post-mode access block.  The mode command stream uses native
 * VGA ports, while the recovered miniport postlude uses these relocated
 * sequencer/control ports.
 */
#define NB10_CTL_INDEX		0x04b0
#define NB10_CTL_DATA		0x04b1
#define NB10_SEQ_INDEX		0x04b4
#define NB10_SEQ_DATA		0x04b5

#define FRAMEBUFFER_BYTES	0x00100000UL
#define NB10_FB_OFFSET		0x00c00000UL
#define CORE_FB_PHYS		0xf0000000UL

#define CL_BLT_BUSY		0x01
#define CL_BLT_START		0x02
#define CL_BLT_RESET		0x04
#define CL_BLT_MEMSYS_SRC	0x04
#define CL_BLT_ROP_SRC		0x0d

#define PACK565(p) \
	((((p) >> 8) & 0xf800U) | (((p) >> 5) & 0x07e0U) | \
	 (((p) >> 3) & 0x001fU))

#define PACK332(p) \
	((((p) >> 16) & 0xe0U) | (((p) >> 11) & 0x1cU) | \
	 (((p) >> 6) & 0x03U))

/* ------------------------------------------------------------------------- */
/* Driver state                                                               */
/* ------------------------------------------------------------------------- */

enum cirrus_route {
	CIRRUS_ROUTE_NONE = 0,
	CIRRUS_ROUTE_COREGRAPH,
	CIRRUS_ROUTE_NB10
};

struct pci_location {
	bool found;
	int bus;
	int dev;
	int fn;
	uint32_t id;
	uint32_t bar0;
	uint32_t saved_command;
};

struct cirrus_state {
	enum cirrus_route route;
	const char *name;

	int width;
	int height;
	int bpp;
	uint32_t pitch;

	uint16_t io_3c0;
	uint16_t io_crtc;
	uint16_t io_status;
	uint16_t io_crtc_color;
	uint16_t io_status_color;
	uint16_t io_crtc_mono;
	uint16_t io_status_mono;

	uint8_t *fb;
	uint32_t fb_phys;
	uint32_t vram_bytes;

	bool fifo_only;

	uint8_t fixed_id;
	uint8_t crt27;

	int ofs_x;
	int ofs_y;
	int draw_w;
	int draw_h;

	/* V13/Core-Graph board state. */
	uint8_t core_saved_sleep;
	uint8_t core_saved_relay;
	uint8_t core_saved_linear;
	bool core_board_saved;
	bool core_entered;

	/* Nb10 PCI/board state. */
	struct pci_location nb10;
	bool nb10_entered;
	bool nb10_extensions_were_locked;
};

static struct cirrus_state cs;

/* ------------------------------------------------------------------------- */
/* Forward declarations                                                       */
/* ------------------------------------------------------------------------- */

struct regpair {
	uint8_t index;
	uint8_t value;
};

static void cl_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t stc,
			  uint16_t d4m, uint16_t stm);
static void cl_select_crtc(int misc);
static void cl_seq_write(int reg, int value);
static void cl_seq_write8(int reg, int value);
static int cl_seq_read(int reg);
static void cl_gfx_write(int reg, int value);
static int cl_gfx_read(int reg);
static void cl_crtc_write(int reg, int value);
static int cl_crtc_read(int reg);
static void cl_misc_write(int value);
static int cl_misc_read(void);
static void cl_hidden_dac_write(int value);
static void cl_load_palette(void);

static void *map_physical(uint32_t phys, uint32_t bytes);
static bool unmap_physical(void *linear);
static void release_mapping(void);

static bool coregraph_detect(void);
static bool coregraph_init(int requested_bpp);
static void coregraph_cleanup(void);
static void coregraph_mode_set(void);

static bool nb10_detect(const struct pci_location *gd7548,
			 const struct pci_location *cgb);
static bool nb10_init(int requested_bpp);
static void nb10_cleanup(void);
static bool nb10_mode_set(void);

static bool fifo_present(void);
static bool direct_present(void);

/* ------------------------------------------------------------------------- */
/* Basic VGA/Cirrus register access                                           */
/* ------------------------------------------------------------------------- */

static void
cl_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t stc,
	      uint16_t d4m, uint16_t stm)
{
	cs.io_3c0 = b3c0;
	cs.io_crtc_color = d4c;
	cs.io_status_color = stc;
	cs.io_crtc_mono = d4m;
	cs.io_status_mono = stm;
	cs.io_crtc = d4c;
	cs.io_status = stc;
}

static void
cl_select_crtc(int misc)
{
	if (misc & 1) {
		cs.io_crtc = cs.io_crtc_color;
		cs.io_status = cs.io_status_color;
	} else {
		cs.io_crtc = cs.io_crtc_mono;
		cs.io_status = cs.io_status_mono;
	}
}

/* Command-stream writes are single 16-bit index+data cycles. */
static void
cl_seq_write(int reg, int value)
{
	outpw(cs.io_3c0 + 4,
	     (uint16_t)(((uint16_t)(value & 0xff) << 8) | (uint16_t)(reg & 0xff)));
}

/* RMW sites in CIRRUS.SYS use separate 8-bit index/data cycles. */
static void
cl_seq_write8(int reg, int value)
{
	outp(cs.io_3c0 + 4, reg);
	outp(cs.io_3c0 + 5, value);
}

static int
cl_seq_read(int reg)
{
	outp(cs.io_3c0 + 4, reg);
	return inp(cs.io_3c0 + 5);
}

static void
cl_gfx_write(int reg, int value)
{
	outpw(cs.io_3c0 + 0x0e,
	      (uint16_t)(((uint16_t)(value & 0xff) << 8) |
			 (uint16_t)(reg & 0xff)));
}

static int
cl_gfx_read(int reg)
{
	outp(cs.io_3c0 + 0x0e, reg);
	return inp(cs.io_3c0 + 0x0f);
}

static void
cl_crtc_write(int reg, int value)
{
	outpw(cs.io_crtc,
	      (uint16_t)(((uint16_t)(value & 0xff) << 8) |
			 (uint16_t)(reg & 0xff)));
}

static int
cl_crtc_read(int reg)
{
	outp(cs.io_crtc, reg);
	return inp(cs.io_crtc + 1);
}

static void
cl_misc_write(int value)
{
	outp(cs.io_3c0 + 2, value);
	cl_select_crtc(value);
}

static int
cl_misc_read(void)
{
	return inp(cs.io_3c0 + 0x0c);
}

/*
 * GD7548 does not reliably disarm the hidden-DAC counter after a read.
 * This driver only needs writes in the production path, so every write starts
 * from a known counter state using the DAC write-index port.
 */
static void
cl_hidden_dac_write(int value)
{
	(void)inp(cs.io_3c0 + 8);
	(void)inp(cs.io_3c0 + 6);
	(void)inp(cs.io_3c0 + 6);
	(void)inp(cs.io_3c0 + 6);
	(void)inp(cs.io_3c0 + 6);
	outp(cs.io_3c0 + 6, value);
}

static void
cl_load_palette(void)
{
	int i;

	outp(cs.io_3c0 + 6, 0xff);
	outp(cs.io_3c0 + 8, 0);

	if (cs.bpp == 8) {
		for (i = 0; i < 256; i++) {
			int r = (i >> 5) & 7;
			int g = (i >> 2) & 7;
			int b = i & 3;

			outp(cs.io_3c0 + 9, r * 63 / 7);
			outp(cs.io_3c0 + 9, g * 63 / 7);
			outp(cs.io_3c0 + 9, b * 63 / 3);
		}
	} else {
		for (i = 0; i < 256; i++) {
			outp(cs.io_3c0 + 9, i >> 2);
			outp(cs.io_3c0 + 9, i >> 2);
			outp(cs.io_3c0 + 9, i >> 2);
		}
	}
}

/* ------------------------------------------------------------------------- */
/* DPMI physical mapping                                                      */
/* ------------------------------------------------------------------------- */

static void *
map_physical(uint32_t phys, uint32_t bytes)
{
	union REGS r;

	if (phys < 0x100000UL)
		return (void *)phys;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0800;
	r.w.bx = (uint16_t)(phys >> 16);
	r.w.cx = (uint16_t)phys;
	r.w.si = (uint16_t)(bytes >> 16);
	r.w.di = (uint16_t)bytes;
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return NULL;

	return (void *)(((uint32_t)r.w.bx << 16) | r.w.cx);
}

static bool
unmap_physical(void *linear)
{
	union REGS r;
	uint32_t address = (uint32_t)linear;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0801;
	r.w.bx = (uint16_t)(address >> 16);
	r.w.cx = (uint16_t)address;
	int386(0x31, &r, &r);
	return !r.w.cflag;
}

static void
release_mapping(void)
{
	if (cs.fb == NULL)
		return;

	if (cs.fb_phys >= 0x100000UL)
		(void)unmap_physical(cs.fb);

	cs.fb = NULL;
	cs.fb_phys = 0;
}

/* ------------------------------------------------------------------------- */
/* PCI scan                                                                   */
/* ------------------------------------------------------------------------- */

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
pci_write32(int bus, int dev, int fn, int reg, uint32_t value)
{
	outpd(PCI_CONFIG_ADDR,
	      0x80000000UL |
	      ((uint32_t)bus << 16) |
	      ((uint32_t)dev << 11) |
	      ((uint32_t)fn << 8) |
	      ((uint32_t)reg & 0xfc));
	outpd(PCI_CONFIG_DATA, value);
}

static void
pci_scan(struct pci_location *gd7548, struct pci_location *cgb)
{
	int bus, dev, fn, nfn;
	uint32_t id, hdr;

	memset(gd7548, 0, sizeof(*gd7548));
	memset(cgb, 0, sizeof(*cgb));

	for (bus = 0; bus < 4; bus++) {
		for (dev = 0; dev < 32; dev++) {
			id = pci_read32(bus, dev, 0, 0);
			if ((id & 0xffff) == 0xffff || (id & 0xffff) == 0)
				continue;

			hdr = pci_read32(bus, dev, 0, 0x0c);
			nfn = (hdr & 0x00800000UL) ? 8 : 1;

			for (fn = 0; fn < nfn; fn++) {
				uint16_t vendor, device;

				id = pci_read32(bus, dev, fn, 0);
				vendor = (uint16_t)id;
				device = (uint16_t)(id >> 16);
				if (vendor == 0xffff || vendor == 0)
					continue;

				if (!cgb->found &&
				    vendor == PCI_VENDOR_NEC &&
				    device == PCI_DEVICE_CGB) {
					cgb->found = true;
					cgb->bus = bus;
					cgb->dev = dev;
					cgb->fn = fn;
					cgb->id = id;
				}

				if (!gd7548->found &&
				    vendor == PCI_VENDOR_CIRRUS &&
				    device == PCI_DEVICE_GD7548) {
					gd7548->found = true;
					gd7548->bus = bus;
					gd7548->dev = dev;
					gd7548->fn = fn;
					gd7548->id = id;
					gd7548->bar0 =
					    pci_read32(bus, dev, fn, 0x10);
					gd7548->saved_command =
					    pci_read32(bus, dev, fn, 0x04);
				}
			}
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Shared mode helpers                                                        */
/* ------------------------------------------------------------------------- */

static int
resolve_bpp(int requested, int default_bpp)
{
	if (requested == -1)
		return default_bpp;
	if (requested == 8 || requested == 16 || requested == 24)
		return requested;
	return -1;
}

static uint32_t
pitch_for_bpp(int bpp)
{
	if (bpp == 8)
		return 640;
	if (bpp == 16)
		return 1280;
	return 2048;
}

static bool
clear_visible_dwords(void)
{
	volatile uint32_t *dst;
	uint32_t bytes, dwords, i;

	if (cs.fb == NULL)
		return false;

	bytes = cs.pitch * (uint32_t)cs.height;
	if (bytes > cs.vram_bytes)
		return false;

	/*
	 * V13 places MMIO in the final 256 bytes of the 1MB window.  All three
	 * 640x480 surfaces end below that area.
	 */
	if (cs.route == CIRRUS_ROUTE_COREGRAPH &&
	    bytes > cs.vram_bytes - 0x100)
		return false;

	dst = (volatile uint32_t *)cs.fb;
	dwords = bytes / 4;
	for (i = 0; i < dwords; i++)
		dst[i] = 0;

	return true;
}

static bool
clear_nb10_nt4_vram(void)
{
	volatile uint32_t *dst32;
	volatile uint8_t *dst8;
	uint32_t bytes, dwords, i;
	uint8_t sr02;

	if (cs.fb == NULL || cs.vram_bytes < 4)
		return false;

	/*
	 * sub_1B458: use the family-40h relocated sequencer access and clear
	 * VideoMemorySize-1 bytes before switching FAC to the accelerator.
	 */
	outp(NB10_SEQ_INDEX, 0x02);
	sr02 = (uint8_t)inp(NB10_SEQ_DATA);
	outp(NB10_SEQ_DATA, sr02 | 0x0f);

	bytes = cs.vram_bytes - 1;
	dwords = bytes / 4;
	dst32 = (volatile uint32_t *)cs.fb;
	for (i = 0; i < dwords; i++)
		dst32[i] = 0;

	dst8 = (volatile uint8_t *)cs.fb;
	for (i = dwords * 4; i < bytes; i++)
		dst8[i] = 0;

	return true;
}

/* ------------------------------------------------------------------------- */
/* V13 / path-08 Core-Graph route                                             */
/* ------------------------------------------------------------------------- */

static void
wab_write(int reg, int value)
{
	outp(WAB_INDEX, reg);
	outp(WAB_DATA, value);
}

static int
wab_read(int reg)
{
	outp(WAB_INDEX, reg);
	return inp(WAB_DATA);
}

static bool
coregraph_id(uint8_t id)
{
	return id >= 0x58 && id <= 0x5d;
}

/*
 * Validate the fixed-interface ID against the relocated Cirrus block.
 * The temporary wake/access changes are restored before returning.
 */
static bool
coregraph_detect(void)
{
	uint8_t id, sleep, relay, sr06, cr27;
	int misc;

	id = (uint8_t)wab_read(WAB_REG_ID);
	if (!coregraph_id(id))
		return false;

	cl_set_iobase(CORE_IO_3C0, CORE_IO_3D4, CORE_IO_3DA,
		      CORE_IO_3B4, CORE_IO_3BA);

	sleep = (uint8_t)inp(CORE_SLEEP);
	relay = (uint8_t)wab_read(WAB_REG_RELAY);

	outp(CORE_SLEEP, sleep | 1);
	wab_write(WAB_REG_RELAY, (relay & ~2) | 1);

	misc = cl_misc_read();
	cl_select_crtc(misc);

	sr06 = (uint8_t)cl_seq_read(0x06);
	cl_seq_write(0x06, 0x12);
	if ((uint8_t)cl_seq_read(0x06) == 0x12)
		cr27 = (uint8_t)cl_crtc_read(0x27);
	else
		cr27 = 0xff;

	cl_seq_write(0x06, sr06);
	wab_write(WAB_REG_RELAY, relay);
	outp(CORE_SLEEP, sleep);

	if (cr27 == 0 || cr27 == 0xff)
		return false;

	cs.fixed_id = id;
	cs.crt27 = cr27;
	return true;
}

static void
coregraph_gate_enter(void)
{
	outp(PC98_GDC_MODE_PORT, 0x0e);
	outp(PC98_VRAM_SW_PORT, 0x07);
	outp(PC98_VRAM_SW_PORT, 0x8f);
	outp(PC98_VRAM_SW_PORT, 0x06);
	wab_write(WAB_REG_RELAY, 0x03);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_WAIT_PORT, 0);
	outp(CORE_SLEEP, 0x01);
	cs.core_entered = true;
}

static void
coregraph_gate_leave(void)
{
	unsigned long i;

	if (!cs.core_entered)
		return;

	outp(CORE_SLEEP, 0x00);
	wab_write(WAB_REG_RELAY, 0x00);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_VRAM_SW_PORT, 0x07);
	outp(PC98_VRAM_SW_PORT, 0x8e);
	outp(PC98_VRAM_SW_PORT, 0x06);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0);
	outp(PC98_GDC_MODE_PORT, 0x0f);

	cs.core_entered = false;
}

/*
 * Verified path-08h mode stream used by V13-class GD5430/GD5440 machines.
 */
static void
coregraph_mode_set(void)
{
	static const uint8_t seq_index[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x08,
		0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x16, 0x18,
		0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	static const uint8_t seq8[] = {
		0x01,0x01,0x0f,0x00,0x0e,0x11,0x00,
		0x66,0x48,0x56,0x60,0x30,0x58,0x40,
		0x3b,0x23,0x3d,0x3b,0x20
	};
	static const uint8_t seq16[] = {
		0x01,0x01,0x0f,0x00,0x0e,0x13,0x00,
		0x6d,0x48,0x56,0x60,0x30,0x58,0x40,
		0x3e,0x23,0x3d,0x3b,0x20
	};
	static const uint8_t seq24[] = {
		0x01,0x01,0x0f,0x00,0x0e,0x15,0x00,
		0x3a,0x48,0x56,0x60,0x30,0x58,0x40,
		0x16,0x23,0x3d,0x3b,0x20
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
	uint8_t sr0f;

	if (cs.bpp == 8) {
		seq = seq8;
		crtc = crtc8;
		hdr = 0x20;
	} else if (cs.bpp == 16) {
		seq = seq16;
		crtc = crtc16;
		hdr = 0xe1;
	} else {
		seq = seq24;
		crtc = crtc24;
		hdr = 0xe5;
	}

	cl_gfx_write(0x33, 0);
	cl_gfx_write(0x31, CL_BLT_RESET);
	cl_gfx_write(0x31, 0);

	cl_seq_write(0x06, 0x12);
	cl_seq_write(0x12, 0);
	for (i = 0; i < (int)sizeof(seq_index); i++)
		cl_seq_write(seq_index[i], seq[i]);

	sr0f = (uint8_t)cl_seq_read(0x0f);
	cl_seq_write8(0x0f, (sr0f & 0xdf) | 0x20);

	cl_misc_write(0xe3);
	cl_gfx_write(0x06, 0x05);
	cl_seq_write(0x00, 0x03);

	cl_crtc_write(0x11, 0x20);
	for (i = 0; i < 0x1c; i++)
		cl_crtc_write(i, crtc[i]);

	for (i = 0; i < 9; i++)
		cl_gfx_write(i, gfx[i]);

	(void)inp(cs.io_status);
	for (i = 0; i < 21; i++) {
		outp(cs.io_3c0, i);
		outp(cs.io_3c0, attr[i]);
	}
	(void)inp(cs.io_status);
	outp(cs.io_3c0, 0x20);

	cl_hidden_dac_write(hdr);
	outp(cs.io_3c0 + 6, 0xff);

	cl_gfx_write(0x09, 0);
	cl_gfx_write(0x0a, 0);
	cl_gfx_write(0x0b, 0x21);

	cl_seq_write8(0x17, cl_seq_read(0x17) | 0x44);
	cl_seq_write8(0x18, cl_seq_read(0x18) & 0xbf);

	cl_gfx_write(0x31, CL_BLT_RESET);
	cl_gfx_write(0x31, 0);

	cl_load_palette();

	/* Keep the screen blank while VRAM is cleared. */
	cl_seq_write(0x01, 0x21);
}

static bool
coregraph_init(int requested_bpp)
{
	int bpp;

	bpp = resolve_bpp(requested_bpp, 24);
	if (bpp < 0)
		return false;

	cs.route = CIRRUS_ROUTE_COREGRAPH;
	cs.name = "Cirrus GD5430/GD5440 on NEC Core-Graph";
	cs.width = 640;
	cs.height = 480;
	cs.bpp = bpp;
	cs.pitch = pitch_for_bpp(bpp);
	cs.vram_bytes = FRAMEBUFFER_BYTES;
	cs.fifo_only = false;

	cl_set_iobase(CORE_IO_3C0, CORE_IO_3D4, CORE_IO_3DA,
		      CORE_IO_3B4, CORE_IO_3BA);

	cs.core_saved_sleep = (uint8_t)inp(CORE_SLEEP);
	cs.core_saved_relay = (uint8_t)wab_read(WAB_REG_RELAY);
	cs.core_saved_linear = (uint8_t)wab_read(WAB_REG_LINEAR);
	cs.core_board_saved = true;

	/* Register access before opening the linear window. */
	outp(CORE_SLEEP, cs.core_saved_sleep | 1);
	wab_write(WAB_REG_RELAY, (cs.core_saved_relay & ~2) | 1);
	cl_select_crtc(cl_misc_read());
	cl_seq_write(0x06, 0x12);
	if ((uint8_t)cl_seq_read(0x06) != 0x12)
		goto fail;

	wab_write(WAB_REG_LINEAR, 0xf0);
	if ((uint8_t)wab_read(WAB_REG_LINEAR) != 0xf0)
		goto fail;

	cs.fb_phys = CORE_FB_PHYS;
	cs.fb = (uint8_t *)map_physical(cs.fb_phys, cs.vram_bytes);
	if (cs.fb == NULL)
		goto fail;

	coregraph_gate_enter();
	coregraph_mode_set();

	if (!clear_visible_dwords())
		goto fail;

	cl_seq_write(0x01, 0x01);
	cs.crt27 = (uint8_t)cl_crtc_read(0x27);

	return true;

fail:
	coregraph_cleanup();
	return false;
}

static void
coregraph_cleanup(void)
{
	if (cs.route != CIRRUS_ROUTE_COREGRAPH &&
	    !cs.core_board_saved && !cs.core_entered)
		return;

	/* Keep register access while blanking and stopping the BLT engine. */
	if (cs.io_3c0 != 0) {
		cl_gfx_write(0x31, CL_BLT_RESET);
		cl_gfx_write(0x31, 0);
		cl_seq_write(0x01, 0x21);
	}

	coregraph_gate_leave();

	if (cs.core_board_saved) {
		wab_write(WAB_REG_LINEAR, cs.core_saved_linear);
		wab_write(WAB_REG_RELAY, cs.core_saved_relay);
		outp(CORE_SLEEP, cs.core_saved_sleep);
		cs.core_board_saved = false;
	}

	release_mapping();
}

/* ------------------------------------------------------------------------- */
/* Nb10 / GD7548 model-0Eh route                                              */
/* ------------------------------------------------------------------------- */

static void
nb10_seq_write(int reg, int value)
{
	outp(NB10_SEQ_INDEX, reg);
	outp(NB10_SEQ_DATA, value);
}

static int
nb10_seq_read(int reg)
{
	outp(NB10_SEQ_INDEX, reg);
	return inp(NB10_SEQ_DATA);
}

static void
nb10_nec_unlock(void)
{
	uint16_t value;

	outpw(NB10_NEC_INDEX, 0x0052);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_WAIT_PORT, 0);
	value = (uint16_t)inpw(NB10_NEC_DATA);
	outpw(NB10_NEC_DATA, value | 0x0080);

	outpw(NB10_NEC_INDEX, 0x0060);
	value = (uint16_t)inpw(NB10_NEC_DATA);
	outpw(NB10_NEC_DATA, value & 0xffef);
}

static void
nb10_nec_lock(void)
{
	uint16_t value;

	outpw(NB10_NEC_INDEX, 0x0060);
	value = (uint16_t)inpw(NB10_NEC_DATA);
	outpw(NB10_NEC_DATA, value | 0x0010);

	outpw(NB10_NEC_INDEX, 0x0052);
	outp(PC98_WAIT_PORT, 0);
	outp(PC98_WAIT_PORT, 0);
	value = (uint16_t)inpw(NB10_NEC_DATA);
	outpw(NB10_NEC_DATA, value & 0xff7f);
}

static void
nb10_attr_preamble(void)
{
	int cr24, value;

	outp(cs.io_crtc, 0x24);
	cr24 = inp(cs.io_crtc + 1);
	if (cr24 & 0x80) {
		value = inp(NB10_CTL_DATA);
		outp(NB10_CTL_INDEX, value);
	}
	outp(NB10_CTL_INDEX, 0x31);
	outp(NB10_CTL_INDEX, 0x00);
	outp(NB10_CTL_INDEX, 0x00);
}

static void
nb10_attr_enable(void)
{
	int cr24, value;

	outp(cs.io_crtc, 0x24);
	cr24 = inp(cs.io_crtc + 1);
	if (cr24 & 0x80) {
		value = inp(NB10_CTL_DATA);
		outp(NB10_CTL_INDEX, value);
	}
	outp(NB10_CTL_INDEX, 0x20);
}

static void
stream_seq(const struct regpair *stream, int count)
{
	int i;

	for (i = 0; i < count; i++)
		cl_seq_write(stream[i].index, stream[i].value);
}

static void
stream_crtc(const struct regpair *stream, int count)
{
	int i;

	for (i = 0; i < count; i++)
		cl_crtc_write(stream[i].index, stream[i].value);
}

static void
stream_gfx(const struct regpair *stream, int count)
{
	int i;

	for (i = 0; i < count; i++)
		cl_gfx_write(stream[i].index, stream[i].value);
}

/* 640x480x8, NT4 family-40h/model-0Eh. */
static const struct regpair nb10_8_seq[] = {
	{0x06,0x12},{0x12,0x00},{0x00,0x01},{0x01,0x01},
	{0x02,0x0f},{0x03,0x00},{0x04,0x0e},{0x07,0xc1},
	{0x0f,0x31},{0x16,0xf3},{0x1f,0x23},{0x21,0x08},
	{0x25,0x04},{0x2a,0x00},{0x2b,0x80},{0x2c,0x00},
	{0x2d,0x00},{0x2e,0x08},{0x2f,0x02},{0x0b,0x66},
	{0x0c,0x53},{0x0d,0x5f},{0x0e,0x6e},{0x1b,0x3b},
	{0x1c,0x30},{0x1d,0x23},{0x1e,0x2a}
};
static const struct regpair nb10_8_crtc[] = {
	{0x11,0x20},{0x00,0x5f},{0x01,0x4f},{0x02,0x50},
	{0x03,0x82},{0x04,0x55},{0x05,0x9f},{0x06,0x0b},
	{0x07,0x3e},{0x08,0x00},{0x09,0x40},{0x0a,0x00},
	{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},{0x0e,0x00},
	{0x0f,0x00},{0x10,0xe5},{0x11,0xa7},{0x12,0xdf},
	{0x13,0x50},{0x14,0x00},{0x15,0xe7},{0x16,0x04},
	{0x17,0xe3},{0x18,0xff},{0x19,0x00},{0x1a,0x00},
	{0x1b,0x02},{0x1d,0x10},{0x1e,0x21},{0x1f,0x00},
	{0x20,0x62},{0x21,0x00},{0x23,0x10},{0x2c,0xc3},
	{0x2e,0x00},{0x30,0x00},{0x3c,0x00},{0x40,0xc0},
	{0x41,0x00},{0x42,0x00},{0x43,0x02},{0x44,0xa5},
	{0x47,0xa2},{0x48,0x10},{0x49,0x00},{0x4a,0xdf},
	{0x4b,0x00},{0x4c,0x00},{0x4d,0x66},{0x4e,0x40},
	{0x2d,0x80},{0x02,0x00},{0x03,0xcc},{0x04,0xe5},
	{0x05,0xec},{0x06,0x15},{0x07,0x8d},{0x08,0x00},
	{0x09,0x02},{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},
	{0x0e,0x00},{0x2d,0x11}
};

/* 640x480x16. */
static const struct regpair nb10_16_seq[] = {
	{0x06,0x12},{0x12,0x00},{0x00,0x01},{0x01,0x01},
	{0x02,0x0f},{0x03,0x00},{0x04,0x0e},{0x07,0xc3},
	{0x0f,0x31},{0x16,0xf7},{0x1f,0x23},{0x21,0x08},
	{0x25,0x04},{0x2a,0x00},{0x2b,0x80},{0x2c,0x00},
	{0x2d,0x00},{0x2e,0x08},{0x2f,0x02},{0x0b,0x66},
	{0x0c,0x53},{0x0d,0x5f},{0x0e,0x66},{0x1b,0x3b},
	{0x1c,0x30},{0x1d,0x23},{0x1e,0x3a}
};
static const struct regpair nb10_16_crtc[] = {
	{0x11,0x20},{0x00,0x5f},{0x01,0x4f},{0x02,0x50},
	{0x03,0x82},{0x04,0x54},{0x05,0x9e},{0x06,0x0b},
	{0x07,0x3e},{0x08,0x00},{0x09,0x40},{0x0a,0x00},
	{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},{0x0e,0x00},
	{0x0f,0x00},{0x10,0xe5},{0x11,0xa7},{0x12,0xdf},
	{0x13,0xa0},{0x14,0x00},{0x15,0xe7},{0x16,0x04},
	{0x17,0xe3},{0x18,0xff},{0x19,0x00},{0x1a,0x00},
	{0x1b,0x02},{0x1d,0x10},{0x1e,0x21},{0x1f,0x00},
	{0x20,0x62},{0x21,0x00},{0x23,0x10},{0x2c,0xc3},
	{0x2e,0x00},{0x30,0x00},{0x3c,0x00},{0x40,0xbf},
	{0x41,0x00},{0x42,0x00},{0x43,0x01},{0x44,0xa5},
	{0x47,0xa2},{0x48,0x10},{0x49,0x00},{0x4a,0xdf},
	{0x4b,0x00},{0x4c,0x00},{0x4d,0x66},{0x4e,0x40},
	{0x2d,0x80},{0x02,0x00},{0x03,0xcc},{0x04,0xe5},
	{0x05,0xec},{0x06,0x15},{0x07,0x8d},{0x08,0x00},
	{0x09,0x02},{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},
	{0x0e,0x00},{0x2d,0x11}
};

/* 640x480x24. */
static const struct regpair nb10_24_seq[] = {
	{0x06,0x12},{0x12,0x00},{0x00,0x01},{0x01,0x01},
	{0x02,0x0f},{0x03,0x00},{0x04,0x0e},{0x07,0xc5},
	{0x0f,0x31},{0x16,0xfe},{0x1f,0x23},{0x21,0x08},
	{0x25,0x04},{0x2a,0x00},{0x2b,0x80},{0x2c,0x00},
	{0x2d,0x00},{0x2e,0x08},{0x2f,0x02},{0x0b,0x66},
	{0x0c,0x53},{0x0d,0x5f},{0x0e,0x6e},{0x1b,0x3b},
	{0x1c,0x30},{0x1d,0x23},{0x1e,0x2a}
};
static const struct regpair nb10_24_crtc[] = {
	{0x11,0x20},{0x00,0x5f},{0x01,0x4f},{0x02,0x50},
	{0x03,0x82},{0x04,0x54},{0x05,0x9e},{0x06,0x0b},
	{0x07,0x3e},{0x08,0x00},{0x09,0x40},{0x0a,0x00},
	{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},{0x0e,0x00},
	{0x0f,0x00},{0x10,0xe5},{0x11,0xa7},{0x12,0xdf},
	{0x13,0x00},{0x14,0x00},{0x15,0xe7},{0x16,0x04},
	{0x17,0xe3},{0x18,0xff},{0x19,0x00},{0x1a,0x00},
	{0x1b,0x12},{0x1d,0x10},{0x1e,0x21},{0x1f,0x00},
	{0x20,0x62},{0x21,0x00},{0x23,0x10},{0x2c,0xc3},
	{0x2e,0x00},{0x30,0x00},{0x3c,0x00},{0x40,0xbf},
	{0x41,0x00},{0x42,0x00},{0x43,0x00},{0x44,0xa5},
	{0x47,0xa2},{0x48,0x10},{0x49,0x00},{0x4a,0xdf},
	{0x4b,0x00},{0x4c,0x00},{0x4d,0x66},{0x4e,0x40},
	{0x2d,0x80},{0x02,0x00},{0x03,0xcc},{0x04,0xe5},
	{0x05,0xec},{0x06,0x15},{0x07,0x8d},{0x08,0x00},
	{0x09,0x02},{0x0b,0x00},{0x0c,0x00},{0x0d,0x00},
	{0x0e,0x00},{0x2d,0x11}
};

static const struct regpair nb10_gfx[] = {
	{0x00,0x00},{0x01,0x00},{0x02,0x00},{0x03,0x00},
	{0x04,0x00},{0x05,0x40},{0x06,0x05},{0x07,0x0f},
	{0x08,0xff}
};

static const uint8_t nb10_attr[21] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x01,0x00,0x0f,0x00,0x00
};

static void
nb10_program_chip(void)
{
	const struct regpair *seq, *crtc;
	int seq_count, crtc_count;
	int misc, hdr, i;
	uint8_t sr0f;

	if (cs.bpp == 8) {
		seq = nb10_8_seq;
		seq_count = (int)(sizeof(nb10_8_seq) /
				  sizeof(nb10_8_seq[0]));
		crtc = nb10_8_crtc;
		crtc_count = (int)(sizeof(nb10_8_crtc) /
				   sizeof(nb10_8_crtc[0]));
		misc = 0xe2;
		hdr = 0x00;
	} else if (cs.bpp == 16) {
		seq = nb10_16_seq;
		seq_count = (int)(sizeof(nb10_16_seq) /
				  sizeof(nb10_16_seq[0]));
		crtc = nb10_16_crtc;
		crtc_count = (int)(sizeof(nb10_16_crtc) /
				   sizeof(nb10_16_crtc[0]));
		misc = 0xee;
		hdr = 0xe1;
	} else {
		seq = nb10_24_seq;
		seq_count = (int)(sizeof(nb10_24_seq) /
				  sizeof(nb10_24_seq[0]));
		crtc = nb10_24_crtc;
		crtc_count = (int)(sizeof(nb10_24_crtc) /
				   sizeof(nb10_24_crtc[0]));
		misc = 0xee;
		hdr = 0xe5;
	}

	stream_seq(seq, seq_count);

	sr0f = (uint8_t)cl_seq_read(0x0f);
	cl_seq_write8(0x0f, (sr0f & 0xdf) | 0x20);

	cl_misc_write(misc);
	cl_gfx_write(0x06, 0x05);

	/* Release the synchronous reset asserted by SR00=01 in the stream. */
	cl_seq_write(0x00, 0x03);

	stream_crtc(crtc, crtc_count);
	stream_gfx(nb10_gfx,
		   (int)(sizeof(nb10_gfx) / sizeof(nb10_gfx[0])));

	(void)inp(cs.io_status);
	for (i = 0; i < 21; i++) {
		outp(cs.io_3c0, i);
		outp(cs.io_3c0, nb10_attr[i]);
	}
	/* AC remains disabled until the model-0Eh output-enable postlude. */

	cl_hidden_dac_write(hdr);
	outp(cs.io_3c0 + 6, 0xff);

	cl_gfx_write(0x09, 0);
	cl_gfx_write(0x0a, 0);
	cl_gfx_write(0x0b, 0x20);
	cl_gfx_write(0x31, 0);
	cl_gfx_write(0x0e, 0);
}

static void
nb10_dll_postlude(void)
{
	cl_gfx_write(0x0b, (cl_gfx_read(0x0b) & 0x20) | 0x04);
	cl_gfx_write(0x39, 0);
	cl_gfx_write(0x38, 0);
	cl_gfx_write(0x31, CL_BLT_RESET);
	cl_gfx_write(0x31, 0);
}

static bool
nb10_mode_set(void)
{
	unsigned long i;
	uint8_t sr17, sr12;

	nb10_nec_unlock();

	outp(PC98_GDC_MODE_PORT, 0x0e);
	outp(PC98_VRAM_SW_PORT, 0x07);
	outp(PC98_VRAM_SW_PORT, 0x8f);
	outp(PC98_VRAM_SW_PORT, 0x06);

	/*
	 * Model 0Eh does not touch 0FAAh/0FABh reg03.  V13 does; Nb10 does not.
	 */
	nb10_attr_preamble();

	nb10_program_chip();

	/* Family-40h post-mode RMW through 4B4h/4B5h. */
	sr17 = (uint8_t)nb10_seq_read(0x17);
	nb10_seq_write(0x17, sr17 | 0x44);

	if (!clear_nb10_nt4_vram()) {
		nb10_nec_lock();
		return false;
	}

	sr12 = (uint8_t)nb10_seq_read(0x12);
	nb10_seq_write(0x12, sr12 & 0xbf);

	outp(NB10_RELAY, 0x02);
	for (i = 0; i < 200000UL; i++)
		outp(PC98_WAIT_PORT, 0);

	nb10_attr_enable();
	outp(cs.io_3c0 + 6, 0xff);
	nb10_nec_lock();

	/* The display DLL applies this after SET_CURRENT_MODE. */
	nb10_dll_postlude();

	if (cs.bpp == 8)
		cl_load_palette();

	cs.nb10_entered = true;
	return true;
}

static bool
nb10_detect(const struct pci_location *gd7548,
	     const struct pci_location *cgb)
{
	if (!gd7548->found)
		return false;

	/*
	 * Nb10 is the independently enumerated 1013:0038 route.  The NEC
	 * 1033:0009 marker is expected, but an exact Cirrus ID and a valid BAR
	 * are sufficient to avoid rejecting a firmware variant that hides it.
	 */
	if ((gd7548->bar0 & ~0x0fUL) == 0)
		return false;

	cs.nb10 = *gd7548;

	if (!cgb->found)
		hal_log_info("CIRRUS-NB10: NEC 1033:0009 marker was not "
			     "enumerated; continuing with exact 1013:0038 ID.");

	return true;
}

static bool
nb10_init(int requested_bpp)
{
	uint32_t command, bar0;
	int bpp;
	uint8_t sleep;
	int misc;

	bpp = resolve_bpp(requested_bpp, 16);
	if (bpp < 0)
		return false;

	cs.route = CIRRUS_ROUTE_NB10;
	cs.name = "Cirrus GD7548 / NEC Nb10";
	cs.width = 640;
	cs.height = 480;
	cs.bpp = bpp;
	cs.pitch = pitch_for_bpp(bpp);
	cs.vram_bytes = FRAMEBUFFER_BYTES;
	cs.fifo_only = true;

	command = pci_read32(cs.nb10.bus, cs.nb10.dev, cs.nb10.fn, 0x04);
	cs.nb10.saved_command = command;
	pci_write32(cs.nb10.bus, cs.nb10.dev, cs.nb10.fn, 0x04,
		    command | 0x03);

	bar0 = pci_read32(cs.nb10.bus, cs.nb10.dev, cs.nb10.fn, 0x10);
	bar0 &= ~0x0fUL;
	if (bar0 == 0)
		goto fail;

	cs.fb_phys = bar0 + NB10_FB_OFFSET;
	cs.fb = (uint8_t *)map_physical(cs.fb_phys, cs.vram_bytes);
	if (cs.fb == NULL)
		goto fail;

	cl_set_iobase(NATIVE_IO_3C0, NATIVE_IO_3D4, NATIVE_IO_3DA,
		      NATIVE_IO_3B4, NATIVE_IO_3BA);

	/*
	 * Minimal liveness bracket.  The mode stream still writes SR06=12;
	 * this early write only guarantees that CR24 and the command stream are
	 * addressed to a live native GD7548 block.
	 */
	sleep = (uint8_t)inp(NATIVE_SLEEP);
	if (sleep != 0xff && !(sleep & 1))
		outp(NATIVE_SLEEP, sleep | 1);

	cs.nb10_extensions_were_locked =
	    ((uint8_t)cl_seq_read(0x06) != 0x12);
	cl_seq_write(0x06, 0x12);
	if ((uint8_t)cl_seq_read(0x06) != 0x12)
		goto fail;

	misc = cl_misc_read();
	cl_select_crtc(misc);

	if (!nb10_mode_set())
		goto fail;

	cs.crt27 = (uint8_t)cl_crtc_read(0x27);
	return true;

fail:
	nb10_cleanup();
	return false;
}

static void
nb10_cleanup(void)
{
	unsigned long i;
	uint8_t sr12;

	if (cs.route != CIRRUS_ROUTE_NB10 && !cs.nb10_entered &&
	    !cs.nb10.found) {
		release_mapping();
		return;
	}

	/* A pending MEMSYSSRC command must not consume cleanup writes. */
	if (cs.io_3c0 != 0) {
		cl_gfx_write(0x31, CL_BLT_RESET);
		cl_gfx_write(0x31, 0);
	}

	if (cs.nb10_entered) {
		nb10_nec_unlock();

		sr12 = (uint8_t)nb10_seq_read(0x12);
		nb10_seq_write(0x12, sr12 | 0x40);
		outp(PC98_WAIT_PORT, 0);

		outp(PC98_VRAM_SW_PORT, 0x07);
		outp(PC98_VRAM_SW_PORT, 0x8e);
		outp(PC98_VRAM_SW_PORT, 0x06);

		outp(NB10_RELAY, 0x00);
		for (i = 0; i < 200000UL; i++)
			outp(PC98_WAIT_PORT, 0);

		outp(PC98_GDC_MODE_PORT, 0x0f);
		nb10_nec_lock();
		cs.nb10_entered = false;
	}

	if (cs.nb10.found) {
		pci_write32(cs.nb10.bus, cs.nb10.dev, cs.nb10.fn, 0x04,
			    cs.nb10.saved_command);
	}

	release_mapping();
}

/* ------------------------------------------------------------------------- */
/* CPU-source FIFO                                                            */
/* ------------------------------------------------------------------------- */

static bool
blt_wait_idle(unsigned long limit)
{
	unsigned long i;

	for (i = 0; i < limit; i++) {
		if (!(cl_gfx_read(0x31) & CL_BLT_BUSY))
			return true;
	}
	return false;
}

static void
blt_reset(void)
{
	cl_gfx_write(0x31, CL_BLT_RESET);
	cl_gfx_write(0x31, 0);
}

static void
blt_write16(int low_reg, uint32_t value)
{
	cl_gfx_write(low_reg, value & 0xff);
	cl_gfx_write(low_reg + 1, (value >> 8) & 0xff);
}

static void
blt_write24(int low_reg, uint32_t value)
{
	cl_gfx_write(low_reg, value & 0xff);
	cl_gfx_write(low_reg + 1, (value >> 8) & 0xff);
	cl_gfx_write(low_reg + 2, (value >> 16) & 0x3f);
}

static bool
blt_fifo_start(uint32_t destination, uint32_t row_bytes, uint32_t rows)
{
	unsigned long i;

	if (!blt_wait_idle(2000000UL))
		return false;

	blt_reset();
	blt_write16(0x20, row_bytes - 1);
	blt_write16(0x22, rows - 1);
	blt_write16(0x24, cs.pitch);
	blt_write16(0x26, row_bytes);
	blt_write24(0x28, destination);
	blt_write24(0x2c, 0);
	cl_gfx_write(0x2f, 0);
	cl_gfx_write(0x30, CL_BLT_MEMSYS_SRC);
	cl_gfx_write(0x32, CL_BLT_ROP_SRC);
	cl_gfx_write(0x33, 0);
	cl_gfx_write(0x31, CL_BLT_START);

	/* The engine should assert BUSY while waiting for host source data. */
	for (i = 0; i < 65536UL; i++) {
		if (cl_gfx_read(0x31) & CL_BLT_BUSY)
			return true;
	}

	blt_reset();
	return false;
}

static void
blt_fifo_feed32(const uint8_t *source, uint32_t bytes)
{
	volatile uint32_t *fifo = (volatile uint32_t *)cs.fb;
	uint32_t value;

	while (bytes >= 4) {
		value = (uint32_t)source[0] |
			((uint32_t)source[1] << 8) |
			((uint32_t)source[2] << 16) |
			((uint32_t)source[3] << 24);
		fifo[0] = value;
		source += 4;
		bytes -= 4;
	}

	if (bytes != 0) {
		value = source[0];
		if (bytes > 1)
			value |= (uint32_t)source[1] << 8;
		if (bytes > 2)
			value |= (uint32_t)source[2] << 16;
		fifo[0] = value;
	}
}

/* ------------------------------------------------------------------------- */
/* Pixel conversion and presentation                                         */
/* ------------------------------------------------------------------------- */

static void
convert_row24(uint8_t *dst8, const uint32_t *src, int pixels)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < pixels; x += 4) {
		uint32_t p0 = src[x] & 0x00ffffffUL;
		uint32_t p1 = src[x + 1] & 0x00ffffffUL;
		uint32_t p2 = src[x + 2] & 0x00ffffffUL;
		uint32_t p3 = src[x + 3] & 0x00ffffffUL;

		dst[0] = p0 | (p1 << 24);
		dst[1] = (p1 >> 8) | (p2 << 16);
		dst[2] = (p2 >> 16) | (p3 << 8);
		dst += 3;
	}
}

static void
convert_row16(uint8_t *dst8, const uint32_t *src, int pixels)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < pixels; x += 2) {
		dst[x / 2] =
		    (uint32_t)(uint16_t)PACK565(src[x]) |
		    ((uint32_t)(uint16_t)PACK565(src[x + 1]) << 16);
	}
}

static void
convert_row8(uint8_t *dst8, const uint32_t *src, int pixels)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < pixels; x += 4) {
		dst[x / 4] =
		    PACK332(src[x]) |
		    (PACK332(src[x + 1]) << 8) |
		    (PACK332(src[x + 2]) << 16) |
		    (PACK332(src[x + 3]) << 24);
	}
}

static void
wait_vblank_start(void)
{
	unsigned long i;

	for (i = 0; i < 400000UL && (inp(cs.io_status) & 0x08); i++)
		;
	for (i = 0; i < 400000UL && !(inp(cs.io_status) & 0x08); i++)
		;
}

static bool
fifo_present(void)
{
	static uint32_t row32[512];
	uint8_t *row = (uint8_t *)row32;
	const uint32_t *pixels;
	uint32_t row_bytes, destination;
	int y;

	if (back_image == NULL || back_image->pixels == NULL)
		return false;

	pixels = back_image->pixels;
	row_bytes = (uint32_t)cs.draw_w * (uint32_t)(cs.bpp / 8);
	if (row_bytes > sizeof(row32))
		return false;

	destination = (uint32_t)cs.ofs_y * cs.pitch +
		      (uint32_t)cs.ofs_x * (uint32_t)(cs.bpp / 8);

	wait_vblank_start();
	if (!blt_fifo_start(destination, row_bytes, (uint32_t)cs.draw_h))
		return false;

	for (y = 0; y < cs.draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;

		if (cs.bpp == 24)
			convert_row24(row, src, cs.draw_w);
		else if (cs.bpp == 16)
			convert_row16(row, src, cs.draw_w);
		else
			convert_row8(row, src, cs.draw_w);

		blt_fifo_feed32(row, row_bytes);
	}

	if (!blt_wait_idle(4000000UL)) {
		blt_reset();
		return false;
	}

	return true;
}

static bool
direct_present(void)
{
	static uint32_t row32[512];
	uint8_t *row = (uint8_t *)row32;
	const uint32_t *pixels;
	uint32_t row_bytes, offset;
	int y;

	if (back_image == NULL || back_image->pixels == NULL || cs.fb == NULL)
		return false;

	pixels = back_image->pixels;
	row_bytes = (uint32_t)cs.draw_w * (uint32_t)(cs.bpp / 8);
	if (row_bytes > sizeof(row32))
		return false;

	wait_vblank_start();

	for (y = 0; y < cs.draw_h; y++) {
		volatile uint32_t *dst;
		const uint32_t *src = pixels + y * game_width;
		uint32_t dwords, i;

		if (cs.bpp == 24)
			convert_row24(row, src, cs.draw_w);
		else if (cs.bpp == 16)
			convert_row16(row, src, cs.draw_w);
		else
			convert_row8(row, src, cs.draw_w);

		offset = (uint32_t)(y + cs.ofs_y) * cs.pitch +
			 (uint32_t)cs.ofs_x * (uint32_t)(cs.bpp / 8);
		dst = (volatile uint32_t *)(cs.fb + offset);
		dwords = row_bytes / 4;
		for (i = 0; i < dwords; i++)
			dst[i] = row32[i];
	}

	return true;
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

bool
cirrus_init_disp(int mode, int requested_bpp)
{
	struct pci_location gd7548, cgb;
	bool core_present;
	int width, height;

	if (mode < DISP_640X480 || mode > DISP_1280X1024)
		return false;
	if (requested_bpp != -1 && requested_bpp != 8 &&
	    requested_bpp != 16 && requested_bpp != 24)
		return false;

	width = (mode == DISP_640X480) ? 640 :
		(mode == DISP_800X600) ? 800 :
		(mode == DISP_1024X768) ? 1024 : 1280;
	height = (mode == DISP_640X480) ? 480 :
		 (mode == DISP_800X600) ? 600 :
		 (mode == DISP_1024X768) ? 768 : 1024;

	/*
	 * Only the recovered 640x480 streams are carried in this clean file.
	 */
	if (width != 640 || height != 480) {
		hal_log_info("CIRRUS: clean Core-Graph/Nb10 driver currently "
			     "supports 640x480 only.");
		return false;
	}

	memset(&cs, 0, sizeof(cs));
	pci_scan(&gd7548, &cgb);

	/*
	 * Route selection deliberately prefers a validated fixed-interface
	 * Core-Graph backend.  A V13 has the NEC bridge but no enumerable Cirrus
	 * PCI function.  Nb10 has an enumerable GD7548 and normally reads FFh at
	 * the fixed interface, so it falls through to the model-0Eh route.
	 */
	core_present = coregraph_detect();
	if (core_present) {
		if (!coregraph_init(requested_bpp))
			return false;
	} else {
		if (!nb10_detect(&gd7548, &cgb))
			return false;
		if (!nb10_init(requested_bpp))
			return false;
	}

	cs.ofs_x = (cs.width - game_width) / 2;
	cs.ofs_y = (cs.height - game_height) / 2;
	if (cs.ofs_x < 0)
		cs.ofs_x = 0;
	if (cs.ofs_y < 0)
		cs.ofs_y = 0;

	cs.draw_w = game_width < cs.width ? game_width : cs.width;
	cs.draw_h = game_height < cs.height ? game_height : cs.height;

	/* Every row converter emits dwords. */
	if (cs.bpp == 16)
		cs.draw_w &= ~1;
	else
		cs.draw_w &= ~3;

	/*
	 * Packed 24bpp rows require the destination byte offset to be dword
	 * aligned: ofs_x * 3 must be divisible by four.
	 */
	if (cs.bpp == 24)
		cs.ofs_x &= ~3;

	hal_log_info("CIRRUS: %s selected: %dx%d %dbpp, pitch=%lu, "
		     "host=%s at %08lXh.",
		     cs.name, cs.width, cs.height, cs.bpp,
		     (unsigned long)cs.pitch,
		     cs.fifo_only ? "CPU-source FIFO" : "linear framebuffer",
		     (unsigned long)cs.fb_phys);

	return true;
}

void
cirrus_cleanup_disp(void)
{
	switch (cs.route) {
	case CIRRUS_ROUTE_COREGRAPH:
		coregraph_cleanup();
		break;
	case CIRRUS_ROUTE_NB10:
		nb10_cleanup();
		break;
	default:
		release_mapping();
		break;
	}

	memset(&cs, 0, sizeof(cs));
}

void
cirrus_flip(void)
{
	if (cs.route == CIRRUS_ROUTE_NONE)
		return;

	if (cs.fifo_only)
		(void)fifo_present();
	else
		(void)direct_present();
}
