/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Main code for NEC PC-9800 series (DOS/4G)
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

/* HAL */
#include <strato/strato.h>	/* Public Interface */
#include "stdfile.h"		/* Standard C File Implementation */

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>
#include <assert.h>

/* DOS */
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include <stdint.h>

/*
 * CIRRUS CL-GD54xx
 */

/*
 * PC-9821 built-in window accelerator (CIRRUS CL-GD5428/5430/5440)
 *
 * References:
 *  - UNDOCUMENTED 9801/9821 Vol.2 (io_wab.txt)
 *  - Neko Project 21/W CL-GD54xx analysis notes
 *  - Linux cirrusfb driver (for the chip-side mode set values)
 */

/* Cirrus screen size */
#define CIRRUS_WIDTH	640
#define CIRRUS_HEIGHT	480
#define CIRRUS_BPP	24
#define CIRRUS_PITCH	(CIRRUS_WIDTH * 3)

/* Two-stage indexed I/O for the built-in accelerator */
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
 * bit0 = register/MMIO access enable.
 * (Semantics per NP21/W cirrusvga_ofab(): "dat & 0x2" drives the
 * relay, "dat & 0x1" is the access enable.  Earlier revisions of
 * this file had the two bits swapped, so the relay was never
 * switched back to the GDC on cleanup.)
 */
#define WAB_RELAY_GDC	0x00	/* GDC output, access off (exit state) */
#define WAB_RELAY_SETUP	0x01	/* GDC output, register access on */
#define WAB_RELAY_WAB	0x03	/* accelerator output, access on */

/* PCI models (Xa7e etc.) have a second relay at 0FACh */
#define WAB_RELAY2_PORT	0x0fac

/* VRAM ownership switch (shared between GDC and the accelerator) */
#define VRAM_SW_PORT	0x6a
#define VRAM_SW_GDC	0x8e
#define VRAM_SW_WAB	0x8f

/* Wakeup ports */
#define WAB_P904	0x0904	/* 102 Access Control (bit5) */
#define WAB_PFF82	0xff82	/* POS102 (bit0 = video subsystem enable) */

/* Relocated VGA-compatible ports (native 3Cxh -> 0CAxh, etc.) */
#define P_ATTR		0x0ca0	/* 3C0: Attribute Controller index/data */
#define P_MISC_W	0x0ca2	/* 3C2: Miscellaneous Output (write) */
#define P_SLEEP		0x0ca3	/* 3C3: Sleep Address (bit0 = enable) */
#define P_SEQ_I		0x0ca4	/* 3C4: Sequencer index */
#define P_SEQ_D		0x0ca5	/* 3C5: Sequencer data */
#define P_DAC_MASK	0x0ca6	/* 3C6: Pixel Mask / Hidden DAC */
#define P_DAC_WI	0x0ca8	/* 3C8: Palette write index */
#define P_DAC_DATA	0x0ca9	/* 3C9: Palette data */
#define P_GFX_I		0x0cae	/* 3CE: Graphics Controller index */
#define P_GFX_D		0x0caf	/* 3CF: Graphics Controller data */
#define P_CRTC_I	0x0da4	/* 3D4: CRTC index */
#define P_CRTC_D	0x0da5	/* 3D5: CRTC data */
#define P_STAT1		0x0daa	/* 3DA: Input Status 1 (resets AC flip-flop) */

/* Cirrus bank granularity: 16KB (GR0B bit5 = 1) */
#define BANK_SHIFT	14
#define BANK_MASK	0x3fff

static int cur_bank;		/* current Cirrus bank (GR09) */
static uint8_t cirrus_id;	/* WAB machine ID */
static uint8_t cirrus_crt27;	/* Cirrus chip ID (CR27) */
static uint8_t *fb;		/* mapped Cirrus VRAM window (32KB) */
static int ofs_x;
static int ofs_y;

extern struct hal_image *back_image;
extern int game_width;
extern int game_height;

static bool detect_cirrus(void);
static void wab_write(int reg, int val);
static int wab_read(int reg);
static void seq_write(int reg, int val);
static int seq_read(int reg);
static void gfx_write(int reg, int val);
static int crtc_read(int reg);
static void crtc_write(int reg, int val);
static void attr_write(int reg, int val);
static void hidden_dac_write(int val);
static void set_bank(int bank);
static void *dpmi_map_physical(uint32_t phys, uint32_t size);

