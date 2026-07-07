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
#include "98sound.h"		/* PC98 Sound Implementation */

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

/* VRAM Address (PC-98 GDC) */
#define GVRAM_B		0x000A8000UL
#define GVRAM_R		0x000B0000UL
#define GVRAM_G		0x000B8000UL
#define GVRAM_I		0x000E0000UL
#define TVRAM_TEXT	0x000A0000UL
#define TVRAM_ATTR	0x000A2000UL

/* Screen Size (PC-98 GDC) */
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	400
#define LINE_BYTES	(640 / 8)

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

/* WAB_REG_RELAY: bit1 = register enable, bit0 = 1:98 GDC / 0:accelerator */
#define WAB_RELAY_GDC	0x03
#define WAB_RELAY_WAB	0x02

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

/* Log */
#define LOG_FILE	"log.txt"

/* Game Info */
static char *game_title;
static int game_width;
static int game_height;

/* Screen */
static struct hal_image *back_image;
static uint8_t *fb;		/* mapped Cirrus VRAM window (32KB) */
static int fb_bpp;		/* 24 (Cirrus) or 4 (GDC fallback) */
static int ofs_x;
static int ofs_y;
static int cur_bank;		/* current Cirrus bank (GR09) */
static uint8_t cirrus_id;	/* WAB machine ID */
static uint8_t cirrus_crt27;	/* Cirrus chip ID (CR27) */
static bool is_true_color_enabled;

/* Log */
static FILE *log_fp;

/* Callback. */
static struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* argc/argv */
int hal_argc;
char **hal_argv;

/* Forward Declaration */
static void init_vram(void);
static void cleanup_vram(void);
static void init_vram_gdc(void);
static bool init_vram_cirrus(void);
static bool detect_cirrus(void);
static void wab_write(int reg, int val);
static int wab_read(int reg);
static void seq_write(int reg, int val);
static void gfx_write(int reg, int val);
static int crtc_read(int reg);
static void crtc_write(int reg, int val);
static void attr_write(int reg, int val);
static void hidden_dac_write(int val);
static void set_bank(int bank);
static void *dpmi_map_physical(uint32_t phys, uint32_t size);
static void process_input(void);
static void flip_24bpp(void);
static void flip_4bpp(void);
static bool open_log_file(void);

int hal_main(int argc, char *argv[])
{
	hal_argc = argc;
	hal_argv = argv;

	printf("\n"
	       "Suika3 Game Engine for PC-9801\n"
	       "Copyright (c) 2026 Awe Morris\n");

	if (argc >= 2) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("Version 2026.05\n");
			return 0;
		}
		if (strcmp(argv[1], "--true-color") == 0)
			is_true_color_enabled = true;
	}

	if (!init_file()) {
		printf("Failed to initialize the file system.\n");
		return 1;
	}

	if (!init_sound())
		printf("No sound card.\n");

	getchar();

	if (!hal_bootstrap_ptr(
		    &game_title,
		    &game_width,
		    &game_height,
		    &hal_callback))
		return 1;

	if (game_width > SCREEN_WIDTH || game_height > SCREEN_HEIGHT) {
		printf("Screen size too large.\n");
		return 1;
	}

	if (!hal_create_image(game_width, game_height, &back_image)) {
		printf("Error on creating image.\n");
		return 1;
	}

	if (!hal_callback.on_start()) {
		printf("Error on start.\n");
		return 1;
	}

	init_vram();

	while (1) {
		sb16_sound_poll();
		process_input();

		hal_clear_image(back_image, 0);

		if (!hal_callback.on_update())
			break;

		hal_callback.on_render();

		if (fb_bpp == 24)
			flip_24bpp();
		else
			flip_4bpp();
	}

	cleanup_vram();

	return 0;
}

/* Initialize G-VRAM. */
static void
init_vram(void)
{
	init_vram_gdc();

	if (is_true_color_enabled) {
		if (init_vram_cirrus()) {
			fb_bpp = 24;
			ofs_x = (CIRRUS_WIDTH - game_width) / 2;
			ofs_y = (CIRRUS_HEIGHT - game_height) / 2;

			return;
		}
	}
}

