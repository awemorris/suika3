/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux X11 Software Rendering
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
#include "stratohal/platform.h"		/* Public Interface */
#include "stdfile.h"			/* Standard C File Implementation */
#if defined(HAL_TARGET_LINUX)
#include "asound.h"			/* ALSA Sound Implemenatation */
#else
#include "bsdsound.h"			/* BSD Sound Implementation */
#endif
#include "evgamepad.h"			/* Gamepad */

/* Xlib */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>

/* POSIX */
#include <sys/types.h>
#include <sys/stat.h>	/* stat(), mkdir() */
#include <sys/time.h>	/* gettimeofday() */
#include <unistd.h>	/* usleep(), access() */

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

/* Gstreamer Video HAL */
#include "gstplay.h"

/* Color Format */
#define DEPTH		(24)

/* Log File */
#define LOG_FILE	"log.txt"

/* Save Directory */
#define SAVE_DIR	"save"

/* Frame Time */
#define FRAME_MILLI	(16)	/* Millisec of a frame */
#define SLEEP_MILLI	(5)	/* Millisec to sleep */

/* Window Config */
static char *window_title;
static int screen_width;
static int screen_height;
static int viewport_width;
static int viewport_height;
static float mouse_scale = 1.0f;
static int mouse_ofs_x;
static int mouse_ofs_y;
static bool is_full_screen;
static int physical_width;
static int physical_height;
static int bpp;

/* X11 Objects */
static Display *display;
static int screen;
static XVisualInfo vi;
static Colormap colormap = BadValue;
static XColor palette[256];
static Window window = BadAlloc;
static Pixmap icon = BadAlloc;
static Pixmap icon_mask = BadAlloc;
static Atom delete_message = BadAlloc;
static XImage *ximage;

/* Image. */
static struct hal_image *back_image;
static uint8_t *low_bpp_pixels;

/* Frame Start Time */
static struct timeval tv_start;

/* Log File */
#define LOG_BUF_SIZE	(4096)
static FILE *log_fp;

/* Locale */
const char *playfield_lang_code;

/* Flag to indicate whether we are playing a video or not */
static bool is_gst_playing;

/* Flag to indicate whether a video is skippable or not */
static bool is_gst_skippable;

/* Icon */
extern char *icon_xpm[35];

/* Images. */

/* forward declaration */
static void init_locale(void);
static bool init_hal(int argc, char *argv[]);
static bool open_log_file(void);
static void close_log_file(void);
static bool init_x11_graphics(void);
static void cleanup_x11_graphics(void);
static bool open_display(void);
static bool create_window(void);
static bool set_window_title(void);
static bool show_window(void);
static void set_window_size(void);
static bool set_event_mask(void);
static void set_close_button(void);
static void close_display(void);
static bool setup_window(void);
static bool create_icon_image(void);
static void cleanup_hal(void);
static void destroy_window(void);
static void destroy_icon_image(void);
static void run_game_loop(void);
static bool run_frame(void);
static bool wait_for_next_frame(void);
static bool next_event(void);
static void event_key_press(XEvent *event);
static void event_key_release(XEvent *event);
static int get_key_code(XEvent *event);
static void event_button_press(XEvent *event);
static void event_button_release(XEvent *event);
static void event_motion_notify(XEvent *event);
static void event_resize(XEvent *event);
static void update_viewport_size(int width, int height);
static Bool want_configure(Display* d, XEvent* ev, XPointer arg);

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
		hal_log_warn("Can't initialize sound.\n");
	}

	/* Open an X11 display. */
	if (!open_display()) {
		hal_log_error("Can't open display.\n");
		return false;
	}

	/* Initialze X11 window. */
	if (!init_x11_graphics()) {
		hal_log_error("Failed to initialize graphics.");
		return false;
	}

	/* Setup the window. */
	if (!setup_window()) {
		hal_log_error("Failed to setup a window.");
		return false;
	}

	/* Create an icon. */
	if (!create_icon_image()) {
		hal_log_error("Can't create icon.\n");
		return false;
	}

	/* Initialize the gamepad. */
	init_evgamepad();

	/* Initialize the viddeo HAL. */
	gstplay_init(argc, argv);

	return true;
}