/*
 * Initialize the CIRRUS chip and set 640x480x24bpp.
 *
 * There is no VGA BIOS on PC-98, so the full VGA register set is
 * programmed by hand. Values follow the Linux cirrusfb driver and
 * the standard 640x480@60Hz (25.175MHz dot clock) timing.
 */
bool
cirrus_init_disp(void)
{
	/*
	 * CRTC values for 640x480@60Hz (25.175MHz dot clock).
	 *
	 * These are NOT the plain IBM VGA table values: they are the
	 * exact values the Linux cirrusfb driver computes and writes
	 * on real Alpine (GD5430/5434/5440) hardware.  The important
	 * difference is horizontal blanking: on the Cirrus chips the
	 * Horizontal Blanking End compare is extended to 8 bits with
	 * CR1A[5:4] as bits <7:6>, so blanking end is programmed as
	 * the full horizontal total (100 characters = 0110 0100b):
	 *   CR03[4:0] = 00100b, CR05[7] = 1, CR1A[5:4] = 01b.
	 * The stock VGA table (CR03=82h/CR1A=00h) leaves the 8-bit
	 * compare value at 34, so on the real chip the blanking pulse
	 * never terminates where it should - one cause of a torn or
	 * blank picture that an emulator (which ignores blanking
	 * timing entirely) will never show.
	 */
	static const uint8_t crtc_tab[] = {
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
		0xf0,	/* 13: Offset (pitch/8 = 1920/8 = 240) */
		0x00,	/* 14: Underline */
		0xe0,	/* 15: Vertical Blanking Start (480, low byte) */
		0x0b,	/* 16: Vertical Blanking End (523, low byte) */
		0xc3,	/* 17: Mode Control (byte mode, wrap) */
		0xff	/* 18: Line Compare */
	};
	int i, sr07;
	bool is_alpine;

	if (!detect_cirrus())
		return false;

	hal_log_info("CIRRUS: CL-GD54xx card found.");

	/*
	 * Wake up the video subsystem.
	 * 0904h bit5 enables access to POS102 (FF82h); POS102 bit0
	 * and Sleep Address (3C3 -> 0CA3h) bit0 enable the chip.
	 */
	outp(WAB_P904, 0x20);
	outp(WAB_PFF82, 0x01);
	outp(P_SLEEP, 0x01);

	/* Enable WAB register access, keep the 98 GDC on screen for now. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_SETUP);

	/* Place the VRAM window at 0xF20000. */
	wab_write(WAB_REG_WINDOW, WAB_WINDOW_F2);

	/* Map the window into our address space. */
	fb = (uint8_t *)dpmi_map_physical(WAB_WINDOW_ADDR, WAB_WINDOW_SIZE);
	if (fb == NULL) {
		printf("Can't map the CIRRUS VRAM window.\n");
		return false;
	}

	/* Hand the shared VRAM over to the accelerator. */
	outp(VRAM_SW_PORT, VRAM_SW_WAB);

	/* Blank the screen during the mode set (SR1 bit5). */
	seq_write(0x00, 0x03);	/* sequencer: run */
	seq_write(0x01, 0x21);	/* 8-dot clock, screen off */

	/* Unlock all Cirrus extension registers. */
	seq_write(0x06, 0x12);

	/*
	 * Identify the chip generation from CR27:
	 * 0xA0 and above = GD5430/5440 (Alpine family).
	 */
	cirrus_crt27 = (uint8_t)crtc_read(0x27);
	is_alpine = (cirrus_crt27 >= 0xa0);

	/*
	 * On PC-98 no VGA BIOS has ever run, so extended registers may
	 * hold whatever the NEC firmware / a previous OS driver left in
	 * them.  Explicitly clear the state that can corrupt the
	 * display (per cirrusfb's init_vgachip):
	 */
	if (is_alpine)
		gfx_write(0x33, 0x00);	/* BLT: back to 542x-compatible */
	gfx_write(0x31, 0x04);		/* BitBLT reset... */
	gfx_write(0x31, 0x00);		/* ...end of reset */
	seq_write(0x10, 0x00);		/* HW cursor X */
	seq_write(0x11, 0x00);		/* HW cursor Y */
	seq_write(0x12, 0x00);		/* HW cursor attributes: OFF */
	seq_write(0x13, 0x00);		/* HW cursor pattern address */

	if (!is_alpine) {
		/* GD5428: performance/DRAM control (per cirrusfb) */
		seq_write(0x16, 0x0f);
		seq_write(0x0f, 0xb0);
	}

	/* Sequencer basics */
	seq_write(0x02, 0xff);	/* plane write mask */
	seq_write(0x03, 0x00);	/* character map */
	seq_write(0x04, 0x0a);	/* memory mode: ext memory, chain4 */
	seq_write(0x17, 0x00);	/* ext control: MMIO off */

	/*
	 * Extended Sequencer Mode (SR07) for 24bpp packed pixel:
	 *   GD5430/5440 (Alpine): 0xA5, VCLK = dot clock x3
	 *   GD5428:               0x25
	 */
	sr07 = is_alpine ? 0xa5 : 0x25;
	seq_write(0x07, sr07);

	/*
	 * VCLK3 (selected by MISC clock select = 11b).
	 * VClk = 14.31818MHz * N / (D * (1 + P)),
	 * SR0E = N, SR1E = (D << 1) | P.
	 *
	 * On the Alpine family (GD5430/34/40) SR1E bit7 MUST also be
	 * set (cirrusfb: "6 bit denom; ONLY 5434!!! (bugged me 10
	 * days)"); without it the denominator is misinterpreted and
	 * the pixel clock - and therefore the H/V sync frequencies -
	 * come out wrong on real silicon.  Emulators ignore the clock
	 * registers completely, which is why this was invisible on
	 * NP21/W.
	 */
	if (is_alpine) {
		/* 3 x 25.175 = 75.525MHz -> N=95, D=18, P=0 (75.57MHz) */
		seq_write(0x0e, 0x5f);
		seq_write(0x1e, 0x80 | 0x24);
	} else {
		/* 25.175MHz (5428 does not need x3) -> N=74, D=21, P=1 */
		seq_write(0x0e, 0x4a);
		seq_write(0x1e, 0x2b);
	}

	/*
	 * SR1F bit6 = "derive VCLK from MCLK".  If a previous driver
	 * (e.g. the Windows one) left it set, the SR0E/SR1E values
	 * above would simply be ignored.  Clear it, but preserve the
	 * MCLK frequency bits NEC programmed for this board's DRAM.
	 */
	seq_write(0x1f, seq_read(0x1f) & ~0x40);

	/*
	 * Miscellaneous Output: negative H/V sync (480-line mode),
	 * clock select = VCLK3, display memory enabled, color I/O.
	 */
	outp(P_MISC_W, 0xcf);

	/* CRTC: unprotect CR0-7 (CR11 bit7), then program the table.
	   CR11 in the table keeps bit7 clear (cirrusfb leaves the
	   CRTC unprotected too). */
	crtc_write(0x11, crtc_read(0x11) & 0x7f);
	for (i = 0; i < (int)sizeof(crtc_tab); i++)
		crtc_write(i, crtc_tab[i]);
	/* CR1A: no interlace; bits5:4 = Horiz. Blanking End <7:6>.
	   Htotal is 100 characters (01100100b) -> bit6 set. */
	crtc_write(0x1a, 0x10);
	crtc_write(0x1b, 0x22);	/* ext display: 16bit wrap, pitch bit8=0 */
	crtc_write(0x1d, 0x00);	/* ext overflow: start address bit19 = 0 */

	/* Graphics Controller */
	gfx_write(0x00, 0x00);
	gfx_write(0x01, 0x00);
	gfx_write(0x02, 0x00);
	gfx_write(0x03, 0x00);
	gfx_write(0x04, 0x00);
	gfx_write(0x05, 0x40);	/* mode: 256-color shift (packed pixel) */
	gfx_write(0x06, 0x05);	/* misc: graphics mode, A0000 64KB map */
	gfx_write(0x07, 0x0f);
	gfx_write(0x08, 0xff);
	gfx_write(0x09, 0x00);	/* Offset Register 0 (bank) */
	gfx_write(0x0a, 0x00);	/* Offset Register 1 */
	gfx_write(0x0b, is_alpine ? 0x20 : 0x28); /* 16KB granularity */
	cur_bank = 0;

	/* Attribute Controller: identity palette, graphics mode */
	for (i = 0; i < 16; i++)
		attr_write(i, i);
	attr_write(0x10, 0x01);	/* mode: graphics */
	attr_write(0x11, 0x00);	/* overscan */
	attr_write(0x12, 0x0f);	/* plane enable */
	attr_write(0x13, 0x00);	/* pixel panning */
	attr_write(0x14, 0x00);	/* color select */
	(void)inp(P_STAT1);
	outp(P_ATTR, 0x20);	/* re-enable video output */

	/* DAC: no pixel mask; identity ramp just in case. */
	outp(P_DAC_MASK, 0xff);
	outp(P_DAC_WI, 0x00);
	for (i = 0; i < 256; i++) {
		outp(P_DAC_DATA, i >> 2);
		outp(P_DAC_DATA, i >> 2);
		outp(P_DAC_DATA, i >> 2);
	}

	/* Hidden DAC: 8-8-8 truecolor (24bpp) */
	hidden_dac_write(0xc5);

	/* Clear the whole 1MB VRAM through the banked window. */
	for (i = 0; i < (1024 * 1024) >> BANK_SHIFT; i++) {
		set_bank(i);
		memset(fb, 0, 1 << BANK_SHIFT);
	}
	set_bank(0);

	/* Screen back on. */
	seq_write(0x01, 0x01);

	/* Switch the video output relay to the accelerator. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_WAB);
	outp(WAB_RELAY2_PORT, 0x02);	/* PCI models (harmless elsewhere) */

	ofs_x = (CIRRUS_WIDTH - game_width) / 2;
	ofs_y = (CIRRUS_HEIGHT - game_height) / 2;

	return true;
}

