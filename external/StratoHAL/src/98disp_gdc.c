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

void hal_poll_sound(void);
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

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	400

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

static int ofs_x;
static int ofs_y;

extern int game_width;
extern int game_height;
extern struct hal_image *back_image;

bool
gdc_init_disp(void)
{
	union REGS r;
	volatile uint16_t *text;
	int i;

	if (game_width > SCREEN_WIDTH || game_height > SCREEN_HEIGHT)
		return false;

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
	text = (volatile uint16_t *)0xa0000;
	for (i = 0; i < 80 * 25; i++)
		text[i] = 0x0000;

	/*
	 * Start displaying G-VRAM.
	 *  - INT 18h, AH=40h
	 */
	r.w.ax = 0x4000;
	int386(0x18, &r, &r);

	ofs_x = (SCREEN_WIDTH - game_width) / 2;
	ofs_y = (SCREEN_HEIGHT - game_height) / 2;

	/* Text OFF*/
        outp(0x62, 0x0c);

	return true;
}

void
gdc_cleanup_disp(void)
{
	union REGS r;
	volatile uint16_t *text;
	volatile unsigned char *gvram;
	int i;

	/*
	 * Stop displaying G-VRAM.
	 *  - INT 18h, AH=41h
	 */
	r.w.ax = 0x4100;
	int386(0x18, &r, &r);

	/* Hide Text VRAM. */
	text = (volatile uint16_t *)0xa0000;
	for (i = 0; i < 80 * 25; i++)
		text[i] = 0x0000;

	/* Hide G-VRAM. */
	gvram = (volatile char *)0xa8000;
	for (i = 0; i < 640 * 400 / 8; i++)
		gvram[i] = 0;
	gvram = (volatile char *)0xb0000;
	for (i = 0; i < 640 * 400 / 8; i++)
		gvram[i] = 0;
	gvram = (volatile char *)0xb8000;
	for (i = 0; i < 640 * 400 / 8; i++)
		gvram[i] = 0;
	gvram = (volatile char *)0xe0000;
	for (i = 0; i < 640 * 400 / 8; i++)
		gvram[i] = 0;

        /* Text ON. */
        outp(0x62, 0x0d);
}

/*
 * Blit back image to VRAM. (PC-98 GDC 4-bpp)
 */
void
gdc_flip(void)
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

		/* Let the sound buffer be refilled while we convert the screen. */
		if ((y & 31) == 0)
			hal_poll_sound();

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
