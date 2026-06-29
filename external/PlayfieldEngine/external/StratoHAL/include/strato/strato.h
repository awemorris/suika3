/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Public header
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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

#ifndef STRATOHAL_STRATOHAL_H
#define STRATOHAL_STRATOHAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <strato/c89compat.h>

/* --- */

/*
 * Callbacks
 * - Callbacks are defined in user applications.
 */

/*
 * Keycode.
 */
enum hal_key_code {
	HAL_KEY_ESCAPE          = 0,
	HAL_KEY_RETURN          = 1,
	HAL_KEY_SPACE           = 2,
	HAL_KEY_TAB             = 3,
	HAL_KEY_BACKSPACE       = 4,
	HAL_KEY_DELETE          = 5,
	HAL_KEY_HOME            = 6,
	HAL_KEY_END             = 7,
	HAL_KEY_PAGEUP          = 8,
	HAL_KEY_PAGEDOWN        = 9,
	HAL_KEY_SHIFT           = 10,
	HAL_KEY_CONTROL         = 11,
	HAL_KEY_ALT             = 12,
	HAL_KEY_UP              = 13,
	HAL_KEY_DOWN            = 14,
	HAL_KEY_LEFT            = 15,
	HAL_KEY_RIGHT           = 16,
	HAL_KEY_A               = 17,
	HAL_KEY_B               = 18,
	HAL_KEY_C               = 19,
	HAL_KEY_D               = 20,
	HAL_KEY_E               = 21,
	HAL_KEY_F               = 22,
	HAL_KEY_G               = 23,
	HAL_KEY_H               = 24,
	HAL_KEY_I               = 25,
	HAL_KEY_J               = 26,
	HAL_KEY_K               = 27,
	HAL_KEY_L               = 28,
	HAL_KEY_M               = 29,
	HAL_KEY_N               = 30,
	HAL_KEY_O               = 31,
	HAL_KEY_P               = 32,
	HAL_KEY_Q               = 33,
	HAL_KEY_R               = 34,
	HAL_KEY_S               = 35,
	HAL_KEY_T               = 36,
	HAL_KEY_U               = 37,
	HAL_KEY_V               = 38,
	HAL_KEY_W               = 39,
	HAL_KEY_X               = 40,
	HAL_KEY_Y               = 41,
	HAL_KEY_Z               = 42,
	HAL_KEY_1               = 43,
	HAL_KEY_2               = 44,
	HAL_KEY_3               = 45,
	HAL_KEY_4               = 46,
	HAL_KEY_5               = 47,
	HAL_KEY_6               = 48,
	HAL_KEY_7               = 49,
	HAL_KEY_8               = 50,
	HAL_KEY_9               = 51,
	HAL_KEY_0               = 52,
	HAL_KEY_F1              = 53,
	HAL_KEY_F2              = 54,
	HAL_KEY_F3              = 55,
	HAL_KEY_F4              = 56,
	HAL_KEY_F5              = 57,
	HAL_KEY_F6              = 58,
	HAL_KEY_F7              = 59,
	HAL_KEY_F8              = 60,
	HAL_KEY_F9              = 61,
	HAL_KEY_F10             = 62,
	HAL_KEY_F11             = 63,
	HAL_KEY_F12             = 64,
	HAL_KEY_GAMEPAD_UP      = 65,
	HAL_KEY_GAMEPAD_DOWN    = 66,
	HAL_KEY_GAMEPAD_LEFT    = 67,
	HAL_KEY_GAMEPAD_RIGHT   = 68,
	HAL_KEY_GAMEPAD_A       = 69,
	HAL_KEY_GAMEPAD_B       = 70,
	HAL_KEY_GAMEPAD_X       = 71,
	HAL_KEY_GAMEPAD_Y       = 72,
	HAL_KEY_GAMEPAD_L       = 73,
	HAL_KEY_GAMEPAD_R       = 74,
	HAL_KEY_MAX             = 75,
};

/*
 * Mouse button.
 */
