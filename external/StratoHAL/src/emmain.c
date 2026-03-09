/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for Emscripten
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

/* Base */
#include "stratohal/platform.h"

/* HAL */
#include "stdfile.h"	/* File */
#include "glrender.h"	/* Graphics */
#include "alsound.h"	/* Sound */

/* Emscripten */
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

/* POSIX */
#include <sys/types.h>
#include <sys/stat.h>	/* stat(), mkdir() */
#include <sys/time.h>	/* gettimeofday() */

/* C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/*
 * Touch Y Coordinate
 */
static int touch_start_x;
static int touch_start_y;
static int touch_last_y;

/* Is continuous swipe enabled? */
static bool is_continuous_swipe_enabled;

/* Is full screen mode? */
static bool is_full_screen;

/* Window configuration. */
static char *window_title;
static int screen_width;
static int screen_height;

/* Locale */
static const char *lang_code;

/*
 * Forward Declaration
 */
static EM_BOOL loop_iter(double time, void *userData);
static EM_BOOL cb_mousemove(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
static EM_BOOL cb_mousedown(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
static EM_BOOL cb_mouseup(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
static EM_BOOL cb_wheel(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData);
static EM_BOOL cb_keydown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
static EM_BOOL cb_keyup(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
static int get_keycode(const char *key);
static EM_BOOL cb_touchstart(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData);
static EM_BOOL cb_touchmove(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData);
static EM_BOOL cb_touchend(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData);
static EM_BOOL cb_touchcancel(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData);
static void init_lang_code(void);

/*
 * main()
 */
int
main(void)
{
	/* Initialize the locale. */
	init_lang_code();

	/* Initialize the file HAL. */
	if(!init_file())
		return 1;

	/* Do a boot callback. */
	if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
		return 1;

	/* Mount IDBFS for save data. */
	EM_ASM_({
		FS.mkdir(Module.UTF8ToString($0));
		FS.mount(IDBFS, {}, Module.UTF8ToString($0));
		FS.syncfs(true, function (err) { Module.ccall('main_continue', 'v'); });
	}, window_title);

	/* Continue to main_continue() (the above call is asynchronous) */
	emscripten_exit_with_live_runtime();
}

/*
 * Main (continued)
 */
EMSCRIPTEN_KEEPALIVE
void
main_continue(void)
{
	/* Initialize the sound HAL. */
	if (!init_openal())
		return;

	/* Initialize the window title. */
	EM_ASM_({
		document.title = Module.UTF8ToString($0);
	}, window_title);

	/* Set a canvas size. */
	emscripten_set_canvas_element_size("canvas", screen_width, screen_height);
	EM_ASM_({ onResizeWindow(); });

	/* Initialize WebGL. */
	EmscriptenWebGLContextAttributes attr;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
	emscripten_webgl_init_context_attributes(&attr);
	attr.majorVersion = 2;
	attr.minorVersion = 0;
	context = emscripten_webgl_create_context("canvas", &attr);
	emscripten_webgl_make_context_current(context);
	if (!init_opengl(screen_width, screen_height))
		return;

	/* Do a start callback. */
	if (!hal_callback_on_event_start())
		return;

	/* Register input device events. */
	emscripten_set_mousedown_callback("canvas", 0, true, cb_mousedown);
	emscripten_set_mouseup_callback("canvas", 0, true, cb_mouseup);
	emscripten_set_mousemove_callback("canvas", 0, true, cb_mousemove);
	emscripten_set_wheel_callback("canvas", 0, true, cb_wheel);
	emscripten_set_keydown_callback("canvas", 0, true, cb_keydown);
	emscripten_set_keyup_callback("canvas", 0, true, cb_keyup);
	emscripten_set_touchstart_callback("canvas", 0, true, cb_touchstart);
	emscripten_set_touchmove_callback("canvas", 0, true, cb_touchmove);
	emscripten_set_touchend_callback("canvas", 0, true, cb_touchend);
	emscripten_set_touchcancel_callback("canvas", 0, true, cb_touchcancel);

	/* Register other handlers. */
	EM_ASM_({
		window.addEventListener('resize', onResizeWindow);
		document.addEventListener('visibilitychange', function () {
			if(document.visibilityState === 'visible') {
				Module.ccall('setVisible');
				document.getElementById('canvas').focus();
			} else if(document.visibilityState === 'hidden') {
				Module.ccall('setHidden');
			}
		});
		document.getElementById('canvas').addEventListener('mouseleave', function () {
			Module.ccall('mouseLeave');
		});
	});

	/* Start animation. */
	emscripten_request_animation_frame_loop(loop_iter, 0);
}

EM_JS(void, onResizeWindow, (void),
{
	var canvas = document.getElementById('canvas');
	var cw = canvas.width;
	var ch = canvas.height;
	var aspect = cw / ch;
	var winw = window.innerWidth;
	var winh = window.innerHeight;
	var w = winw;
	var h = winw / aspect;
	if(h > winh) {
		h = winh;
		w = winh * aspect;
	}
	canvas.style.width = w + 'px';
	canvas.style.height = h + 'px';
	canvas.focus();
});

/* Process a frame. */
static EM_BOOL
loop_iter(
	double time,
	void *userData)
{
	static bool stop = false;

	/* If already stopped. */
	if (stop)
		return EM_FALSE;

	/* Fill sound buffer. */
	fill_sound_buffer();

	/* Start a frame rendering. */
	opengl_start_rendering();

	/* Call a frame callback. */
	if (!hal_callback_on_event_frame()) {
		/* Exit. */
		stop = true;
		EM_FALSE;
	}

	/* End a frame rendering. */
	opengl_end_rendering();

	return EM_TRUE;
}

/* mousemove callback */
static EM_BOOL
cb_mousemove(
	int eventType,
	const EmscriptenMouseEvent *mouseEvent,
	void *userData)
{
	double w, h, scale_x, scale_y;
	int x, y;

	/* Get a CSS size of canvs. */
	emscripten_get_element_css_size("canvas", &w, &h);

	/* Scale a mouse position. */
	scale_x = w / (double)screen_width;
	scale_y = h / (double)screen_height;
	x = (int)((double)mouseEvent->targetX / scale_x);
	y = (int)((double)mouseEvent->targetY / scale_y);

	hal_callback_on_event_mouse_move(x, y);
	return EM_TRUE;
}

/* mousedown callback */
static EM_BOOL
cb_mousedown(
	int eventType,
	const EmscriptenMouseEvent *mouseEvent,
	void *userData)
{
	double w, h, scale_x, scale_y;
	int x, y, button;

	/* Scale a mouse position. */
	emscripten_get_element_css_size("canvas", &w, &h);
	scale_x = w / (double)screen_width;
	scale_y = h / (double)screen_height;
	x = (int)((double)mouseEvent->targetX / scale_x);
	y = (int)((double)mouseEvent->targetY / scale_y);

	if (mouseEvent->button == 0)
		button = HAL_MOUSE_LEFT;
	else
		button = HAL_MOUSE_RIGHT;

	hal_callback_on_event_mouse_press(button, x, y);
	return EM_TRUE;
}

/* mouseup callback */
static EM_BOOL
cb_mouseup(
	int eventType,
	const EmscriptenMouseEvent *mouseEvent,
	void *userData)
{
	double w, h, scale_x, scale_y;
	int x, y, button;

	/* Scale a mouse position. */
	emscripten_get_element_css_size("canvas", &w, &h);
	scale_x = w / (double)screen_width;
	scale_y = h / (double)screen_height;
	x = (int)((double)mouseEvent->targetX / scale_x);
	y = (int)((double)mouseEvent->targetY / scale_y);

	if (mouseEvent->button == 0)
		button = HAL_MOUSE_LEFT;
	else
		button = HAL_MOUSE_RIGHT;

	hal_callback_on_event_mouse_release(button, x, y);
	return EM_TRUE;
}

/* wheel callback */
static EM_BOOL
cb_wheel(
	int eventType,
	const EmscriptenWheelEvent *wheelEvent,
	void *userData)
{
	if (wheelEvent->deltaY > 0) {
		hal_callback_on_event_key_press(HAL_KEY_DOWN);
		hal_callback_on_event_key_release(HAL_KEY_DOWN);
	} else {
		hal_callback_on_event_key_press(HAL_KEY_UP);
		hal_callback_on_event_key_release(HAL_KEY_UP);
	}
	return EM_TRUE;
}

/* keydown callback */
static EM_BOOL
cb_keydown(int eventType,
	   const EmscriptenKeyboardEvent *keyEvent,
	   void *userData)
{
	int keycode;

	keycode = get_keycode(keyEvent->key);
	if (keycode == -1)
		return EM_TRUE;

	hal_callback_on_event_key_press(keycode);
	return EM_TRUE;
}

/* keyup callback */
static EM_BOOL
cb_keyup(int eventType,
	 const EmscriptenKeyboardEvent *keyEvent,
	 void *userData)
{
	int keycode;

	keycode = get_keycode(keyEvent->key);
	if (keycode == -1)
		return EM_TRUE;

	hal_callback_on_event_key_release(keycode);
	return EM_TRUE;
}

/* Get a key code. */
static int
get_keycode(
	const char *key)
{
	if (strcmp(key, "Escape") == 0) {
		return HAL_KEY_ESCAPE;
	} else if (strcmp(key, "Enter") == 0) {
		return HAL_KEY_RETURN;
	} else if (strcmp(key, " ") == 0) {
		return HAL_KEY_SPACE;
	} else if (strcmp(key, "Tab") == 0) {
		return HAL_KEY_TAB;
	} else if (strcmp(key, "Backspace") == 0) {
		return HAL_KEY_BACKSPACE;
	} else if (strcmp(key, "Delete") == 0) {
		return HAL_KEY_DELETE;
	} else if (strcmp(key, "Home") == 0) {
		return HAL_KEY_HOME;
	} else if (strcmp(key, "End") == 0) {
		return HAL_KEY_END;
	} else if (strcmp(key, "PageUp") == 0) {
		return HAL_KEY_PAGEUP;
	} else if (strcmp(key, "PageDown") == 0) {
		return HAL_KEY_PAGEDOWN;
	} else if (strcmp(key, "Shift") == 0) {
		return HAL_KEY_SHIFT;
	} else if (strcmp(key, "Control") == 0) {
		return HAL_KEY_CONTROL;
	} else if (strcmp(key, "Alt") == 0) {
		return HAL_KEY_ALT;
	} else if (strcmp(key, "ArrowUp") == 0) {
		return HAL_KEY_UP;
	} else if (strcmp(key, "ArrowDown") == 0) {
		return HAL_KEY_DOWN;
	} else if (strcmp(key, "ArrowLeft") == 0) {
		return HAL_KEY_LEFT;
	} else if (strcmp(key, "ArrowRight") == 0) {
		return HAL_KEY_RIGHT;
	} else if (strcmp(key, "A") == 0) {
		return HAL_KEY_A;
	} else if (strcmp(key, "B") == 0) {
		return HAL_KEY_B;
	} else if (strcmp(key, "C") == 0) {
		return HAL_KEY_C;
	} else if (strcmp(key, "D") == 0) {
		return HAL_KEY_D;
	} else if (strcmp(key, "E") == 0) {
		return HAL_KEY_E;
	} else if (strcmp(key, "F") == 0) {
		return HAL_KEY_F;
	} else if (strcmp(key, "G") == 0) {
		return HAL_KEY_G;
	} else if (strcmp(key, "H") == 0) {
		return HAL_KEY_H;
	} else if (strcmp(key, "I") == 0) {
		return HAL_KEY_I;
	} else if (strcmp(key, "J") == 0) {
		return HAL_KEY_J;
	} else if (strcmp(key, "K") == 0) {
		return HAL_KEY_K;
	} else if (strcmp(key, "L") == 0) {
		return HAL_KEY_L;
	} else if (strcmp(key, "M") == 0) {
		return HAL_KEY_M;
	} else if (strcmp(key, "N") == 0) {
		return HAL_KEY_N;
	} else if (strcmp(key, "O") == 0) {
		return HAL_KEY_O;
	} else if (strcmp(key, "P") == 0) {
		return HAL_KEY_P;
	} else if (strcmp(key, "Q") == 0) {
		return HAL_KEY_Q;
	} else if (strcmp(key, "R") == 0) {
		return HAL_KEY_R;
	} else if (strcmp(key, "S") == 0) {
		return HAL_KEY_S;
	} else if (strcmp(key, "T") == 0) {
		return HAL_KEY_T;
	} else if (strcmp(key, "U") == 0) {
		return HAL_KEY_U;
	} else if (strcmp(key, "V") == 0) {
		return HAL_KEY_V;
	} else if (strcmp(key, "W") == 0) {
		return HAL_KEY_W;
	} else if (strcmp(key, "X") == 0) {
		return HAL_KEY_X;
	} else if (strcmp(key, "Y") == 0) {
		return HAL_KEY_Y;
	} else if (strcmp(key, "Z") == 0) {
		return HAL_KEY_Z;
	} else if (strcmp(key, "1") == 0) {
		return HAL_KEY_1;
	} else if (strcmp(key, "2") == 0) {
		return HAL_KEY_2;
	} else if (strcmp(key, "3") == 0) {
		return HAL_KEY_3;
	} else if (strcmp(key, "4") == 0) {
		return HAL_KEY_4;
	} else if (strcmp(key, "5") == 0) {
		return HAL_KEY_5;
	} else if (strcmp(key, "6") == 0) {
		return HAL_KEY_6;
	} else if (strcmp(key, "7") == 0) {
		return HAL_KEY_7;
	} else if (strcmp(key, "8") == 0) {
		return HAL_KEY_8;
	} else if (strcmp(key, "9") == 0) {
		return HAL_KEY_9;
	} else if (strcmp(key, "0") == 0) {
		return HAL_KEY_0;
	} else if (strcmp(key, "F1") == 0) {
		return HAL_KEY_F1;
	} else if (strcmp(key, "F2") == 0) {
		return HAL_KEY_F2;
	} else if (strcmp(key, "F3") == 0) {
		return HAL_KEY_F3;
	} else if (strcmp(key, "F4") == 0) {
		return HAL_KEY_F4;
	} else if (strcmp(key, "F5") == 0) {
		return HAL_KEY_F5;
	} else if (strcmp(key, "F6") == 0) {
		return HAL_KEY_F6;
	} else if (strcmp(key, "F7") == 0) {
		return HAL_KEY_F7;
	} else if (strcmp(key, "F8") == 0) {
		return HAL_KEY_F8;
	} else if (strcmp(key, "F9") == 0) {
		return HAL_KEY_F9;
	} else if (strcmp(key, "F10") == 0) {
		return HAL_KEY_F10;
	} else if (strcmp(key, "F11") == 0) {
		return HAL_KEY_F11;
	} else if (strcmp(key, "F12") == 0) {
		return HAL_KEY_F12;
	}
	return -1;
}

/* touchstart callback */
static EM_BOOL
cb_touchstart(
	int eventType,
	const EmscriptenTouchEvent *touchEvent,
	void *userData)
{
	double w, h, scale;
	int x, y;

	touch_start_x = touchEvent->touches[0].targetX;
	touch_start_y = touchEvent->touches[0].targetY;
	touch_last_y = touch_start_y;

	/* Scale a mouse position. */
	emscripten_get_element_css_size("canvas", &w, &h);
	scale = w / (double)screen_width;
	x = (int)((double)touchEvent->touches[0].targetX / scale);
	y = (int)((double)touchEvent->touches[0].targetY / scale);

	/* Reset an existing touch move state. */
	hal_callback_on_event_touch_cancel();

	/* Process as a mouse down. (even if it's a touch start) */
	hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, x, y);

	return EM_TRUE;
}

/* touchmove callback */
static EM_BOOL
cb_touchmove(
	int eventType,
	const EmscriptenTouchEvent *touchEvent,
	void *userData)
{
	const int FLICK_X_DISTANCE = 10;
	const int FLICK_Y_DISTANCE = 30;
	double w, h, scale;
	int delta_x, delta_y, x, y;

	/* Start dragging. */
	delta_y = touchEvent->touches[0].targetY - touch_last_y;
	touch_last_y = touchEvent->touches[0].targetY;
	if (is_continuous_swipe_enabled) {
		if (delta_y > 0 && delta_y < FLICK_Y_DISTANCE) {
			hal_callback_on_event_key_press(HAL_KEY_DOWN);
			hal_callback_on_event_key_release(HAL_KEY_DOWN);
			return EM_TRUE;
		}
	}

	/* Scale a mouse position. */
	emscripten_get_element_css_size("canvas", &w, &h);
	scale = w / (double)screen_width;
	x = (int)((double)touchEvent->touches[0].targetX / scale);
	y = (int)((double)touchEvent->touches[0].targetY / scale);

	/* Process as a mouse move. (even if it's a touch start) */
	hal_callback_on_event_mouse_move(x, y);

	return EM_TRUE;
}

/* touchend callback */
static EM_BOOL
cb_touchend(
	int eventType,
	const EmscriptenTouchEvent *touchEvent,
	void *userData)
{
	const int FLICK_Y_DISTANCE = 50;
	const int FINGER_DISTANCE = 10;
	double w, h, scale;
	int x, y, delta_y;

	delta_y = touchEvent->touches[0].targetY - touch_start_y;
	if (delta_y > FLICK_Y_DISTANCE) {
		hal_callback_on_event_touch_cancel();
		hal_callback_on_event_swipe_down();
		return EM_TRUE;
	} else if (delta_y < -FLICK_Y_DISTANCE) {
		hal_callback_on_event_touch_cancel();
		hal_callback_on_event_swipe_up();
		return EM_TRUE;
	}

	/* Scale a mouse position. */
	emscripten_get_element_css_size("canvas", &w, &h);
	scale = w / (double)screen_width;
	x = (int)((double)touchEvent->touches[0].targetX / scale);
	y = (int)((double)touchEvent->touches[0].targetY / scale);
	if (x < 0 || x >= screen_width ||
	    y < 0 || y >= screen_height) {
		/* Out of canvas. */
		return EM_TRUE;
	}

	/* Process as a left click if one-finger and small-distance. */
	if (touchEvent->numTouches == 1 &&
	    abs(touchEvent->touches[0].targetX - touch_start_x) < FINGER_DISTANCE &&
	    abs(touchEvent->touches[0].targetY - touch_start_y) < FINGER_DISTANCE) {
		hal_callback_on_event_touch_cancel();
		hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, x, y);
		hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, x, y);
		return EM_TRUE;
	}

	/* Process as a right click if two-figer. */
	if (touchEvent->numTouches == 2) {
		hal_callback_on_event_touch_cancel();
		hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, x, y);
		hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, x, y);
		return EM_TRUE;
	}

	/* Otherwise, just cancel a touch move. */
	hal_callback_on_event_touch_cancel();

	return EM_TRUE;
}

