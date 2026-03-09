/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux GBM framebuffer
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

/* GBM */
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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
static bool is_full_screen;

/* Physical Display Config */
static int display_width;
static int display_height;
static int display_refresh;

/* GBM/DRM/EGL Objects */
static int fd;
static struct gbm_bo *bo;
struct gbm_surface *gsurf;
static drmModeRes *res;
static drmModeEncoder *enc;
static drmModeConnector *conn;
static drmModeCrtc *orig;
static drmModeModeInfo mode;
static uint32_t crtc_id;
static uint32_t fb;
static EGLDisplay dpy;
static EGLSurface esurf;

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
static drmModeConnector* find_conn(int fd, drmModeRes *res, drmModeEncoder **out_enc);
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

/*
 * Main
 */
int
main(
	int argc,
	char *argv[])
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
static void
init_locale(void)
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
static bool
init_hal(
	int argc,
	char *argv[])
{
	/* Initialize the locale. */
	init_locale();

	/* Initialize the file HAL. */
	if (!init_file())
		return false;

	/* Do a boot callback. */
	if (!hal_callback_on_event_boot(&window_title,
					&screen_width,
					&screen_height))
		return false;

	/* Initialize the sound HAL. */
	if (!init_sound()) {
		/* Ignore a failure. */
		log_warn("Can't initialize sound.");
	}

	/* Open a display. */
	if (!open_display()) {
		log_error("Can't open display.");
		return false;
	}

	/* Initizalize OpenGL. */
	if (!init_opengl(screen_width, screen_height)) {
		log_error("Can't initialize OpenGL.");
	}
#if !defined(USE_ROT90)
	update_viewport_size(display_width, display_height);
#else
	update_viewport_size(display_height, display_width);
#endif

	/* Initialize the gamepad. */
	init_evgamepad();

#if 0
	/* Initialize the viddeo HAL. */
	gstplay_init(argc, argv);
#endif

	return true;
}

