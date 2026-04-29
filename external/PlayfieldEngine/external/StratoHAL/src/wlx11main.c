/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux Wayland/X11 Dual
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
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* HAL */
#include "stratohal/platform.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>

#define LOG_BUF_SIZE	4096

enum running_mode {
	MODE_INIT,
	MODE_WAYLAND,
	MODE_X11,
};

static int mode;

static bool is_wslg(void);

bool main_init_wl(int argc, char *argv[]);
bool main_run_wl(void);

bool main_init_x11(int argc, char *argv[]);
bool main_run_x11(void);

bool hal_log_info_wl(const char *s, ...);
bool hal_log_warn_wl(const char *s, ...);
bool hal_log_error_wl(const char *s, ...);
bool hal_log_out_of_memory_wl(void);
bool hal_make_save_directory_wl(void);
char *hal_make_real_path_wl(const char *fname);
void hal_reset_lap_timer_wl(uint64_t *t);
uint64_t hal_get_lap_timer_millisec_wl(uint64_t *t);
void hal_notify_image_update_wl(struct hal_image *img);
void hal_notify_image_free_wl(struct hal_image *img);
void hal_render_image_normal_wl(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_add_wl(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_sub_wl(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_dim_wl(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_rule_wl(struct hal_image *src_img, struct hal_image *rule_img, int threshold);
void hal_render_image_melt_wl(struct hal_image *src_img, struct hal_image *rule_img, int progress);
void hal_render_image_cross_wl(struct hal_image *src1_img, struct hal_image *src2_img, float src1_left, float src1_top, float src2_left, float src2_top, int alpha);
void hal_render_image_3d_normal_wl(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_add_wl(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_sub_wl(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top,int src_width, int src_height, int alpha);
void hal_render_image_3d_dim_wl(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_cross_wl(struct hal_image *src1_img, struct hal_image *src2_img, float src1_x1, float src1_y1,float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha);
bool hal_play_video_wl(const char *fname, bool is_skippable);
void hal_stop_video_wl(void);
bool hal_is_video_playing_wl(void);
bool hal_is_full_screen_supported_wl(void);
bool hal_is_full_screen_mode_wl(void);
void hal_enter_full_screen_mode_wl(void);
void hal_leave_full_screen_mode_wl(void);
const char *hal_get_system_language_wl(void);
void hal_set_continuous_swipe_enabled_wl(bool is_enabled);

bool hal_log_info_x11(const char *s, ...);
bool hal_log_warn_x11(const char *s, ...);
bool hal_log_error_x11(const char *s, ...);
bool hal_log_out_of_memory_x11(void);
bool hal_make_save_directory_x11(void);
char *hal_make_real_path_x11(const char *fname);
void hal_reset_lap_timer_x11(uint64_t *t);
uint64_t hal_get_lap_timer_millisec_x11(uint64_t *t);
void hal_notify_image_update_x11(struct hal_image *img);
void hal_notify_image_free_x11(struct hal_image *img);
void hal_render_image_normal_x11(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_add_x11(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_sub_x11(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_dim_x11(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_rule_x11(struct hal_image *src_img, struct hal_image *rule_img, int threshold);
void hal_render_image_melt_x11(struct hal_image *src_img, struct hal_image *rule_img, int progress);
void hal_render_image_cross_x11(struct hal_image *src1_img, struct hal_image *src2_img, float src1_left, float src1_top, float src2_left, float src2_top, int alpha);
void hal_render_image_3d_normal_x11(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_add_x11(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_sub_x11(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top,int src_width, int src_height, int alpha);
void hal_render_image_3d_dim_x11(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
void hal_render_image_3d_cross_x11(struct hal_image *src1_img, struct hal_image *src2_img, float src1_x1, float src1_y1,float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha);
bool hal_play_video_x11(const char *fname, bool is_skippable);
void hal_stop_video_x11(void);
bool hal_is_video_playing_x11(void);
bool hal_is_full_screen_supported_x11(void);
bool hal_is_full_screen_mode_x11(void);
void hal_enter_full_screen_mode_x11(void);
void hal_leave_full_screen_mode_x11(void);
const char *hal_get_system_language_x11(void);
void hal_set_continuous_swipe_enabled_x11(bool is_enabled);

int main(int argc, char *argv[])
{
	if (!is_wslg()) {
		mode = MODE_WAYLAND;
		if (!main_init_wl(argc, argv)) {
			mode = MODE_X11;
			if (!main_init_x11(argc, argv)) {
				printf("Failed to initialize.\n");
				return 1;
			}
		}
	} else {
		mode = MODE_X11;
		if (!main_init_x11(argc, argv)) {
			printf("Failed to initialize.\n");
			return 1;
		}
	}

	if (mode == MODE_WAYLAND) {
		if (!main_run_wl())
			return 1;
	} else if (mode == MODE_X11) {
		if (!main_run_x11())
			return 1;
	}

	return 0;
}

static bool
is_wslg(void)
{
	return getenv("WSL_DISTRO_NAME") != NULL ||
	       getenv("WSLG_USE_INTERPOLATION") != NULL ||
	       access("/mnt/wslg", F_OK) == 0;
}

bool
hal_log_info(
	const char *s,
	...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	if (mode == MODE_WAYLAND) {
		if (!hal_log_info_wl(buf))
			return false;
	} else if (mode == MODE_X11) {
		if (!hal_log_info_x11(buf))
			return false;
	}

	return true;
}

bool
hal_log_warn(
	const char *s,
	...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	if (mode == MODE_WAYLAND) {
		if (!hal_log_warn_wl(buf))
			return false;
	} else if (mode == MODE_X11) {
		if (!hal_log_warn_x11(buf))
			return false;
	}

	return true;
}

bool
hal_log_error(
	const char *s,
	...)
{
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	if (mode == MODE_WAYLAND) {
		if (!hal_log_warn_wl(buf))
			return false;
	} else if (mode == MODE_X11) {
		if (!hal_log_warn_x11(buf))
			return false;
	}

	return true;
}

bool
hal_log_out_of_memory(void)
{
	if (mode == MODE_WAYLAND)
		return hal_log_out_of_memory_wl();
	else if (mode == MODE_X11)
		return hal_log_out_of_memory_x11();

	return false;
}

bool
hal_make_save_directory(void)
{
	if (mode == MODE_WAYLAND)
		return hal_make_save_directory_wl();
	else if (mode == MODE_X11)
		return hal_make_save_directory_x11();

	return false;
}

char *
hal_make_real_path(
	const char *fname)
{
	if (mode == MODE_WAYLAND)
		return hal_make_real_path_wl(fname);
	else if (mode == MODE_X11)
		return hal_make_real_path_x11(fname);

	return NULL;
}

void
hal_reset_lap_timer(
	uint64_t *t)
{
	if (mode == MODE_WAYLAND)
		hal_reset_lap_timer_wl(t);
	else if (mode == MODE_X11)
		hal_reset_lap_timer_x11(t);
}

uint64_t
hal_get_lap_timer_millisec(
	uint64_t *t)
{
	if (mode == MODE_WAYLAND)
		return hal_get_lap_timer_millisec_wl(t);
	else if (mode == MODE_X11)
		return hal_get_lap_timer_millisec_x11(t);

	return 0;
}

void
hal_notify_image_update(
	struct hal_image *img)
{
	if (mode == MODE_WAYLAND)
		hal_notify_image_update_wl(img);
	else if (mode == MODE_X11)
		hal_notify_image_update_x11(img);
}

void
hal_notify_image_free(
	struct hal_image *img)
{
	if (mode == MODE_WAYLAND)
		hal_notify_image_free_wl(img);
	else if (mode == MODE_X11)
		hal_notify_image_free_x11(img);
}

void
hal_render_image_normal(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_normal_wl(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_normal_x11(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
hal_render_image_add(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_add_wl(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_add_x11(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void hal_render_image_sub(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_sub_wl(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_sub_x11(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void hal_render_image_dim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_dim_wl(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_dim_x11(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
hal_render_image_rule(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int threshold)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_rule_wl(src_img, rule_img, threshold);
	else if (mode == MODE_X11)
		hal_render_image_rule_x11(src_img, rule_img, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int progress)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_melt_wl(src_img, rule_img, progress);
	else if (mode == MODE_X11)
		hal_render_image_melt_x11(src_img, rule_img, progress);
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
	if (mode == MODE_WAYLAND)
		hal_render_image_cross_wl(src1_img, src2_img, src1_left, src1_top, src2_left, src2_top, alpha);
	else if (mode == MODE_X11)
		hal_render_image_cross_x11(src1_img, src2_img, src1_left, src1_top, src2_left, src2_top, alpha);
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
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_3d_normal_wl(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_3d_normal_x11(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
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
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_3d_add_wl(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_3d_add_x11(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
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
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_3d_sub_wl(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_3d_sub_x11(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
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
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (mode == MODE_WAYLAND)
		hal_render_image_3d_dim_wl(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
	else if (mode == MODE_X11)
		hal_render_image_3d_dim_x11(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
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
	if (mode == MODE_WAYLAND)
		hal_render_image_3d_cross_wl(src1_img, src2_img, src1_x1, src1_y1, src1_x2, src1_y2, src1_x3, src1_y3, src1_x4, src1_y4, src2_x1, src2_y1, src2_x2, src2_y2, src2_x3, src2_y3, src2_x4, src2_y4, alpha);
	else if (mode == MODE_X11)
		hal_render_image_3d_cross_x11(src1_img, src2_img, src1_x1, src1_y1, src1_x2, src1_y2, src1_x3, src1_y3, src1_x4, src1_y4, src2_x1, src2_y1, src2_x2, src2_y2, src2_x3, src2_y3, src2_x4, src2_y4, alpha);
}

bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	if (mode == MODE_WAYLAND)
		return hal_play_video_wl(fname, is_skippable);
	else if (mode == MODE_X11)
		return hal_play_video_x11(fname, is_skippable);

	return false;
}

void
hal_stop_video(void)
{
	if (mode == MODE_WAYLAND)
		hal_stop_video_wl();
	else if (mode == MODE_X11)
		hal_stop_video_x11();
}

bool
hal_is_video_playing(void)
{
	if (mode == MODE_WAYLAND)
		return hal_is_video_playing_wl();
	else if (mode == MODE_X11)
		return hal_is_video_playing_x11();

	return false;
}

bool
hal_is_full_screen_supported(void)
{
	if (mode == MODE_WAYLAND)
		return hal_is_full_screen_supported_wl();
	else if (mode == MODE_X11)
		return hal_is_full_screen_supported_x11();

	return false;
}

bool
hal_is_full_screen_mode(void)
{
	if (mode == MODE_WAYLAND)
		return hal_is_full_screen_mode_wl();
	else if (mode == MODE_X11)
		return hal_is_full_screen_mode_x11();

	return false;
}

void
hal_enter_full_screen_mode(void)
{
	if (mode == MODE_WAYLAND)
		hal_enter_full_screen_mode_wl();
	else if (mode == MODE_X11)
		hal_enter_full_screen_mode_x11();
}

void
hal_leave_full_screen_mode(void)
{
	if (mode == MODE_WAYLAND)
		hal_leave_full_screen_mode_wl();
	else if (mode == MODE_X11)
		hal_leave_full_screen_mode_x11();
}

const char *
hal_get_system_language(void)
{
	if (mode == MODE_WAYLAND)
		return hal_get_system_language_wl();
	else if (mode == MODE_X11)
		return hal_get_system_language_x11();

	return NULL;
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	if (mode == MODE_WAYLAND)
		hal_set_continuous_swipe_enabled_wl(is_enabled);
	else if (mode == MODE_X11)
		hal_set_continuous_swipe_enabled_x11(is_enabled);
}