/* touchcancel callback */
static EM_BOOL
cb_touchcancel(
	int eventType,
	const EmscriptenTouchEvent *touchEvent,
	void *userData)
{
	/* FIXME: When is this called? */
	hal_callback_on_event_touch_cancel();
	return EM_TRUE;
}

/*
 * Called from JavaScript
 */

/* When the tab is shown. */
EMSCRIPTEN_KEEPALIVE
void
setVisible(void)
{
	resume_sound();
}

/* When the tab is hidden. */
EMSCRIPTEN_KEEPALIVE
void
setHidden(void)
{
	pause_sound();
}

/* When the mouse pointer leaves Canvas. */
EMSCRIPTEN_KEEPALIVE
void
mouseLeave(void)
{
	hal_callback_on_event_touch_cancel();
}

/*
 * HAL
 */

/*
 * Show an INFO log.
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

	EM_ASM({
		alert(UTF8ToString($0));
	}, buf);

	return true;
}

/*
 * Show a WARN log.
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

	EM_ASM({
		alert(UTF8ToString($0));
	}, buf);

	return true;
}

/*
 * Show an ERROR log.
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

	EM_ASM({
		alert(UTF8ToString($0));
	}, buf);

	return true;
}

bool
hal_log_out_of_memory(void)
{
	hal_log_error("Out of memory.");
	return true;
}

/*
 * Notify an image update.
 */