/* Open a display. */
static bool
open_display(void)
{
	struct gbm_device *gbm;
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
	static const EGLint base_cfg_attrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	EGLConfig cfg;
	EGLint ncfg;
	EGLConfig *cfgs;
	EGLContext ctx;
	uint32_t handle;
	uint32_t stride;
	int r;

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
		if (fd < 0) {
			perror("open(/dev/dri/card?)");
			return false;
		}
	}

	drmModeRes *res = drmModeGetResources(fd);
	if (res == NULL) {
		fprintf(stderr, "drmModeGetResources failed\n");
		return false;
	}

	enc = NULL;
	conn = find_conn(fd, res, &enc);
	if (conn == NULL) {
		fprintf(stderr, "no connected connector\n");
		return false;
	}

	mode = conn->modes[0]; /* Use the first mode. */
	crtc_id = enc ? enc->crtc_id : 0;
	if (!crtc_id) {
		/* TODO: Select an CRTC. */
		crtc_id = res->crtcs[0];
	}

	display_width = conn->modes[0].hdisplay;
	display_height = conn->modes[0].vdisplay;
	display_refresh = conn->modes[0].vrefresh;

	gbm = gbm_create_device(fd);
	if (gbm == NULL) {
		fprintf(stderr, "gbm_create_device failed\n");
		return false;
	}

	gsurf = gbm_surface_create(gbm,
				   mode.hdisplay,
				   mode.vdisplay,
				   GBM_FORMAT_XRGB8888,
				   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (gsurf == NULL) {
	        fprintf(stderr, "gbm_surface_create failed (XRGB8888)\n");
		return false;
	}

	// EGL display
	eglGetPlatformDisplayEXT = (void*)eglGetProcAddress("eglGetPlatformDisplayEXT");
	if (eglGetPlatformDisplayEXT == NULL) {
		fprintf(stderr, "eglGetPlatformDisplayEXT not found (check EGL_EXT_platform_base / EGL_KHR_platform_gbm)\n");
		return false;
	}

	dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm, NULL);
	if (dpy == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetPlatformDisplayEXT returned NO_DISPLAY\n");
		return false;
	}

	if (!eglInitialize(dpy, NULL, NULL)) {
		fprintf(stderr, "eglInitialize failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		fprintf(stderr, "eglBindAPI(EGL_OPENGL_ES_API) failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglChooseConfig(dpy, base_cfg_attrs, NULL, 0, &ncfg)) {
		fprintf(stderr, "eglChooseConfig(count) failed or no configs: 0x%04x\n", eglGetError());
		return false;
	}
	if (ncfg <= 0)
		return false;

	cfgs = (EGLConfig*)malloc(sizeof(EGLConfig) * ncfg);
	if (!cfgs)
		return false;

	if (!eglChooseConfig(dpy, base_cfg_attrs, cfgs, ncfg, &ncfg) || ncfg <= 0) {
		fprintf(stderr, "eglChooseConfig(list) failed: 0x%04x\n", eglGetError());
		free(cfgs);
		return false;
	}

	// XRGB first.
	for (int i = 0; i < ncfg; ++i) {
		EGLint id = 0, a = 0, rsz=0, gsz=0, bsz=0;
		eglGetConfigAttrib(dpy, cfgs[i], EGL_NATIVE_VISUAL_ID, &id);
		eglGetConfigAttrib(dpy, cfgs[i], EGL_ALPHA_SIZE, &a);
		eglGetConfigAttrib(dpy, cfgs[i], EGL_RED_SIZE, &rsz);
		eglGetConfigAttrib(dpy, cfgs[i], EGL_GREEN_SIZE, &gsz);
		eglGetConfigAttrib(dpy, cfgs[i], EGL_BLUE_SIZE, &bsz);
		if ((uint32_t)id == GBM_FORMAT_XRGB8888 && a == 0 && rsz==8 && gsz==8 && bsz==8) {
			cfg = cfgs[i];
			break;
		}
	}

	// ARGB fallback.
	if (!cfg) {
		for (int i = 0; i < ncfg; ++i) {
			EGLint id = 0;
			eglGetConfigAttrib(dpy, cfgs[i], EGL_NATIVE_VISUAL_ID, &id);
			if ((uint32_t)id == GBM_FORMAT_ARGB8888) {
				cfg = cfgs[i];
				break;
			}
		}
		if (cfg) {
			gbm_surface_destroy(gsurf);
			gsurf = gbm_surface_create(gbm,
						   mode.hdisplay,
						   mode.vdisplay,
						   GBM_FORMAT_ARGB8888,
						   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
			if (!gsurf) {
				fprintf(stderr, "gbm_surface_create failed (fallback ARGB8888)\n");
				free(cfgs);
				return false;
			}
		}
	}

	free(cfgs);

	if (!cfg) {
		fprintf(stderr, "No matching EGLConfig for GBM format (XRGB/ARGB)\n");
		return false;
	}

	ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, (EGLint[]){EGL_CONTEXT_CLIENT_VERSION,2, EGL_NONE});
	if (ctx == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext failed: 0x%04x\n", eglGetError());
		return false;
	}

	esurf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)gsurf, NULL);
	if (esurf == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface failed: 0x%04x\n", eglGetError());
		return false;
	}

	if (!eglMakeCurrent(dpy, esurf, esurf, ctx)) {
		fprintf(stderr, "eglMakeCurrent failed: 0x%04x\n", eglGetError());
		return false;
	}

	// Draw 1 frame to lock the front buffer.
	glViewport(0, 0, display_width, display_height);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(dpy, esurf);

	// Make a BO for the first scan out, then set to CRTC.
	bo = gbm_surface_lock_front_buffer(gsurf);
	if (!bo) {
		fprintf(stderr, "gbm_surface_lock_front_buffer failed\n");
		return false;
	}

	handle = gbm_bo_get_handle(bo).u32;
	stride = gbm_bo_get_stride(bo);

	r = drmModeAddFB(fd, mode.hdisplay, mode.vdisplay, 24, 32, stride, handle, &fb);
	if (r != 0) {
		perror("drmModeAddFB");
		return false;
	}

	orig = drmModeGetCrtc(fd, crtc_id);
	if (drmModeSetCrtc(fd, crtc_id, fb, 0, 0, &conn->connector_id, 1, &mode)) {
		fprintf(stderr, "drmModeSetCrtc failed\n");
		return false;
	}

	return true;
}

