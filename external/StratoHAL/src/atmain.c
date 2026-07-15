/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Main code for IBM PC/AT VESA VBE 2.0 (DOS/4GW)
 *   - 640x480, 32bpp, linear framebuffer
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

/* Screen Size */
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define SCREEN_BPP	32

/* VBE mode attribute bits */
#define VBE_ATTR_SUPPORTED	0x0001
#define VBE_ATTR_GRAPHICS	0x0010
#define VBE_ATTR_LFB		0x0080

/* VBE set-mode flag: use linear framebuffer */
#define VBE_MODE_LFB		0x4000

/* VBE memory model: direct color */
#define VBE_MM_DIRECT		0x06

/* VRAM address */
#define VGA_VRAM	0x000A0000UL

/* VGA line bytes */
#define VGA_LINE_BYTES	(640 / 8)

/*
 * VBE 2.0 structures (must be byte-packed)
 */
#pragma pack(push, 1)

struct vbe_info {
	char		VbeSignature[4];	/* "VESA" */
	uint16_t	VbeVersion;		/* 0x0200 or higher */
	uint32_t	OemStringPtr;		/* real mode far pointer */
	uint32_t	Capabilities;
	uint32_t	VideoModePtr;		/* real mode far pointer */
	uint16_t	TotalMemory;		/* in 64KB blocks */
	uint16_t	OemSoftwareRev;
	uint32_t	OemVendorNamePtr;
	uint32_t	OemProductNamePtr;
	uint32_t	OemProductRevPtr;
	uint8_t		Reserved[222];
	uint8_t		OemData[256];
};						/* 512 bytes */

struct vbe_mode_info {
	uint16_t	ModeAttributes;
	uint8_t		WinAAttributes;
	uint8_t		WinBAttributes;
	uint16_t	WinGranularity;
	uint16_t	WinSize;
	uint16_t	WinASegment;
	uint16_t	WinBSegment;
	uint32_t	WinFuncPtr;
	uint16_t	BytesPerScanLine;
	uint16_t	XResolution;
	uint16_t	YResolution;
	uint8_t		XCharSize;
	uint8_t		YCharSize;
	uint8_t		NumberOfPlanes;
	uint8_t		BitsPerPixel;
	uint8_t		NumberOfBanks;
	uint8_t		MemoryModel;
	uint8_t		BankSize;
	uint8_t		NumberOfImagePages;
	uint8_t		Reserved1;
	uint8_t		RedMaskSize;
	uint8_t		RedFieldPosition;
	uint8_t		GreenMaskSize;
	uint8_t		GreenFieldPosition;
	uint8_t		BlueMaskSize;
	uint8_t		BlueFieldPosition;
	uint8_t		RsvdMaskSize;
	uint8_t		RsvdFieldPosition;
	uint8_t		DirectColorModeInfo;
	uint32_t	PhysBasePtr;		/* LFB physical address */
	uint32_t	OffScreenMemOffset;
	uint16_t	OffScreenMemSize;
	uint8_t		Reserved2[206];
};						/* 256 bytes */

/* DPMI 0x0300: real mode interrupt call structure */
struct rminfo {
	uint32_t	edi;
	uint32_t	esi;
	uint32_t	ebp;
	uint32_t	reserved;
	uint32_t	ebx;
	uint32_t	edx;
	uint32_t	ecx;
	uint32_t	eax;
	uint16_t	flags;
	uint16_t	es;
	uint16_t	ds;
	uint16_t	fs;
	uint16_t	gs;
	uint16_t	ip;
	uint16_t	cs;
	uint16_t	sp;
	uint16_t	ss;
};

#pragma pack(pop)

/* Log */
#define LOG_FILE	"log.txt"

/* Game Info */
static char *game_title;
static int game_width;
static int game_height;