enum hal_mouse_button {
	HAL_MOUSE_LEFT  = 0,
	HAL_MOUSE_RIGHT = 1,
};

/*
 * Analog input.
 */
enum hal_analog_input {
	HAL_ANALOG_X1 = 0,
	HAL_ANALOG_Y1 = 1,
	HAL_ANALOG_X2 = 2,
	HAL_ANALOG_Y2 = 3,
	HAL_ANALOG_L  = 4,
	HAL_ANALOG_R  = 5,
};

/*
 * Callback struct. (to keep ABI)
 */
struct hal_callback {
	/* Callback for initial resource load. */
	bool (*on_start)(void);

	/* Callback for pause. */
	void (*on_pause)(void);

	/* Callback for application quit. */
	void (*on_stop)(void);

	/* Callback for frame update. */
	bool (*on_update)(void);

	/* Callback for frame rendering. */
	void (*on_render)(void);

	/* Callback for keyboard or gamepad press. */
	void (*on_key_press)(int key);

	/* Callback for keyboard or gamepad release. */
	void (*on_key_release)(int key);

	/* Callback for mouse button or one-finger press. */
	void (*on_mouse_press)(int button, int x, int y);

	/* Callback for mouse button or one-finger release. */
	void (*on_mouse_release)(int button, int x, int y);

	/* Callback for mouse move or finger move. */
	void (*on_mouse_move)(int x, int y);

	/* Callback for mouse wheel. */
	void (*on_mouse_wheel)(int v, int h);

	/* Callback for one-finger cancel. (outside the screen) */
	void (*on_touch_cancel)(void);

	/* Callback for gamepad analog input. */
	void (*on_analog_input)(int input, int val);

	/* Callback for two-finger swipe start. (cancels the mouse press) */
	void (*on_swipe_start)(void);

	/* Callback for two-finger swipe down.*/
	void (*on_swipe_down)(float speed, float amount);

	/* Callback for two-finger swipe up.*/
	void (*on_swipe_up)(float speed, float amount);

	void *reserved[48];
};

/* --- */

/*
 * Files and Directories
 */

/*
 * Package file name
 */
#define HAL_PACKAGE_FILE	"assets.arc"

/*
 * Save directory name
 */
#define HAL_SAVE_DIR		"save"

/* --- */

/*
 * File Read
 */

/*
 * File read handle.
 */
struct hal_rfile;

/*
 * Check whether a file exists.
 */
HAL_DLL
bool
hal_check_file_exist(
	const char *file);

/*
 * Open a file stream.
 */
HAL_DLL
bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **f);

/*
 * Enable de-obfuscation on a read file stream.
 */
HAL_DLL
void
hal_decode_rfile(
	struct hal_rfile *f);

/*
 * Get a file size.
 */
HAL_DLL
bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *ret);

/*
 * Read from a file stream.
 */
HAL_DLL
bool
hal_read_rfile(
	struct hal_rfile *rf,
	void *buf,
	size_t size,
	size_t *ret);

/*
 * Read a u64 from a file stream.
 */
HAL_DLL
bool
hal_get_rfile_u64(
	struct hal_rfile *rf,
	uint64_t *data);

/*
 * Read a u32 from a file stream.
 */
HAL_DLL
bool
hal_get_rfile_u32(
	struct hal_rfile *rf,
	uint32_t *data);

/*
 * Read a u16 from a file stream.
 */
HAL_DLL
bool
hal_get_rfile_u16(
	struct hal_rfile *rf,
	uint16_t *data);

/*
 * Read a u8 from a file stream.
 */
HAL_DLL
bool
hal_get_rfile_u8(
	struct hal_rfile *rf,
	uint8_t *data);

/*
 * Read a string from a file stream.
 */
HAL_DLL
bool
hal_get_rfile_string(
	struct hal_rfile *rf,
	char *buf,
	size_t size);

/*
 * Close a file stream.
 */
HAL_DLL
void
hal_close_rfile(
	struct hal_rfile *rf);

/*
 * Rewind a file stream.
 */
HAL_DLL
void
hal_rewind_rfile(
	struct hal_rfile *rf);

