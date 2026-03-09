/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux Wayland OpenGL
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

/* HAL */
#include "stratohal/platform.h"	/* Public Interface */
#include "stdfile.h"			/* Standard C File Implementation */
#if defined(HAL_TARGET_LINUX)
#include "asound.h"			/* ALSA Sound Implemenatation */
#else
#include "bsdsound.h"			/* BSD Sound Implementation */
#endif
#include "evgamepad.h"			/* evdev Gamepad */

/* OpenGL */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include "glrender.h"			/* OpenGL Compatibility Helper */

/* Wayland */
#include <wayland-client.h>
#include <wayland-egl.h>
#include "xdg-shell-client-protocol.h" /* Generated */
#include "zxdg-decoration-unstable-v1-client-protocol.h" /* Generated */

/* POSIX */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>	/* stat(), mkdir() */
#include <sys/time.h>	/* gettimeofday() */
#include <fcntl.h>
#include <unistd.h>	/* usleep(), access() */

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <locale.h>

#if 0
/* Gstreamer Video HAL */
#include "gstplay.h"
#endif

/* Color Format */
#define DEPTH		(24)
#define BPP		(32)

/* Log File */
#define LOG_FILE	"log.txt"

/* Save Directory */
#define SAVE_DIR	"save"

/* Frame Time */
#define FRAME_MILLI	(16)	/* Millisec of a frame */
#define SLEEP_MILLI	(5)	/* Millisec to sleep */

/* Virtual Screen Config */
static char *window_title;
static int screen_width;
static int screen_height;
static int viewport_width;
static int viewport_height;
static float mouse_scale = 1.0f;
static int mouse_ofs_x;
static int mouse_ofs_y;
static bool is_full_screen;

/* Physical Display Config */
static int display_width;
static int display_height;
static int display_refresh;

/* Wayland */
static struct wl_display *wl_dpy;
static struct wl_compositor *wl_compositor;
static struct xdg_wm_base *xdg_base;
static struct wl_surface *wl_surface;
static struct xdg_surface *xdg_surface;
static struct xdg_toplevel *xdg_toplevel;
static struct wl_egl_window *wlegl_win;
static struct zxdg_decoration_manager_v1 *decoration_mgr;
static struct wl_seat *wl_seat;
static struct wl_pointer *wl_pointer;
static struct wl_keyboard *wl_keyboard;

/* EGL Objects */
static EGLDisplay egl_dpy;
static EGLConfig egl_cfg;
static EGLContext egl_ctx;
static EGLSurface egl_surf;

/* Frame Start Time */
static struct timeval tv_start;

/* Log File */
#define LOG_BUF_SIZE	(4096)
static FILE *log_fp;

/* Locale */
const char *playfield_lang_code;

#if 0
/* Flag to indicate whether we are playing a video or not */
static bool is_gst_playing;

/* Flag to indicate whether a video is skippable or not */
static bool is_gst_skippable;
#endif

/* forward declaration */
static void init_locale(void);
static bool init_hal(int argc, char *argv[]);
static bool open_log_file(void);
static void close_log_file(void);
static bool open_display(void);
static void set_rotation(void);
static void close_display(void);
static void cleanup_hal(void);
static void destroy_window(void);
static void destroy_icon_image(void);
static void run_game_loop(void);
static bool run_frame(void);
static void flip(void);
static bool wait_for_next_frame(void);
static bool next_event(void);
static void update_viewport_size(int width, int height);
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *base, uint32_t serial);
static void xdg_surface_configure(void *data, struct xdg_surface* s, uint32_t serial);
static void xdg_toplevel_configure(void *data, struct xdg_toplevel* tl, int32_t w, int32_t h, struct wl_array *states);
static void xdg_toplevel_close(void *data, struct xdg_toplevel* tl);
static void registry_global(void *data, struct wl_registry *reg, uint32_t name, const char *interface, uint32_t version);
static void registry_remove(void *data, struct wl_registry *reg, uint32_t name);
static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
static void pointer_frame(void *data, struct wl_pointer *pointer) { /* no-op */ }
static void pointer_axis_source(void *data, struct wl_pointer *pointer, uint32_t axis_source) { /* no-op */ }
static void pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis) { /* no-op */ }
static void pointer_axis_discrete(void *data, struct wl_pointer *pointer, uint32_t axis, int32_t discrete) { /* no-op */ }
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size);
static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static int get_keycode(int kc);
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay);