/* Open an X11 display. */
static bool
open_display(void)
{
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		hal_log_error("XOpenDisplay() failed.");
		return false;
	}
	return true;
}

bool init_x11_graphics(void)
{
	Window root;
	hal_pixel_t *pixels;
	int screen;
	int i;

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	if (XMatchVisualInfo(display, screen, 32, TrueColor, &vi)) {
		/* 32-bpp, 24-bit depth, 8-bit ignored padding. */
		bpp = 32;

		/* Allocate an image buffer which may be freed by XDestroyImage(). */
		if (posix_memalign((void **)&pixels, 64, (size_t)(screen_width * screen_height * 4)) != 0)
			return false;

		/* Create a back image. */
		if (!hal_create_image_with_pixels(screen_width, screen_height, pixels, &back_image)) {
			free(pixels);
			return false;
		}

		/* Create an XImage object that holds the back image buffer. */
		ximage = XCreateImage(display,
				      vi.visual,
				      32,
				      ZPixmap,
				      0,
				      (char *)pixels,
				      (unsigned int)screen_width,
				      (unsigned int)screen_height,
				      32,
				      screen_width * 4);
		if (ximage == NULL) {
			hal_destroy_image(back_image);
			back_image = NULL;
			free(pixels);
			return false;
		}

		colormap = XCreateColormap(display, root, vi.visual, AllocNone);
		if (colormap == None) {
			hal_log_error("XCreateColormap() failed.");
			return false;
		}
	} else if (XMatchVisualInfo(display, screen, 24, TrueColor, &vi)) {
		/* 24-bpp, 24-bit depth, no padding. */
		bpp = 24;

		/* Allocate an image buffer which may be freed by XDestroyImage(). */
		if (posix_memalign((void **)&pixels, 64, (size_t)(screen_width * screen_height * 4)) != 0)
			return false;

		/* Create a back image. */
		if (!hal_create_image_with_pixels(screen_width, screen_height, pixels, &back_image)) {
			free(pixels);
			return false;
		}

		/* Create an XImage object that holds the back image buffer. */
		ximage = XCreateImage(display,
				      vi.visual,
				      24,
				      ZPixmap,
				      0,
				      (char *)pixels,
				      (unsigned int)screen_width,
				      (unsigned int)screen_height,
				      32,
				      screen_width * 4);
		if (ximage == NULL) {
			hal_destroy_image(back_image);
			back_image = NULL;
			free(pixels);
			return false;
		}

		colormap = XCreateColormap(display, root, vi.visual, AllocNone);
		if (colormap == None) {
			hal_log_error("XCreateColormap() failed.");
			return false;
		}
	} else if (XMatchVisualInfo(display, screen, 16, TrueColor, &vi)) {
		/* 16-bpp, 16-bit depth, no 1-bit padding. */
		bpp = 16;

		/* Create a back image. */
		if (!hal_create_image(screen_width, screen_height, &back_image)) {
			free(pixels);
			return false;
		}

		/* Allocate an image buffer which may be freed by XDestroyImage(). */
		if (posix_memalign((void **)&low_bpp_pixels, 64, (size_t)(screen_width * screen_height * 2)) != 0)
			return false;

		/* Create an image. */
		ximage = XCreateImage(display,
				      vi.visual,
				      16,
				      ZPixmap,
				      0,
				      (char *)low_bpp_pixels,
				      (unsigned int)screen_width,
				      (unsigned int)screen_height,
				      8,
				      screen_width * 2);

		colormap = XCreateColormap(display, root, vi.visual, AllocNone);
		if (colormap == None) {
			hal_log_error("XCreateColormap() failed.");
			return false;
		}
	} else if (XMatchVisualInfo(display, screen, 8, PseudoColor, &vi)) {
		/* 8-bpp, 256-color palette. */
		bpp = 8;

		/* Create a back image. */
		if (!hal_create_image(screen_width, screen_height, &back_image)) {
			return false;
		}

		/* Allocate an image buffer which may be freed by XDestroyImage(). */
		if (posix_memalign((void **)&low_bpp_pixels, 64, (size_t)(screen_width * screen_height * 1)) != 0)
			return false;

		/* Create an image. */
		ximage = XCreateImage(display,
				      vi.visual,
				      8,
				      ZPixmap,
				      0,
				      (char *)low_bpp_pixels,
				      (unsigned int)screen_width,
				      (unsigned int)screen_height,
				      8,
				      screen_width * 1);

		/* Create a private colormap with fixed 256-color palette. */
		colormap = XCreateColormap(display, RootWindow(display, screen), vi.visual, AllocAll);
		if (colormap == None) {
			hal_destroy_image(back_image);
			back_image = NULL;
			return false;
		}

		/* Fill the color palette. */
		for (i = 0; i < 256; i++) {
			int r3 = (i >> 5) & 0x07;
			int g3 = (i >> 2) & 0x07;
			int b2 = i & 0x03;

			/* expand to 16-bit XColor space */
			palette[i].pixel = (unsigned long)i;
			palette[i].red   = (unsigned short)((r3 * 65535) / 7);
			palette[i].green = (unsigned short)((g3 * 65535) / 7);
			palette[i].blue  = (unsigned short)((b2 * 65535) / 3);
			palette[i].flags = DoRed | DoGreen | DoBlue;
		}

		/* Apply the color palette. */
		XStoreColors(display, colormap, palette, 256);
	} else {
		hal_log_error("No X11 visual matched.");
		return false;
	}

	/* Create a window. */
	if (!create_window())
		return false;

	return true;
}