void
hal_notify_image_update(
	struct hal_image *img)
{
	fill_sound_buffer();
	opengl_notify_image_update(img);
	fill_sound_buffer();
}

/*
 * Notify an image destroy.
 */
void
hal_notify_image_free(
	struct hal_image *img)
{
	opengl_notify_image_free(img);
}

/*
 * Render an image.
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
	opengl_render_image_normal(dst_left,
				   dst_top,
				   dst_width,
				   dst_height,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

/*
 * Render an image.
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
	opengl_render_image_add(dst_left,
				dst_top,
				dst_width,
				dst_height,
				src_image,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha);
}

/*
 * Render an image.
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
	opengl_render_image_sub(dst_left,
				dst_top,
				dst_width,
				dst_height,
				src_image,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha);
}

/*
 * Render an image.
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
	opengl_render_image_dim(dst_left,
				dst_top,
				dst_width,
				dst_height,
				src_image,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha);
}

/*
 * Render an image.
 */
void
hal_render_image_rule(
	struct hal_image * RESTRICT src_img,
	struct hal_image * RESTRICT rule_img,
	int threshold)
{
	opengl_render_image_rule(src_img, rule_img, threshold);
}

/*
 * Render an image.
 */
void
hal_render_image_melt(
	struct hal_image * RESTRICT src_img,
	struct hal_image * RESTRICT rule_img,
	int threshold)
{
	opengl_render_image_melt(src_img, rule_img, threshold);
}

