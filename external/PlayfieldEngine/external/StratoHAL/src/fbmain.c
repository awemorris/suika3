/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux fbdev framebuffer
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
#include "stratohal/platform.h"	/* Public Interface */
#include "stdfile.h"		/* Standard C File Implementation */
#include "asound.h"		/* ALSA Sound Implemenatation */

/* Linux */
#include <linux/fb.h>
#include <linux/input.h>

/* POSIX */
#include <sys/types.h>
#include <sys/stat.h>	/* stat(), mkdir() */
#include <sys/time.h>	/* gettimeofday() */
#include <sys/mman.h>	/* mmap() */
#include <sys/ioctl.h>	/* ioctl() */
#include <unistd.h>	/* usleep(), access() */
#include <fcntl.h>
#include <poll.h>

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>
#include <assert.h>

/* Gstreamer Video HAL */
#include "gstplay.h"

/* Log File */
#define LOG_FILE	"log.txt"

/* Save Directory */
#define SAVE_DIR	"save"

/* Back Image */
static struct hal_image *image;

/* Screen Info */
static char *window_title;
static int fb_fd;
static size_t fb_size;
static hal_pixel_t *fb_pixels;
static int fb_width;
static int fb_height;
static int screen_width;
static int screen_height;

/* Input Info */
#define EV_DEV_MAX	64
static int ev_fd[EV_DEV_MAX];
static int ev_count;
static struct pollfd ev_fds[EV_DEV_MAX];
static int mouse_x;
static int mouse_y;

/* Log */
static FILE *log_fp;

/* Locale */
static const char *lang_code;

/* Flag to indicate whether we are playing a video or not */
static bool is_gst_playing;

/* Flag to indicate whether a video is skippable or not */
static bool is_gst_skippable;

/* Forward Declaration */
static void init_locale(void);
static bool init_fb(void);
static void cleanup_fb(void);
static bool init_input(void);
static void cleanup_input(void);
static void process_input(void);
static void process_event(int index);
static bool open_log_file(void);
static bool draw_video_frame(void);

int
main(
	int argc,
	char *argv[])
{
	int x, y;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	init_locale();

	if (!init_fb())
		return 1;

	if (!init_file())
		return 1;

	if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
		return 1;

	hal_create_image(screen_width, screen_height, &image);

	init_sound();
	init_input();
	gstplay_init(argc, argv);

	if (!hal_callback_on_event_start())
		return 1;

	while (1) {
		bool need_flip;

		process_input();

		hal_clear_image(image, 0);

		if (is_gst_playing) {
			need_flip = draw_video_frame();

			if (!gstplay_is_playing()) {
				gstplay_stop();
				is_gst_playing = false;
			}

			if (!hal_callback_on_event_frame())
				break;
		} else {
			need_flip = true;

			if (!hal_callback_on_event_frame())
				break;
		}

		if (need_flip) {
			int fb_orig_x = (fb_width - screen_width) / 2;
			int fb_orig_y = (fb_height - screen_height) / 2;	
			for (y = 0; y < screen_height; y++) {
				for (x = 0; x < screen_width; x++) {
					fb_pixels[(y + fb_orig_y) * fb_width + (x + fb_orig_x)] = image->pixels[y * image->width + x];
				}
			}
		}
	}


	return 0;

}