/* Cleanup G-VRAM. */
static void
cleanup_vram(void)
{
	union REGS r;

	if (fb_bpp == 24) {
		/* Relay back to the 98 GDC output. */
		wab_write(WAB_REG_RELAY, WAB_RELAY_GDC);
		outp(WAB_RELAY2_PORT, 0x00);

		/* Return the shared VRAM to the GDC. */
		outp(VRAM_SW_PORT, VRAM_SW_GDC);
	}

	/*
	 * Stop displaying G-VRAM.
	 *  - INT 18h, AH=41h
	 */
	r.w.ax = 0x4100;
	int386(0x18, &r, &r);
}

/*
 * GDC (640x400x4)
 */

/* Initialize G-VRAM (PC-98 GDC, 640x400 4-bpp). */
static void
init_vram_gdc(void)
{
	volatile uint16_t *text, *attr;
	union REGS r;
	int i;

	/*
	 * Set CRT display mode and G-VRAM areas.
	 *  - 640x400 4-bpp
	 *  - INT 18h, AH=42h, CH=C0h
	 */
	r.w.ax = 0x4200; 
	r.h.ch = 192; 
	int386(0x18, &r, &r);

	outp(0x6a, 1);

	/* Hide Text VRAM. */
	text = (volatile uint16_t *)TVRAM_TEXT;
	attr = (volatile uint16_t *)TVRAM_ATTR;
	for (i = 0; i < 80 * 25; i++) {
		text[i] = 0x0000;
		attr[i] = 0x0000;
	}

	/*
	 * Start displaying G-VRAM.
	 *  - INT 18h, AH=40h
	 */
	r.w.ax = 0x4000;
	int386(0x18, &r, &r);

	fb_bpp = 4;
	ofs_x = (SCREEN_WIDTH - game_width) / 2;
	ofs_y = (SCREEN_HEIGHT - game_height) / 2;
}

/*
 * CIRRUS CL-GD54xx
 */

/*
 * Initialize the CIRRUS chip and set 640x480x24bpp.
 *
 * There is no VGA BIOS on PC-98, so the full VGA register set is
 * programmed by hand. Values follow the Linux cirrusfb driver and
 * the standard 640x480@60Hz (25.175MHz dot clock) timing.
 */
