/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Linux X11 OpenGL
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
#include "evgamepad.h"			/* evdev Gamepad */

/* Xlib */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>

/* OpenGL */
#include <GL/gl.h>			/* OpenGL */
#include <GL/glx.h>			/* GLX */
#include "glrender.h"			/* OpenGL Compatibility Helper */

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
#define BPP		(32)

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

/* X11 Objects */
Display *display;
Window window = BadAlloc;
static int screen;
static Pixmap icon = BadAlloc;
static Pixmap icon_mask = BadAlloc;
static Atom delete_message = BadAlloc;

/* GLX Objects */
static GLXWindow glx_window = None;
static GLXContext glx_context = None;

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

/* OpenGL 3.0 API */
GLuint (APIENTRY *glCreateShader)(GLenum type);
void (APIENTRY *glShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
void (APIENTRY *glCompileShader)(GLuint shader);
void (APIENTRY *glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
void (APIENTRY *glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void (APIENTRY *glAttachShader)(GLuint program, GLuint shader);
void (APIENTRY *glLinkProgram)(GLuint program);
void (APIENTRY *glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
void (APIENTRY *glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLuint (APIENTRY *glCreateProgram)(void);
void (APIENTRY *glUseProgram)(GLuint program);
void (APIENTRY *glGenVertexArrays)(GLsizei n, GLuint *arrays);
void (APIENTRY *glBindVertexArray)(GLuint array);
void (APIENTRY *glGenBuffers)(GLsizei n, GLuint *buffers);
void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
GLint (APIENTRY *glGetAttribLocation)(GLuint program, const GLchar *name);
void (APIENTRY *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
void (APIENTRY *glEnableVertexAttribArray)(GLuint index);
GLint (APIENTRY *glGetUniformLocation)(GLuint program, const GLchar *name);
void (APIENTRY *glUniform1i)(GLint location, GLint v0);
void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void (APIENTRY *glDeleteShader)(GLuint shader);
void (APIENTRY *glDeleteProgram)(GLuint program);
void (APIENTRY *glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint *buffers);
struct API
{
	void **func;
	const char *name;
};
static struct API api[] =
{
	{(void **)&glCreateShader, "glCreateShader"},
	{(void **)&glShaderSource, "glShaderSource"},
	{(void **)&glCompileShader, "glCompileShader"},
	{(void **)&glGetShaderiv, "glGetShaderiv"},
	{(void **)&glGetShaderInfoLog, "glGetShaderInfoLog"},
	{(void **)&glAttachShader, "glAttachShader"},
	{(void **)&glLinkProgram, "glLinkProgram"},
	{(void **)&glGetProgramiv, "glGetProgramiv"},
	{(void **)&glGetProgramInfoLog, "glGetProgramInfoLog"},
	{(void **)&glCreateProgram, "glCreateProgram"},
	{(void **)&glUseProgram, "glUseProgram"},
	{(void **)&glGenVertexArrays, "glGenVertexArrays"},
	{(void **)&glBindVertexArray, "glBindVertexArray"},
	{(void **)&glGenBuffers, "glGenBuffers"},
	{(void **)&glBindBuffer, "glBindBuffer"},
	{(void **)&glGetAttribLocation, "glGetAttribLocation"},
	{(void **)&glVertexAttribPointer, "glVertexAttribPointer"},
	{(void **)&glEnableVertexAttribArray, "glEnableVertexAttribArray"},
	{(void **)&glGetUniformLocation, "glGetUniformLocation"},
	{(void **)&glUniform1i, "glUniform1i"},
	{(void **)&glBufferData, "glBufferData"},
	{(void **)&glDeleteShader, "glDeleteShader"},
	{(void **)&glDeleteProgram, "glDeleteProgram"},
	{(void **)&glDeleteVertexArrays, "glDeleteVertexArrays"},
	{(void **)&glDeleteBuffers, "glDeleteBuffers"},
};

/* forward declaration */
static void init_locale(void);
static bool init_hal(int argc, char *argv[]);
static bool open_log_file(void);
static void close_log_file(void);
static bool open_display(void);
static bool create_glx_window(void);
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

	/* Create a GLX window. */
	if (!create_glx_window()) {
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

/* Create a GLX window. */
static bool
create_glx_window(void)
{
	int pix_attr[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		None
	};
	int ctx_attr[]= {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_FLAGS_ARB, 0,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};
	GLXContext (*glXCreateContextAttribsARB)(Display *dpy,
						 GLXFBConfig config,
						 GLXContext share_context,
						 Bool direct,
						 const int *attrib_list);
	GLXFBConfig *config;
	XVisualInfo *vi;
	XSetWindowAttributes swa;
	XEvent event;
	int i, n;

	viewport_width = screen_width;
	viewport_height = screen_height;

	/* Get the physical display size. */
	screen = DefaultScreen(display);
	physical_width = DisplayWidth(display, screen);
	physical_height = DisplayHeight(display, screen);

	/* Choose a framebuffer format. */
	config = glXChooseFBConfig(display, screen, pix_attr, &n);
	if (config == NULL)
		return false;
	vi = glXGetVisualFromFBConfig(display, config[0]);

	/* Create a window. */
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	swa.colormap = XCreateColormap(display,
				       RootWindow(display, vi->screen),
				       vi->visual,
				       AllocNone);
	window = XCreateWindow(display,
			       RootWindow(display, vi->screen),
			       0,
			       0,
			       (unsigned int)screen_width,
			       (unsigned int)screen_height,
			       0,
			       vi->depth,
			       InputOutput,
			       vi->visual,
			       CWBorderPixel | CWColormap | CWEventMask,
			       &swa);
	XFree(vi);

	/* Create a GLX context. */
	glXCreateContextAttribsARB = (void *)glXGetProcAddress((const unsigned char *)"glXCreateContextAttribsARB");
	if (glXCreateContextAttribsARB == NULL) {
		XDestroyWindow(display, window);
		return false;
	}
	glx_context = glXCreateContextAttribsARB(display, config[0], 0, True, ctx_attr);
	if (glx_context == NULL) {
		XDestroyWindow(display, window);
		return false;
	}

	/* Create a GLX window. */
	glx_window = glXCreateWindow(display, config[0], window, NULL);
	XFree(config);

	/* Map the window to the screen, and wait for showing. */
	XMapWindow(display, window);
	XNextEvent(display, &event);

	/* Bind the GLX context to the window. */
	glXMakeContextCurrent(display, glx_window, glx_window, glx_context);

	/* Get the API pointers. */
	for (i = 0; i < (int)(sizeof(api)/sizeof(struct API)); i++) {
		*api[i].func = (void *)glXGetProcAddress((const unsigned char *)api[i].name);
		if(*api[i].func == NULL) {
			hal_log_info("Failed to get %s", api[i].name);
			glXMakeContextCurrent(display, None, None, None);
			glXDestroyContext(display, glx_context);
			glXDestroyWindow(display, glx_window);
			glx_context = None;
			glx_window = None;
			return false;
		}
	}

	/* Initialize the OpenGL rendering subsystem. */
	if (!init_opengl(screen_width, screen_height)) {
		glXMakeContextCurrent(display, None, None, None);
		glXDestroyContext(display, glx_context);
		glXDestroyWindow(display, glx_window);
		glx_context = None;
		glx_window = None;
		return false;
	}

	return true;
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
	Colormap cm;
	int ret;

	/* Create a color map. */
	cm = XCreateColormap(display, window,
			     DefaultVisual(display, DefaultScreen(display)),
			     AllocNone);
	if (cm == BadAlloc || cm == BadMatch || cm == BadValue ||
	    cm == BadWindow) {
		hal_log_error("XCreateColorMap() failed.");
		return false;
	}

	/* Create a pixmap. */
	attr.valuemask = XpmColormap;
	attr.colormap = cm;
	ret = XpmCreatePixmapFromData(display, window, icon_xpm, &icon, &icon_mask, &attr);
	if (ret != XpmSuccess) {
		hal_log_error("XpmCreatePixmapFromData() failed.");
		XFreeColormap(display, cm);
		return false;
	}

	/* Allocate for a WMHints. */
	win_hints = XAllocWMHints();
	if (!win_hints) {
		XFreeColormap(display, cm);
		return false;
	}

	/* Set the icon. */
	win_hints->flags = IconPixmapHint | IconMaskHint;
	win_hints->icon_pixmap = icon;
	win_hints->icon_mask = icon_mask;
	XSetWMHints(display, window, win_hints);

	/* Free the temporary objects. */
	XFree(win_hints);
	XFreeColormap(display,cm);
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
	cleanup_opengl();

	glXMakeContextCurrent(display, None, None, None);

	if (glx_context != None) {
		glXDestroyContext(display, glx_context);
		glx_context = None;
	}

	if (glx_window != None) {
		glXDestroyWindow(display, glx_window);
		glx_window = None;
	}

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
	bool cont;

	/* Read the gamepad. */
	update_evgamepad();

	/* Start rendering. */
	if (!is_gst_playing)
		opengl_start_rendering();

	/* Call a frame event. */
	cont = hal_callback_on_event_frame();

	/* End rendering. */
	if (!is_gst_playing) {
		opengl_end_rendering();
		glXSwapBuffers(display, glx_window);
	}

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
	update_viewport_size(event->xconfigure.width, event->xconfigure.height);
}

/*
 * Sets the window size.
 */
void update_viewport_size(
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
	mouse_ofs_x = orig_x;
	mouse_ofs_y = orig_y;

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