/* Wayland */
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close     = xdg_toplevel_close,
};
static const struct wl_registry_listener registry_listener = {
	.global        = registry_global,
	.global_remove = registry_remove,
};
static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete,
};
static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info,
};

/*
 * Main
 */
int main(int argc, char *argv[])
{
	/* Initialize HAL. */
	if (!init_hal(argc, argv))
		return 1;

	/* Do a start callback. */
	if (!hal_callback_on_event_start())
		return 1;

	/* Run game loop. */
	run_game_loop();

	/* Do a stop callback.. */
	hal_callback_on_event_stop();

	/* Cleanup HAL. */
	cleanup_hal();

	return 0;
}

/* Initialize the locale. */
static void init_locale(void)
{
	const char *locale;

	locale = setlocale(LC_ALL, "");

	if (locale == NULL || locale[0] == '\0' || locale[1] == '\0')
		playfield_lang_code = "en";
	else if (strncmp(locale, "en", 2) == 0)
		playfield_lang_code = "en";
	else if (strncmp(locale, "fr", 2) == 0)
		playfield_lang_code = "fr";
	else if (strncmp(locale, "de", 2) == 0)
		playfield_lang_code = "de";
	else if (strncmp(locale, "it", 2) == 0)
		playfield_lang_code = "it";
	else if (strncmp(locale, "es", 2) == 0)
		playfield_lang_code = "es";
	else if (strncmp(locale, "el", 2) == 0)
		playfield_lang_code = "el";
	else if (strncmp(locale, "ru", 2) == 0)
		playfield_lang_code = "ru";
	else if (strncmp(locale, "zh_CN", 5) == 0)
		playfield_lang_code = "zh";
	else if (strncmp(locale, "zh_TW", 5) == 0)
		playfield_lang_code = "tw";
	else if (strncmp(locale, "ja", 2) == 0)
		playfield_lang_code = "ja";
	else
		playfield_lang_code = "en";

	setlocale(LC_ALL, "C");
}

/* Initialize HAL. */
static bool init_hal(int argc, char *argv[])
{
	/* Initialize the locale. */
	init_locale();

	/* Initialize the file HAL. */
	if (!init_file())
		return false;

	/* Do a boot callback. */
	if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
		return false;

	/* Initialize the sound HAL. */
	if (!init_sound()) {
		/* Ignore a failure. */
		hal_log_warn("Can't initialize sound.");
	}

	/* Open an X11 display. */
	if (!open_display()) {
		hal_log_error("Can't open display.");
		return false;
	}

	/* Initizalize OpenGL. */
	if (!init_opengl(screen_width, screen_height)) {
		hal_log_error("Can't initialize OpenGL.");
	}
	update_viewport_size(display_width, display_height);

	/* Initialize the gamepad. */
	init_evgamepad();

#if 0
	/* Initialize the viddeo HAL. */
	gstplay_init(argc, argv);
#endif

	return true;
}

