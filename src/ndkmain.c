/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Android NDK main code
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

/* Base */
#include <strato/strato.h>
#include "callback.h"

/* HAL */
#include "ndkmain.h"
#include "ndkfile.h"
#include "glrender.h"
#include "slsound.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <assert.h>

/* POSIX */
#include <sys/time.h>

/* NDK */
#include <android/log.h>

/*
 * Constants
 */
#define LOG_BUF_SIZE			(1024)
#define SCROLL_DOWN_MARGIN		(5)
#define DELAYED_RFILE_FREE_SLOTS	(16)

/*
 * Variables
 */

/* The global reference to the MainActivity instance. */
jobject main_activity;

/* A temporal reference to a JNIEnv. */
JNIEnv *jni_env;

/* A flag that indicates if a video is playing back. */
static bool state_video;

/* Touch status. */
static int touch_start_x;
static int touch_start_y;
static int touch_last_y;
static bool is_continuous_swipe_enabled;

/* Callback */
struct hal_callback hal_callback;
HAL_DLL bool (*hal_bootstrap_ptr)(char **title, int *width, int *height, struct hal_callback *callback);

/* Delayed removal of (struct hal_rfile *) references. */
struct hal_rfile *delayed_rfile_free_slot[DELAYED_RFILE_FREE_SLOTS];

/* Locale */
static const char *lang_code;

/* Window Configuration */
static char *window_title;
static int screen_width;
static int screen_height;

/* Forward declarations. */
static void init_locale(JNIEnv *env);
static void do_delayed_remove_rfile_ref(void);

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnBootstrap(
	JNIEnv *env,
	jobject instance)
{
	jni_env = env;

	/* Retain the main activity instance globally. */
	main_activity = (*env)->NewGlobalRef(env, instance);

	/* Clear the video state. */
	state_video = false;

	/* Init the locale. */
	init_locale(env);

	/* Init the file HAL. */
	if (!init_file()) {
		hal_log_error("Failed to initialize config.");
		return;
	}

	/* Do a boot callback to acquire a screen configuration. */
	if (!hal_bootstrap_ptr(&window_title,
			       &screen_width,
			       &screen_height,
			       &hal_callback)) {
		hal_log_error("Initialization failed.");
		return;
	}

	jni_env = NULL;
}


JNIEXPORT jint JNICALL
Java_io_noctvm_strato_MainActivity_nativeGetScreenWidth(
	JNIEnv *env,
	jobject instance)
{
	return screen_width;
}

JNIEXPORT jint JNICALL
Java_io_noctvm_strato_MainActivity_nativeGetScreenHeight(
	JNIEnv *env,
	jobject instance)
{
	return screen_height;
}