#if 0
static bool create_window(void)
{
	Window root;
	unsigned long black, white;
	int screen;

	/* Get display information. */
	screen = DefaultScreen(display);
	root  = RootWindow(display, screen);
	black = BlackPixel(display, screen);
	white = WhitePixel(display, screen);

	/* Create a window. */
	window = XCreateSimpleWindow(display, root, 0, 0,
				     (unsigned int)screen_width,
				     (unsigned int)screen_height,
				     1, black, white);
	if (window == BadAlloc || window == BadMatch || window == BadValue || window == BadWindow) {
		hal_log_error("XCreateSimpleWindow");
		return false;
	}

	return true;
}
#endif

static bool create_window(void)
{
	Window root;
	XSetWindowAttributes attrs;
	unsigned long mask;

	/* Get display information. */
	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	memset(&attrs, 0, sizeof(attrs));
	attrs.background_pixel = BlackPixel(display, screen);
	attrs.border_pixel = BlackPixel(display, screen);
	attrs.event_mask = KeyPressMask |
		           ExposureMask |
		           ButtonPressMask |
		           ButtonReleaseMask |
 		           KeyReleaseMask |
		           PointerMotionMask |
		           StructureNotifyMask;
	attrs.colormap = colormap;
	mask = CWBackPixel | CWBorderPixel | CWEventMask |CWColormap;

	window = XCreateWindow(display,
			       root,
			       0, 0,
			       (unsigned int)screen_width,
			       (unsigned int)screen_height,
			       0,
			       vi.depth,
			       InputOutput,
			       vi.visual,
			       mask,
			       &attrs);
	if (window == 0) {
		hal_log_error("XCreateWindow() failed.");
		return false;
	}

	return true;
}


void cleanup_x11_graphics(void)
{
	if (ximage != NULL) {
		XDestroyImage(ximage);
		ximage = NULL;
	}

	if (back_image != NULL) {
		hal_destroy_image(back_image);
		back_image = NULL;
	}
}

/* Setup the window. */
static bool
setup_window(void)
{
	/* Set the window title. */
	if (!set_window_title())
		return false;

	/* Show the window. */
	if (!show_window())
		return false;

	/* Set the fixed window size. */
	set_window_size();

	/* Set the event mask. */
	if (!set_event_mask())
		return false;

	/* Capture close button events if possible. */
	set_close_button();

	return true;
}

/* Set the window title. */
static bool
set_window_title(void)
{
	XTextProperty tp;
	int ret;

	ret = XmbTextListToTextProperty(display, &window_title, 1, XCompoundTextStyle, &tp);
	if (ret == XNoMemory || ret == XLocaleNotSupported) {
		hal_log_error("XmbTextListToTextProperty() failed.");
		return false;
	}
	XSetWMName(display, window, &tp);
	XFree(tp.value);

	return true;
}