/* --- */

/*
 * File Write
 */

/*
 * File write handle.
 */
struct hal_wfile;

/*
 * Open a write file stream.
 */
HAL_DLL
bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **wf);

/*
 * Write to a file stream.
 */
HAL_DLL
bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret);

/*
 * Close a write file stream.
 */
HAL_DLL
void
hal_close_wfile(
	struct hal_wfile *wf);

/*
 * Remove a file.
 */
HAL_DLL
bool
hal_remove_file(
	const char *file);

/* --- */

/*
 * Image
 */

/*
 * Pixel value.
 */
typedef uint32_t hal_pixel_t;

/*
 * Image structure.
 */
struct hal_image {
	/* Members are all private. */

	/* Size. (public) */
	int width;
	int height;

	/* Pixels. */
	hal_pixel_t *pixels;

	/* Externally managed pixels? */
	bool no_free;

	/* Need to upload pixels to GPU? */
	bool need_upload;

	/* Texture pointer. (platform handle) */
	void *texture;

	/* Texture ID. (platform handle) */
	int id;

	/* Context ID. (platform handle) */
	int context;
};

/*
 * Determine the pixel byte order.
 */
#if defined(HAL_TARGET_SOLARIS11) && defined(HAL_ARCH_BE)
#define ORDER_RGBA
#elif !defined(HAL_USE_QT) && \
    ( \
        defined(HAL_TARGET_WINDOWS) || \
        defined(HAL_TARGET_MACOS) || \
        defined(HAL_TARGET_IOS) || \
        defined(HAL_USE_X11_SOFTRENDER) || \
	defined(HAL_USE_FBDEV) || \
        defined(HAL_TARGET_HAIKU) || \
	defined(HAL_TARGET_UNITY) \
    )
	/* Use BGRA on Direct3D, Metal, Unit, and Soft3D. */
#define ORDER_BGRA
#else
	/* Use RGBA on OpenGL. */
#define ORDER_RGBA
#define HAL_ORDER_OPENGL
#endif

/*
 * Compose a pixel value.
 */
static INLINE hal_pixel_t
hal_make_pixel(
	uint32_t a,
	uint32_t r,
	uint32_t g,
	uint32_t b)
{
#ifdef ORDER_RGBA
	return (((hal_pixel_t)a) << 24) | (((hal_pixel_t)b) << 16) | (((hal_pixel_t)g) << 8) | ((hal_pixel_t)r);
#else
	return (((hal_pixel_t)a) << 24) | (((hal_pixel_t)r) << 16) | (((hal_pixel_t)g) << 8) | ((hal_pixel_t)b);
#endif
}

/*
 * Get an alpha component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_a(
	hal_pixel_t p)
{
	return (p >> 24) & 0xff;
}

/*
 * Get a red component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_r(
	hal_pixel_t p)
{
#ifdef ORDER_RGBA
	return p & 0xff;
#else
	return (p >> 16) & 0xff;
#endif
}

/*
 * Get a green component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_g(
	hal_pixel_t p)
{
	return (p >> 8) & 0xff;
}

/*
 * Get a blue component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_b(
	hal_pixel_t p)
{
#ifdef ORDER_RGBA
	return (p >> 16) & 0xff;
#else
	return p & 0xff;
#endif
}

/*
 * Get a fixed component slot 1 from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_c1(
	hal_pixel_t p)
{
	return (p >> 16) & 0xff;
}

/*
 * Get a fixed component slot 2 from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_c2(
	hal_pixel_t p)
{
	return (p >> 8) & 0xff;
}

/*
 * Get a fixed component slot 3 from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_c3(
	hal_pixel_t p)
{
	return p & 0xff;
}

/*
 * Compose a pixel value from fixed component slots.
 */
static INLINE hal_pixel_t
hal_make_pixel_fast(
	uint32_t a,
	uint32_t c1,
	uint32_t c2,
	uint32_t c3)
{
	return (((hal_pixel_t)a) << 24) | (((hal_pixel_t)c1) << 16) | (((hal_pixel_t)c2) << 8) | ((hal_pixel_t)c3);
}