/* Screen */
static struct hal_image *back_image;
static uint8_t *fb;		/* mapped linear framebuffer */
static int fb_stride;		/* bytes per scanline */
static int fb_bpp;		/* 32 (or 24 as a fallback) */
static uint16_t vesa_mode;	/* selected VBE mode number */
static int ofs_x;
static int ofs_y;

/* Precomputed channel shifts (from VBE mode info) */
static int shift_r;
static int shift_g;
static int shift_b;

/* Precomputed right shifts to reduce 8-bit channels (8 - mask size) */
static int rshift_r;
static int rshift_g;
static int rshift_b;

/* DOS (conventional) memory block for real mode VBE calls */
static uint16_t dos_seg;	/* real mode segment */
static uint16_t dos_sel;	/* protected mode selector (for freeing) */

/* Alpha blend table. */
uint8_t alphatable[256][256];

/* Log */
static FILE *log_fp;

/* Callback. */
static struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* argc/argv */
int hal_argc;
char **hal_argv;

/* Forward Declaration */
static void init_alphatable(void);
static void init_vram(void);
static void init_vram_vga_fallback(void);
static void cleanup_vram(void);
static void flip_32bpp(void);
static void flip_24bpp(void);
static void flip_16bpp(void);
static void flip_4bpp(void);
static void process_input(void);
static bool dpmi_dos_alloc(int paragraphs, uint16_t *seg, uint16_t *sel);
static void dpmi_dos_free(uint16_t sel);
static void dpmi_rm_int(int inum, struct rminfo *rmi);
static void *dpmi_map_physical(uint32_t phys, uint32_t size);
static bool vbe_get_info(struct vbe_info *info);
static bool vbe_get_mode_info(uint16_t mode, struct vbe_mode_info *mi);
static bool vbe_set_mode(uint16_t mode);
static bool find_vbe_mode(const struct vbe_info *info, int bpp, uint16_t *mode_ret, struct vbe_mode_info *mi_ret);
static bool open_log_file(void);

int hal_main(int argc, char *argv[])
{
	hal_argc = argc;
	hal_argv = argv;

	printf("\n"
	       "Suika3 Game Engine for IBM PC/AT VESA VBE 2.0\n"
	       "Copyright (c) 2026 Awe Morris\n");

	if (argc >= 2) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("Version 2026.07\n");
			return 0;
		}
	}

	/* Initialize the asset package file. */
	if (!init_file()) {
		printf("Failed to initialize the file system.\n");
		return 1;
	}

	/* Initialize the downstream app that uses this StratoHAL. */
	if (!hal_bootstrap_ptr(
		    &game_title,
		    &game_width,
		    &game_height,
		    &hal_callback)) {
		printf("Error on boot.\n");
		return 1;
	}

	/* Check for the game screen size. */
	if (game_width > 640 || game_height > 480) {
		printf("Screen size too large.\n");
		return 1;
	}

	/* Calculate the viewport offset. */
	ofs_x = (640 - game_width) / 2;
	ofs_y = (480 - game_height) / 2;

	/* Create the back image. */
	if (!hal_create_image(game_width, game_height, &back_image)) {
		printf("Error on creating image.\n");
		return 1;
	}

	/* Do start callback. */
	if (!hal_callback.on_start()) {
		printf("Error on start.\n");
		return 1;
	}

	/* Initialize a VBE/VGA graphics mode. */
	init_vram();

	/* Game loop. */
	while (1) {
		/* Process inputs. */
		process_input();

		/* Clear the back image. */
		hal_clear_image(back_image, 0);

		/* Frame update. */
		if (!hal_callback.on_update())
			break;

		/* Frame rendering. */
		hal_callback.on_render();

		/* Flip the back image to VRAM. */
		if (fb_bpp == 32)
			flip_32bpp();
		else if (fb_bpp == 24)
			flip_24bpp();
		else if (fb_bpp == 16 || fb_bpp == 15)
			flip_16bpp();
		else
			flip_4bpp();
	}

	/* Finish using the graphics mode. */
	cleanup_vram();

	return 0;
}