/* Open a display. */
static bool open_display(void)
{
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
	static const EGLint cfg_attrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	EGLint ncfg;
	EGLConfig configs[64];

	wl_dpy = wl_display_connect(NULL);
	if (!wl_dpy) {
		hal_log_error("wl_display_connect failed");
		return false;
	}

	struct wl_registry* reg = wl_display_get_registry(wl_dpy);
	wl_registry_add_listener(reg, &registry_listener, NULL);
	wl_display_roundtrip(wl_dpy);
	if (!wl_compositor || !xdg_base) {
		hal_log_error("required globals missing");
		return false;
	}

	wl_surface = wl_compositor_create_surface(wl_compositor);
	xdg_surface = xdg_wm_base_get_xdg_surface(xdg_base, wl_surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

	xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(xdg_toplevel, window_title);

	wl_surface_commit(wl_surface);

	wlegl_win = wl_egl_window_create(wl_surface, screen_width, screen_height);
	if (!wlegl_win) {
		hal_log_error("wl_egl_window_create failed");
		return false;
	}

	egl_dpy = eglGetDisplay((EGLNativeDisplayType)wl_dpy);
	if (egl_dpy == EGL_NO_DISPLAY) {
		hal_log_error("eglGetDisplay failed");
		return false;
	}

	if (!eglInitialize(egl_dpy, NULL, NULL)) {
		hal_log_error("eglInitialize failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		hal_log_error("eglBindAPI(EGL_OPENGL_ES_API) failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglChooseConfig(egl_dpy, cfg_attrs, configs, 64, &ncfg)) {
		hal_log_error("eglChooseConfig() failed: 0x%04x\n", eglGetError());
		return false;
	}
	if (ncfg <= 0) {
		hal_log_error("eglChooseConfig(count) failed.\n");
		return false;
	}

	egl_cfg = configs[0];
	for (int i = 0; i < ncfg; i++) {
		EGLint surface_type = 0, r = 0, g = 0, b = 0, a = 0, depth = 0;
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_SURFACE_TYPE, &surface_type);
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_RED_SIZE, &r);
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_BLUE_SIZE, &b);
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_ALPHA_SIZE, &a);
		eglGetConfigAttrib(egl_dpy, configs[i], EGL_DEPTH_SIZE, &depth);

		if ((surface_type & EGL_WINDOW_BIT) &&
		    r == 8 && g == 8 && b == 8 && a == 8) {
			egl_cfg = configs[i];
			break;
		}
	}

	egl_ctx = eglCreateContext(egl_dpy, egl_cfg, EGL_NO_CONTEXT, (EGLint[]){EGL_CONTEXT_CLIENT_VERSION,2, EGL_NONE});
	if (egl_ctx == EGL_NO_CONTEXT) {
		hal_log_error("eglCreateContext failed: 0x%04x\n", eglGetError());
		return false;
	}

	egl_surf = eglCreateWindowSurface(egl_dpy, egl_cfg, (EGLNativeWindowType)wlegl_win, NULL);
	if (egl_surf == EGL_NO_SURFACE) {
		hal_log_error("eglCreateWindowSurface failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
		hal_log_error("eglMakeCurrent failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (decoration_mgr) {
		struct zxdg_toplevel_decoration_v1 *deco =
			zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_mgr, xdg_toplevel);
		zxdg_toplevel_decoration_v1_set_mode(deco, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
	}

	wl_display_roundtrip(wl_dpy);

	if (wl_seat) {
		wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(wl_pointer, &pointer_listener, NULL);
		wl_keyboard = wl_seat_get_keyboard(wl_seat);
		wl_keyboard_add_listener(wl_keyboard, &keyboard_listener, NULL);
	}

	return true;
}

/* Cleanup the subsystems. */
static void cleanup_hal(void)
{
	/* Cleanup sound. */
	cleanup_sound();

	/* Close the display. */
	close_display();

	/* Close the log file. */
	close_log_file();
}

/* Close the display. */
static void close_display(void)
{
	eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(egl_dpy, egl_surf);
	eglDestroyContext(egl_dpy, egl_ctx);
	eglTerminate(egl_dpy);
    
	wl_egl_window_destroy(wlegl_win);
	xdg_toplevel_destroy(xdg_toplevel);
	xdg_surface_destroy(xdg_surface);
	wl_surface_destroy(wl_surface);
	wl_display_disconnect(wl_dpy);
}

/* Close the log file. */
static void close_log_file(void)
{
	if (log_fp != NULL)
		fclose(log_fp);
}

/* Run the event loop. */
static void run_game_loop(void)
{
	/* Get the frame start time. */
	gettimeofday(&tv_start, NULL);

	/* Main Loop */
	while (true) {
#if 0
		/* Process video playback. */
		if (is_gst_playing) {
			gstplay_loop_iteration();
			if (!gstplay_is_playing()) {
				gstplay_stop();
				is_gst_playing = false;
			}
		}
#endif

		/* Run a frame. */
		if (!run_frame())
			break;

		/* Wait for the next frame timing. */
		if (!wait_for_next_frame())
			break;	/* Close button was pressed. */

		/* Get the frame start time. */
		gettimeofday(&tv_start, NULL);
	}
}

/* Run a frame. */
static bool run_frame(void)
{
	bool cont;

	/* Read the gamepad. */
	update_evgamepad();

	/* Start rendering. */
#if 0
	if (!is_gst_playing)
		opengl_start_rendering();
#endif
	wl_display_dispatch_pending(wl_dpy);
	wl_display_flush(wl_dpy);
	opengl_start_rendering();

	/* Call a frame event. */
	cont = hal_callback_on_event_frame();

	/* End rendering. */
#if 0
	if (!is_gst_playing) {
		opengl_end_rendering();
		eglSwapBuffers(dpy, esurf);
		wl_display_flush(wl_dpy);
	}
#endif
	opengl_end_rendering();
	eglSwapBuffers(egl_dpy, egl_surf);
	wl_display_flush(wl_dpy);

	return cont;
}

/* Wait for the next frame timing. */
static bool wait_for_next_frame(void)
{
	struct timeval tv_end;
	uint32_t lap, wait, span;

	span = FRAME_MILLI;

	/* Do event processing and sleep until the time of next frame start. */
	do {
		/* Get a lap time. */
		gettimeofday(&tv_end, NULL);
		lap = (uint32_t)((tv_end.tv_sec - tv_start.tv_sec) * 1000 +
				 (tv_end.tv_usec - tv_start.tv_usec) / 1000);

		/* Break if the time has come. */
		if (lap > span) {
			tv_start = tv_end;
			break;
		}

		/* Calc a sleep time. */
		wait = (span - lap > SLEEP_MILLI) ? SLEEP_MILLI : span - lap;

		/* Do sleep. */
		usleep(wait * 1000);

	} while(wait > 0);

	return true;
}

/*
 * Sets the window size.
 */
static void update_viewport_size(int width, int height)
{
	float aspect, use_width, use_height;
	int orig_x, orig_y;
	int viewport_width, viewport_height;

	/* Calc the aspect ratio of the game. */
	aspect = (float)screen_height / (float)screen_width;

	/* Calc the height. (temporarily with "width-first") */
	use_width = (float)width;
	use_height = use_width * aspect;
	mouse_scale = (float)screen_width / (float)width;

	/* If height is not enough, calc the width. (with "height-first") */
	if(use_height > (float)screen_width) {
		use_height = (float)height;
		use_width = (float)use_height / aspect;
		mouse_scale = (float)screen_height / (float)height;
	}

	/* Calc the viewport origin. */
	orig_x = (int)((((float)width - use_width) / 2.0f) + 0.5);
	orig_y = (int)((((float)height - use_height) / 2.0f) + 0.5);
	mouse_ofs_x = orig_x;
	mouse_ofs_y = orig_y;
	printf("!mouse_scale = %f\n", mouse_scale);

	/* Calc the viewport size. */
	viewport_width = (int)use_width;
	viewport_height = (int)use_height;

	/* Update the screen offset and scale for drawing subsystem. */
	opengl_set_screen(orig_x, orig_y, viewport_width, viewport_height);
}

/*
 * HAL
 */

/*
 * Put an INFO log.
 */
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

	printf("%s\n", buf);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}

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
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	printf("%s\n", buf);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}

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
	char buf[LOG_BUF_SIZE];
	va_list ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);

	printf("%s\n", buf);

	open_log_file();
	if (log_fp != NULL) {
		fprintf(log_fp, "%s\n", buf);
		fflush(log_fp);
		if (ferror(log_fp))
			return false;
	}
	
	return true;
}