void
cirrus_cleanup_disp(void)
{
	/* Blank the accelerator output first (SR1 bit5). */
	seq_write(0x01, 0x21);

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

/*
 * Blit back image to VRAM. (CIRRUS 24-bpp)
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R.
 * Cirrus 24bpp VRAM layout is also B, G, R, so the low 3 bytes of
 * each pixel are copied as-is.
 */
void
cirrus_flip(void)
{
	uint32_t *pixels;
	int x, y;

	pixels = back_image->pixels;

	for (y = 0; y < game_height; y++) {
		uint32_t *src = pixels + y * game_width;
		uint32_t off = (uint32_t)(y + ofs_y) * CIRRUS_PITCH + (uint32_t)ofs_x * 3;
		uint32_t *dst;

		/*
		 * One 16KB bank switch puts the row start within the
		 * first 16KB of the 32KB window, so a 1920-byte row
		 * never crosses the window end.
		 */
		set_bank((int)(off >> BANK_SHIFT));
		dst = (uint32_t *)(fb + (off & BANK_MASK));

		for (x = 0; x < game_width; x += 4) {
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
}

/*
 * Detect the built-in CIRRUS accelerator.
 *
 * WAB register 00h returns the machine ID; CIRRUS models use
 * 50h-5Dh and 70h. (Other values are S3/Matrox/Trident models
 * or 00h/FFh when no two-stage I/O accelerator is present.)
 */
static bool
detect_cirrus(void)
{
	cirrus_id = (uint8_t)wab_read(WAB_REG_ID);

	if (!((cirrus_id >= 0x50 && cirrus_id <= 0x5d) ||
	      cirrus_id == 0x70))
		return false;

	return true;
}

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

static void
seq_write(int reg, int val)
{
	outp(P_SEQ_I, reg);
	outp(P_SEQ_D, val);
}

static int
seq_read(int reg)
{
	outp(P_SEQ_I, reg);
	return inp(P_SEQ_D);
}

static void
gfx_write(int reg, int val)
{
	outp(P_GFX_I, reg);
	outp(P_GFX_D, val);
}

static int
crtc_read(int reg)
{
	outp(P_CRTC_I, reg);
	return inp(P_CRTC_D);
}

static void
crtc_write(int reg, int val)
{
	outp(P_CRTC_I, reg);
	outp(P_CRTC_D, val);
}

/* The Attribute Controller uses an index/data flip-flop. */
static void
attr_write(int reg, int val)
{
	(void)inp(P_STAT1);	/* reset the flip-flop */
	outp(P_ATTR, reg);
	outp(P_ATTR, val);
}

/*
 * Write the Cirrus Hidden DAC Register: it is accessed by reading
 * the Pixel Mask register (3C6) four times, then writing.
 */
static void
hidden_dac_write(int val)
{
	(void)inp(P_DAC_MASK);
	(void)inp(P_DAC_MASK);
	(void)inp(P_DAC_MASK);
	(void)inp(P_DAC_MASK);
	outp(P_DAC_MASK, val);
}

/* Select a 16KB VRAM bank via GR09 (Offset Register 0). */
static void
set_bank(int bank)
{
	if (bank != cur_bank) {
		gfx_write(0x09, bank);
		cur_bank = bank;
	}
}

/*
 * DPMI
 */

/*
 * DPMI 0x0800: Map a physical address into linear address space.
 * (DOS/4GW uses a zero-based flat address space, so the returned
 * linear address is directly usable as a pointer.)
 */
static void *
dpmi_map_physical(uint32_t phys, uint32_t size)
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