/*
 * Render an image to the screen with the "normal" shader pipeline.
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
	opengl_render_image_3d_normal(x1,
				      y1,
				      x2,
				      y2,
				      x3,
				      y3,
				      x4,
				      y4,
				      src_image,
				      src_left,
				      src_top,
				      src_width,
				      src_height,
				      alpha);
}

/*
 * Render an image to the screen with the "add" shader pipeline.
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
	opengl_render_image_3d_add(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

/*
 * Render an image to the screen with the "add" shader pipeline.
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
	opengl_render_image_3d_sub(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

/*
 * Render an image to the screen with the "add" shader pipeline.
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
	opengl_render_image_3d_dim(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

/*
 * Create a save directory.
 */
bool
hal_make_save_directory(void)
{
	return true;
}

/*
 * Create a real path.
 */
char *
hal_make_real_path(
	const char *fname)
{
	char buf[1204];
	char *ret;
	size_t len;

	/* If it is a save directory. */
	if (strncmp(fname, "save/", 5) == 0) {
		char *space;
		snprintf(buf, sizeof(buf), "%s/%s", window_title, fname);
		ret = strdup(buf);
		if (ret == NULL) {
			hal_log_out_of_memory();
			return NULL;
		}
		while ((space = strchr(buf, ' ')) != NULL)
			*space = '_';
		return ret;
	}

	/* Other files. */
	snprintf(buf, sizeof(buf), "%s", fname);
	ret = strdup(buf);
	if (ret == NULL) {
		hal_log_out_of_memory();
		return NULL;
	}
	return ret;
}