/*
 * Put an out-of-memory error.
 */
bool
hal_log_out_of_memory(void)
{
	hal_log_error(HAL_TR("Out of memory."));
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

/*
 * Make a save directory.
 */
bool
hal_make_save_directory(void)
{
	struct stat st = {0};

	if (stat(SAVE_DIR, &st) == -1)
		mkdir(SAVE_DIR, 0700);

	return true;
}

/*
 * Make an effective path from a directory name and a file name.
 */
char *
hal_make_real_path(const char *fname)
{
	char *buf;
	size_t len;

	/* Allocate a path buffer. */
	len = strlen(fname) + 1;
	buf = malloc(len);
	if (buf == NULL) {
		hal_log_out_of_memory();
		return NULL;
	}

	/* Copy as is. */
	snprintf(buf, len, "%s", fname);

	return buf;
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

/*
 * Notify an image update.
 */
void
hal_notify_image_update(
	struct hal_image *img)
{
	opengl_notify_image_update(img);
}

/*
 * Notify an image free.
 */
void
hal_notify_image_free(
	struct hal_image *img)
{
	opengl_notify_image_free(img);
}

/*
 * Render an image. (alpha blend)
 */
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
	opengl_render_image_normal(
		dst_left,
		dst_top,
		dst_width,
		dst_height,
		src_image,
		src_left,
		src_top,
		src_width,
		src_height,
		alpha
	);
}

/*
 * Render an image. (add blend)
 */
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
	opengl_render_image_add(
		dst_left,
		dst_top,
		dst_width,
		dst_height,
		src_image,
		src_left,
		src_top,
		src_width,
		src_height,
		alpha
	);
}

