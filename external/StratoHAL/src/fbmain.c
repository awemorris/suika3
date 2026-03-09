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
#include <assert.h>

/* Log File */
#define LOG_FILE	"log.txt"

/* Save Directory */
#define SAVE_DIR	"save"

/* Back Image */
static struct image *image;

/* Screen Info */
static char *window_title;
static int fb_fd;
static size_t fb_size;
static pixel_t *fb_pixels;
static int fb_width;
static int fb_height;
static int screen_width;
static int screen_height;

/* Input Info */
#define EV_DEV_MAX	16
static int ev_fd[EV_DEV_MAX];
static int ev_count;
static struct pollfd ev_fds[EV_DEV_MAX];
static int mouse_x;
static int mouse_y;

/* Log */
static FILE *log_fp;

/* Forward Declaration */
static bool init_fb(void);
static void cleanup_fb(void);
static bool init_input(void);
static void cleanup_input(void);
static void process_input(void);
static void process_event(int index);
static bool open_log_file(void);

int
main(
	int argc,
	char *argv[])
{
	int x, y;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	if (!init_fb())
		return 1;

	if (!init_file())
		return 1;

	if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
		return 1;

	hal_create_image(screen_width, screen_height, &image);

	init_sound();
	init_input();

	if (!hal_callback_on_event_start())
		return 1;

	while (1) {
		process_input();

		hal_clear_image(image, 0);
		if (!hal_callback_on_event_frame())
			break;

		for (y = 0; y < screen_height; y++) {
			for (x = 0; x < screen_width; x++) {
				fb_pixels[y * fb_width + x] = image->pixels[y * image->width + x];
			}
		}
	}


	return 0;

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
	fb_pixels = (pixel_t *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
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

static void process_event(int index)
{
	struct input_event e;

	/* Read an event. */
	if (read(ev_fd[index], &e, sizeof(e)) != sizeof(e)) {
		printf("Wrror: no input.\n");
		return;
	}

	//printf("Event: type=%d, code=%d, value=%d\n", e.type, e.code, e.value);

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
		on_event_mouse_move(mouse_x, mouse_y);
	} else if (e.type == EV_KEY) {
		/* XXX: currently only mouse buttons are supported. */
		if (e.code == 272) {
			if (e.value == 1)
				hal_callback_on_event_mouse_press(MOUSE_LEFT, mouse_x, mouse_y);
			else
				hal_callback_on_event_mouse_release(MOUSE_LEFT, mouse_x, mouse_y);
		} else if (e.code ==273) {
			if (e.value == 1)
				hal_callback_on_event_mouse_press(MOUSE_RIGHT, mouse_x, mouse_y);
			else
				hal_callback_on_event_mouse_release(MOUSE_RIGHT, mouse_x, mouse_y);
		}
	}
}

/*
 * HAL
 */

void hal_notify_image_update(struct image *img)
{
	UNUSED_PARAMETER(img);
}

void hal_notify_image_free(struct image *img)
{
	UNUSED_PARAMETER(img);
}

void
hal_render_image_normal(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] The image to be rendered */
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
	struct image *src_image,	/* [IN] The image to be rendered */
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
hal_render_image_dim(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] The image to be rendered */
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
hal_render_image_3d_normal(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
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
	struct image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
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
	const char *fname,	/* file name */
	bool is_skippable)	/* allow skip for a unseen video */
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
	return "en";
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}