JNIEXPORT jstring JNICALL
Java_io_noctvm_strato_MainActivity_nativeGetTitle(
	JNIEnv *env,
	jobject instance)
{
	jstring s = (*env)->NewStringUTF(env, window_title);

	return s;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnStart(
	JNIEnv *env,
	jobject instance)
{
	jni_env = env;

	/* Init the graphics HAL. */
	if (!init_opengl(screen_width, screen_height)) {
		hal_log_error("Failed to initialize OpenGL.");
		return;
	}

	/* Init the sound HAL. */
	init_opensl_es();

	/* Do a start callback. */
	if(!hal_callback.on_start()) {
		hal_log_error("Failed to initialize event loop.");
		return;
	}

	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnRestart(
        JNIEnv *env,
	jobject instance)
{
	jni_env = env;

	/* Delete the global reference to the main activity instance. */
	if (main_activity != NULL) {
		(*env)->DeleteGlobalRef(env, main_activity);
		main_activity = NULL;
	}

	/* Retain the main activity instance globally. */
	main_activity = (*env)->NewGlobalRef(env, instance);

	/* Re-initialize OpenGL. */
	if (!init_opengl(screen_width, screen_height)) {
		hal_log_error("Failed to initialize OpenGL.");
		return;
	}

	state_video = false;

	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnStop(
	JNIEnv *env)
{
	jni_env = env;

	hal_callback.on_stop();

	/* Delete the global reference to the main activity instance. */
	if (main_activity != NULL) {
		(*env)->DeleteGlobalRef(env, main_activity);
		main_activity = NULL;
	}

	jni_env = NULL;
}

JNIEXPORT jboolean JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnFrame(
	JNIEnv *env,
	jobject instance)
{
	jni_env = env;

	/* Process a video playback. */
	if (state_video) {
		jclass cls = (*jni_env)->FindClass(jni_env, "io/noctvm/strato/MainActivity");
		jmethodID mid = (*jni_env)->GetMethodID(jni_env, cls, "bridgeIsVideoPlaying", "()Z");
		state_video = (*jni_env)->CallBooleanMethod(jni_env, main_activity, mid);
	}

	/* Run a frame. */
	jboolean ret = JNI_TRUE;
	if (!hal_callback.on_update())
		ret = JNI_FALSE;

	/* Do rendering if video is not playing. */
	if (!state_video) {
		/* Start a rendering. */
		opengl_start_rendering();

		/* Do rendering. */
		hal_callback.on_render();

		/* Finish a rendering. */
		opengl_end_rendering();
	}

	/* Do delayed removal of rfile refereces. */
	do_delayed_remove_rfile_ref();

	jni_env = NULL;

	return ret;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnPause(
        JNIEnv *env,
        jobject instance)
{
	jni_env = env;

	sl_pause_sound();

	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnResume(
        JNIEnv *env,
        jobject instance)
{
	jni_env = env;

	sl_resume_sound();

	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnTouchStart(
        JNIEnv *env,
        jobject instance,
        jint x,
        jint y,
	jint points)
{
	jni_env = env;
	{
		touch_start_x = x;
		touch_start_y = y;
		touch_last_y = y;
		if (hal_callback.on_mouse_press != NULL)
			hal_callback.on_mouse_press(HAL_MOUSE_LEFT, x, y);
	}
	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnTouchMove(
	JNIEnv *env,
	jobject instance,
	jint x,
	jint y)
{
	jni_env = env;
	do {
		/* Emulate a wheel down. */
		const int FLICK_Y_DISTANCE = 30;
		int delta_y = y - touch_last_y;
		touch_last_y = y;
		if (is_continuous_swipe_enabled) {
			if (delta_y > 0 && delta_y < FLICK_Y_DISTANCE) {
				if (hal_callback.on_mouse_wheel != NULL)
					hal_callback.on_mouse_wheel(delta_y, 0);
				break;
			}
		}

		/* Emulate a mouse move. */
		if (hal_callback.on_mouse_move != NULL)
			hal_callback.on_mouse_move(x, y);
	} while (0);
	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnTouchEnd(
	JNIEnv *env,
	jobject instance,
	jint x,
	jint y,
	jint points)
{
	jni_env = env;

	do {
		/* Detect a down/up swipe. */
		const int FLICK_Y_DISTANCE = 50;
		int delta_y = y - touch_start_y;
		if (delta_y > FLICK_Y_DISTANCE) {
			if (hal_callback.on_touch_cancel != NULL)
				hal_callback.on_touch_cancel();
			if (hal_callback.on_swipe_down != NULL)
				hal_callback.on_swipe_down(0, 0);
			break;
		} else if (delta_y < -FLICK_Y_DISTANCE) {
			if (hal_callback.on_touch_cancel != NULL)
				hal_callback.on_touch_cancel();
			if (hal_callback.on_swipe_up != NULL)
				hal_callback.on_swipe_up(0, 0);
			break;
		}

		/* Emulate a left click. */
		const int FINGER_DISTANCE = 10;
		if (points == 1 &&
		    abs(x - touch_start_x) < FINGER_DISTANCE &&
		    abs(y - touch_start_y) < FINGER_DISTANCE) {
			if (hal_callback.on_touch_cancel != NULL)
				hal_callback.on_touch_cancel();
			if (hal_callback.on_mouse_press != NULL)
				hal_callback.on_mouse_press(HAL_MOUSE_LEFT, x, y);
			if (hal_callback.on_mouse_release != NULL)
				hal_callback.on_mouse_release(HAL_MOUSE_LEFT, x, y);
			break;
		}

		/* Emulate a right click. */
		if (points == 2 &&
		    abs(x - touch_start_x) < FINGER_DISTANCE &&
		    abs(y - touch_start_y) < FINGER_DISTANCE) {
			if (hal_callback.on_touch_cancel != NULL)
				hal_callback.on_touch_cancel();
			if (hal_callback.on_mouse_press != NULL)
				hal_callback.on_mouse_press(HAL_MOUSE_RIGHT, x, y);
			if (hal_callback.on_mouse_release != NULL)
				hal_callback.on_mouse_release(HAL_MOUSE_RIGHT, x, y);
			break;
		}

		/* Cancel the touch move. */
		if (hal_callback.on_touch_cancel != NULL)
			hal_callback.on_touch_cancel();
	} while (0);

	jni_env = NULL;
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnGamepadAnalog(
	JNIEnv *env,
	jobject instance,
	jfloat x1,
	jfloat y1,
	jfloat x2,
	jfloat y2,
	jfloat l,
	jfloat r)
{
	if (hal_callback.on_analog_input != NULL) {
		hal_callback.on_analog_input(HAL_ANALOG_X1, (int)(x1 * 32767.0f));
		hal_callback.on_analog_input(HAL_ANALOG_Y1, (int)(y1 * 32767.0f));
		hal_callback.on_analog_input(HAL_ANALOG_X2, (int)(x2 * 32767.0f));
		hal_callback.on_analog_input(HAL_ANALOG_Y2, (int)(y2 * 32767.0f));
		hal_callback.on_analog_input(HAL_ANALOG_L, (int)(l * 32767.0f));
		hal_callback.on_analog_input(HAL_ANALOG_R, (int)(r * 32767.0f));
	}
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnKeyDown(
	JNIEnv *env,
	jobject instance,
	int key)
{
	if (hal_callback.on_key_press != NULL)
		hal_callback.on_key_press(key);
}

JNIEXPORT void JNICALL
Java_io_noctvm_strato_MainActivity_nativeOnKeyUp(
	JNIEnv *env,
	jobject instance,
	int key)
{
	if (hal_callback.on_key_release != NULL)
		hal_callback.on_key_release(key);
}

/*
 * Helpers
 */

static void
init_locale(
	JNIEnv *env)
{
    const char *langCode;
    jclass clazz;
    jfieldID fid;
    jstring jLang;

    do {
	    langCode = NULL;
	    jLang = NULL;

	    /* Get the MainActivity class. */
	    clazz = (*env)->FindClass(env, "io/noctvm/strato/MainActivity");
	    if (clazz == NULL)
		    break;

	    /* Get the MainActivity.language field. */
	    fid = (*env)->GetStaticFieldID(env, clazz, "language", "Ljava/lang/String;");
	    if (fid == NULL)
		    break;

	    /* Get the String obeject. */
	    jLang = (jstring)(*env)->GetStaticObjectField(env, clazz, fid);
	    if (jLang == NULL)
		    break;

	    // Get the char pointer.
	    langCode = (*env)->GetStringUTFChars(env, jLang, NULL);
    } while (0);

    /* Fallback */
    if (langCode == NULL || langCode[0] == '\0') {
        lang_code = "en";
        return;
    }

    /* English */
    if (strncmp(langCode, "en_AU", 5) == 0)
	    lang_code = "en-au";
    else if (strncmp(langCode, "en_GB", 5) == 0)
	    lang_code = "en-gb";
    else if (strncmp(langCode, "en_NZ", 5) == 0)
	    lang_code = "en-nz";
    else if (strncmp(langCode, "en_US", 5) == 0)
	    lang_code = "en-us";
    else if (strncmp(langCode, "en", 2) == 0)
	    lang_code = "en";
    /* French */
    else if (strncmp(langCode, "fr_CA", 5) == 0)
	    lang_code = "fr-ca";
    else if (strncmp(langCode, "fr", 2) == 0)
	    lang_code = "fr-fr";
    /* Spanish */
    else if (strncmp(langCode, "es_ES", 5) == 0)
	    lang_code = "es-es";
    else if (strncmp(langCode, "es", 2) == 0)
	    lang_code = "es-la";
    /* Chinese */
    else if (strncmp(langCode, "zh_TW", 5) == 0 ||
	     strncmp(langCode, "zh_HK", 5) == 0)
	    lang_code = "zh-tw";
    else if (strncmp(langCode, "zh", 2) == 0)
	    lang_code = "zh-cn";
    /* Others */
    else if (strncmp(langCode, "ja", 2) == 0)
	    lang_code = "ja";
    else if (strncmp(langCode, "de", 2) == 0)
	    lang_code = "de";
    else if (strncmp(langCode, "it", 2) == 0)
	    lang_code = "it";
    else if (strncmp(langCode, "el", 2) == 0)
	    lang_code = "el";
    else if (strncmp(langCode, "ru", 2) == 0)
	    lang_code = "ru";
    else if (strncmp(langCode, "ko", 2) == 0)
	    lang_code = "ko";
    /* Fallback */
    else
        lang_code = "en";

    if (jLang != NULL)
	    (*env)->ReleaseStringUTFChars(env, jLang, langCode);
}

/*
 * Reserve a deferred removal of a file reference. In the OpenSL
 * callback, we don't have an env reference and we can't remove a file
 * reference. Thus, we have to defer the removal when the main thread
 * runs.
 */
void
post_delayed_remove_rfile_ref(
	struct hal_rfile *rf)
{
	int i;

	for (i = 0; i < DELAYED_RFILE_FREE_SLOTS; i++) {
		if (delayed_rfile_free_slot[i] == NULL) {
			delayed_rfile_free_slot[i] = rf;
			return;
		}
	}
	assert(0);
}

static void
do_delayed_remove_rfile_ref(void)
{
	int i;

	for (i = 0; i < DELAYED_RFILE_FREE_SLOTS; i++) {
		if (delayed_rfile_free_slot[i] != NULL) {
			(*jni_env)->ReleaseByteArrayElements(jni_env,
							     delayed_rfile_free_slot[i]->array,
							     (jbyte *)delayed_rfile_free_slot[i]->buf,
							     JNI_ABORT);
			(*jni_env)->DeleteGlobalRef(jni_env,
						    delayed_rfile_free_slot[i]->array);
			free(delayed_rfile_free_slot[i]);
			delayed_rfile_free_slot[i] = NULL;
		}
	}
}

/*
 * HAL
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
	__android_log_print(ANDROID_LOG_INFO, window_title, "%s", buf);
	va_end(ap);
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
	__android_log_print(ANDROID_LOG_WARN, window_title, "%s", buf);
	va_end(ap);
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
	__android_log_print(ANDROID_LOG_ERROR, window_title, "%s", buf);
	va_end(ap);
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
	opengl_notify_image_update(img);
}

void
hal_notify_image_free(
	struct hal_image *img)
{
	opengl_notify_image_free(img);
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

void
hal_render_image_rule(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int threshold)
{
	opengl_render_image_rule(src_img, rule_img, threshold);
}

void
hal_render_image_melt(
	struct hal_image *src_img,
	struct hal_image *rule_img,
	int progress)
{
	opengl_render_image_melt(src_img, rule_img, progress);
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
	opengl_render_image_cross(src1_img,
				  src2_img,
				  src1_left,
				  src1_top,
				  src2_left,
				  src2_top,
				  alpha);
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
	opengl_render_image_cross_3d(src1_img,
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

void
hal_reset_lap_timer(
	uint64_t *t)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	*t = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

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
	state_video = true;

	jstring file = (*jni_env)->NewStringUTF(jni_env, fname);

	jclass cls = (*jni_env)->FindClass(jni_env, "io/noctvm/strato/MainActivity");
	jmethodID mid = (*jni_env)->GetMethodID(jni_env, cls, "bridgePlayVideo", "(Ljava/lang/String;Z)V");
	(*jni_env)->CallVoidMethod(jni_env,
				   main_activity,
				   mid,
				   file,
				   is_skippable ? JNI_TRUE : JNI_FALSE);

	return true;
}

void
hal_stop_video(void)
{
	state_video = false;

	jclass cls = (*jni_env)->FindClass(jni_env, "io/noctvm/strato/MainActivity");
	jmethodID mid = (*jni_env)->GetMethodID(jni_env, cls, "bridgeStopVideo", "()V");
	(*jni_env)->CallVoidMethod(jni_env, main_activity, mid);
}

bool
hal_is_video_playing(void)
{
	if (state_video) {
		jclass cls = (*jni_env)->FindClass(jni_env, "io/noctvm/strato/MainActivity");
		jmethodID mid = (*jni_env)->GetMethodID(jni_env, cls, "bridgeIsVideoPlaying", "()Z");
		if (!(*jni_env)->CallBooleanMethod(jni_env, main_activity, mid)) {
			state_video = false;
			return false;
		}
		return true;
	}
	return false;
}

void
hal_update_window_title(void)
{
	/* FIXME: Do we have a window name on ChromeOS? */
}

bool
hal_is_full_screen_supported(void)
{
	/* FIXME: Do we have a full screen support on ChromeOS? */
	return false;
}

bool
hal_is_full_screen_mode(void)
{
	/* FIXME: Do we have a full screen support on ChromeOS? */
	return false;
}

void
hal_enter_full_screen_mode(void)
{
	/* FIXME: Do we have a full screen support on ChromeOS? */
}

void
hal_leave_full_screen_mode(void)
{
	/* FIXME: Do we have a full screen support on ChromeOS? */
}

const char *
hal_get_system_language(void)
{
	return lang_code;
}

void
hal_speak_text(
	const char *text)
{
	UNUSED_PARAMETER(text);
}

void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	is_continuous_swipe_enabled = is_enabled;
}
