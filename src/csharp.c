/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * HAL for Unity C#
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
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <strato/strato.h>
#include "csharp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

struct hal_rfile {
	char *data;
	uint64_t size;
	uint64_t cur;
};

struct hal_wfile {
	int dummy;
};

/* C# function pointers. */
void (*cs_log_info)(UNSAFEPTR(const char *) s);
void (*cs_log_warn)(UNSAFEPTR(const char *) s);
void (*cs_log_error)(UNSAFEPTR(const char *) s);
void (*cs_log_out_of_memory)(UNSAFEPTR(const char *) s);
void (*cs_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels);
void (*cs_notify_image_free)(int id);
void (*cs_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_rule)(int src_img, int rule_img, int threshold);
void (*cs_render_image_melt)(int src_img, int rule_img, int progress);
void (*cs_render_image_cross)(int src1_img, int src2_img, int src1_left, int src1_top, int src2_left, int src2_top, int alpha);
void (*cs_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void (*cs_render_image_3d_cross)(int src1_img, int src2_img, float src1_x1, float src1_y1, float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha);
void (*cs_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin);
uint64_t (*cs_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin);
void (*cs_play_sound)(int stream, UNSAFEPTR(void (**) wave));
void (*cs_stop_sound)(int stream);
void (*cs_set_sound_volume)(int stream, float vol);
bool (*cs_is_sound_finished)(int stream);
bool (*cs_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable);
void (*cs_stop_video)(void);
bool (*cs_is_video_playing)(void);
bool (*cs_is_full_screen_supported)(void);
bool (*cs_is_full_screen_mode)(void);
void (*cs_enter_full_screen_mode)(void);
void (*cs_leave_full_screen_mode)(void);
void *(*cs_get_system_language)(void);
void (*cs_set_continuous_swipe_enabled)(bool is_enabled);
bool (*cs_check_file_exist)(UNSAFEPTR(const char *) file_name);
UNSAFEPTR(void *) (*cs_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len);
void (*cs_open_save_file)(UNSAFEPTR(const char *) file_name);
void (*cs_write_save_file)(int b);
void (*cs_close_save_file)(void);
void (*cs_free_shared)(UNSAFEPTR(void (**) p));

/*
 * Initializer.
 */
void init_hal_func_table(
	void (*p_log_info)(UNSAFEPTR(const char *) s),
	void (*p_log_warn)(UNSAFEPTR(const char *) s),
	void (*p_log_error)(UNSAFEPTR(const char *) s),
	void (*p_log_out_of_memory)(UNSAFEPTR(const char *) s),
	void (*p_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels),
	void (*p_notify_image_free)(int id),
	void (*p_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_rule)(int src_img, int rule_img, int threshold),
	void (*p_render_image_melt)(int src_img, int rule_img, int progress),
	void (*p_render_image_cross)(int src1_img, int src2_img, int src1_left, int src1_top, int src2_left, int src2_top, int alpha),
	void (*p_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void (*p_render_image_3d_cross)(int src1_img, int src2_img, float src1_x1, float src1_y1, float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha),
	void (*p_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin),
	uint64_t (*p_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin),
	void (*p_play_sound)(int stream, UNSAFEPTR(void *) wave),
	void (*p_stop_sound)(int stream),
	void (*p_set_sound_volume)(int stream, float vol),
	bool (*p_is_sound_finished)(int stream),
	bool (*p_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable),
	void (*p_stop_video)(void),
	bool (*p_is_video_playing)(void),
	bool (*p_is_full_screen_supported)(void),
	bool (*p_is_full_screen_mode)(void),
	void (*p_enter_full_screen_mode)(void),
	void (*p_leave_full_screen_mode)(void),
	void *(*p_get_system_language)(void),
	void (*p_set_continuous_swipe_enabled)(bool is_enabled),
	bool (*p_check_file_exist)(UNSAFEPTR(const char *) file_name),
	UNSAFEPTR(void *) (*p_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len),
	void (*p_open_save_file)(UNSAFEPTR(const char *) file_name),
	void (*p_write_save_file)(int b),
	void (*p_close_save_file)(void),
	void (*p_free_shared)(UNSAFEPTR(void *) p))
{
	cs_log_info = p_log_info;
	cs_log_warn = p_log_warn;
	cs_log_error = p_log_error;
	cs_log_out_of_memory = p_log_out_of_memory;
	cs_notify_image_update = p_notify_image_update;
	cs_notify_image_free = p_notify_image_free;
	cs_render_image_normal = p_render_image_normal;
	cs_render_image_add = p_render_image_add;
	cs_render_image_sub = p_render_image_sub;
	cs_render_image_dim = p_render_image_dim;
	cs_render_image_rule = p_render_image_rule;
	cs_render_image_melt = p_render_image_melt;
	cs_render_image_cross = p_render_image_cross;
	cs_render_image_3d_normal = p_render_image_3d_normal;
	cs_render_image_3d_add = p_render_image_3d_add;
	cs_render_image_3d_sub = p_render_image_3d_sub;
	cs_render_image_3d_dim = p_render_image_3d_dim;
	cs_render_image_3d_cross = p_render_image_3d_cross;
	cs_reset_lap_timer = p_reset_lap_timer;
	cs_get_lap_timer_millisec = p_get_lap_timer_millisec;
	cs_play_sound = p_play_sound;
	cs_stop_sound = p_stop_sound;
	cs_set_sound_volume = p_set_sound_volume;
	cs_is_sound_finished = p_is_sound_finished;
	cs_play_video = p_play_video;
	cs_stop_video = p_stop_video;
	cs_is_video_playing = p_is_video_playing;
	cs_is_full_screen_supported = p_is_full_screen_supported;
	cs_is_full_screen_mode = p_is_full_screen_mode;
	cs_enter_full_screen_mode = p_enter_full_screen_mode;
	cs_leave_full_screen_mode = p_leave_full_screen_mode;
	cs_get_system_language = p_get_system_language;
	cs_set_continuous_swipe_enabled = p_set_continuous_swipe_enabled;
	cs_check_file_exist = p_check_file_exist;
	cs_get_file_contents = p_get_file_contents;
	cs_open_save_file = p_open_save_file;
	cs_write_save_file = p_write_save_file;
	cs_close_save_file = p_close_save_file;
	cs_free_shared = p_free_shared;
}

/*
 * Callback Wrappers
 */

static struct hal_callback hal_callback;

static int screen_width;
static int screen_height;
static char *window_title;

void so_init(void);

int
on_event_setup(void)
{
	so_init();

	if (!hal_bootstrap(&window_title, &screen_width, &screen_height, &hal_callback))
		return 0;

	return true;
}

int
on_event_start(void)
{
	if (hal_callback.on_start != NULL)
		if (!hal_callback.on_start())
			return 0;

	return 1;
}

int
on_event_update(void)
{
	if (hal_callback.on_update != NULL)
		if (!hal_callback.on_update())
			return 0;

	return 1;
}

void
on_event_render(void)
{
	if (hal_callback.on_render != NULL)
		hal_callback.on_render();
}

void
on_event_key_press(
	int key)
{
	if (hal_callback.on_key_press != NULL)
		hal_callback.on_key_press(key);
}

void
on_event_key_release(
	int key)
{
	if (hal_callback.on_key_press != NULL)
		hal_callback.on_key_press(key);
}

void
on_event_mouse_press(
	int button,
	int x,
	int y)
{
	if (hal_callback.on_mouse_press != NULL)
		hal_callback.on_mouse_press(button, x, y);
}

void
on_event_mouse_release(
	int button,
	int x,
	int y)
{
	if (hal_callback.on_mouse_release != NULL)
		hal_callback.on_mouse_release(button, x, y);
}

void
on_event_mouse_move(
	int x,
	int y)
{
        if (hal_callback.on_mouse_move != NULL) {
		hal_callback.on_mouse_move(x, y);
	}
}

void
on_event_mouse_wheel(
	int v,
	int h)
{
        if (hal_callback.on_mouse_wheel != NULL) {
		hal_callback.on_mouse_wheel(v, h);
	}
}

void
on_event_touch_cancel(void)
{
	if (hal_callback.on_touch_cancel != NULL)
		hal_callback.on_touch_cancel();
}

void
on_event_swipe_down(
	float speed,
	float amount)
{
	if (hal_callback.on_swipe_down != NULL)
		hal_callback.on_swipe_down(speed, amount);
}

void
on_event_swipe_up(
	float speed,
	float amount)
{
	if (hal_callback.on_swipe_up != NULL)
		hal_callback.on_swipe_up(speed, amount);
}

void on_event_analog_input(int input, int val)
{
	if (hal_callback.on_analog_input != NULL)
		hal_callback.on_analog_input(input, val);
}

/*
 * HAL
 */

bool
hal_log_info(
	const char *s,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	cs_log_info((UNSAFEPTR(const char *))buf);

	return true;
}

bool
hal_log_warn(
	const char *s,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	cs_log_warn((UNSAFEPTR(const char *))buf);

	return true;
}

bool
hal_log_error(
	const char *s,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	cs_log_error((UNSAFEPTR(const char *))buf);

	return true;
}

bool
hal_log_out_of_memory(void)
{
	hal_log_error("Out of memory.");
	return true;
}

void
hal_notify_image_update(
	struct hal_image *img)
{
	cs_notify_image_update(
		img->id,
		img->width,
		img->height,
		(UNSAFEPTR(uint32_t *))img->pixels);
}

void
hal_notify_image_free(
	struct hal_image *img)
{
	cs_notify_image_free(img->id);
}

void
hal_render_image_normal(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_img->width;
	if (dst_height == -1)
		dst_height = src_img->height;
	if (src_width == -1)
		src_width = src_img->width;
	if (src_height == -1)
		src_height = src_img->height;

	cs_render_image_normal(dst_left, dst_top, dst_width, dst_height, src_img->id, src_left, src_top, src_width, src_height, alpha);
}

void
hal_render_image_add(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_img->width;
	if (dst_height == -1)
		dst_height = src_img->height;
	if (src_width == -1)
		src_width = src_img->width;
	if (src_height == -1)
		src_height = src_img->height;

	cs_render_image_add(dst_left,
			      dst_top,
			      dst_width,
			      dst_height,
			      src_img->id,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      alpha);
}

void
hal_render_image_sub(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_img->width;
	if (dst_height == -1)
		dst_height = src_img->height;
	if (src_width == -1)
		src_width = src_img->width;
	if (src_height == -1)
		src_height = src_img->height;

	cs_render_image_sub(dst_left,
			      dst_top,
			      dst_width,
			      dst_height,
			      src_img->id,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      alpha);
}

void
hal_render_image_dim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_img->width;
	if (dst_height == -1)
		dst_height = src_img->height;
	if (src_width == -1)
		src_width = src_img->width;
	if (src_height == -1)
		src_height = src_img->height;

	cs_render_image_dim(dst_left,
			      dst_top,
			      dst_width,
			      dst_height,
			      src_img->id,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      alpha);
}

void
hal_render_image_rule(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int threshold)
{
	cs_render_image_rule(src_img->id, rule_img->id, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int progress)
{
	cs_render_image_melt(src_img->id, rule_img->id, progress);
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
	cs_render_image_cross(src1_img->id, src2_img->id, src1_left, src1_top, src2_left, src2_top, alpha);
}

void
hal_render_image_3d_normal(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	cs_render_image_3d_normal(x1,
				    y1,
				    x2,
				    y2,
				    x3,
				    y3,
				    x4,
				    y4,
				    src_img->id,
				    src_left,
				    src_top,
				    src_width,
				    src_height,
				    alpha);
}

void
hal_render_image_3d_add(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	cs_render_image_3d_add(x1,
				 y1,
				 x2,
				 y2,
				 x3,
				 y3,
				 x4,
				 y4,
				 src_img->id,
				 src_left,
				 src_top,
				 src_width,
				 src_height,
				 alpha);
}

void
hal_render_image_3d_sub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	cs_render_image_3d_sub(x1,
				 y1,
				 x2,
				 y2,
				 x3,
				 y3,
				 x4,
				 y4,
				 src_img->id,
				 src_left,
				 src_top,
				 src_width,
				 src_height,
				 alpha);
}

void
hal_render_image_3d_dim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_img,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	cs_render_image_3d_dim(x1,
				 y1,
				 x2,
				 y2,
				 x3,
				 y3,
				 x4,
				 y4,
				 src_img->id,
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
	cs_render_image_3d_cross(src1_img->id,
				 src2_img->id,
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
	uint64_t *origin)
{
	cs_reset_lap_timer((UNSAFEPTR(uint64_t *))origin);
}

uint64_t
hal_get_lap_timer_millisec(
	uint64_t *origin)
{
	uint64_t ret;
	ret = cs_get_lap_timer_millisec((UNSAFEPTR(uint64_t *))origin);
	return ret;
}

bool
hal_play_sound(
	int stream,
	struct hal_wave *w)
{
	cs_play_sound(stream, (UNSAFEPTR(void *))w);
	return true;
}

bool
hal_stop_sound(
	int stream)
{
	cs_stop_sound(stream);
	return true;
}

bool
hal_set_sound_volume(
	int stream,
	float vol)
{
	cs_set_sound_volume(stream, vol);
	return true;
}

bool
hal_is_sound_finished(
	int stream)
{
	bool ret;
	ret =  cs_is_sound_finished(stream);
	return ret;
}

bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	bool ret;
	ret =  cs_play_video((UNSAFEPTR(const char *))fname, is_skippable);
	return true;
}

void
hal_stop_video(void)
{
	cs_stop_video();
}

bool
hal_is_video_playing(void)
{
	bool ret;
	ret = cs_is_video_playing();
	return ret;
}

bool
hal_is_full_screen_supported(void)
{
	bool ret;
	ret = cs_is_full_screen_supported();
	return ret;
}

bool
hal_is_full_screen_mode(void)
{
	bool ret;
	ret = cs_is_full_screen_mode();
	return ret;
}

void
hal_enter_full_screen_mode(void)
{
	cs_enter_full_screen_mode();
}

void
hal_leave_full_screen_mode(void)
{
	cs_leave_full_screen_mode();
}

const char *
hal_get_system_language(void)
{
	static char buf[64];
	const char *p;

	memset(buf, 0, sizeof(buf));

	p = cs_get_system_language();
	if (p != NULL)
		strncpy(buf, p, 8);

	return buf;
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	cs_set_continuous_swipe_enabled(is_enabled);
}

bool
hal_check_file_exist(
	const char *file)
{
	if (!cs_check_file_exist((UNSAFEPTR(char *))file))
		return false;

	return true;
}

bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **rf)
{
	void *p;
	int len;

	*rf = malloc(sizeof(struct hal_rfile));
	if (*rf == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	p = (void *)cs_get_file_contents((UNSAFEPTR(char *))file, (UNSAFEPTR(int *))&len);
	if (p == NULL) {
		free(*rf);
		return false;
	}

	(*rf)->data = (char *)p;
	(*rf)->size = len;
	(*rf)->cur = 0;
	return true;
}

bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *ret)
{
	*ret = rf->size;
	return true;
}