/*
 * Reset a lap timer.
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
 * Play a video.
 */
bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	char *path;

	path = hal_make_real_path(fname);

	EM_ASM_({
		document.getElementById("canvas").style.display = "none";

		var v = document.getElementById("video");
		v.style.display = "block";
		v.src = Module.UTF8ToString($0, $1);
		v.load();
		v.addEventListener('ended', function() {
			document.getElementById("canvas").style.display = "block";
			document.getElementById("video").style.display = "none";
		});
		v.play();
	}, path, strlen(path));

	free(path);

	return true;
}

/*
 * Stop a video.
 */
void
hal_stop_video(void)
{
	EM_ASM_(
		var c = document.getElementById("canvas");
		c.style.display = "block";

		var v = document.getElementById("video");
		v.style.display = "none";
		v.pause();
		v.src = "";
		v.load();
	);
}

/*
 * Check if video is playing back.
 */
bool
hal_is_video_playing(void)
{
	int ended;

	ended = EM_ASM_INT(
		var v = document.getElementById("video");
		return v.ended;
	);

	return !ended;
}

/*
 * Check if the full screen mode is supported.
 */
bool
hal_is_full_screen_supported(void)
{
	return true;
}

/*
 * Check if in the full screen mode.
 */
bool
hal_is_full_screen_mode(void)
{
	return is_full_screen;
}