#undef ORDER_RGBA
#undef ORDER_BGRA

/*
 * Create an image.
 */
HAL_DLL
bool
hal_create_image(
	int w,
	int h,
	struct hal_image **img);

/*
 * Create an image with a pixel buffer.
 */
HAL_DLL
bool
hal_create_image_with_pixels(
	int w,
	int h,
	hal_pixel_t *pixels,
	struct hal_image **img);

/*
 * Create an image with a PNG file.
 */
HAL_DLL
bool
hal_create_image_with_png(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Create an image with a JPEG file.
 */
HAL_DLL
bool
hal_create_image_with_jpeg(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Create an image with a WebP file.
 */
HAL_DLL
bool
hal_create_image_with_webp(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Destroy an image.
 */
HAL_DLL
void
hal_destroy_image(
	struct hal_image *img);

/*
 * Get an image width.
 */
HAL_DLL
int
hal_get_image_width(
	struct hal_image *img);

/*
 * Get an image height.
 */
HAL_DLL
int
hal_get_image_height(
	struct hal_image *img);

/*
 * Get image pixels.
 */
HAL_DLL
hal_pixel_t *
hal_get_image_pixels(
	struct hal_image *img);

/*
 * Clear an image with a uniform color.
 */
HAL_DLL
void
hal_clear_image(
	struct hal_image *img,
	hal_pixel_t color);

/*
 * Clear an image rectangle with a uniform color.
 */
HAL_DLL
void
hal_clear_image_rect(
	struct hal_image *img,
	int x,
	int y,
	int w,
	int h,
	hal_pixel_t color);

/*
 * Clear alpha channel with 255.
 */
HAL_DLL
void
hal_fill_image_alpha(
	struct hal_image *img);

/*
 * Draw an image on an image. (copy)
 */
HAL_DLL
void
hal_draw_image_copy(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top);

/*
 * Draw an image on an image. (alpha-blending, dst_alpha=255)
 */
HAL_DLL
void
hal_draw_image_alpha(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image on an image. (add-blending)
 */
HAL_DLL
void
hal_draw_image_add(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image on an image. (add-blending)
 */
HAL_DLL
void
hal_draw_image_sub(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a glyph image on an image. (alphablending, special alpha value)
 */
HAL_DLL
void
hal_draw_image_glyph(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an emoji image on an image. (alphablending, special alpha value)
 */
HAL_DLL
void
hal_draw_image_emoji(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image on an image. (50% dimming)
 */
HAL_DLL
void
hal_draw_image_dim(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image with 1-bit rule image.
 */
HAL_DLL
void
hal_draw_image_rule(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold);

/*
 * Draw an image with 8-bit rule image.
 */
HAL_DLL
void
hal_draw_image_melt(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold);

/*
 * Draw two images for a cross fading.
 */
HAL_DLL
void
hal_draw_image_cross(
	struct hal_image *dst_image,
	struct hal_image *src1_img,
	struct hal_image *src2_img,
	int src1_left,
	int src1_top,
	int src2_left,
	int src2_top,
	int alpha);

/*
 * Draw an image on an image. (3D)
 */
HAL_DLL
void
hal_draw_image_3d_alpha(
	struct hal_image *dst_image,
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
	int alpha);

/*
 * Draw an image on an image. (3D)
 */
HAL_DLL
void
hal_draw_image_3d_add(
	struct hal_image *dst_image,
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
	int alpha);

/*
 * Draw an image on an image. (3D)
 */
HAL_DLL
void
hal_draw_image_3d_sub(
	struct hal_image *dst_image,
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
	int alpha);

/*
 * Draw an image on an image. (3D)
 */
HAL_DLL
void
hal_draw_image_3d_dim(
	struct hal_image *dst_image,
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
	int alpha);

/*
 * Draw two images for a cross fading.
 */
HAL_DLL
void
hal_draw_image_3d_cross(
	struct hal_image *dst_img,
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
	int alpha);

/*
 * Clip a rectangle by a source size.
 */
HAL_DLL
bool
hal_clip_by_source(
	int src_cx,
	int src_cy,
	int *cx,
	int *cy,
	int *dst_x,
	int *dst_y, int *src_x, int *src_y);

/*
 * Clip a rectangle by a destination size.
 */
HAL_DLL
bool
hal_clip_by_dest(
	int dst_cx,
	int dst_cy,
	int *cx,
	int *cy,
	int *dst_x,
	int *dst_y,
	int *src_x,
	int *src_y);

/*
 * Notify an image update.
 *  - This function tells a HAL that an image needs to be uploaded to GPU.
 */
HAL_DLL
void
hal_notify_image_update(
	struct hal_image *img);

/*
 * Notify an image free.
 *  - This function tells a HAL that an image is no longer used.
 *  - This function must be called from destroy_image() only.
 */
HAL_DLL
void
hal_notify_image_free(
	struct hal_image *img);

/* --- */

/*
 * Glyph
 */

/*
 * Maximum font count.
 */
#define HAL_GLYPH_DATA_COUNT	(4)

/*
 * Load a glyph data. (TTF)
 */
HAL_DLL
bool
hal_load_glyph_data(
	int slot,
	const uint8_t *data,
	size_t len);

/*
 * Destroy a glyph data.
 */
HAL_DLL
void
hal_destroy_glyph_data(
	int index);

/*
 * Get a top character of a utf-8 string as utf-32.
 */
HAL_DLL
int
hal_utf8_to_utf32(
	const char *mbs,
	uint32_t *wc);

/*
 * Get a character count of a utf-8 string.
 */
HAL_DLL
int
hal_count_utf8_chars(
	const char *mbs);

/*
 * Get a width for a character.
 */
HAL_DLL
int
hal_get_glyph_width(
	int slot,
	int size,
	uint32_t codepoint);

/*
 * Get a height for a character.
 */
HAL_DLL
int
hal_get_glyph_height(
	int slot,
	int size,
	uint32_t codepoint);

/*
 * Get a width and a height for a character.
 */
HAL_DLL
void
hal_get_glyph_width_and_height(
	int slot,
	int size,
	uint32_t codepoint,
	int *width,
	int *height);

/*
 * Get a width for a string.
 */
HAL_DLL
int
hal_get_string_width(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Get a height for a string.
 */
HAL_DLL
int
hal_get_string_height(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Get a width and a height for a string.
 */
HAL_DLL
void
hal_get_string_width_and_height(
	int slot,
	int size,
	const char *mbs,
	int *width,
	int *height);

/*
 * Draw a character.
 */
HAL_DLL
bool
hal_draw_glyph(
	struct hal_image *img,
	int font_index,
	int font_size,
	int base_font_size,
	int outline_size,
	int x,
	int y,
	hal_pixel_t color,
	hal_pixel_t outline_color,
	uint32_t codepoint,
	int *ret_w,
	int *ret_h,
	bool is_dim);

/* --- */

/*
 * PCM Sound Stream
 */

/*
 * PCM stream handle.
 */
struct hal_wave;

/*
 * Create a wave stream from a file.
 */
HAL_DLL
bool
hal_create_wave_from_file(
	const char *file,
	bool loop,
	struct hal_wave **w);

/*
 * Destroy a wave stream.
 */
HAL_DLL
void
hal_destroy_wave(
	struct hal_wave *w);

/*
 * Set a repeat count of a wave stream.
 */
HAL_DLL
void
hal_set_wave_repeat_times(
	struct hal_wave *w,
	int n);

/*
 * Get whether a wave stream is reached end-of-stream or not.
 */
HAL_DLL
bool
hal_is_wave_eos(
	struct hal_wave *w);

/*
 * Get PCM samples from a wave stream.
 */
HAL_DLL
int
hal_get_wave_samples(
	struct hal_wave *w,
	uint32_t *buf,
	int samples);

/* --- */

/*
 * On-Screen Rendering
 */

/*
 * Render an image to the screen with the "normal" shader pipeline.
 *  - The "normal" shader pipeline renders pixels with alpha blending.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen with the "add" shader pipeline.
 *  - The "add" shader pipeline renders pixels with add blending.
 */
HAL_DLL
void
hal_render_image_add(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen with the "sub" shader pipeline.
 *  - The "sub" shader pipeline renders pixels with subtract blending.
 */
HAL_DLL
void
hal_render_image_sub(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen with the "dim" shader pipeline.
 *  - The "dim" shader pipeline renders pixels at 50% value for each RGB component.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen with the "rule" shader pipeline.
 *  - The "rule" shader pipeline is a variation of "universal transition" with a threshold value.
 *  - A rule image must have the same size as the screen.
 */
HAL_DLL
void
hal_render_image_rule(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int threshold);			/* The threshold (0 to 255) */

/*
 * Render an image to the screen with the "melt" shader pipeline.
 *  - The "melt" shader pipeline is a variation of "universal transition" with a progress value
 *  - A rule image must have the same size as the screen
 */
HAL_DLL
void
hal_render_image_melt(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int progress);			/* The progress (0 to 255) */

/*
 * Render two images for a cross fading.
 */
HAL_DLL
void
hal_render_image_cross(
	struct hal_image *src1_img,	/* [IN] The source image 1 */
	struct hal_image *src2_img,	/* [IN] The source image 2 */
	float src1_left,
	float src1_top,
	float src2_left,
	float src2_top,
	int alpha);

/*
 * Render an image to the screen as a triangle strip with the "normal" shader pipeline.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen as a triangle strip with the "add" shader pipeline.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen as a triangle strip with the "sub" shader pipeline.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render an image to the screen as a triangle strip with the "dim" shader pipeline.
 */
HAL_DLL
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
	int alpha);			/* The alpha value (0 to 255) */

/*
 * Render two images for a cross fading.
 */
HAL_DLL
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
	int alpha);

/* --- */

/*
 * Lap Timer
 */

/*
 * Reset a lap timer and initializes it with a current time.
 */
HAL_DLL
void
hal_reset_lap_timer(
	uint64_t *origin);

/*
 * Get a lap time in milliseconds.
 */
HAL_DLL
uint64_t
hal_get_lap_timer_millisec(
	uint64_t *origin);

/* --- */

/*
 * Sound Playback
 */

/*
 * Sound tracks.
 */
#define HAL_SOUND_BGM		(0)
#define HAL_SOUND_SE		(1)
#define HAL_SOUND_VOICE		(2)
#define HAL_SOUND_SYS1		(3)		
#define HAL_SOUND_SYS2		(4)
#define HAL_SOUND_TRACKS	(5)

/*
 * Start playing a sound file on a track.
 */
HAL_DLL
bool
hal_play_sound(
	int stream,		/* A sound stream index */
	struct hal_wave *w);	/* [IN] A sound object, ownership will be delegated to the callee */

/*
 * Stop playing a sound track.
 */
HAL_DLL
bool
hal_stop_sound(
	int stream);

/*
 * Set sound volume.
 */
HAL_DLL
bool
hal_set_sound_volume(
	int stream,
	float vol);

/*
 * Return whether a sound playback for a stream is already finished.
 */
HAL_DLL
bool
hal_is_sound_finished(
	int stream);

/* --- */

/*
 * Video Playback
 */

/*
 * Start playing a video file.
 */
HAL_DLL
bool
hal_play_video(
	const char *fname,	/* File name */
	bool is_skippable);	/* Allow skip for a unseen video */

/*
 * Stop playing music stream.
 */
HAL_DLL
void
hal_stop_video(void);

/*
 * Return whether a video playback is running.
 */
HAL_DLL
bool
hal_is_video_playing(void);

/* --- */

/*
 * Window Manipulation
 */

/*
 * Return whether the current HAL supports the "full screen mode".
 *  - The "full screen mode" includes the dock-in of some game consoles.
 *  - A HAL can implement the "full screen mode" but it is optional.
 */
HAL_DLL
bool
hal_is_full_screen_supported(void);

/*
 * Return whether the current HAL is in the "full screen mode".
 */
HAL_DLL
bool
hal_is_full_screen_mode(void);

/*
 * Enter the full screen mode.
 *  - A HAL can ignore this call.
 */
HAL_DLL
void
hal_enter_full_screen_mode(void);

/*
 * Leave the full screen mode.
 *  - A HAL can ignore this call.
 */
HAL_DLL
void
hal_leave_full_screen_mode(void);

/* --- */

/*
 * Locale
 */

/*
 * Gets the system language.
 *  - Return value can be:
 *    - "en"      ... English
 *    - "en-us"   ... English (America)
 *    - "en-gb"   ... English (Britain)
 *    - "es"      ... Spanish
 *    - "es-es"   ... Spanish (Spain)
 *    - "es-la"   ... Spanish (Latin America)
 *    - "fr"      ... French
 *    - "fr-fr"   ... French (France)
 *    - "fr-ca"   ... French (Canada)
 *    - "it"      ... Italian
 *    - "de"      ... Deutsch
 *    - "el"      ... Greek
 *    - "ru"      ... Russian
 *    - "zh-Hans" ... Simplified Chinese
 *    - "zh-Hant" ... Simplified Chinese
 *    - "ja":      Japanese
 */
HAL_DLL
const char *
hal_get_system_language(void);

/* --- */

/*
 * Touch Screen
 */

/*
 * Enable/disable message skip by touch move.
 */
HAL_DLL
void
hal_set_continuous_swipe_enabled(
	bool is_enabled);

/* --- */

/*
 * Logging
 * - Note that non-main threads including sound threads cannot call
 *   the logging functions.
 */

/*
 * Put an "info" level log with printf formats.
 */
HAL_DLL
bool
hal_log_info(
	const char *s,
	...);

/*
 * Put a "warn" level log with printf formats.
 */
HAL_DLL
bool
hal_log_warn(
	const char *s,
	...);

/*
 * Put an "error" level log with printf formats.
 */
HAL_DLL
bool
hal_log_error(
	const char *s,
	...);

/*
 * Log out-of-memory.
 */
HAL_DLL
bool
hal_log_out_of_memory(void);

#ifdef __cplusplus
}
#endif

/*
 * Entrypoint
 */

/*
 * Callback for bootstrap.
 */

/* DLL side. */
HAL_DLL
extern bool
(*hal_bootstrap_ptr)(
	char **title,
	int *width,
	int *height,
	struct hal_callback *callback);

/* App side. */
extern bool
hal_bootstrap(
	char **title,
	int *width,
	int *height,
	struct hal_callback *callback);

#if defined(HAL_TARGET_LINUX)		||	\
    defined(HAL_TARGET_FREEBSD)		||	\
    defined(HAL_TARGET_NETBSD)		||	\
    defined(HAL_TARGET_OPENBSD)		||	\
    defined(HAL_TARGET_SOLARIS10)	||	\
    defined(HAL_TARGET_SOLARIS11)	||	\
    defined(HAL_TARGET_POSIX)		||	\
    defined(HAL_TARGET_MACOS)		||	\
    defined(HAL_TARGET_IOS)             ||      \
    defined(HAL_TARGET_PC98)            ||      \
    defined(HAL_TARGET_PCAT)
#define HAL_DEFINE_MAIN()				\
	int main(int argc, char *argv[])		\
	{						\
		int hal_main(int argc, char *argv[]);	\
		hal_bootstrap_ptr = hal_bootstrap;	\
		return hal_main(argc, argv);		\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	int main(int argc, char *argv[])				\
	{								\
		int hal_main(int argc, char *argv[]);			\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
		return hal_main(argc, argv);				\
	}
#endif

#if defined(HAL_TARGET_MACOS7)
#define HAL_DEFINE_MAIN()				\
	int main(void)					\
	{						\
		int hal_main(int argc, char *argv[]);	\
		hal_bootstrap_ptr = hal_bootstrap;	\
		return hal_main(argc, argv);		\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	int main(void)							\
	{								\
		int hal_main(int argc, char *argv[]);			\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
		return hal_main(1, NULL);				\
	}
#endif

#if defined(HAL_TARGET_WINDOWS) && defined(_UNICODE)
#include <windows.h>
#define HAL_DEFINE_MAIN()					\
	int WINAPI wWinMain(					\
		HINSTANCE hInstance,				\
		HINSTANCE hPrevInstance,			\
		LPWSTR lpszCmd,					\
		int nCmdShow)					\
	{							\
		int WINAPI hal_wWinMain(HINSTANCE,		\
					HINSTANCE,		\
					LPWSTR,			\
					int);			\
		hal_bootstrap_ptr = hal_bootstrap;		\
		return hal_wWinMain(hInstance,			\
				    hPrevInstance,		\
				    lpszCmd,			\
				    nCmdShow);			\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	int WINAPI wWinMain(						\
		HINSTANCE hInstance,					\
		HINSTANCE hPrevInstance,				\
		LPWSTR lpszCmd,						\
		int nCmdShow)						\
	{								\
		int WINAPI hal_wWinMain(HINSTANCE,			\
					HINSTANCE,			\
					LPWSTR,				\
					int);				\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
		return hal_wWinMain(hInstance,				\
				    hPrevInstance,			\
				    lpszCmd,				\
				    nCmdShow);				\
	}
#endif

#if defined(HAL_TARGET_WINDOWS) && !defined(_UNICODE)
#include <windows.h>
#define HAL_DEFINE_MAIN()					\
	int WINAPI WinMain(					\
		HINSTANCE hInstance,				\
		HINSTANCE hPrevInstance,			\
		LPSTR lpszCmd,					\
		int nCmdShow)					\
	{							\
		int WINAPI hal_WinMain(HINSTANCE,		\
				       HINSTANCE,		\
				       LPWSTR,			\
				       int);			\
		hal_bootstrap_ptr = hal_bootstrap;		\
		return hal_WinMain(hInstance,			\
				   hPrevInstance,		\
				   lpszCmd,			\
				   nCmdShow);			\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
int WINAPI WinMain(							\
		HINSTANCE hInstance,					\
		HINSTANCE hPrevInstance,				\
		LPSTR lpszCmd,						\
		int nCmdShow)						\
	{								\
		int WINAPI hal_WinMain(HINSTANCE,			\
				       HINSTANCE,			\
				       LPWSTR,				\
				       int);				\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
		return hal_WinMain(hInstance,				\
				   hPrevInstance,			\
				   lpszCmd,				\
				   nCmdShow);				\
	}
#endif

#if defined(HAL_TARGET_ANDROID) ||				\
    defined(HAL_TARGET_OPENHARMONY) ||				\
    (defined(HAL_TARGET_UNITY) && defined(HAL_USE_DLL))
#define HAL_DEFINE_MAIN()					\
	__attribute__((constructor))				\
	static void so_init(void)				\
	{							\
		hal_bootstrap_ptr = hal_bootstrap;		\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	__attribute__((constructor))					\
	static void so_init(void)					\
	{								\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
	}
#endif

#if defined(HAL_TARGET_UNITY)
#define HAL_DEFINE_MAIN()						\
	void so_init(void)						\
	{								\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	void so_init(void)						\
	{								\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
	}
#endif

#if defined(HAL_TARGET_WASM)
#define HAL_DEFINE_MAIN()					\
	int main(void)						\
	{							\
		int hal_main(void);				\
		hal_bootstrap_ptr = hal_bootstrap;		\
		return hal_main();				\
	}
#define HAL_DEFINE_MAIN_CHAIN(chain_ptr1, chain1, chain_ptr2, chain2)	\
	int main(void)							\
	{								\
		int hal_main(void);					\
		hal_bootstrap_ptr = hal_bootstrap;			\
		chain_ptr1 = chain1;					\
		chain_ptr2 = chain2;					\
		return hal_main();					\
	}
#endif

#endif