static void
init_alphatable(void)
{
	int a, b;

	for (a = 0; a < 256; a++) {
		for (b = 0; b < 256; b++) {
			alphatable[a][b] = (uint8_t)(int)(((float)a / 255.0f) * ((float)b / 255.0f) * 255.0f);
		}
	}
}

/* Initialize G-VRAM. */
static void
init_vram(void)
{
	struct vbe_info info;
	struct vbe_mode_info mi;
	uint32_t fb_size;
	int y;

	/*
	 * Allocate a 512-byte conventional memory buffer for the
	 * real mode VBE calls. (512 bytes = 32 paragraphs)
	 */
	if (!dpmi_dos_alloc(512 / 16, &dos_seg, &dos_sel)) {
		printf("Can't allocate DOS memory.\n");
		printf("Fallback to VGA.\n");

		/* VGA */
		fb_bpp = 4;
		init_vram_vga_fallback();
		return;
	}

	if (!vbe_get_info(&info)) {
		printf("VESA VBE 2.0 not available.\n");
		printf("Fallback to VGA.\n");

		/* VGA */
		fb_bpp = 4;
		init_vram_vga_fallback();
		return;
	}

	/* Prefer 32bpp; fall back to 24bpp, then 16bpp/15bpp. */
	if (find_vbe_mode(&info, 32, &vesa_mode, &mi)) {
		printf("%dx%dx32 LFB mode found.\n", SCREEN_WIDTH, SCREEN_HEIGHT);

		/* VBE */
		fb_bpp = 32;
	} else if (find_vbe_mode(&info, 24, &vesa_mode, &mi)) {
		printf("%dx%dx24 LFB mode found.\n", SCREEN_WIDTH, SCREEN_HEIGHT);

		/* VBE */
		fb_bpp = 24;
	} else if (find_vbe_mode(&info, 16, &vesa_mode, &mi)) {
		printf("%dx%dx16 LFB mode found.\n", SCREEN_WIDTH, SCREEN_HEIGHT);

		/* VBE (usually 5:6:5) */
		fb_bpp = 16;
	} else if (find_vbe_mode(&info, 15, &vesa_mode, &mi)) {
		printf("%dx%dx15 LFB mode found.\n", SCREEN_WIDTH, SCREEN_HEIGHT);

		/* VBE (5:5:5, still 2 bytes per pixel) */
		fb_bpp = 15;
	} else {
		printf("No %dx%d direct color LFB mode found.\n", SCREEN_WIDTH, SCREEN_HEIGHT);
		printf("Fallback to VGA.\n");

		/* VGA */
		fb_bpp = 4;
		init_vram_vga_fallback();
		return;
	}

	fb_stride = mi.BytesPerScanLine;
	shift_r = mi.RedFieldPosition;
	shift_g = mi.GreenFieldPosition;
	shift_b = mi.BlueFieldPosition;
	rshift_r = 8 - mi.RedMaskSize;
	rshift_g = 8 - mi.GreenMaskSize;
	rshift_b = 8 - mi.BlueMaskSize;

	/*
	 * Some BIOSes leave the direct color fields zero for 8:8:8
	 * modes. Fall back to the canonical layout in that case.
	 */
	if (mi.RedMaskSize == 0 || mi.GreenMaskSize == 0 ||
	    mi.BlueMaskSize == 0) {
		if (fb_bpp == 32 || fb_bpp == 24) {
			/* B(0) G(8) R(16) */
			shift_r = 16; shift_g = 8; shift_b = 0;
			rshift_r = 0; rshift_g = 0; rshift_b = 0;
		} else if (fb_bpp == 16) {
			/* 5:6:5 */
			shift_r = 11; shift_g = 5; shift_b = 0;
			rshift_r = 3; rshift_g = 2; rshift_b = 3;
		} else {
			/* 15bpp, 5:5:5 */
			shift_r = 10; shift_g = 5; shift_b = 0;
			rshift_r = 3; rshift_g = 3; rshift_b = 3;
		}
	}

	/* Map the linear framebuffer into our address space. */
	fb_size = (uint32_t)fb_stride * SCREEN_HEIGHT;
	fb = (uint8_t *)dpmi_map_physical(mi.PhysBasePtr, fb_size);
	if (fb == NULL) {
		printf("Can't map the linear framebuffer.\n");
		printf("Fallback to VGA.\n");

		/* VGA */
		fb_bpp = 4;
		init_vram_vga_fallback();
		return;
	}

	if (!vbe_set_mode(vesa_mode)) {
		printf("Can't set VBE mode 0x%03x.\n", vesa_mode);
		printf("Fallback to VGA.\n");

		/* VGA */
		fb_bpp = 4;
		init_vram_vga_fallback();
		return;
	}

	/* Clear the screen. */
	for (y = 0; y < SCREEN_HEIGHT; y++)
		memset(fb + y * fb_stride, 0, (size_t)fb_stride);
}