/*
 * Start the full screen mode.
 */
void
hal_enter_full_screen_mode(void)
{
	is_full_screen = true;
	EM_ASM({
		var canvas = document.getElementById('canvas_holder');
		const method = canvas.requestFullscreen ||
			       canvas.webkitRequestFullscreen ||
			       canvas.mozRequestFullScreen ||
			       canvas.msRequestFullscreen;
		if (method)
			method.call(canvas);
		onResizeWindow();
	});
}

/*
 * Leave the full screen mode.
 */
void
hal_leave_full_screen_mode(void)
{
	is_full_screen = false;
	EM_ASM({
		const method = document.exitFullscreen ||
			       document.webkitExitFullscreen ||
			       document.mozCancelFullScreen ||
			       document.msExitFullscreen;
		if (method)
			method.call(document);
		onResizeWindow();
	});
}

/*
 * Get a system language.
 */
const char *
get_lang_code(void)
{
	return lang_code;
}

EM_JS(int, get_system_lang_code, (void), {
	if (window.navigator.language.startsWith("en"))
		return 0;
	if (window.navigator.language.startsWith("fr"))
		return 1;
	if (window.navigator.language.startsWith("de"))
		return 2;
	if (window.navigator.language.startsWith("it"))
		return 3;
	if (window.navigator.language.startsWith("es"))
		return 4;
	if (window.navigator.language.startsWith("el"))
		return 5;
	if (window.navigator.language.startsWith("ru"))
		return 6;
	if (window.navigator.language.startsWith("zh_CN"))
		return 7;
	if (window.navigator.language.startsWith("zh_TW"))
		return 8;
	if (window.navigator.language.startsWith("ja"))
		return 9;
	return -1;
});
void init_lang_code(void)
{
	switch (get_system_lang_code()) {
	case 0:
		lang_code = "en";
		break;
	case 1:
		lang_code = "fr";
		break;
	case 2:
		lang_code = "de";
		break;
	case 3:
		lang_code = "it";
		break;
	case 4:
		lang_code = "es";
		break;
	case 5:
		lang_code = "el";
		break;
	case 6:
		lang_code = "ru";
		break;
	case 7:
		lang_code = "zh_CN";
		break;
	case 8:
		lang_code = "zh_TW";
		break;
	case 9:
		lang_code = "ja";
		break;
	default:
		lang_code = "en";
		break;
	}
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
	is_continuous_swipe_enabled = is_enabled;
}
