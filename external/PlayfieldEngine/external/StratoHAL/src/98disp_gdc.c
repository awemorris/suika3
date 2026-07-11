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

static INLINE void gdc_cmd(uint8_t c)
{
	/* status: 0xA0 read, bit1=FIFO full */
        while (inp(0xa0) & 0x02)
                ;
        outp(0xa2, c);
}

static INLINE void gdc_prm(uint8_t p)
{
        while (inp(0xa0) & 0x02)
                ;
        outp(0xa0, p);
}

bool gdc_init_disp(void)
{
        volatile uint16_t *text, *attr;
	int i;

	/* 24kHz 640x400 (GDC clock 2.5MHz), Slave GDC SYNC params */
	static const uint8_t sync_params[8] = {
		0x06, /* P1: mode (graphics, slave) */
		0x26, /* P2: AW = 40 words - 2 : 1 word = 16 dots */
		0x03, /* P3: VSl=0, HS */
		0x11, /* P4: HFP, VSh */
		0x03, /* P5: HBP */
		0x07, /* P6: VFP = 7 */
		0x90, /* P7: LF low  (400 = 0x190) */
		0x65  /* P8: VBP=25, LF high=1 */
	};

        /* Clear Text VRAM to prevent garbage from showing. */
        text = (volatile uint16_t *)0x000a0000;
        attr = (volatile uint16_t *)0x000a2000;
        for (i = 0; i < 80 * 25; i++) {
                text[i] = 0x0000;
                attr[i] = 0x0000;
        }

	/* 16 color mode. */
        outp(0x6a, 0x01);

        /* SYNC */
        gdc_cmd(0x0e);
        for (i = 0; i < 8; i++)
                gdc_prm(sync_params[i]);

        /* PITCH = 40 word/line. */
        gdc_cmd(0x47);
        gdc_prm(40);

        /* SCROLL: SAD=0, LEN=400 */
        gdc_cmd(0x70);
        gdc_prm(0x00);	/* SAD low */
        gdc_prm(0x00);  /* SAD high */
        gdc_prm(0x00);  /* LEN[3:0]<<4 */
        gdc_prm(0x19);  /* LEN[9:4]  (400=0x190) */

	/* 400 line: odd raster skip disable. */
	outp(0x68, 0x08);

	/* Slave GDC CSRFORM: LR=0 (1:1 line). */
	gdc_cmd(0x4b);
	gdc_prm(0x00);
	gdc_prm(0x00);
	gdc_prm(0x00);

	/* Graphic ON */
        gdc_cmd(0x0d);

	/* Text OFF*/
        outp(0x62, 0x0c);

	ofs_x = (SCREEN_WIDTH - game_width) / 2;
	ofs_y = (SCREEN_HEIGHT - game_height) / 2;

        return true;
}

void
gdc_cleanup_disp(void)
{
        volatile uint16_t *text, *attr;
        int i;

        /* Clear Text VRAM to prevent garbage from showing. */
        text = (volatile uint16_t *)0x000a0000;
        attr = (volatile uint16_t *)0x000a2000;
        for (i = 0; i < 80 * 25; i++) {
                text[i] = 0x0000;
                attr[i] = 0x0000;
        }

        /* Stop displaying G-VRAM. */
        outp(0xa2, 0x0c);

        /* Start displaying Text VRAM. */
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

				/*
				 * StratoHAL pixel layout (BGRA):
				 * low byte = B, then G, then R
				 */
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