/*
 * Render an image. (sub blend)
 */
void
hal_render_image_sub(
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
	opengl_render_image_sub(
		dst_left,
		dst_top,
		dst_width,
		dst_height,
		src_image,
		src_left,
		src_top,
		src_width,
		src_height,
		alpha
	);
}

/*
 * Render an image. (dim blend)
 */
void
hal_render_image_dim(
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
	opengl_render_image_dim(
		dst_left,
		dst_top,
		dst_width,
		dst_height,
		src_image,
		src_left,
		src_top,
		src_width,
		src_height,
		alpha
	);
}

/*
 * Render an image. (rule universal transition)
 */
void
hal_render_image_rule(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int threshold)
{
	opengl_render_image_rule(src_img, rule_img, threshold);
}

/*
 * Render an image. (melt universal transition)
 */
void
hal_render_image_melt(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int progress)
{
	opengl_render_image_melt(src_img, rule_img, progress);
}

/*
 * Render an image. (3d transform, alpha blending)
 */
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
	opengl_render_image_3d_normal(
		x1, y1, x2, y2, x3, y3, x4, y4,
		src_image, src_left, src_top, src_width, src_height,
		alpha);
}

/*
 * Render an image. (3d transform, alpha blending)
 */
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
	opengl_render_image_3d_add(
		x1, y1, x2, y2, x3, y3, x4, y4,
		src_image, src_left, src_top, src_width, src_height,
		alpha);
}

/*
 * Render an image. (3d transform, sub blending)
 */
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
	opengl_render_image_3d_sub(
		x1, y1, x2, y2, x3, y3, x4, y4,
		src_image, src_left, src_top, src_width, src_height,
		alpha);
}

/*
 * Render an image. (3d transform, dim blending)
 */
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
	opengl_render_image_3d_dim(
		x1, y1, x2, y2, x3, y3, x4, y4,
		src_image, src_left, src_top, src_width, src_height,
		alpha);
}

/*
 * Play a video.
 */
bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
#if 0
	char *path;

	path = make_real_path(fname);

	is_gst_playing = true;
	is_gst_skippable = is_skippable;

	gstplay_play(path, window);

	free(path);
#endif

	return true;
}

/*
 * Stop the video.
 */
void
hal_stop_video(void)
{
#if 0
	gstplay_stop();

	is_gst_playing = false;
#endif
}

/*
 * Check whether a video is playing.
 */
bool
hal_is_video_playing(void)
{
#if 0
	return is_gst_playing;
#endif
	return false;
}

/*
 * Check whether full screen mode is supported.
 */
bool
hal_is_full_screen_supported(void)
{
	return false;
}

/*
 * Check whether we are in full screen mode.
 */
bool
hal_is_full_screen_mode(void)
{
	return false;
}

/*
 * Enter full screen mode.
 */
void
hal_enter_full_screen_mode(void)
{
	if (is_full_screen)
		return;

	xdg_toplevel_set_fullscreen(xdg_toplevel, NULL);
	wl_surface_commit(wl_surface);

	is_full_screen = true;
}