static drmModeConnector*
find_conn(
	int fd,
	drmModeRes *res,
	drmModeEncoder **out_enc)
{
	int i;

	for (i = 0; i < res->count_connectors; i++) {
		drmModeConnector *c = drmModeGetConnector(fd, res->connectors[i]);
		if (c && c->connection == DRM_MODE_CONNECTED && c->count_modes>0) {
			if (out_enc)
				*out_enc = drmModeGetEncoder(fd, c->encoder_id);
			return c;
		}
		if (c)
			drmModeFreeConnector(c);
	}
	return NULL;
}

/* Cleanup the subsystems. */
static void
cleanup_hal(void)
{
	/* Cleanup sound. */
	cleanup_sound();

	/* Close the display. */
	close_display();

	/* Close the log file. */
	close_log_file();
}

/* Close the display. */
static void
close_display(void)
{
    drmModeSetCrtc(fd, orig->crtc_id, orig->buffer_id, orig->x, orig->y, &conn->connector_id, 1, &orig->mode);
    drmModeFreeCrtc(orig);
    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(fd);
}

/* Close the log file. */
static void
close_log_file(void)
{
	if (log_fp != NULL)
		fclose(log_fp);
}

/* Run the event loop. */
static void
run_game_loop(void)
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
static bool
run_frame(void)
{
	bool cont;

	/* Read the gamepad. */
	update_evgamepad();

	/* Start rendering. */
#if 0
	if (!is_gst_playing)
		opengl_start_rendering();
#endif
	opengl_start_rendering();

	/* Call a frame event. */
	cont = on_event_frame();

	/* End rendering. */
#if 0
	if (!is_gst_playing) {
		opengl_end_rendering();
		eglSwapBuffers(dpy, esurf);
		flip();
	}
#endif
	opengl_end_rendering();
	eglSwapBuffers(dpy, esurf);
	flip();

	return cont;
}

/* Flip buffers. */
static void
flip(void)
{
	struct gbm_bo *nbo;
	uint32_t nh;
	uint32_t ns;
	uint32_t nfb;
	fd_set fds;

	// Get the new front fb and flip.
	nbo = gbm_surface_lock_front_buffer(gsurf);
	nh = gbm_bo_get_handle(nbo).u32;
	ns = gbm_bo_get_stride(nbo);
	drmModeAddFB(fd, mode.hdisplay, mode.vdisplay, 24, 32, ns, nh, &nfb);
	drmModePageFlip(fd, crtc_id, nfb, DRM_MODE_PAGE_FLIP_EVENT, NULL);

	// Wait for an event.
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	select(fd + 1, &fds, NULL, NULL, NULL);
	drmEventContext ev = { .version = DRM_EVENT_CONTEXT_VERSION, .page_flip_handler = NULL };
	drmHandleEvent(fd, &ev);

	// Free the old fb.
	drmModeRmFB(fd, fb);
	gbm_surface_release_buffer(gsurf, bo);
	fb = nfb;
	bo = nbo;
}

/* Wait for the next frame timing. */
static bool
wait_for_next_frame(void)
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
static void
update_viewport_size(
	int width,
	int height)
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
	log_error(S_TR("Out of memory."));
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
hal_make_real_path(
	const char *fname)
{
	char *buf;
	size_t len;

	/* Allocate a path buffer. */
	len = strlen(fname) + 1;
	buf = malloc(len);
	if (buf == NULL) {
		log_out_of_memory();
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
	struct image *img)
{
	opengl_notify_image_update(img);
}

/*
 * Notify an image free.
 */
void
hal_notify_image_free(
	struct image *img)
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
	struct image *src_image,
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
 * Render an image. (dim blend)
 */
void
hal_render_image_dim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct image *src_image,
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
	struct image *src_img,
	struct image *rule_img,
	int threshold)
{
	opengl_render_image_rule(src_img, rule_img, threshold);
}

/*
 * Render an image. (melt universal transition)
 */
void
hal_render_image_melt(
	struct image *src_img,
	struct image *rule_img,
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
	struct image *src_image,
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
	struct image *src_image,
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
	/* stub */
}

/*
 * Leave full screen mode.
 */
void
hal_leave_full_screen_mode(void)
{
	/* stub */
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
hal_set_continuous_swipe_enabled(bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}