static INLINE void
set_vga_plane(int plane)
{
	/* Sequencer Map Mask */
	outp(0x3c4, 0x02);
	outp(0x3c5, (unsigned char)(1 << plane));
}

/* Initialize G-VRAM (VGA 4-bpp fallback). */
static void
init_vram_vga_fallback(void)
{
	union REGS r;
	volatile unsigned char *vram;
	int plane, i;

	/*
	 * VGA mode 12h:
	 *   640x480, 16 colors, planar 4bpp
	 */
	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0012;
	int386(0x10, &r, &r);

	vram = (volatile unsigned char *)VGA_VRAM;

	/* Clear all VGA planes. */
	for (plane = 0; plane < 4; plane++) {
		set_vga_plane(plane);
		for (i = 0; i < VGA_LINE_BYTES * SCREEN_HEIGHT; i++)
			vram[i] = 0;
	}
}

/* Cleanup G-VRAM. */
static void
cleanup_vram(void)
{
	union REGS r;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0003;        /* 80x25 text mode */
	int386(0x10, &r, &r);

	if (dos_sel != 0) {
		dpmi_dos_free(dos_sel);
		dos_sel = 0;
	}
}

/*
 * Convert a StratoHAL pixel to the display pixel format.
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R,
 * i.e. 0xAARRGGBB as a little endian uint32.
 * Most VBE 32/24bpp direct color modes use the same B(0) G(8) R(16)
 * layout, but we honor the field positions and mask sizes reported
 * by the mode info, so this also handles 16bpp (5:6:5) and 15bpp
 * (5:5:5).
 */
static INLINE uint32_t
rgb_to_fb(
	uint32_t pix)
{
	uint32_t r, g, b;

	b = pix & 0xff;
	g = (pix >> 8) & 0xff;
	r = (pix >> 16) & 0xff;

	return ((r >> rshift_r) << shift_r) |
	       ((g >> rshift_g) << shift_g) |
	       ((b >> rshift_b) << shift_b);
}

/*
 * Blit back image to VRAM. (32-bpp)
 */
static void
flip_32bpp(void)
{
	uint32_t *pixels;
	uint8_t *dst_line;
	int x, y;

	pixels = back_image->pixels;
	dst_line = fb + ofs_y * fb_stride + ofs_x * 4;

	if (shift_r == 16 && shift_g == 8 && shift_b == 0 &&
	    rshift_r == 0 && rshift_g == 0 && rshift_b == 0) {
		/*
		 * The framebuffer layout matches the StratoHAL BGRA
		 * pixel layout, so rows can be copied directly.
		 * Note: copy game_width pixels, not fb_stride bytes,
		 * to avoid overrunning the back image rows.
		 */
		for (y = 0; y < game_height; y++) {
			memcpy(dst_line,
			       pixels + y * game_width,
			       (size_t)game_width * 4);
			dst_line += fb_stride;
		}
	} else {
		/* Unusual channel layout: convert per pixel. */
		for (y = 0; y < game_height; y++) {
			uint32_t *src = pixels + y * game_width;
			uint32_t *dst = (uint32_t *)dst_line;

			for (x = 0; x < game_width; x++)
				dst[x] = rgb_to_fb(src[x]);

			dst_line += fb_stride;
		}
	}
}

