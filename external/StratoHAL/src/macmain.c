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

/* Macintosh */
#include <MacTypes.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Events.h>
#include <Fonts.h>
#include <Menus.h>

#define LOG_FILE "log.txt"

/* Game Info */
static char *game_title;
static int game_width;
static int game_height;

/* Window */
static WindowPtr window;

/* Back Buffer */
static struct hal_image *back_image;
static GWorldPtr back_gworld;
static PixMapHandle pixmap_handle;

/* Tick Count */
static uint64_t tick_count;

/* Log */
static FILE *log_fp;

/* Callback. */
static struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* Forward declaration. */
static bool process_event(void);
static void on_update(EventRecord event);
static void on_mouse_down(EventRecord event);
static void on_mouse_up(EventRecord event);
static bool lock_image(void);
static void flip(void);
static bool open_log_file(void);

int
hal_main(
	int argc,
	char *argv)
{
	Rect windowRect, imageRect;
	Boolean running = true;

	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NULL);
	InitCursor();

	/* Initialize the file package. */
	if (!init_file())
		return 1;

	/* Call the "setup()" function in the script. */
	if (!hal_bootstrap_ptr(&game_title,
			       &game_width,
			       &game_height,
			       &hal_callback))
		return 1;

	/* Create a backing image. */
	SetRect(&imageRect, 0, 0, game_width, game_height);
	if (NewGWorld(&back_gworld, 32, &imageRect, NULL, NULL, 0) != noErr)
		return 1;
	if (!hal_create_image_with_pixels(game_width, game_height, NULL, &back_image))
		return 1;

	/* Create a window. */
	SetRect(&windowRect, 50, 50, 50 + game_width, 5- + game_height);
	window = NewWindow(NULL,
			   &windowRect,
			   "\pHello, Mac!",
			   true,
			   documentProc,
			   (WindowPtr)-1L,
			   true,
			   0);
	if (window == NULL)
		return 1;

	/* Set the target window. */
	SetPort(window);

	/* Game loop. */
	while (running) {
		if (!process_event())
			break;

		if (!lock_image())
			break;

		hal_clear_image(back_image, 0);

		if (!hal_callback.on_update()) {
			flip();
			break;
		}

		hal_callback.on_render();

		flip();
	}

	hal_destroy_image(back_image);
	DisposeGWorld(back_gworld);
	DisposeWindow(window);

	return 0;
}

static bool
process_event(void)
{
	EventRecord event;

	if (!WaitNextEvent(everyEvent, &event, 0, NULL))
		return true;

	tick_count++;

	switch (event.what) {
	case updateEvt:
		on_update(event);
		break;
	case mouseDown:
		on_mouse_down(event);
		break;
	case keyDown:
		/* Exit the game loop. */
		return false;
	}
}

static void
on_update(EventRecord event)
{
	if ((WindowPtr)event.message == window) {
		BeginUpdate(window);
		EndUpdate(window);
	}
}

static void
on_mouse_down(EventRecord event)
{
	WindowPtr whichWindow;
	short windowPart = FindWindow(event.where, &whichWindow);
                        
	if (whichWindow == window && windowPart == inGoAway) {
		if (TrackGoAway(window, event.where)) {
			/* XXX */
		}
	}
}

static void
on_mouse_up(EventRecord event)
{
}

static bool
lock_image(void)
{
	pixmap_handle = GetGWorldPixMap(back_gworld);
	if (pixmap_handle != NULL) {
		if (LockPixels(pixmap_handle)) {
			back_image->pixels = (hal_pixel_t *)GetPixelsState(pixmap_handle);
			return true;
		}
	}
	return false;
}

static void
flip(void)
{
	if (pixmap_handle != NULL) {
		Rect srcRect = (*pixmap_handle)->bounds;
		Rect dstRect = window->portRect;
    		GrafPtr dstPort = (GrafPtr)window;
    
		CopyBits((BitMap*)*pixmap_handle,
			 &dstPort->portBits,
			 &srcRect,
			 &dstRect,
			 srcCopy,
			 NULL);

		UnlockPixels(pixmap_handle);
		pixmap_handle = NULL;
	}
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

void
hal_reset_lap_timer(
	uint64_t *t)
{
	*t = tick_count;
}

uint64_t
hal_get_lap_timer_millisec(
	uint64_t *t)
{
	uint64_t end;
	
	return (uint64_t)((tick_count - *t));
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