/*
 * Leave full screen mode.
 */
void
hal_leave_full_screen_mode(void)
{
	if (!is_full_screen)
		return;

	xdg_toplevel_unset_fullscreen(xdg_toplevel);
	wl_surface_commit(wl_surface);

	is_full_screen = false;
}

/*
 * Get the system locale.
 */
const char *
hal_get_system_language(void)
{
	return playfield_lang_code;
}

/*
 * Enable/disable message skip by touch move.
 */
void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}

/*
 * Wayland
 */

static int last_mouse_x;
static int last_mouse_y;

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *base, uint32_t serial)
{
	UNUSED_PARAMETER(data);

	xdg_wm_base_pong(base, serial);
}

static void xdg_surface_configure(void *data, struct xdg_surface* s, uint32_t serial)
{
	UNUSED_PARAMETER(data);

	xdg_surface_ack_configure(s, serial);
}

static void xdg_toplevel_configure(void *data, struct xdg_toplevel* tl, int32_t w, int32_t h, struct wl_array *states)
{
	struct wl_region *region;

	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(tl);
	UNUSED_PARAMETER(states);

	if (w > 0 && h > 0) {
		wl_egl_window_resize(wlegl_win, w, h, 0, 0);
		glViewport(0, 0, w, h);
		update_viewport_size(w, h);

		glDisable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(egl_dpy, egl_surf);

		region = wl_compositor_create_region(wl_compositor);
		wl_region_add(region, 0, 0, w, w);
		wl_surface_set_opaque_region(wl_surface, region);
		wl_region_destroy(region);
	}
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel* tl)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(tl);

	wl_display_disconnect(wl_dpy);
	exit(0);
}

static void registry_global(void *data, struct wl_registry *reg, uint32_t name, const char *interface, uint32_t version)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(version);

	if (strcmp(interface, "wl_compositor") == 0) {
		wl_compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		xdg_base = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(xdg_base, &xdg_wm_base_listener, NULL);
	} else if (strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
		decoration_mgr = wl_registry_bind(reg, name, &zxdg_decoration_manager_v1_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		wl_seat = wl_registry_bind(reg, name, &wl_seat_interface, version < 5 ? version : 5);
	}
}

static void registry_remove(void *data, struct wl_registry *reg, uint32_t name)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(reg);
	UNUSED_PARAMETER(name);
}

static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(pointer);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(surface);

	printf("Pointer enter at %.2f, %.2f\n", wl_fixed_to_double(sx), wl_fixed_to_double(sy));
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(pointer);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(surface);
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(pointer);
	UNUSED_PARAMETER(time);

	last_mouse_x = (int)((wl_fixed_to_double(sx) - mouse_ofs_x) * mouse_scale);
	last_mouse_y = (int)((wl_fixed_to_double(sy) - mouse_ofs_y) * mouse_scale);

	hal_callback_on_event_mouse_move(last_mouse_x, last_mouse_y);
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(pointer);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(time);

	if (button == 272) {
		if (state == WL_POINTER_BUTTON_STATE_PRESSED)
			hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, last_mouse_x, last_mouse_y);
		else
			hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, last_mouse_x, last_mouse_y);
	} else if (button == 273) {
		if (state == WL_POINTER_BUTTON_STATE_PRESSED)
			hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, last_mouse_x, last_mouse_y);
		else
			hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, last_mouse_x, last_mouse_y);
	}
}

static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(pointer);
	UNUSED_PARAMETER(time);
}

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
	close(fd);
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	static bool is_alt_pressed;

	int keycode;

	keycode = get_keycode(key);

	if (keycode == HAL_KEY_ALT) {
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
			is_alt_pressed = true;
		else
			is_alt_pressed = false;
	}

	if (keycode == HAL_KEY_RETURN && state == WL_KEYBOARD_KEY_STATE_PRESSED && is_alt_pressed) {
		if (!is_full_screen)
			hal_enter_full_screen_mode();
		else
			hal_leave_full_screen_mode();
		return;
	}

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		hal_callback_on_event_key_press(keycode);
	else
		hal_callback_on_event_key_release(keycode);
}