/* Initialize the locale. */
static void
init_locale(void)
{
	const char *locale = setlocale(LC_MESSAGES, "");
        if (locale == NULL || locale[0] == '\0') {
		locale = getenv("LC_ALL");
		if (locale == NULL || locale[0] == '\0') {
			locale = getenv("LC_MESSAGES");
			if (locale == NULL || locale[0] == '\0')
				locale = getenv("LANG");
		}
	}
	if (locale == NULL || locale[0] == '\0') {
		lang_code = "en";
		return;
	}

	/* English */
	if (strncmp(locale, "en_AU", 5) == 0) {
		lang_code = "en-au";
		return;
	}
	if (strncmp(locale, "en_GB", 5) == 0) {
		lang_code = "en-gb";
		return;
	}
	if (strncmp(locale, "en_NZ", 5) == 0) {
		lang_code = "en-nz";
		return;
	}
	if (strncmp(locale, "en_US", 5) == 0) {
		lang_code = "en-us";
		return;
	}
	if (strncmp(locale, "en", 2) == 0) {
		lang_code = "en";
		return;
	}

	/* French */
	if (strncmp(locale, "fr_CA", 5) == 0) {
		lang_code = "fr-ca";
		return;
	}
	if (strncmp(locale, "fr", 2) == 0) {
		lang_code = "fr";
		return;
	}

	/* Spanish */
	if (strncmp(locale, "es_ES", 5) == 0) {
		lang_code = "es";
		return;
	}
	if (strncmp(locale, "es", 2) == 0) {
		lang_code = "es-la";
		return;
	}

	/* Chinese */
	if (strncmp(locale, "zh_TW", 5) == 0 ||
	    strncmp(locale, "zh_HK", 5) == 0) {
		lang_code = "zh-tw";
		return;
	}
	if (strncmp(locale, "zh", 2) == 0) {
		lang_code = "zh-cn";
		return;
	}

	/* Others */
	if (strncmp(locale, "ja", 2) == 0) {
		lang_code = "ja";
		return;
	}
	if (strncmp(locale, "de", 2) == 0) {
		lang_code = "de";
		return;
	}
	if (strncmp(locale, "it", 2) == 0){
		lang_code = "it";
		return;
	}
	if (strncmp(locale, "el", 2) == 0) {
		lang_code = "el";
		return;
	}
	if (strncmp(locale, "ru", 2) == 0) {
		lang_code = "ru";
		return;
	}
	if (strncmp(locale, "ko", 2) == 0) {
		lang_code = "ko";
		return;
	}

	/* Fallback */
	lang_code = "en";
}

static bool init_fb(void)
{
	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) {
		printf("Failed to open /dev/fb0.\n");
		return false;
	}

	struct fb_var_screeninfo vinfo;
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("ioctl() failed.\n");
		close(fb_fd);
		return false;
	}

	fb_width = (int)vinfo.xres_virtual;
	fb_height = (int)vinfo.yres_virtual;
	fb_size = (size_t)(fb_height * fb_width * (int)vinfo.bits_per_pixel / 8);
	fb_pixels = (hal_pixel_t *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb_pixels == MAP_FAILED) {
		printf("mmap() failed.\n");
		close(fb_fd);
		return false;
	}

	return true;
}

static void cleanup_fb(void)
{
	munmap(fb_pixels, fb_size);
	close(fb_fd);
}

static bool init_input(void)
{
	int i;

	for (i = 0; i < EV_DEV_MAX; i++) {
		char device[32];
		int fd;

		snprintf(&device[0], sizeof(device), "/dev/input/event%d", i);
		fd = open(device, O_RDONLY | O_NONBLOCK);
		if (fd > 0) {
			ev_fd[ev_count] = fd;
			ev_fds[ev_count].fd = fd;
			ev_fds[ev_count].events = POLLIN;
			ev_count++;
		}
	}

	return true;
}

static void cleanup_input(void)
{
	int i;

	for (i = 0; i < EV_DEV_MAX; i++) {
		if (ev_fd[i] > 0) {
			close(ev_fd[i]);
			ev_fd[i] = 0;
		}
	}
}

static void process_input(void)
{
	int i;
	bool processed;

	do {
		processed = false;

		if (poll(ev_fds, (nfds_t)ev_count, 0) == -1) {
			printf("poll() failed.\n");
			return;
		}

		for (i = 0; i < ev_count; i++) {
			if (ev_fds[i].revents & POLLIN) {
				processed = true;
				process_event(i);
			}
		}
	} while (processed);
}