/*
 * Blit back image to VRAM. (24-bpp)
 */
static void
flip_24bpp(void)
{
	uint32_t *pixels;
	uint8_t *dst_line;
	int x, y;

	pixels = back_image->pixels;
	dst_line = fb + ofs_y * fb_stride + ofs_x * 3;

	/* 24bpp: 3 bytes per pixel */
	for (y = 0; y < game_height; y++) {
		uint32_t *src = pixels + y * game_width;
		uint8_t *dst = dst_line;

		for (x = 0; x < game_width; x++) {
			uint32_t out = rgb_to_fb(src[x]);

			dst[0] = (uint8_t)(out & 0xff);
			dst[1] = (uint8_t)((out >> 8) & 0xff);
			dst[2] = (uint8_t)((out >> 16) & 0xff);
			dst += 3;
		}

		dst_line += fb_stride;
	}
}

/*
 * Blit back image to VRAM. (16-bpp / 15-bpp)
 */
static void
flip_16bpp(void)
{
	uint32_t *pixels;
	uint8_t *dst_line;
	int x, y;

	pixels = back_image->pixels;
	dst_line = fb + ofs_y * fb_stride + ofs_x * 2;

	/* 16bpp (5:6:5) or 15bpp (5:5:5): 2 bytes per pixel */
	for (y = 0; y < game_height; y++) {
		uint32_t *src = pixels + y * game_width;
		uint16_t *dst = (uint16_t *)dst_line;

		for (x = 0; x < game_width; x++)
			dst[x] = (uint16_t)rgb_to_fb(src[x]);

		dst_line += fb_stride;
	}
}

static INLINE unsigned char
rgb_to_vga16(uint32_t pix)
{
	unsigned char r, g, b;
	unsigned char c = 0;

	/* StratoHAL pixel layout (BGRA): low byte = B, then G, then R */
	b = pix & 0xff;
	g = (pix >> 8) & 0xff;
	r = (pix >> 16) & 0xff;

	if (b >= 200)
		c |= 0x01;       /* VGA blue */
	if (g >= 200)
		c |= 0x02;       /* VGA green */
	if (r >= 200)
		c |= 0x04;       /* VGA red */
	if ((r | g | b) >= 128)
		c |= 0x08;       /* intensity */

	return c;
}

/*
 * Blit back image to VRAM. (4-bpp)
 */
static void
flip_4bpp(void)
{
	volatile unsigned char *vram;
	uint32_t *pixels;
	int plane, x, y, bit;

	vram = (volatile unsigned char *)VGA_VRAM;
	pixels = back_image->pixels;

	for (plane = 0; plane < 4; plane++) {
		set_vga_plane(plane);

		for (y = 0; y < SCREEN_HEIGHT; y++) {
			if (y >= game_height)
				break;

			for (x = 0; x < VGA_LINE_BYTES; x++) {
				unsigned char out = 0;

				if (x >= (game_width >> 3))
					break;

				for (bit = 0; bit < 8; bit++) {
					int sx;
					uint32_t pix;
					unsigned char c;
					unsigned char mask;

					sx = x * 8 + bit;
					pix = pixels[y * game_width + sx];

					c = rgb_to_vga16(pix);
					mask = (unsigned char)(0x80 >> bit);

					if (c & (1 << plane))
						out |= mask;
				}

				vram[(y + ofs_y) * VGA_LINE_BYTES + x + (ofs_x >> 3)] = out;
			}
		}
	}
}

