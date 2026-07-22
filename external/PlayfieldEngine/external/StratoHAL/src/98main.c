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
#include "98disp.h"

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

/* Log */
#define LOG_FILE	"log.txt"

/* Command Line Options */
static int requested_bpp;

/* Game Info */
static char *game_title;
int game_width;
int game_height;

/* Screen */
struct hal_image *back_image;

/* Log */
static FILE *log_fp;

/* Callback. */
static struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* argc/argv */
int hal_argc;
char **hal_argv;

/* Alpha blend table. */
uint8_t alphatable[256][256];

/* Forward Declaration */
static void init_alphatable(void);
static bool init_disp(void);
static void cleanup_disp(void);
static bool init_sound(void);
static void cleanup_sound(void);
static void sound_poll(void);
void hal_poll_sound(void);
static void process_input(void);
static void flip(void);
static bool open_log_file(void);

int hal_main(int argc, char *argv[])
{
	hal_argc = argc;
	hal_argv = argv;

	printf("\n"
	       "Suika3 Game Engine for PC-9801\n"
	       "Copyright (c) 2026 Awe Morris\n");

	/* Default BPP = 4. */
	requested_bpp = 4;

	/* Parse command line arguments. */
	if (argc >= 2) {
		if (strcmp(argv[1], "--version") == 0) {
			printf("Version 2026.05\n");
			return 0;
		}
		if (strcmp(argv[1], "-24") == 0) {
			requested_bpp = 24;
			hal_argc = 1;
		}
		if (strcmp(argv[1], "-16") == 0) {
			requested_bpp = 16;
			hal_argc = 1;
		}
		if (strcmp(argv[1], "-8") == 0) {
			requested_bpp = 8;
			hal_argc = 1;
		}
		if (strcmp(argv[1], "-4") == 0) {
			requested_bpp = 4;
			hal_argc = 1;
		}
	}

	/* Initialize the file. (Mount the assets.arc file if exists.) */
	if (!init_file()) {
		hal_log_error("Failed to initialize the file system.\n");
		return 1;
	}

	/* Call setup() in the script. */
	if (!hal_bootstrap_ptr(
		    &game_title,
		    &game_width,
		    &game_height,
		    &hal_callback))
		return 1;

	/* Initialize the sound. */
	if (!init_sound()) {
		/* Ignore if no sound card. */
	}

	/* Initialize the display. */
	if (!init_disp()) {
		/* Error: screen is not available. */
		return 1;
	}

	/* Create a backing image. */
	if (!hal_create_image(game_width, game_height, &back_image)) {
		printf("Error on creating image.\n");
		return 1;
	}

	/* Call start() in the script. */
	if (!hal_callback.on_start()) {
		printf("Error on start.\n");
		return 1;
	}

	/* Create the alpha blending LUT. */
	init_alphatable();

	/* Game loop. */
	while (1) {
		sound_poll();

		process_input();

		hal_clear_image(back_image, 0);

		if (!hal_callback.on_update())
			break;

		hal_callback.on_render();

		flip();
	}

	/* Cleanup. */
	cleanup_sound();
	cleanup_disp();

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

/*
 * Display
 */

#define DISP_GDC	0
#define DISP_CIRRUS	1
#define DISP_TRIDENT	2

static int disp_driver;

static bool
init_disp(void)
{
	/*
	 * If no bpp option is specified or 8/16/24-bpp option is
	 * specified, try initializing the SVGA chip.
	 */
	if (requested_bpp == -1 || requested_bpp == 8 ||
	    requested_bpp == 16 || requested_bpp == 24) {
		if (cirrus_init_disp(DISP_640X480, requested_bpp)) {
			disp_driver = DISP_CIRRUS;
			return true;
		}

		if (trident_init_disp(DISP_640X480, requested_bpp)) {
			disp_driver = DISP_TRIDENT;
			return true;
		}

	}

	/*
	 * TODO: If 8-bpp option is specified, try initializing
	 * PEGC 8-bpp.
	 */

	/*
	 * If no bpp option is specified or 4-bpp option is specified,
	 * try initializing GDC 4-bpp.
	 */
	if (requested_bpp == -1 || requested_bpp == 4) {
		if (gdc_init_disp()) {
			disp_driver = DISP_GDC;
			return true;
		}
	}

	/* Failed. */
	return false;
}

static void
cleanup_disp(void)
{
	if (disp_driver == DISP_CIRRUS)
		cirrus_cleanup_disp();

	if (disp_driver == DISP_TRIDENT)
		trident_cleanup_disp();

	gdc_cleanup_disp();
}

static void
flip(void)
{
	switch (disp_driver) {
	case DISP_GDC:
		gdc_flip();
		break;
	case DISP_CIRRUS:
		cirrus_flip();
		break;
	case DISP_TRIDENT:
		trident_flip();
		break;
	}
}

/*
 * Sound
 */

#define SOUND_NONE	0
#define SOUND_SB16	1
#define SOUND_WSS	2

static int sound_driver;

bool sb16_init_sound(void);
void sb16_cleanup_sound(void);
void sb16_sound_poll(void);
bool sb16_play_sound(int n, struct hal_wave *w);
bool sb16_stop_sound(int n);
bool sb16_set_sound_volume(int n, float vol);
bool sb16_is_sound_finished(int n);

bool wss_init_sound(void);
void wss_cleanup_sound(void);
void wss_sound_poll(void);
bool wss_play_sound(int n, struct hal_wave *w);
bool wss_stop_sound(int n);
bool wss_set_sound_volume(int n, float vol);
bool wss_is_sound_finished(int n);

static bool
init_sound(void)
{
	if (sb16_init_sound()) {
		sound_driver = SOUND_SB16;
		return true;
	}

	if (wss_init_sound()) {
		sound_driver = SOUND_WSS;
		return true;
	}

	hal_log_info("No supported sound card found.");

	return false;
}

static void
cleanup_sound(void)
{
	if (sound_driver == SOUND_SB16)
		sb16_cleanup_sound();
	else if (sound_driver == SOUND_WSS)
		wss_cleanup_sound();
}

/*
 * Sound buffer refill is driven from the main loop only (sound_poll()).
 * On a slow machine one iteration of the main loop can take several seconds,
 * while one half of the WSS/SB16 DMA buffer is only 1.024 s of audio.  The
 * ISR then flips the half before the app ever gets a chance to refill it, and
 * the stale half is played again.  Giving the long scanning loops a chance to
 * poll fixes it: hal_poll_sound() returns immediately unless fill_pending is
 * set, so the per-row cost is negligible.
 */
void
hal_poll_sound(void)
{
	/*
	 * Reentrancy guard: hal_poll_sound() is also called from
	 * hal_read_rfile(), and sound_poll() itself reads the ogg stream
	 * through hal_read_rfile().  Without the guard we would recurse.
	 */
	static bool inside = false;

	if (inside)
		return;

	inside = true;
	sound_poll();
	inside = false;
}

static void
sound_poll(void)
{
	if (sound_driver == SOUND_SB16)
		sb16_sound_poll();
	else if (sound_driver == SOUND_WSS)
		wss_sound_poll();
}

bool
hal_play_sound(
	int n,
	struct hal_wave *w)
{
	if (sound_driver == SOUND_SB16)
		return sb16_play_sound(n, w);
	else if (sound_driver == SOUND_WSS)
		return wss_play_sound(n, w);

	return true;
}

bool
hal_stop_sound(
	int n)
{
	if (sound_driver == SOUND_SB16)
		return sb16_stop_sound(n);
	else if (sound_driver == SOUND_WSS)
		return wss_stop_sound(n);

	return true;
}

bool
hal_set_sound_volume(
	int n,
	float vol)
{
	if (sound_driver == SOUND_SB16)
		return sb16_set_sound_volume(n, vol);
	else if (sound_driver == SOUND_WSS)
		return wss_set_sound_volume(n, vol);

	return true;
}

bool
hal_is_sound_finished(
	int n)
{
	if (sound_driver == SOUND_SB16)
		return sb16_is_sound_finished(n);
	else if (sound_driver == SOUND_WSS)
		return wss_is_sound_finished(n);

	return true;
}