static void
process_event(
	int index)
{
	struct input_event e;

	/* Read an event. */
	if (read(ev_fd[index], &e, sizeof(e)) != sizeof(e)) {
		printf("Wrror: no input.\n");
		return;
	}

	/* printf("Event: type=%d, code=%d, value=%d\n", e.type, e.code, e.value); */

	/* Process by a type. */
	if (e.type == EV_REL) {
		/* Calculate the mouse coordinates and notify. */
		if (e.code == 0)
			mouse_x += e.value;
		if (e.code == 1)
			mouse_y += e.value;
		mouse_x = mouse_x < 0 ? 0 : mouse_x;
		mouse_y = mouse_y < 0 ? 0 : mouse_y;
		mouse_x = mouse_x > screen_width ? screen_width : mouse_x;
		mouse_y = mouse_y > screen_height ? screen_height : mouse_y;
		hal_callback_on_event_mouse_move(mouse_x, mouse_y);
	} else if (e.type == EV_KEY) {
		if (e.code == 272) {
			if (e.value == 1)
				hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, mouse_x, mouse_y);
			else
				hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, mouse_x, mouse_y);
		} else if (e.code == 273) {
			if (e.value == 1)
				hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, mouse_x, mouse_y);
			else
				hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, mouse_x, mouse_y);
		} else if (e.code == KEY_ENTER || e.code == KEY_KPENTER) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_RETURN);
			else
				hal_callback_on_event_key_release(HAL_KEY_RETURN);
		} else if (e.code == KEY_LEFT) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_LEFT);
			else
				hal_callback_on_event_key_release(HAL_KEY_LEFT);
		} else if (e.code == KEY_RIGHT) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_RIGHT);
			else
				hal_callback_on_event_key_release(HAL_KEY_RIGHT);
		} else if (e.code == KEY_UP) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_UP);
			else
				hal_callback_on_event_key_release(HAL_KEY_UP);
		} else if (e.code == KEY_DOWN) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_DOWN);
			else
				hal_callback_on_event_key_release(HAL_KEY_DOWN);
		} else if (e.code == KEY_LEFTCTRL || e.code == KEY_RIGHTCTRL) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_CONTROL);
			else
				hal_callback_on_event_key_release(HAL_KEY_CONTROL);
		} else if (e.code == KEY_ESC) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_ESCAPE);
			else
				hal_callback_on_event_key_release(HAL_KEY_ESCAPE);
		} else if (e.code == KEY_SPACE) {
			if (e.value != 0)
				hal_callback_on_event_key_press(HAL_KEY_SPACE);
			else
				hal_callback_on_event_key_release(HAL_KEY_SPACE);
		}
	}
}

static bool
draw_video_frame(void)
{
	struct hal_image *video_image;
	int dst_width, dst_height, dst_x, dst_y;

	/* Update the playback stauts. */
	video_image = gstplay_loop_iteration();
	if (video_image == NULL) {
		/* Rendering is not required for this game frame. */
		return false;
	}

	/* Fit while preserving aspect ratio. */
	if (screen_width * image->height <= screen_height * image->width) {
		dst_width = screen_width;
		dst_height = screen_width * image->height / image->width;
	} else {
		dst_height = screen_height;
		dst_width = screen_height * image->width / image->height;
	}
	dst_x = (screen_width - dst_width) / 2;
	dst_y = (screen_height - dst_height) / 2;

	/* Draw. */
	hal_draw_image_3d_alpha(image,
				dst_x,
				dst_y,
				dst_x + dst_width,
				dst_y,
				dst_x,
				dst_y + dst_height,
				dst_x + dst_width,
				dst_y + dst_height,
				video_image,
				0,
				0,
				video_image->width,
				video_image->height,
				255);

	return true;
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
		image,
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
		image,
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
		image,
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
		image,
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
	hal_draw_image_rule(image, src_img, rule_img, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int progress)			/* The progress (0 to 255) */
{
	hal_draw_image_melt(image, src_img, rule_img, progress);
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
	hal_draw_image_cross(image,
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
	hal_draw_image_3d_alpha(image,
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
	hal_draw_image_3d_alpha(image,
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
	hal_draw_image_3d_sub(image,
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
	hal_draw_image_3d_dim(image,
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
	hal_draw_image_3d_cross(image,
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

/*
 * Reset a timer.
 */
void
hal_reset_lap_timer(
	uint64_t *t)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	*t = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/*
 * Get a timer lap.
 */
uint64_t
hal_get_lap_timer_millisec(
	uint64_t *t)
{
	struct timeval tv;
	uint64_t end;
	
	gettimeofday(&tv, NULL);

	end = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);

	return (uint64_t)(end - *t);
}

bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	char *path;

	path = hal_make_real_path(fname);

	is_gst_playing = true;
	is_gst_skippable = is_skippable;

	gstplay_play(path);

	free(path);

	return true;
}

void
hal_stop_video(void)
{
	gstplay_stop();

	is_gst_playing = false;
}

bool
hal_is_video_playing(void)
{
	return is_gst_playing;
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
hal_make_save_directory(void)
{
	return true;
}

char *
hal_make_real_path(const char *fname)
{
	return strdup(fname);
}

/*
 * Put an INFO log.
 */
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

/*
 * Put a WARN log.
 */
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

/*
 * Put an ERROR log.
 */
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

/* Open the log file. */
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
	return true;
}

const char *
hal_get_system_language(void)
{
	return lang_code;
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}