/*
 * Process inputs.
 */
static void
process_input(void)
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
 * DPMI helpers (DOS/4GW)
 */

/* DPMI 0x0100: Allocate DOS conventional memory. */
static bool
dpmi_dos_alloc(
	int paragraphs,
	uint16_t *seg,
	uint16_t *sel)
{
	union REGS r;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0100;
	r.w.bx = (uint16_t)paragraphs;
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return false;

	*seg = r.w.ax;
	*sel = r.w.dx;
	return true;
}

/* DPMI 0x0101: Free DOS conventional memory. */
static void
dpmi_dos_free(
	uint16_t sel)
{
	union REGS r;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0101;
	r.w.dx = sel;
	int386(0x31, &r, &r);
}

/* DPMI 0x0300: Simulate a real mode interrupt. */
static void
dpmi_rm_int(
	int inum,
	struct rminfo *rmi)
{
	union REGS r;
	struct SREGS s;

	memset(&r, 0, sizeof(r));
	segread(&s);
	s.es = s.ds;		/* flat model: ES:EDI -> rmi */

	r.w.ax = 0x0300;
	r.h.bl = (unsigned char)inum;
	r.h.bh = 0;
	r.w.cx = 0;		/* words to copy from PM stack */
	r.x.edi = (uint32_t)rmi;

	int386x(0x31, &r, &r, &s);
}

/* DPMI 0x0800: Map a physical address into linear address space. */
static void *
dpmi_map_physical(
	uint32_t phys,
	uint32_t size)
{
	union REGS r;

	/* DOS/4GW identity-maps the first megabyte. */
	if (phys < 0x100000UL)
		return (void *)phys;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0800;
	r.w.bx = (uint16_t)(phys >> 16);
	r.w.cx = (uint16_t)(phys & 0xffff);
	r.w.si = (uint16_t)(size >> 16);
	r.w.di = (uint16_t)(size & 0xffff);
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return NULL;

	/*
	 * DOS/4GW uses a zero-based flat address space, so the
	 * returned linear address is directly usable as a pointer.
	 */
	return (void *)(((uint32_t)r.w.bx << 16) | r.w.cx);
}

/* Convert a real mode far pointer (seg:ofs) to a flat pointer. */
static INLINE void *
rm_ptr(
	uint32_t rm_far)
{
	return (void *)(((rm_far >> 16) << 4) + (rm_far & 0xffff));
}

/*
 * VBE 2.0 calls
 *
 * These functions run in real mode, so their buffers must live in
 * conventional memory (dos_seg). Results are copied back out.
 */

/* VBE Function 00h: Return VBE Controller Information. */
static bool
vbe_get_info(
	struct vbe_info *info)
{
	struct rminfo rmi;
	uint8_t *buf;

	buf = (uint8_t *)((uint32_t)dos_seg << 4);

	/* Request VBE 2.0 extended information. */
	memset(buf, 0, sizeof(struct vbe_info));
	memcpy(buf, "VBE2", 4);

	memset(&rmi, 0, sizeof(rmi));
	rmi.eax = 0x4f00;
	rmi.es = dos_seg;
	rmi.edi = 0;
	dpmi_rm_int(0x10, &rmi);
	if ((rmi.eax & 0xffff) != 0x004f)
		return false;

	memcpy(info, buf, sizeof(struct vbe_info));

	if (memcmp(info->VbeSignature, "VESA", 4) != 0)
		return false;
	if (info->VbeVersion < 0x0200)
		return false;

	return true;
}

/* VBE Function 01h: Return VBE Mode Information. */
static bool
vbe_get_mode_info(
	uint16_t mode,
	struct vbe_mode_info *mi)
{
	struct rminfo rmi;
	uint8_t *buf;