static bool
init_vram_cirrus(void)
{
	/* Standard VGA CRTC values for 640x480, plus our pitch. */
	static const uint8_t crtc_tab[] = {
		0x5f,	/* 00: Horizontal Total */
		0x4f,	/* 01: Horizontal Display End */
		0x50,	/* 02: Horizontal Blanking Start */
		0x82,	/* 03: Horizontal Blanking End */
		0x54,	/* 04: Horizontal Sync Start */
		0x80,	/* 05: Horizontal Sync End */
		0x0b,	/* 06: Vertical Total */
		0x3e,	/* 07: Overflow */
		0x00,	/* 08: Preset Row Scan */
		0x40,	/* 09: Max Scan Line */
		0x20,	/* 0A: Cursor Start (off) */
		0x00,	/* 0B: Cursor End */
		0x00,	/* 0C: Start Address High */
		0x00,	/* 0D: Start Address Low */
		0x00,	/* 0E: Cursor Location High */
		0x00,	/* 0F: Cursor Location Low */
		0xea,	/* 10: Vertical Sync Start */
		0x0c,	/* 11: Vertical Sync End (unprotected) */
		0xdf,	/* 12: Vertical Display End */
		0xf0,	/* 13: Offset (pitch/8 = 1920/8 = 240) */
		0x00,	/* 14: Underline (byte mode) */
		0xe7,	/* 15: Vertical Blanking Start */
		0x04,	/* 16: Vertical Blanking End */
		0xc3,	/* 17: Mode Control */
		0xff	/* 18: Line Compare */
	};
	int i, sr07;
	bool is_alpine;

	if (!detect_cirrus())
		return false;

	/*
	 * Wake up the video subsystem.
	 * 0904h bit5 enables access to POS102 (FF82h); POS102 bit0
	 * and Sleep Address (3C3 -> 0CA3h) bit0 enable the chip.
	 */
	outp(WAB_P904, 0x20);
	outp(WAB_PFF82, 0x01);
	outp(P_SLEEP, 0x01);

	/* Enable WAB registers, keep the 98 GDC on screen for now. */
	wab_write(WAB_REG_RELAY, WAB_RELAY_GDC);

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
	 */
	if (is_alpine) {
		/* 3 x 25.175 = 75.525MHz -> N=95, D=18, P=0 (75.57MHz) */
		seq_write(0x0e, 0x5f);
		seq_write(0x1e, 0x24);
	} else {
		/* 25.175MHz (5428 does not need x3) -> N=74, D=21, P=1 */
		seq_write(0x0e, 0x4a);
		seq_write(0x1e, 0x2b);
	}

	/*
	 * Miscellaneous Output: negative H/V sync (480-line mode),
	 * clock select = VCLK3, display memory enabled, color I/O.
	 */
	outp(P_MISC_W, 0xcf);

	/* CRTC: unprotect CR0-7 (CR11 bit7), then program the table. */
	crtc_write(0x11, crtc_read(0x11) & 0x7f);
	for (i = 0; i < (int)sizeof(crtc_tab); i++)
		crtc_write(i, crtc_tab[i]);
	crtc_write(0x11, 0x8c);	/* re-protect */
	crtc_write(0x1a, 0x00);	/* no interlace */
	crtc_write(0x1b, 0x22);	/* ext display: 16bit wrap, pitch bit8=0 */

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

	return true;
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

/*
 * Flip
 */

/*
 * Blit back image to VRAM. (CIRRUS 24-bpp)
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R.
 * Cirrus 24bpp VRAM layout is also B, G, R, so the low 3 bytes of
 * each pixel are copied as-is.
 */
static void
flip_24bpp(void)
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
 * Blit back image to VRAM. (PC-98 GDC 4-bpp)
 */
static void flip_4bpp(void)
{
	volatile unsigned char *vram_b;
	volatile unsigned char *vram_r;
	volatile unsigned char *vram_g;
	volatile unsigned char *vram_i;
	volatile uint32_t *pixels;
	int x, y, bit;
	int dst_index;

	vram_b = (volatile unsigned char *)GVRAM_B;
	vram_r = (volatile unsigned char *)GVRAM_R;
	vram_g = (volatile unsigned char *)GVRAM_G;
	vram_i = (volatile unsigned char *)GVRAM_I;
	pixels = back_image->pixels;

	for (y = 0; y < SCREEN_HEIGHT; y++) {
		if (y >= game_height)
			break;

		for (x = 0; x < LINE_BYTES; x++) {
			unsigned char pb = 0;
			unsigned char pr = 0;
			unsigned char pg = 0;
			unsigned char pi = 0;

			if (x >= game_width >> 3)
				break;

			for (bit = 0; bit < 8; bit++) {
				int sx = x * 8 + bit;
				uint32_t pix;
				unsigned char r, g, b;
				unsigned char mask;

				pix = pixels[y * game_width + sx];
				/* StratoHAL pixel layout (BGRA):
				   low byte = B, then G, then R */
				b = pix & 0xff;
				g = (pix >> 8) & 0xff;
				r = (pix >> 16) & 0xff;

				mask = (unsigned char)(0x80 >> bit);

				if (b >= 200)
					pb |= mask;
				if (g >= 200)
					pg |= mask;
				if (r >= 200)
					pr |= mask;
				if ((r | g | b) >= 128)
					pi |= mask;
			}

			dst_index = (y + ofs_y) * LINE_BYTES + x + (ofs_x >> 3);

			vram_b[dst_index] = pb;
			vram_r[dst_index] = pr;
			vram_g[dst_index] = pg;
			vram_i[dst_index] = pi;
		}
	}
}

/*
 * Input
 */

static void process_input(void)
{
	static bool is_return_key_pressed = false;
	static bool is_space_key_pressed = false;
	static bool is_up_key_pressed = false;
	static bool is_down_key_pressed = false;
	static bool is_left_key_pressed = false;
	static bool is_right_key_pressed = false;
	bool next_is_return_key_pressed = false;
	bool next_is_space_key_pressed = false;
	bool next_is_up_key_pressed = false;
	bool next_is_down_key_pressed = false;
	bool next_is_left_key_pressed = false;
	bool next_is_right_key_pressed = false;

	while (1) {
		int ch;

		if (!kbhit())
			break;

		ch = getch();
		switch (ch) {
		case '\r':
			next_is_return_key_pressed = true;
			break;
		case ' ':
			next_is_space_key_pressed = true;
			break;
		case 0x00:
		case 0xe0:
			/* Extended key. */
			if (!kbhit())
				break;
			ch = getch();
			switch (ch) {
			case 0x100 | 0x48:
				next_is_up_key_pressed = true;
				break;
			case 0x100 | 0x50:
				next_is_down_key_pressed = true;
				break;
			case 0x100 | 0x4b:
				next_is_left_key_pressed = true;
				break;
			case 0x100 | 0x4d:
				next_is_right_key_pressed = true;
				break;
			}
			break;
		}
	}

	if (!is_return_key_pressed && next_is_return_key_pressed)
		hal_callback.on_key_press(HAL_KEY_RETURN);
	if (is_return_key_pressed && !next_is_return_key_pressed)
		hal_callback.on_key_release(HAL_KEY_RETURN);
	is_return_key_pressed = next_is_return_key_pressed;

	if (!is_space_key_pressed && next_is_space_key_pressed)
		hal_callback.on_key_press(HAL_KEY_SPACE);
	if (is_space_key_pressed && !next_is_space_key_pressed)
		hal_callback.on_key_release(HAL_KEY_SPACE);
	is_space_key_pressed = next_is_space_key_pressed;

	if (!is_up_key_pressed && next_is_up_key_pressed)
		hal_callback.on_key_press(HAL_KEY_UP);
	if (is_up_key_pressed && !next_is_up_key_pressed)
		hal_callback.on_key_release(HAL_KEY_UP);
	is_up_key_pressed = next_is_up_key_pressed;
		
	if (!is_down_key_pressed && next_is_down_key_pressed)
		hal_callback.on_key_press(HAL_KEY_DOWN);
	if (is_down_key_pressed && !next_is_down_key_pressed)
		hal_callback.on_key_release(HAL_KEY_DOWN);
	is_down_key_pressed = next_is_down_key_pressed;

	if (!is_left_key_pressed && next_is_left_key_pressed)
		hal_callback.on_key_press(HAL_KEY_LEFT);
	if (is_left_key_pressed && !next_is_left_key_pressed)
		hal_callback.on_key_release(HAL_KEY_LEFT);
	is_left_key_pressed = next_is_left_key_pressed;

	if (!is_right_key_pressed && next_is_right_key_pressed)
		hal_callback.on_key_press(HAL_KEY_RIGHT);
	if (is_right_key_pressed && !next_is_right_key_pressed)
		hal_callback.on_key_release(HAL_KEY_RIGHT);
	is_right_key_pressed = next_is_right_key_pressed;
}

/*
 * HAL
 */

void hal_notify_image_update(struct hal_image *img)
{
	UNUSED_PARAMETER(img);
}

void hal_notify_image_free(struct hal_image *img)
{
	UNUSED_PARAMETER(img);
}

void
hal_render_image_normal(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_alpha(
		back_image,
		dst_left,
		dst_top,
		src_image,
		src_width,
		src_height,
		src_left,
		src_top,
		alpha);
}

void
hal_render_image_add(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The width of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_add(
		back_image,
		dst_left,
		dst_top,
		src_image,
		src_width,
		src_height,
		src_left,
		src_top,
		alpha);
}

void
hal_render_image_sub(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The width of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_sub(
		back_image,
		dst_left,
		dst_top,
		src_image,
		src_width,
		src_height,
		src_left,
		src_top,
		alpha);
}

void
hal_render_image_dim(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_dim(
		back_image,
		dst_left,
		dst_top,
		src_image,
		src_width,
		src_height,
		src_left,
		src_top,
		alpha);
}

void
hal_render_image_rule(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int threshold)			/* The threshold (0 to 255) */
{
	hal_draw_image_rule(back_image, src_img, rule_img, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int progress)			/* The progress (0 to 255) */
{
	hal_draw_image_melt(back_image, src_img, rule_img, progress);
}

void
hal_render_image_cross(
	struct hal_image *src1_img,
	struct hal_image *src2_img,
	float src1_left,
	float src1_top,
	float src2_left,
	float src2_top,
	int alpha)
{
	hal_draw_image_cross(back_image,
			     src1_img,
			     src2_img,
			     src1_left,
			     src1_top,
			     src2_left,
			     src2_top,
			     alpha);
}

void
hal_render_image_3d_normal(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct hal_image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	hal_draw_image_3d_alpha(back_image,
				(float)x1,
				(float)y1,
				(float)x2,
				(float)y2,
				(float)x3,
				(float)y3,
				(float)x4,
				(float)y4,
				src_image,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha);
}

void
hal_render_image_3d_add(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct hal_image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	hal_draw_image_3d_alpha(back_image,
				(float)x1,
				(float)y1,
				(float)x2,
				(float)y2,
				(float)x3,
				(float)y3,
				(float)x4,
				(float)y4,
				src_image,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha);
}

void
hal_render_image_3d_sub(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct hal_image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	hal_draw_image_3d_sub(back_image,
			      (float)x1,
			      (float)y1,
			      (float)x2,
			      (float)y2,
			      (float)x3,
			      (float)y3,
			      (float)x4,
			      (float)y4,
			      src_image,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      alpha);
}

void
hal_render_image_3d_dim(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct hal_image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	hal_draw_image_3d_dim(back_image,
			      (float)x1,
			      (float)y1,
			      (float)x2,
			      (float)y2,
			      (float)x3,
			      (float)y3,
			      (float)x4,
			      (float)y4,
			      src_image,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      alpha);
}

void
hal_render_image_3d_cross(
	struct hal_image *src1_img,
	struct hal_image *src2_img,
	float src1_x1,
	float src1_y1,
	float src1_x2,
	float src1_y2,
	float src1_x3,
	float src1_y3,
	float src1_x4,
	float src1_y4,
	float src2_x1,
	float src2_y1,
	float src2_x2,
	float src2_y2,
	float src2_x3,
	float src2_y3,
	float src2_x4,
	float src2_y4,
	int alpha)
{
	hal_draw_image_3d_cross(back_image,
				src1_img,
				src2_img,
				src1_x1,
				src1_y1,
				src1_x2,
				src1_y2,
				src1_x3,
				src1_y3,
				src1_x4,
				src1_y4,
				src2_x1,
				src2_y1,
				src2_x2,
				src2_y2,
				src2_x3,
				src2_y3,
				src2_x4,
				src2_y4,
				alpha);
}

static uint32_t get_time(void)
{
	union REGS r;
	uint32_t tick;

	r.h.al = 0xff;
	r.h.ah = 0x80;
	int386(0x1c, &r, &r);

	tick = (r.w.cx << 16) | r.w.dx;

	return tick * 1000 / 32;
}

void
hal_reset_lap_timer(
	uint64_t *t)
{
	*t = (uint64_t)get_time();
}

uint64_t
hal_get_lap_timer_millisec(
	uint64_t *t)
{
	uint64_t end;
	
	end = (uint64_t)get_time();

	return (uint64_t)((end - *t));
}

bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	UNUSED_PARAMETER(fname);
	UNUSED_PARAMETER(is_skippable);
	return true;
}

void
hal_stop_video(void)
{
}

bool
hal_is_video_playing(void)
{
	return false;
}

bool
hal_is_full_screen_supported(void)
{
	return false;
}

bool
hal_is_full_screen_mode(void)
{
	return false;
}

void
hal_enter_full_screen_mode(void)
{
}

void
hal_leave_full_screen_mode(void)
{
}

bool
make_save_directory(void)
{
	return true;
}

char *
make_real_path(const char *fname)
{
	char *s, *t;

	s = strdup(fname);
	if (s == NULL) {
		hal_log_out_of_memory();
		return NULL;
	}

	t = s;
	while (*t != '\0') {
		if (*t == '/')
			*t = '\\';
		t++;
	}

	return s;
}

bool
hal_log_info(
	const char *s,
	...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}
	printf("%s\n", buf);

	return true;
}

bool
hal_log_warn(
	const char *s,
	...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}
	printf("%s\n", buf);

	return true;
}

bool
hal_log_error(
	const char *s,
	...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}
	printf("%s\n", buf);
	
	return true;
}

static bool
open_log_file(void)
{
	if (log_fp == NULL) {
		log_fp = fopen(LOG_FILE, "w");
		if (log_fp == NULL) {
			printf("Can't open log file.\n");
			return false;
		}
	}
	return true;
}

bool
hal_log_out_of_memory(void)
{
	hal_log_error("Out of memory.\n");
	return true;
}

const char *
hal_get_system_language(void)
{
	return "ja";
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}

/*
 * Missing C99
 */

double rint(double x)
{
	return floor(x + 0.5);
}