/* Show the window. */
static bool
show_window(void)
{
	int ret;

	ret = XMapWindow(display, window);
	if (ret == BadWindow) {
		hal_log_error("XMapWindow() failed.");
		return false;
	}

	return true;
}

/* Set the fixed window size. */
static void
set_window_size(void)
{
#if 0
	XSizeHints *sh;

	sh = XAllocSizeHints();
	sh->flags = PMinSize | PMaxSize;
	sh->min_width = screen_width;
	sh->min_height = screen_height;
	sh->max_width = screen_width;
	sh->max_height = screen_height;
	XSetWMSizeHints(display, window, sh, XA_WM_NORMAL_HINTS);
	XFree(sh);
#endif
}

/* Set the event mask. */
static bool
set_event_mask(void)
{
	int ret;

	ret = XSelectInput(display,
			   window,
			   KeyPressMask |
			   ExposureMask |
			   ButtonPressMask |
			   ButtonReleaseMask |
			   KeyReleaseMask |
			   PointerMotionMask |
			   StructureNotifyMask);
	if (ret == BadWindow) {
		hal_log_error("XSelectInput() failed.");
		return false;
	}

	return true;
}

/* Capture close button events if possible. */
static void
set_close_button(void)
{
	delete_message = XInternAtom(display, "WM_DELETE_WINDOW", True);
	if (delete_message != None && delete_message != BadAlloc && delete_message != BadValue)
		XSetWMProtocols(display, window, &delete_message, 1);
}

/* Create an icon image. */
static bool
create_icon_image(void)
{
	XWMHints *win_hints;
	XpmAttributes attr;
	bool is_colormap_created;
	int ret;

	/* Create a color map. */
	is_colormap_created = false;
	if (colormap != BadValue) {
		colormap = XCreateColormap(display, window,
					   DefaultVisual(display, DefaultScreen(display)),
					   AllocNone);
		if (colormap == BadAlloc ||
		    colormap == BadMatch ||
		    colormap == BadValue ||
		    colormap == BadWindow) {
			hal_log_error("XCreateColorMap() failed.");
			return false;
		}
		is_colormap_created = true;
	}

	/* Create a pixmap. */
	attr.valuemask = XpmColormap;
	attr.colormap = colormap;
	ret = XpmCreatePixmapFromData(display, window, icon_xpm, &icon, &icon_mask, &attr);
	if (ret != XpmSuccess) {
		hal_log_error("XpmCreatePixmapFromData() failed.");
		if (is_colormap_created) {
			XFreeColormap(display, colormap);
			colormap = BadValue;
		}
		return false;
	}

	/* Allocate for a WMHints. */
	win_hints = XAllocWMHints();
	if (!win_hints) {
		if (is_colormap_created) {
			XFreeColormap(display, colormap);
			colormap = BadValue;
		}
		return false;
	}

	/* Set the icon. */
	win_hints->flags = IconPixmapHint | IconMaskHint;
	win_hints->icon_pixmap = icon;
	win_hints->icon_mask = icon_mask;
	XSetWMHints(display, window, win_hints);

	/* Free the temporary objects. */
	XFree(win_hints);
	if (is_colormap_created) {
		XFreeColormap(display, colormap);
		colormap = BadValue;
	}
	return true;
}

/* Cleanup the subsystems. */
static void
cleanup_hal(void)
{
	/* Cleanup sound. */
	cleanup_sound();

	/* Destroy the window. */
	destroy_window();

	/* Destroy the icon. */
	destroy_icon_image();

	/* Close the display. */
	close_display();

	/* Close the log file. */
	close_log_file();
}

/* Destroy the window. */
static void
destroy_window(void)
{
	if (display != NULL) {
		if (window != BadAlloc)
			XDestroyWindow(display, window);
	}
}

/* Destroy the icon image. */
static void
destroy_icon_image(void)
{
	if (display != NULL) {
		if (icon != BadAlloc)
			XFreePixmap(display, icon);
		if (icon_mask != BadAlloc)
			XFreePixmap(display, icon_mask);
	}
}