	buf = (uint8_t *)((uint32_t)dos_seg << 4);
	memset(buf, 0, sizeof(struct vbe_mode_info));

	memset(&rmi, 0, sizeof(rmi));
	rmi.eax = 0x4f01;
	rmi.ecx = mode;
	rmi.es = dos_seg;
	rmi.edi = 0;
	dpmi_rm_int(0x10, &rmi);
	if ((rmi.eax & 0xffff) != 0x004f)
		return false;

	memcpy(mi, buf, sizeof(struct vbe_mode_info));
	return true;
}

/* VBE Function 02h: Set VBE Mode (with linear framebuffer). */
static bool
vbe_set_mode(
	uint16_t mode)
{
	struct rminfo rmi;

	memset(&rmi, 0, sizeof(rmi));
	rmi.eax = 0x4f02;
	rmi.ebx = (uint32_t)mode | VBE_MODE_LFB;
	dpmi_rm_int(0x10, &rmi);

	return (rmi.eax & 0xffff) == 0x004f;
}

/*
 * Search the VBE mode list for SCREEN_WIDTH x SCREEN_HEIGHT with the
 * given color depth and LFB support.
 */
static bool
find_vbe_mode(
	const struct vbe_info *info,
	int bpp,
	uint16_t *mode_ret,
	struct vbe_mode_info *mi_ret)
{
	uint16_t modes[256];
	uint16_t *list;
	int count, i;

	/*
	 * VideoModePtr may point into the info block's own buffer,
	 * which we will reuse for mode info calls, so copy the list
	 * out first.
	 */
	list = (uint16_t *)rm_ptr(info->VideoModePtr);
	count = 0;
	while (count < 256 && list[count] != 0xffff)
		count++;
	memcpy(modes, list, (size_t)count * sizeof(uint16_t));

	for (i = 0; i < count; i++) {
		struct vbe_mode_info mi;
		uint16_t attrs;

		if (!vbe_get_mode_info(modes[i], &mi))
			continue;

		attrs = VBE_ATTR_SUPPORTED | VBE_ATTR_GRAPHICS |
			VBE_ATTR_LFB;
		if ((mi.ModeAttributes & attrs) != attrs)
			continue;
		if (mi.MemoryModel != VBE_MM_DIRECT)
			continue;
		if (mi.XResolution != SCREEN_WIDTH ||
		    mi.YResolution != SCREEN_HEIGHT)
			continue;
		if (mi.BitsPerPixel != bpp)
			continue;
		if (mi.PhysBasePtr == 0)
			continue;

		*mode_ret = modes[i];
		*mi_ret = mi;
		return true;
	}

	return false;
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

static uint32_t
get_time(void)
{
	union REGS r;
	uint32_t tick;

	/*
	 * BIOS timer tick
	 * 18.2065 Hz
	 */
	r.h.ah = 0x00;
	int386(0x1a, &r, &r);

	tick = ((uint32_t)r.w.cx << 16) | r.w.dx;

	/*
	 * Convert to milliseconds.
	 *
	 * 18.2065 ticks/sec:
	 *   ms = tick * 1000 / 18.2065
	 *
	 * Approximation:
	 */
	return (tick * 54925UL) / 1000UL;
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

bool
hal_play_sound(
	int stream,		/* A sound stream index */
	struct hal_wave *w)	/* [IN] A sound object, ownership will be delegated to the callee */
{
	UNUSED_PARAMETER(stream);
	UNUSED_PARAMETER(w);
	return true;
}

bool
hal_stop_sound(
	int stream)
{
	UNUSED_PARAMETER(stream);
	return true;
}

bool
hal_set_sound_volume(
	int stream,
	float vol)
{
	UNUSED_PARAMETER(stream);
	UNUSED_PARAMETER(vol);
	return true;
}

bool
hal_is_sound_finished(
	int stream)
{
	UNUSED_PARAMETER(stream);
	return true;
}
