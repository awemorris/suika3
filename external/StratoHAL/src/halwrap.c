/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Helpers for foreign languages (C# and Swift)
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

#include "stratohal/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/*
 * Function Pointers
 */

#if defined(USE_CSHARP) && !defined(NO_CDECL)
#define CDECL __cdecl
#else
#define CDECL
#endif

void CDECL (*wrap_log_info)(UNSAFEPTR(const char *) s);
void CDECL (*wrap_log_warn)(UNSAFEPTR(const char *) s);
void CDECL (*wrap_log_error)(UNSAFEPTR(const char *) s);
void CDECL (*wrap_log_out_of_memory)(UNSAFEPTR(const char *) s);
void CDECL (*wrap_make_save_directory)(void);
void CDECL (*wrap_make_real_path)(UNSAFEPTR(const char *) fname, UNSAFEPTR(const char *) dst, int len);
void CDECL (*wrap_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels);
void CDECL (*wrap_notify_image_free)(int id);
void CDECL (*wrap_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_rule)(int src_img, int rule_img, int threshold);
void CDECL (*wrap_render_image_melt)(int src_img, int rule_img, int progress);
void CDECL (*wrap_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
void CDECL (*wrap_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin);
uint64_t CDECL (*wrap_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin);
void CDECL (*wrap_play_sound)(int stream, UNSAFEPTR(void *) wave);
void CDECL (*wrap_stop_sound)(int stream);
void CDECL (*wrap_set_sound_volume)(int stream, float vol);
bool CDECL (*wrap_is_sound_finished)(int stream);
bool CDECL (*wrap_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable);
void CDECL (*wrap_stop_video)(void);
bool CDECL (*wrap_is_video_playing)(void);
bool CDECL (*wrap_is_full_screen_supported)(void);
bool CDECL (*wrap_is_full_screen_mode)(void);
void CDECL (*wrap_enter_full_screen_mode)(void);
void CDECL (*wrap_leave_full_screen_mode)(void);
void CDECL (*wrap_get_system_language)(UNSAFEPTR(char *) dst, int len);
void CDECL (*wrap_set_continuous_swipe_enabled)(bool is_enabled);
void CDECL (*wrap_free_shared)(UNSAFEPTR(void *) p);
bool CDECL (*wrap_check_file_exist)(UNSAFEPTR(const char *) file_name);
UNSAFEPTR(void *) CDECL (*wrap_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len);
void CDECL (*wrap_open_save_file)(UNSAFEPTR(const char *) file_name);
void CDECL (*wrap_write_save_file)(int b);
void CDECL (*wrap_close_save_file)(void);

/*
 * Initializer
 *  - C#: Call init_hal_func_table() from C# to initialize the wrap_* pointers.
 *  - Swift: Assign the wrap_* pointers directly from Swift.
 */

#if defined(USE_CSHARP)
void init_hal_func_table
(
 void CDECL (*p_log_info)(UNSAFEPTR(const char *) s),
 void CDECL (*p_log_warn)(UNSAFEPTR(const char *) s),
 void CDECL (*p_log_error)(UNSAFEPTR(const char *) s),
 void CDECL (*p_log_out_of_memory)(UNSAFEPTR(const char *) s),
 void CDECL (*p_make_save_directory)(void),
 void CDECL (*p_make_real_path)(UNSAFEPTR(const char *) fname, UNSAFEPTR(const char *) dst, int len),
 void CDECL (*p_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels),
 void CDECL (*p_notify_image_free)(int id),
 void CDECL (*p_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_rule)(int src_img, int rule_img, int threshold),
 void CDECL (*p_render_image_melt)(int src_img, int rule_img, int progress),
 void CDECL (*p_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
 void CDECL (*p_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin),
 uint64_t CDECL (*p_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin),
 void CDECL (*p_play_sound)(int stream, UNSAFEPTR(void *) wave),
 void CDECL (*p_stop_sound)(int stream),
 void CDECL (*p_set_sound_volume)(int stream, float vol),
 bool CDECL (*p_is_sound_finished)(int stream),
 bool CDECL (*p_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable),
 void CDECL (*p_stop_video)(void),
 bool CDECL (*p_is_video_playing)(void),
 bool CDECL (*p_is_full_screen_supported)(void),
 bool CDECL (*p_is_full_screen_mode)(void),
 void CDECL (*p_enter_full_screen_mode)(void),
 void CDECL (*p_leave_full_screen_mode)(void),
 void CDECL (*p_get_system_language)(UNSAFEPTR(char *) dst, int len),
 void CDECL (*p_set_continuous_swipe_enabled)(bool is_enabled),
 void CDECL (*p_free_shared)(UNSAFEPTR(void *) p),
 bool CDECL (*p_check_file_exist)(UNSAFEPTR(const char *) file_name),
 UNSAFEPTR(void *) CDECL (*p_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len),
 void CDECL (*p_open_save_file)(UNSAFEPTR(const char *) file_name),
 void CDECL (*p_write_save_file)(int b),
 void CDECL (*p_close_save_file)(void)
)
{
	wrap_log_info = p_log_info;
	wrap_log_warn = p_log_warn;
	wrap_log_error = p_log_error;
	wrap_log_out_of_memory = p_log_out_of_memory;
	wrap_make_save_directory = p_make_save_directory;
	wrap_make_real_path = p_make_real_path;
	wrap_notify_image_update = p_notify_image_update;
	wrap_notify_image_free = p_notify_image_free;
	wrap_render_image_normal = p_render_image_normal;
	wrap_render_image_add = p_render_image_add;
	wrap_render_image_sub = p_render_image_sub;
	wrap_render_image_dim = p_render_image_dim;
	wrap_render_image_rule = p_render_image_rule;
	wrap_render_image_melt = p_render_image_melt;
	wrap_render_image_3d_normal = p_render_image_3d_normal;
	wrap_render_image_3d_add = p_render_image_3d_add;
	wrap_render_image_3d_sub = p_render_image_3d_sub;
	wrap_render_image_3d_dim = p_render_image_3d_dim;
	wrap_reset_lap_timer = p_reset_lap_timer;
	wrap_get_lap_timer_millisec = p_get_lap_timer_millisec;
	wrap_play_sound = p_play_sound;
	wrap_stop_sound = p_stop_sound;
	wrap_set_sound_volume = p_set_sound_volume;
	wrap_is_sound_finished = p_is_sound_finished;
	wrap_play_video = p_play_video;
	wrap_stop_video = p_stop_video;
	wrap_is_video_playing = p_is_video_playing;
	wrap_is_full_screen_supported = p_is_full_screen_supported;
	wrap_is_full_screen_mode = p_is_full_screen_mode;
	wrap_enter_full_screen_mode = p_enter_full_screen_mode;
	wrap_leave_full_screen_mode = p_leave_full_screen_mode;
	wrap_get_system_language = p_get_system_language;
	wrap_set_continuous_swipe_enabled = p_set_continuous_swipe_enabled;
	wrap_free_shared = p_free_shared;
	wrap_check_file_exist = p_check_file_exist;
	wrap_get_file_contents = p_get_file_contents;
	wrap_open_save_file = p_open_save_file;
	wrap_write_save_file = p_write_save_file;
	wrap_close_save_file = p_close_save_file;
}
#endif /* defined(USE_CSHARP) */

/*
 * Wrappers
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

	wrap_log_info((UNSAFEPTR(const char *))buf);

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

	wrap_log_warn((UNSAFEPTR(const char *))buf);

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

	wrap_log_error((UNSAFEPTR(const char *))buf);

	return true;
}

bool
hal_log_out_of_memory(void)
{
	hal_log_error("Out of memory.");
	return true;
}

bool
hal_make_save_directory(void)
{
	wrap_make_save_directory();
	return true;
}

char *
hal_make_real_path(
	const char *fname)
{
	char buf[4096];
	char *ret;

	memset(buf, 0, sizeof(buf));
	wrap_make_real_path((UNSAFEPTR(const char *))fname, (UNSAFEPTR(char *))buf, sizeof(buf));

	ret = strdup(buf);
	if (ret == NULL) {
		hal_log_out_of_memory();
		return NULL;
	}
	return ret;
}

void
hal_notify_image_update(
	struct hal_image *img)
{
	wrap_notify_image_update(
		img->id,
		img->width,
		img->height,
		(UNSAFEPTR(uint32_t *))img->pixels);
}

void
hal_notify_image_free(
	struct hal_image *img)
{
	wrap_notify_image_free(img->id);
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

	wrap_render_image_normal(dst_left, dst_top, dst_width, dst_height, src_img->id, src_left, src_top, src_width, src_height, alpha);
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

	wrap_render_image_add(dst_left,
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

	wrap_render_image_sub(dst_left,
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

	wrap_render_image_dim(dst_left,
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
	wrap_render_image_rule(src_img->id, rule_img->id, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int progress)
{
	wrap_render_image_melt(src_img->id, rule_img->id, progress);
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
	wrap_render_image_3d_normal(x1,
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
	wrap_render_image_3d_add(x1,
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
	wrap_render_image_3d_sub(x1,
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
	wrap_render_image_3d_dim(x1,
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
hal_reset_lap_timer(
	uint64_t *origin)
{
	wrap_reset_lap_timer((UNSAFEPTR(uint64_t *))origin);
}

uint64_t
hal_get_lap_timer_millisec(
	uint64_t *origin)
{
	uint64_t ret;
	ret = wrap_get_lap_timer_millisec((UNSAFEPTR(uint64_t *))origin);
	return ret;
}

#if defined(HAL_TARGET_UNITY)
bool
hal_play_sound(
	int stream,
	struct hal_wave *w)
{
	wrap_play_sound(stream, (UNSAFEPTR(void *))w);
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_stop_sound(
	int stream)
{
	wrap_stop_sound(stream);
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_set_sound_volume(
	int stream,
	float vol)
{
	wrap_set_sound_volume(stream, vol);
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_is_sound_finished(
	int stream)
{
	bool ret;
	ret =  wrap_is_sound_finished(stream);
	return ret;
}
#endif

bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	bool ret;
	ret =  wrap_play_video((UNSAFEPTR(const char *))fname, is_skippable);
	return true;
}

void
hal_stop_video(void)
{
	wrap_stop_video();
}

bool
hal_is_video_playing(void)
{
	bool ret;
	ret = wrap_is_video_playing();
	return ret;
}

bool
hal_is_full_screen_supported(void)
{
	bool ret;
	ret = wrap_is_full_screen_supported();
	return ret;
}

bool
hal_is_full_screen_mode(void)
{
	bool ret;
	ret = wrap_is_full_screen_mode();
	return ret;
}

void
hal_enter_full_screen_mode(void)
{
	wrap_enter_full_screen_mode();
}

void
hal_leave_full_screen_mode(void)
{
	wrap_leave_full_screen_mode();
}

const char *
hal_get_system_language(void)
{
	static char buf[64];

	memset(buf, 0, sizeof(buf));
	wrap_get_system_language((UNSAFEPTR(char *))buf, sizeof(buf));

	return buf;
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	wrap_set_continuous_swipe_enabled(is_enabled);
}

#if defined(HAL_TARGET_UNITY)
struct hal_rfile {
	char *data;
	uint64_t size;
	uint64_t cur;
};
#endif

#if defined(HAL_TARGET_UNITY)
struct hal_wfile {
	int dummy;
};
#endif

#if defined(HAL_TARGET_UNITY)
bool
init_file(void)
{
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
void
cleanup_file(void)
{
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_check_file_exist(
	const char *file)
{
	char *path;
	bool ret;

	path = hal_make_real_path(file);
	if (path == NULL)
		return false;

	ret = wrap_check_file_exist((UNSAFEPTR(char *))path);
	free(path);

	if (!ret)
		return false;

	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **rf)
{
	UNSAFEPTR(char *) p;
	char *path;
	int len;

	*rf = malloc(sizeof(struct hal_rfile));
	if (*rf == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	path = hal_make_real_path(file);
	if (path == NULL) {
		free(*rf);
		return false;
	}

	p = wrap_get_file_contents((UNSAFEPTR(char *))path, (UNSAFEPTR(int *))&len);
	free(path);

	if (p == 0) {
		free(*rf);
		return false;
	}

	(*rf)->data = (char *)p;
	(*rf)->size = len;
	(*rf)->cur = 0;
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *ret)
{
	*ret = rf->size;
	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
void
hal_decode_rfile(
	struct hal_rfile *f)
{
	UNUSED_PARAMETER(f);
}
#endif

#if defined(HAL_TARGET_UNITY)
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
#endif

#if defined(HAL_TARGET_UNITY)
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
#endif

#if defined(HAL_TARGET_UNITY)
void
hal_rewind_rfile(
	struct hal_rfile *rf)
{
	rf->cur = 0;
}
#endif

#if defined(HAL_TARGET_UNITY)
void
hal_close_rfile(
	struct hal_rfile *rf)
{
	wrap_free_shared((UNSAFEPTR(char *))rf->data);
	free(rf);
}
#endif

#if defined(HAL_TARGET_UNITY)
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

	wrap_open_save_file((UNSAFEPTR(const char *))file);

	return wf;
}
#endif

#if defined(HAL_TARGET_UNITY)
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
		wrap_write_save_file((int)*p++);
	*ret = size;

	return true;
}
#endif

#if defined(HAL_TARGET_UNITY)
void
hal_close_wfile(
	struct hal_wfile *wf)
{
	wrap_close_save_file();
}
#endif

#if defined(HAL_TARGET_UNITY)
bool
hal_remove_file(
	const char *file)
{
	// TODO
	return false;
}
#endif