/* Close the X11 display. */
static void
close_display(void)
{
	if (display != NULL)
		XCloseDisplay(display);
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
		/* Process video playback. */
		if (is_gst_playing) {
			gstplay_loop_iteration();
			if (!gstplay_is_playing()) {
				gstplay_stop();
				is_gst_playing = false;
			}
		}

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
	GC gc;
	bool cont;

	/* Read the gamepad. */
	update_evgamepad();

	/* Clear the back image. */
	hal_clear_image(back_image, 0);

	/* Call a frame event. */
	cont = hal_callback_on_event_frame();

	/* Quantize the back image if bpp != 32. */
	if (bpp == 8) {
		int x, y;
		hal_pixel_t *src = (hal_pixel_t *)back_image->pixels;
		for (y = 0; y < screen_height; y++) {
			uint8_t *dst_row = low_bpp_pixels + y * ximage->bytes_per_line;
			hal_pixel_t *src_row = src + y * screen_width;
			for (x = 0; x < screen_width; x++) {
				uint8_t r = hal_get_pixel_r(src_row[x]);
				uint8_t g = hal_get_pixel_g(src_row[x]);
				uint8_t b = hal_get_pixel_b(src_row[x]);
				dst_row[x] = (uint8_t)((r & 0xe0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6));
			}
		}
	} else if (bpp == 16) {
		int x, y;
		hal_pixel_t *src = (hal_pixel_t *)back_image->pixels;
		for (y = 0; y < screen_height; y++) {
			uint16_t *dst_row = (uint16_t *)(low_bpp_pixels + y * ximage->bytes_per_line);
			hal_pixel_t *src_row = src + y * screen_width;
			for (x = 0; x < screen_width; x++) {
				uint8_t r = hal_get_pixel_r(src_row[x]);
				uint8_t g = hal_get_pixel_g(src_row[x]);
				uint8_t b = hal_get_pixel_b(src_row[x]);
				dst_row[x] = (uint16_t)(((uint16_t)(r & 0xf8) << 8) |	/* RGB565 */
							((uint16_t)(g & 0xfc) << 3) |
							((uint16_t)(b & 0xf8) >> 3));
			}
		}
	}
#if 0
	else if (bpp == 24) {
		int x, y;
		hal_pixel_t *src = (hal_pixel_t *)back_image->pixels;
		for (y = 0; y < screen_height; y++) {
			uint8_t *dst_row = (uint8_t *)ximage->data + y * ximage->bytes_per_line;
			hal_pixel_t *src_row = src + y * screen_width;
			for (x = 0; x < screen_width; x++) {
				dst_row[x * 3 + 0] = hal_get_pixel_b(src_row[x]);
				dst_row[x * 3 + 1] = hal_get_pixel_g(src_row[x]);
				dst_row[x * 3 + 2] = hal_get_pixel_r(src_row[x]);
			}
		}
	}
#endif

	/* Transfer the bit block. */
	gc = XCreateGC(display, window, 0, 0);
	XPutImage(display, window, gc, ximage, 0, 0, 0, 0, (unsigned int)screen_width, (unsigned int)screen_height);
	XFreeGC(display, gc);

	return cont;
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
		/* Process events if exist. */
		while (XEventsQueued(display, QueuedAfterFlush) > 0)
			if (!next_event())
				return false;

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

/* Process an event. */
static bool
next_event(void)
{
	XEvent event;

	XNextEvent(display, &event);
	switch (event.type) {
	case KeyPress:
		event_key_press(&event);
		break;
	case KeyRelease:
		event_key_release(&event);
		break;
	case ButtonPress:
		event_button_press(&event);
		break;
	case ButtonRelease:
		event_button_release(&event);
		break;
	case MotionNotify:
		event_motion_notify(&event);
		break;
	case MappingNotify:
		XRefreshKeyboardMapping(&event.xmapping);
		break;
	case ConfigureNotify:
		event_resize(&event);
		break;
	case ClientMessage:
		/* Close button was pressed. */
		if ((Atom)event.xclient.data.l[0] == delete_message)
			return false;
		break;
	}
	return true;
}

/* Process a KeyPress event. */
static void
event_key_press(
	XEvent *event)
{
	int key;
	KeySym keysym;

	keysym = XLookupKeysym(&event->xkey, 0);
	if ((event->xkey.state & Mod1Mask) && (keysym == XK_Return)) {
		if (!is_full_screen)
			hal_enter_full_screen_mode();
		else
			hal_leave_full_screen_mode();
		return;
	}

	/* Get a key code. */
	key = get_key_code(event);
	if (key == -1)
		return;

	/* Call an event handler. */
	hal_callback_on_event_key_press(key);
}

/* Process a KeyRelease event. */
static void
event_key_release(
	XEvent *event)
{
	int key;

	/* Ignore auto repeat events. */
	if (XEventsQueued(display, QueuedAfterReading) > 0) {
		XEvent next;
		XPeekEvent(display, &next);
		if (next.type == KeyPress &&
		    next.xkey.keycode == event->xkey.keycode &&
		    next.xkey.time == event->xkey.time) {
			XNextEvent(display, &next);
			return;
		}
	}

	/* Get a key code. */
	key = get_key_code(event);
	if (key == -1)
		return;

	/* Call an event handler. */
	hal_callback_on_event_key_release(key);
}

/* Convert 'KeySym' to 'enum key_code'. */
static int
get_key_code(
	XEvent *event)
{
	char text[255];
	KeySym keysym;

	/* Get a KeySym. */
	XLookupString(&event->xkey, text, sizeof(text), &keysym, 0);

	/* Convert to key_code. */
	switch (keysym) {
	case XK_Escape:
		return HAL_KEY_ESCAPE;
	case XK_Return:
	case XK_KP_Enter:
		return HAL_KEY_RETURN;
	case XK_space:
		return HAL_KEY_SPACE;
	case XK_Tab:
		return HAL_KEY_TAB;
	case XK_BackSpace:
		return HAL_KEY_BACKSPACE;
	case XK_Delete:
		return HAL_KEY_DELETE;
	case XK_Home:
		return HAL_KEY_HOME;
	case XK_End:
		return HAL_KEY_END;
	case XK_Page_Up:
		return HAL_KEY_PAGEUP;
	case XK_Page_Down:
		return HAL_KEY_PAGEDOWN;
	case XK_Shift_L:
	case XK_Shift_R:
		return HAL_KEY_SHIFT;
	case XK_Control_L:
	case XK_Control_R:
		return HAL_KEY_CONTROL;
	case XK_Alt_L:
	case XK_Alt_R:
		return HAL_KEY_ALT;
	case XK_Down:
		return HAL_KEY_DOWN;
	case XK_Up:
		return HAL_KEY_UP;
	case XK_Left:
		return HAL_KEY_LEFT;
	case XK_Right:
		return HAL_KEY_RIGHT;
	case XK_A:
		return HAL_KEY_A;
	case XK_B:
		return HAL_KEY_B;
	case XK_C:
		return HAL_KEY_C;
	case XK_D:
		return HAL_KEY_D;
	case XK_E:
		return HAL_KEY_E;
	case XK_F:
		return HAL_KEY_F;
	case XK_G:
		return HAL_KEY_G;
	case XK_H:
		return HAL_KEY_H;
	case XK_I:
		return HAL_KEY_I;
	case XK_J:
		return HAL_KEY_J;
	case XK_K:
		return HAL_KEY_K;
	case XK_L:
		return HAL_KEY_L;
	case XK_M:
		return HAL_KEY_M;
	case XK_N:
		return HAL_KEY_N;
	case XK_O:
		return HAL_KEY_O;
	case XK_P:
		return HAL_KEY_P;
	case XK_Q:
		return HAL_KEY_Q;
	case XK_R:
		return HAL_KEY_R;
	case XK_S:
		return HAL_KEY_S;
	case XK_T:
		return HAL_KEY_T;
	case XK_U:
		return HAL_KEY_U;
	case XK_V:
		return HAL_KEY_V;
	case XK_W:
		return HAL_KEY_W;
	case XK_X:
		return HAL_KEY_X;
	case XK_Y:
		return HAL_KEY_Y;
	case XK_Z:
		return HAL_KEY_Z;
	case XK_1:
		return HAL_KEY_1;
	case XK_2:
		return HAL_KEY_2;
	case XK_3:
		return HAL_KEY_3;
	case XK_4:
		return HAL_KEY_4;
	case XK_5:
		return HAL_KEY_5;
	case XK_6:
		return HAL_KEY_6;
	case XK_7:
		return HAL_KEY_7;
	case XK_8:
		return HAL_KEY_8;
	case XK_9:
		return HAL_KEY_9;
	case XK_0:
		return HAL_KEY_0;
	case XK_F1:
		return HAL_KEY_F1;
	case XK_F2:
		return HAL_KEY_F2;
	case XK_F3:
		return HAL_KEY_F3;
	case XK_F4:
		return HAL_KEY_F4;
	case XK_F5:
		return HAL_KEY_F5;
	case XK_F6:
		return HAL_KEY_F6;
	case XK_F7:
		return HAL_KEY_F7;
	case XK_F8:
		return HAL_KEY_F8;
	case XK_F9:
		return HAL_KEY_F9;
	case XK_F10:
		return HAL_KEY_F10;
	case XK_F11:
		return HAL_KEY_F11;
	case XK_F12:
		return HAL_KEY_F12;
	default:
		break;
	}
	return -1;
}

/* Process a ButtonPress event. */
static void
event_button_press(
	XEvent *event)
{
	/* See the button type and dispatch. */
	switch (event->xbutton.button) {
	case Button1:
		hal_callback_on_event_mouse_press(
			HAL_MOUSE_LEFT,
			(int)((event->xbutton.x - mouse_ofs_x) * mouse_scale),
			(int)((event->xbutton.y - mouse_ofs_y) * mouse_scale));
		break;
	case Button3:
		hal_callback_on_event_mouse_press(
			HAL_MOUSE_RIGHT,
			(int)((event->xbutton.x - mouse_ofs_x) * mouse_scale),
			(int)((event->xbutton.y - mouse_ofs_y) * mouse_scale));
		break;
	case Button4:
		hal_callback_on_event_key_press(HAL_KEY_UP);
		hal_callback_on_event_key_release(HAL_KEY_UP);
		break;
	case Button5:
		hal_callback_on_event_key_press(HAL_KEY_DOWN);
		hal_callback_on_event_key_release(HAL_KEY_DOWN);
		break;
	default:
		break;
	}
}

/* Process a ButtonPress event. */
static void
event_button_release(
	XEvent *event)
{
	/* See the button type and dispatch. */
	switch (event->xbutton.button) {
	case Button1:
		hal_callback_on_event_mouse_release(
			HAL_MOUSE_LEFT,
			(int)(event->xbutton.x / mouse_scale),
			(int)(event->xbutton.y / mouse_scale));
		break;
	case Button3:
		hal_callback_on_event_mouse_release(
			HAL_MOUSE_RIGHT,
			(int)(event->xbutton.x / mouse_scale),
			(int)(event->xbutton.y / mouse_scale));
		break;
	}
}

/* Process a MotionNotify event. */
static void event_motion_notify(XEvent *event)
{
	/* Call an event handler. */
	hal_callback_on_event_mouse_move(
		(int)((event->xbutton.x - mouse_ofs_x) * mouse_scale),
		(int)((event->xbutton.y - mouse_ofs_y) * mouse_scale));
}

/* Process a ConfigureNotify event. */
static void
event_resize(
	XEvent *event)
{
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
hal_make_real_path(
	const char *fname)
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
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	*t = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;

#if 0
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*t = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/*
 * Get a timer lap.
 */
uint64_t
hal_get_lap_timer_millisec(
	uint64_t *t)
{
	struct timespec ts;
	uint64_t end;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	end = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
	return (end - *t);

#if 0
	struct timeval tv;
	uint64_t end;
	gettimeofday(&tv, NULL);
	end = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
	return (uint64_t)(end - *t);
#endif
}

/*
 * Notify an image update.
 */
void
hal_notify_image_update(
	struct hal_image *img)
{
}

/*
 * Notify an image free.
 */
void
hal_notify_image_free(
	struct hal_image *img)
{
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
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	if (dst_width != src_width ||
	    dst_height != src_height) {
		hal_draw_image_3d_alpha(back_image,
					dst_left,
					dst_top,
					dst_left + dst_width,
					dst_top,
					dst_left,
					dst_top + dst_height,
					dst_left + dst_width,
					dst_top + dst_height,
					src_image,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha);
	} else {
		hal_draw_image_alpha(back_image,
				     dst_left,
				     dst_top,
				     src_image,
				     src_width,
				     src_height,
				     src_left,
				     src_top,
				     alpha);
	}
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
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	if (dst_width != src_width ||
	    dst_height != src_height) {
		hal_draw_image_3d_add(back_image,
				      dst_left,
				      dst_top,
				      dst_left + dst_width,
				      dst_top,
				      dst_left,
				      dst_top + dst_height,
				      dst_left + dst_width,
				      dst_top + dst_height,
				      src_image,
				      src_left,
				      src_top,
				      src_width,
				      src_height,
				      alpha);
	} else {
		hal_draw_image_add(back_image,
				   dst_left,
				   dst_top,
				   src_image,
				   src_width,
				   src_height,
				   src_left,
				   src_top,
				   alpha);
	}
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
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	if (dst_width != src_width ||
	    dst_height != src_height) {
		hal_draw_image_3d_sub(back_image,
				      dst_left,
				      dst_top,
				      dst_left + dst_width,
				      dst_top,
				      dst_left,
				      dst_top + dst_height,
				      dst_left + dst_width,
				      dst_top + dst_height,
				      src_image,
				      src_left,
				      src_top,
				      src_width,
				      src_height,
				      alpha);
	} else {
		hal_draw_image_sub(back_image,
				   dst_left,
				   dst_top,
				   src_image,
				   src_width,
				   src_height,
				   src_left,
				   src_top,
				   alpha);
	}
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
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	if (dst_width != src_width ||
	    dst_height != src_height) {
		hal_draw_image_3d_dim(back_image,
				      dst_left,
				      dst_top,
				      dst_left + dst_width,
				      dst_top,
				      dst_left,
				      dst_top + dst_height,
				      dst_left + dst_width,
				      dst_top + dst_height,
				      src_image,
				      src_left,
				      src_top,
				      src_width,
				      src_height,
				      alpha);
	} else {
		hal_draw_image_dim(back_image,
				   dst_left,
				   dst_top,
				   src_image,
				   src_width,
				   src_height,
				   src_left,
				   src_top,
				   alpha);
	}
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
	hal_draw_image_rule(back_image, src_img, rule_img, threshold);
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
	hal_draw_image_melt(back_image, src_img, rule_img, progress);
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

/*
 * Render an image. (3d transform, add blending)
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

/*
 * Play a video.
 */
bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	char *path;

	path = hal_make_real_path(fname);

	is_gst_playing = true;
	is_gst_skippable = is_skippable;

	gstplay_play(path, window);

	free(path);

	return true;
}

/*
 * Stop the video.
 */
void
hal_stop_video(void)
{
	gstplay_stop();

	is_gst_playing = false;
}

/*
 * Check whether a video is playing.
 */
bool
hal_is_video_playing(void)
{
	return is_gst_playing;
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
	Atom wm_state;
	Atom fs_atom;
	XEvent xev = {0};

	if (is_full_screen)
		return;

	wm_state = XInternAtom(display, "_NET_WM_STATE", False);
	fs_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

	xev.type = ClientMessage;
	xev.xclient.window = window;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = fs_atom;
	xev.xclient.data.l[2] = 0; // no second property
	xev.xclient.data.l[3] = 1;

	XSendEvent(display,
		   DefaultRootWindow(display),
		   False,
		   SubstructureRedirectMask | SubstructureNotifyMask,
		   &xev);

	is_full_screen = true;
}

/*
 * Leave full screen mode.
 */
void
hal_leave_full_screen_mode(void)
{
	Atom wm_state;
	Atom fs_atom;
	XEvent xev = {0};

	if (!is_full_screen)
		return;

	wm_state = XInternAtom(display, "_NET_WM_STATE", False);
	fs_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

	xev.type = ClientMessage;
	xev.xclient.window = window;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 0;
	xev.xclient.data.l[1] = fs_atom;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 1;

	XSendEvent(display,
		   DefaultRootWindow(display),
		   False,
		   SubstructureRedirectMask | SubstructureNotifyMask,
		   &xev);

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