static int get_keycode(int kc)
{
	switch (kc) {
	case 1:   return HAL_KEY_ESCAPE;    // HAL_KEY_ESC
	case 28:  return HAL_KEY_RETURN;    // HAL_KEY_ENTER
	case 96:  return HAL_KEY_RETURN;    // HAL_KEY_KPENTER
	case 57:  return HAL_KEY_SPACE;     // HAL_KEY_SPACE
	case 15:  return HAL_KEY_TAB;       // HAL_KEY_TAB
	case 14:  return HAL_KEY_BACKSPACE; // HAL_KEY_BACKSPACE
	case 111: return HAL_KEY_DELETE;    // HAL_KEY_DELETE
	case 102: return HAL_KEY_HOME;      // HAL_KEY_HOME
	case 107: return HAL_KEY_END;       // HAL_KEY_END
	case 104: return HAL_KEY_PAGEUP;    // HAL_KEY_PAGEUP
	case 109: return HAL_KEY_PAGEDOWN;  // HAL_KEY_PAGEDOWN
	case 42:                        // HAL_KEY_LEFTSHIFT
	case 54:  return HAL_KEY_SHIFT;     // HAL_KEY_RIGHTSHIFT
	case 29:                        // HAL_KEY_LEFTCTRL
	case 97:                        // HAL_KEY_RIGHTCTRL
		return HAL_KEY_CONTROL;
	case 56:                        // HAL_KEY_LEFTALT
	case 100:                       // HAL_KEY_RIGHTALT
		return HAL_KEY_ALT;
	case 108: return HAL_KEY_DOWN;
	case 103: return HAL_KEY_UP;
	case 105: return HAL_KEY_LEFT;
	case 106: return HAL_KEY_RIGHT;

	// Letters (A=30 … Z=44)
	case 30: return HAL_KEY_A;
	case 48: return HAL_KEY_B;
	case 46: return HAL_KEY_C;
	case 32: return HAL_KEY_D;
	case 18: return HAL_KEY_E;
	case 33: return HAL_KEY_F;
	case 34: return HAL_KEY_G;
	case 35: return HAL_KEY_H;
	case 23: return HAL_KEY_I;
	case 36: return HAL_KEY_J;
	case 37: return HAL_KEY_K;
	case 38: return HAL_KEY_L;
	case 50: return HAL_KEY_M;
	case 49: return HAL_KEY_N;
	case 24: return HAL_KEY_O;
	case 25: return HAL_KEY_P;
	case 16: return HAL_KEY_Q;
	case 19: return HAL_KEY_R;
	case 31: return HAL_KEY_S;
	case 20: return HAL_KEY_T;
	case 22: return HAL_KEY_U;
	case 47: return HAL_KEY_V;
	case 17: return HAL_KEY_W;
	case 45: return HAL_KEY_X;
	case 21: return HAL_KEY_Y;
	case 44: return HAL_KEY_Z;

	// Numbers (1=2 … 0=11)
	case 2:  return HAL_KEY_1;
	case 3:  return HAL_KEY_2;
	case 4:  return HAL_KEY_3;
	case 5:  return HAL_KEY_4;
	case 6:  return HAL_KEY_5;
	case 7:  return HAL_KEY_6;
	case 8:  return HAL_KEY_7;
	case 9:  return HAL_KEY_8;
	case 10: return HAL_KEY_9;
	case 11: return HAL_KEY_0;

	// Function keys
	case 59: return HAL_KEY_F1;
	case 60: return HAL_KEY_F2;
	case 61: return HAL_KEY_F3;
	case 62: return HAL_KEY_F4;
	case 63: return HAL_KEY_F5;
	case 64: return HAL_KEY_F6;
	case 65: return HAL_KEY_F7;
	case 66: return HAL_KEY_F8;
	case 67: return HAL_KEY_F9;
	case 68: return HAL_KEY_F10;
	case 87: return HAL_KEY_F11;
	case 88: return HAL_KEY_F12;

	default:
		break;
	}
	return 0;
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
                               uint32_t group)
{
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
}