void
hal_decode_rfile(
	struct hal_rfile *f)
{
	UNUSED_PARAMETER(f);
}

bool
hal_read_rfile(
	struct hal_rfile *rf,
	void *buf,
	size_t size,
	size_t *ret)
{
	size_t len;

	if (size <= rf->size - rf->cur)
		len = size;
	else
		len = rf->size - rf->cur;

	memcpy(buf, rf->data + rf->cur, len);
	rf->cur += len;

	*ret = len;
	return true;
}

bool
hal_gets_rfile(
	struct hal_rfile *rf,
	char *buf,
	size_t size)
{
	char *ptr;
	size_t len, ret;
	char c;

	ptr = buf;
	for (len = 0; len < size - 1; len++) {
		if (!hal_read_rfile(rf, &c, 1, &ret)) {
			*ptr = '\0';
			return len == 0 ? false : true;
		}
		if (c == '\n' || c == '\0') {
			*ptr = '\0';
			return true;
		}
		if (c == '\r') {
			if (!hal_read_rfile(rf, &c, 1, &ret)) {
				*ptr = '\0';
				return true;
			}
			if (c == '\n') {
				*ptr = '\0';
				return true;
			}
			rf->cur--;
			*ptr = '\0';
			return true;
		}
		*ptr++ = c;
	}
	*ptr = '\0';
	if (len == 0)
		return false;
	return true;
}

void
hal_rewind_rfile(
	struct hal_rfile *rf)
{
	rf->cur = 0;
}

void
hal_close_rfile(
	struct hal_rfile *rf)
{
	cs_free_shared((UNSAFEPTR(char *))rf->data);
	free(rf);
}

bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **wf)
{
	*wf = malloc(sizeof(struct hal_wfile));
	if (*wf == NULL) {
		hal_log_out_of_memory();
		return NULL;
	}

	cs_open_save_file((UNSAFEPTR(const char *))file);

	return wf;
}

bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret)
{
	const char *p;
	size_t i;

	p = buf;
	for (i = 0; i < size; i++)
		cs_write_save_file((int)*p++);
	*ret = size;

	return true;
}

bool
hal_write_wfile_plain(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret)
{
	return hal_write_wfile(wf, buf, size, ret);
}

void
hal_close_wfile(
	struct hal_wfile *wf)
{
	cs_close_save_file();
}
