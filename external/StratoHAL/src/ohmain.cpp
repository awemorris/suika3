/* -*- coding: utf-8; indent-tabs-mode: nil; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * File HAL for OpenHarmony
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

// Base
extern "C" {
#include "stratohal/platform.h"
};

// HAL
extern "C" {
#include "ohmain.h"
#include "ohfile.h"
#include "glrender.h"
#include "slsound.h"
};

// C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

// C++
#include <string>

// POSIX
#include <sys/time.h>

// HormonyOS
#include <native_window/external_window.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <resourcemanager/ohresmgr.h>
#include <rawfile/raw_file_manager.h>
#include <hilog/log.h>

// OpenGL ES
#include <EGL/egl.h>
#include <GLES3/gl3.h>

// Log buffer size.
#define LOG_BUF_SIZE	4096

// Global.
extern int VIEWPORT_WIDTH;
extern int VIEWPORT_HEIGHT;

// Global.
NativeResourceManager* nativeResMgr;
char *sandboxDir;
const char *langName;

// Engine boot config.
static char *window_title;
static int screen_width;
static int screen_height;

// Graphics
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLConfig config = nullptr;
static EGLContext context = EGL_NO_CONTEXT;
static EGLSurface surface = EGL_NO_SURFACE;
static volatile bool isRunning;
static pthread_t thread;
static float viewportScale = 1.0f;
static int viewportX;
static int viewportY;
static int viewportWidth;
static int viewportHeight;
static float scale_factor;

// Touch status.
static int touchStartX;
static int touchStartY;
static int touchLastY;
static bool isContinuousSwipeEnabled;

// Forward declaration.
extern "C" {
static napi_value Init(napi_env env, napi_value exports);
static napi_value InitResourceManager(napi_env env, napi_callback_info info);
static napi_value InitFileDir(napi_env env, napi_callback_info info);
static napi_value SetLanguage(napi_env env, napi_callback_info info);
static napi_value StartGame(napi_env env, napi_callback_info info);
static void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window);
static void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window);
static void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window);
static void DispatchTouchEventCB(OH_NativeXComponent* component, void* window);
static void *RenderingThread(void *p);
static bool onInit(void);
static bool onFrame(void);
static void onUpdateViewport(int width, int height);
static void onTouchStart(int x, int y, int points);
static void onTouchMove(int x, int y);
static void onTouchEnd(int x, int y, int points);
static void onTouchCancel(void);
};

// Module info.
static napi_module engineModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "engine",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

// Function table.
static napi_property_descriptor funcDesc[] = {
    { "initResourceManager", nullptr, InitResourceManager, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "initFileDir", nullptr, InitFileDir, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "setLanguage", nullptr, SetLanguage, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "startGame", nullptr, StartGame, nullptr, nullptr, nullptr, napi_default, nullptr },
};

// Callback table.
static OH_NativeXComponent_Callback callback = {
    .OnSurfaceCreated = OnSurfaceCreatedCB,
    .OnSurfaceChanged = OnSurfaceChangedCB,
    .OnSurfaceDestroyed = OnSurfaceDestroyedCB,
    .DispatchTouchEvent = DispatchTouchEventCB,
};

//
// Initializer for shared object load
//
void onLoadSharedObject(void)
{
    napi_module_register(&engineModule);
}

//
// Initializer for component load.
//
static napi_value
Init(
     napi_env env,
     napi_value exports)
{
    // Register JS functions.
    napi_define_properties(env, exports, sizeof(funcDesc) / sizeof(funcDesc[0]), funcDesc);

    // Register native callback functions.
    napi_value exportInstance;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
        return exports;
    }

    OH_NativeXComponent *nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent)) != napi_ok) {
        return exports;
    }
    OH_NativeXComponent_RegisterCallback(nativeXComponent, &callback);

    return exports;
}

//
// ArkTS: initResourceManager(resMgr: object)
//
static napi_value
InitResourceManager(
    napi_env env,
    napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};

    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    // Save the pointer.
    nativeResMgr = OH_ResourceManager_InitNativeResourceManager(env, argv[0]);

    return nullptr;
}

//
// ArkTS: initFileDir(fileDir: string)
//
static napi_value
InitFileDir(
    napi_env env,
    napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    size_t length = 0;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &length);

    std::string path(length, '\0');
    napi_get_value_string_utf8(env, argv[0], &path[0], length + 1, &length);

    sandboxDir = strdup(path.c_str());

    return nullptr;
}

//
// ArkTS: setLanguage(language: string)
//
static napi_value
SetLanguage(
    napi_env env,
    napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    size_t length = 0;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &length);
    std::vector<char> language(length + 1);
    napi_get_value_string_utf8(env, argv[0], language.data(), language.size(), &length);

    const char *lang = language.data();
    if (strncmp(lang, "en", 2) == 0) {
        langName = "en";
    } else if (strncmp(lang, "fr", 2) == 0) {
        langName = "fr";
    } else if (strncmp(lang, "it", 2) == 0) {
        langName = "it";
    } else if (strncmp(lang, "es", 2) == 0) {
        langName = "es";
    } else if (strncmp(lang, "de", 2) == 0) {
        langName = "de";
    } else if (strncmp(lang, "el", 2) == 0) {
        langName = "el";
    } else if (strncmp(lang, "ru", 2) == 0) {
        langName = "ru";
    } else if (strncmp(lang, "zh-Hans", 7) == 0) {
            langName = "zh";
    } else if (strncmp(lang, "zh-CN", 5) == 0) {
            langName = "zh";
    } else if (strncmp(lang, "zh-Hant", 7) == 0) {
            langName = "tw";
    } else if (strncmp(lang, "zh-TW", 5) == 0) {
            langName = "tw";
    } else if (strncmp(lang, "zh-HK", 5) == 0) {
            langName = "tw";
    } else if (strncmp(lang, "zh", 2) == 0) {
            langName = "zh";
    } else if (strcmp(lang, "ja") == 0) {
        langName = "ja";
    } else {
        langName = "other";
    }

    return nullptr;
}

//
// ArkTS: startGame()
//
static napi_value
StartGame(
    napi_env env,
    napi_callback_info info)
{
    isRunning = true;
    return nullptr;
}

//
// Native: XComponent Callback: OnSurfaceCreatedCB()
//
static void
OnSurfaceCreatedCB(
    OH_NativeXComponent* component,
    void* window)
{
    OHNativeWindow* nativeWindow = static_cast<OHNativeWindow*>(window);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_NONE
    };
    EGLint numConfigs;
    eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    surface = eglCreateWindowSurface(display, config, reinterpret_cast<EGLNativeWindowType>(nativeWindow), nullptr);

    //eglMakeCurrent(display, surface, surface, context);

    pthread_create(&thread, NULL, &RenderingThread, nullptr);
}

//
// Native: XComponent Callback: OnSurfaceChangedCB()
//
static void
OnSurfaceChangedCB(
    OH_NativeXComponent* component,
    void* window)
{
    OHNativeWindow* nativeWindow = static_cast<OHNativeWindow*>(window);

    uint64_t width = 0;
    uint64_t height = 0;
    int32_t ret = OH_NativeXComponent_GetXComponentSize(component, nativeWindow, &width, &height);

    if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS && width > 0 && height > 0)
        onUpdateViewport(width, height);
}

//
// Native: XComponent Callback: OnSurfaceDestroyedCB()
//
static void
OnSurfaceDestroyedCB(
    OH_NativeXComponent* component,
    void* window)
{
    isRunning = false;
    pthread_join(thread, nullptr);

    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;
        }
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
            context = EGL_NO_CONTEXT;
        }
        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }

    if (sandboxDir != nullptr) {
        free(sandboxDir);
        sandboxDir = nullptr;
    }
}

//
// Native: XComponent Callback: DispatchTouchEventCB()
//
static void
DispatchTouchEventCB(
    OH_NativeXComponent* component,
    void* window)
{
    OH_NativeXComponent_TouchEvent touchEvent;
    int32_t ret = OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        return;

	int x = (int)((touchEvent.touchPoints[0].x - viewportX) / viewportScale);
	int y = (int)((touchEvent.touchPoints[0].y - viewportY) / viewportScale);
    switch (touchEvent.touchPoints[0].type) {
    case OH_NATIVEXCOMPONENT_DOWN:
        onTouchStart(x, y, touchEvent.numPoints);
        break;
    case OH_NATIVEXCOMPONENT_MOVE:
        onTouchMove(x, y);
        break;
    case OH_NATIVEXCOMPONENT_UP:
        onTouchEnd(x, y, touchEvent.numPoints);
        break;
    case OH_NATIVEXCOMPONENT_CANCEL:
        onTouchCancel();
        break;
    default:
        break;
    }
}

//
// Native: Rendering thread.
//
static void *
RenderingThread(
    void *p)
{
    // Wait for startGame(). 
    while (!isRunning)
        ;

    // Use OpenGL ES.
    eglMakeCurrent(display, surface, surface, context);
    glClearColor(1.0f, 0.4f, 0.7f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);

    // Start the game.
    if (!onInit())
        return nullptr;

    // Game loop.
    while (isRunning) {
        // Do a frame.
        if (!onFrame())
            break;
        eglSwapBuffers(display, surface);
    }

    return nullptr;
}

//
// Called for initialization.
//
static bool
onInit(void)
{
    // Initialize the file HAL.
    if(!init_file())
        return false;

    // Do a boot callback to acquire a screen configuration.
    if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
        return false;
    
    // Init the graphics HAL.
    if (!init_opengl(screen_width, screen_height)) {
        hal_log_error("Failed to initialize OpenGL.");
        return false;
    }

    // Initialize the sound HAL.
    init_opensl_es();

    // Do a start callback.
    if(!hal_callback_on_event_start()) {
        hal_log_error("Failed to initialize event loop.");
        return false;
    }

    return true;
}

//
// Called for viewport layout.
//
static void
onUpdateViewport(
    int width,
    int height)
{
    // Get the aspect ratio.
    float aspect = (float)VIEWPORT_HEIGHT / (float)VIEWPORT_WIDTH;

    // Width-first.
    viewportWidth = (int)width;
    viewportHeight = (int)(width * aspect);
    viewportScale = viewportWidth / (float)VIEWPORT_WIDTH;
    viewportX = 0;
    viewportY = (int)((float)(height - viewportHeight) / 2.0f);
    if(viewportHeight > height) {
        // Height-first.
        viewportHeight = height;
        viewportWidth = (int)(height / aspect);
        viewportScale = (int)((float)viewportHeight / (float)VIEWPORT_HEIGHT);
        viewportX = (int)((float)(width - viewportWidth) / 2.0f);
        viewportY = 0;
    }

    // Call glViewport().
    opengl_set_screen(viewportX, viewportY, viewportWidth, viewportHeight);
}

//
// Called every frame.
//
bool onFrame(void)
{
    opengl_start_rendering();

    if (!hal_callback_on_event_frame())
        return false;

    opengl_end_rendering();

    return true;
}

//
// Process a touch start.
//
void onTouchStart(int x, int y, int points)
{
    touchStartX = x;
    touchStartY = y;
    touchLastY = y;

    hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, x, y);
}

//
// Process a touch move.
//
void onTouchMove(int x, int y)
{
    // Emulate a wheel down.
    const int FLICK_Y_DISTANCE = 30;
    int deltaY = y - touchLastY;
    touchLastY = y;
    if (isContinuousSwipeEnabled) {
        if (deltaY > 0 && deltaY < FLICK_Y_DISTANCE) {
            hal_callback_on_event_key_press(HAL_KEY_DOWN);
            hal_callback_on_event_key_release(HAL_KEY_DOWN);
            return;
        }
    }

    // Emulate a mouse move.
    hal_callback_on_event_mouse_move(x, y);
}

//
// Process a touch end.
//
void onTouchEnd(int x, int y, int points)
{
    const int FLICK_Y_DISTANCE = 50;
    const int FINGER_DISTANCE = 10;

    int deltaY = y - touchStartY;
    if (deltaY > FLICK_Y_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_swipe_down();
        return;
    } else if (deltaY < -FLICK_Y_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_swipe_up();
        return;
    }

    // Emulate a left click.
    if (points == 1 &&
        abs(x - touchStartX) < FINGER_DISTANCE &&
        abs(y - touchStartY) < FINGER_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, x, y);
        hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, x, y);
        return;
    }

    // Emulate a right click.
    if (points == 2 &&
        abs(x - touchStartX) < FINGER_DISTANCE &&
        abs(y - touchStartY) < FINGER_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, x, y);
        hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, x, y);
        return;
    }

    // Cancel the touch move.
    hal_callback_on_event_touch_cancel();
}

//
// Process a touch cancel.
//
void onTouchCancel(void)
{
    hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, touchStartX, touchStartY);
}

//
// HAL
//

bool
hal_log_info(
    const char *s,
    ...)
{
    char buf[LOG_BUF_SIZE];
    va_list ap;

    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0x0000, "playfield", "%s", buf);
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
    OH_LOG_Print(LOG_APP, LOG_WARN, 0x0000, "playfield", "%s", buf);
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
    OH_LOG_Print(LOG_APP, LOG_ERROR, 0x0000, "playfield", "%s", buf);
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

bool
hal_make_save_directory(void)
{
    // Note: not used on HarmonyOS.
    return true;
}

char *
hal_make_valid_path(
    const char *dir,
    const char *fname)
{
    // Note: not used on HarmonyOS.
    assert(0);
    return NULL;
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
    // TODO
    return true;
}

void
hal_stop_video(void)
{
    // TODO
}

bool
hal_is_video_playing(void)
{
    // TODO
    return false;
}

void
hal_update_window_title(void)
{
    // FIXME: Do we have a window name on HarmonyOS for desktop?
}

bool
hal_is_full_screen_supported(void)
{
    // FIXME: Do we have a full screen support on HarmonyOS for desktop?
    return false;
}

bool
hal_is_full_screen_mode(void)
{
    // FIXME: Do we have a full screen support on ChromeOS?
    return false;
}

void
hal_enter_full_screen_mode(void)
{
    // FIXME: Do we have a full screen support on HarmonyOS for desktop?
}

void
hal_leave_full_screen_mode(void)
{
    // FIXME: Do we have a full screen support on HarmonyOS for desktop?
}

const char *
hal_get_system_language(void)
{
    return langName;
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
    isContinuousSwipeEnabled = is_enabled;
}

void
hal_get_local_time(
    int *year,
    int *month,
    int *day,
    int *dow,
    int *hour,
    int *min,
    int *sec)
{
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    *year = tm_info->tm_year + 1900;
    *month = tm_info->tm_mon + 1;
    *day = tm_info->tm_mday;
    *dow = tm_info->tm_wday;
    *hour = tm_info->tm_hour;
    *min = tm_info->tm_min;
    *sec = tm_info->tm_sec;
}
