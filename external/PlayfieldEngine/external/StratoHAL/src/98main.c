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

/* VRAM Address */
#define GVRAM_B		0x000A8000UL
#define GVRAM_R		0x000B0000UL
#define GVRAM_G		0x000B8000UL
#define GVRAM_I		0x000E0000UL
#define TVRAM_TEXT	0x000A0000UL
#define TVRAM_ATTR	0x000A2000UL

/* Screen Size */
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	400
#define LINE_BYTES	(640 / 8)

/* Log */
#define LOG_FILE	"log.txt"

/* Game Info */
static char *game_title;
static int game_width;
static int game_height;

/* Screen */
static struct hal_image *back_image;
static uint8_t *fb;
static int ofs_x;
static int ofs_y;

/* Log */
static FILE *log_fp;

/* Callback. */
static struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* Forward Declaration */
static void init_vram(void);
static void cleanup_vram(void);
static void process_input(void);
static void flip(void);
static bool open_log_file(void);

int hal_main(int argc, char *argv[])
{
	printf("\n"
	       "Suika3 Game Engine for PC-9801\n"
	       "Copyright (c) 2026 Awe Morris\n");

	if (argc >= 2) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("Version 2026.05\n");
			return 0;
		}
	}

	if (!init_file()) {
		printf("Failed to initialize the file system.\n");
		return 1;
	}

	if (!hal_bootstrap_ptr(
		    &game_title,
		    &game_width,
		    &game_height,
		    &hal_callback)) {
		printf("Error on boot.\n");
		return 1;
	}
	if (game_width > SCREEN_WIDTH || game_height > SCREEN_HEIGHT) {
		printf("Screen size too large.\n");
		return 1;
	}
	ofs_x = (SCREEN_WIDTH - game_width) / 2;
	ofs_y = (SCREEN_HEIGHT - game_height) / 2;

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
		process_input();
		hal_clear_image(back_image, 0);
		if (!hal_callback.on_update())
			break;
		hal_callback.on_render();
		flip();
	}

	cleanup_vram();
	return 0;
}

/* Initialize G-VRAM. */
static void
init_vram(void)
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
}

/* Cleanup G-VRAM. */
static void
cleanup_vram(void)
{
	union REGS r;

	/*
	 * Stop displaying G-VRAM.
	 *  - INT 18h, AH=41h
	 */
	r.w.ax = 0x4100;
	int386(0x18, &r, &r);
}

static void flip(void)
{
	volatile unsigned char *vram_b;
	volatile unsigned char *vram_r;
	volatile unsigned char *vram_g;
	volatile unsigned char *vram_i;
	volatile uint32_t *pixels;
	int x, y, bit;
	int src_index;
	int dst_index;

	vram_b = (volatile unsigned char *)GVRAM_B;
	vram_r = (volatile unsigned char *)GVRAM_R;
	vram_g = (volatile unsigned char *)GVRAM_G;
	vram_i = (volatile unsigned char *)GVRAM_I;
	pixels = back_image->pixels;

	for (y = 0; y < SCREEN_HEIGHT; y++) {
		if (y < ofs_y)
			continue;
		if (y >= game_height)
			break;

		for (x = 0; x < LINE_BYTES; x++) {
			unsigned char pb = 0;
			unsigned char pr = 0;
			unsigned char pg = 0;
			unsigned char pi = 0;

			if (x < ofs_x >> 3)
				continue;
			if (x >= game_width >> 3)
				break;

			for (bit = 0; bit < 8; bit++) {
				int sx = x * 8 + bit;
				uint32_t pix;
				unsigned char r, g, b;
				unsigned char mask;

				pix = pixels[y * SCREEN_WIDTH + sx];
				r = pix & 0xff;
				g = (pix >> 8) & 0xff;
				b = (pix >> 16) & 0xff;

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

			dst_index = y * LINE_BYTES + x;

			vram_b[dst_index] = pb;
			vram_r[dst_index] = pr;
			vram_g[dst_index] = pg;
			vram_i[dst_index] = pi;
		}
	}
}

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
		int ch, keycode;

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
