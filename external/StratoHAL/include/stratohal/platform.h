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

#ifndef STRATOHAL_PLATFORM_H
#define STRATOHAL_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stratohal/c89compat.h"

/*************************
 * Files and Directories *
 *************************/

/* Package file name */
#define HAL_PACKAGE_FILE	"assets.arc"

/* Save directory name */
#define HAL_SAVE_DIR		"save"

/*************
 * File Read *
 *************/

struct hal_rfile;

/*
 * Check whether a file exists.
 */
bool
hal_check_file_exist(
	const char *file);

/*
 * Open a file stream.
 */
bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **f);

/*
 * Enable de-obfuscation on a read file stream.
 */
void
hal_decode_rfile(
	struct hal_rfile *f);

/*
 * Get a file size.
 */
bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *ret);

/*
 * Read from a file stream.
 */
bool
hal_read_rfile(
	struct hal_rfile *rf,
	void *buf,
	size_t size,
	size_t *ret);

/*
 * Read a u64 from a file stream.
 */
bool
hal_get_rfile_u64(
	struct hal_rfile *rf,
	uint64_t *data);

/*
 * Read a u32 from a file stream.
 */
bool
hal_get_rfile_u32(
	struct hal_rfile *rf,
	uint32_t *data);

/*
 * Read a u16 from a file stream.
 */
bool
hal_get_rfile_u16(
	struct hal_rfile *rf,
	uint16_t *data);

/*
 * Read a u8 from a file stream.
 */
bool
hal_get_rfile_u8(
	struct hal_rfile *rf,
	uint8_t *data);

/*
 * Read a string from a file stream.
 */
bool
hal_get_rfile_string(
	struct hal_rfile *rf,
	char *buf,
	size_t size);

/*
 * Close a file stream.
 */
void
hal_close_rfile(
	struct hal_rfile *rf);

/*
 * Rewind a file stream.
 */
void
hal_rewind_rfile(
	struct hal_rfile *rf);

/**************
 * File Write *
 **************/

struct hal_wfile;

/*
 * Open a write file stream.
 */
bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **wf);

/*
 * Write to a file stream.
 */
bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret);

/*
 * Close a write file stream.
 */
void
hal_close_wfile(
	struct hal_wfile *wf);

/*
 * Remove a file.
 */
bool
hal_remove_file(
	const char *file);

/*********
 * Image *
 *********/

/*
 * Pixel value type.
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

#if !defined(HAL_USE_QT) && \
    ( \
        defined(HAL_TARGET_WINDOWS) || \
        defined(HAL_TARGET_MACOS) || \
        defined(HAL_TARGET_IOS) || \
        (defined(HAL_TARGET_POSIX) && defined(HAL_USE_X11_SOFTRENDER)) || \
	defined(HAL_TARGET_UNITY) \
    )
#define ORDER_BGRA	/* Use BGRA on Direct3D and Metal. */
#else
#define ORDER_RGBA	/* Use RGBA on OpenGL. */
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
	return (p >> 16) & 0xff;
#else
	return p & 0xff;
#endif
}

/*
 * Get a green component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_g(
	hal_pixel_t p)
{
#ifdef ORDER_RGBA
	return (p >> 8) & 0xff;
#else
	return (p >> 8) & 0xff;
#endif
}

/*
 * Get a blue component value from a pixel value.
 */
static INLINE uint32_t
hal_get_pixel_b(
	hal_pixel_t p)
{
#ifdef ORDER_RGBA
	return p & 0xff;
#else
	return (p >> 16) & 0xff;
#endif
}

#undef ORDER_RGBA
#undef ORDER_BGRA

/*
 * Create an image.
 */
bool
hal_create_image(
	int w,
	int h,
	struct hal_image **img);

/*
 * Create an image with a pixel buffer.
 */
bool
hal_create_image_with_pixels(
	int w,
	int h,
	hal_pixel_t *pixels,
	struct hal_image **img);

/*
 * Create an image with a PNG file.
 */
bool
hal_create_image_with_png(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Create an image with a JPEG file.
 */
bool
hal_create_image_with_jpeg(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Create an image with a WebP file.
 */
bool
hal_create_image_with_webp(
	const uint8_t *data,
	size_t size,
	struct hal_image **img);

/*
 * Destroy an image.
 */
void
hal_destroy_image(
	struct hal_image *img);

/*
 * Get an image width.
 */
int
hal_get_image_width(
	struct hal_image *img);

/*
 * Get an image height.
 */
int
hal_get_image_height(
	struct hal_image *img);

/*
 * Get image pixels.
 */
hal_pixel_t *
hal_get_image_pixels(
	struct hal_image *img);

/*
 * Clear an image with a uniform color.
 */
void
hal_clear_image(
	struct hal_image *img,
	hal_pixel_t color);

/*
 * Clear an image rectangle with a uniform color.
 */
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
void
hal_fill_image_alpha(
	struct hal_image *img);

/*
 * Draw an image on an image. (copy)
 */
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
void
hal_draw_image_rule(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold);

/*
 * Draw an image with 8-bit rule image.
 */
void
hal_draw_image_melt(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold);

/*
 * Draw an image with scaling.
 */
void
hal_draw_image_scale(
	struct hal_image *dst_image,
	int virtual_dst_width,
	int virtual_dst_height,
	int virtual_dst_left,
	int virtual_dst_top,
	struct hal_image *src_image);

/*
 * Clip a rectangle by a source size.
 */
bool
hal_clip_by_source(
	int src_cx,
	int src_cy,
	int *cx,
	int *cy,
	int *dst_x,
	int *dst_y, int *src_x, int *src_y);

/* Clip a rectangle by a destination size. */
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

/*********
 * Glyph *
 *********/

/*
 * Unicode Codepoints
 */
#define HAL_CHAR_TOUTEN		(0x3001)
#define HAL_CHAR_KUTEN		(0x3002)
#define HAL_CHAR_YENSIGN	(0x00a5)

/*
 * Maximum Font Index
 */
#define HAL_GLYPH_DATA_COUNT	(4)

/*
 * Initialize the font renderer.
 */
bool
hal_init_glyph(void);

/*
 * Cleanup the font renderer.
 */
void
hal_cleanup_glyph(void);

/*
 * Load a glyph data. (TTF)
 */
bool
hal_load_glyph_data(
	int slot,
	const uint8_t *data,
	size_t len);

/*
 * Destroy a glyph data.
 */
void
hal_destroy_glyph_data(
	int index);

/*
 * Get a top character of a utf-8 string as utf-32.
 */
int
hal_utf8_to_utf32(
	const char *mbs,
	uint32_t *wc);

/*
 * Get a character count of a utf-8 string.
 */
int
hal_count_utf8_chars(
	const char *mbs);

/*
 * Get a width for a character.
 */
int
hal_get_glyph_width(
	int slot,
	int size,
	uint32_t codepoint);

/*
 * Get a height for a character.
 */
int
hal_get_glyph_height(
	int slot,
	int size,
	uint32_t codepoint);

/*
 * Get a width and a height for a character.
 */
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
int
hal_get_string_width(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Get a height for a string.
 */
int
hal_get_string_height(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Get a width and a height for a string.
 */
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

/*********
 * Sound *
 *********/

/*
 * PCM Stream
 */
struct hal_wave;

/*
 * Create a wave stream from a file.
 */
bool
hal_create_wave_from_file(
	const char *file,
	bool loop,
	struct hal_wave **w);

/*
 * Destroy a wave stream.
 */
void
hal_destroy_wave(
	struct hal_wave *w);

/*
 * Set a repeat count of a wave stream.
 */
void
hal_set_wave_repeat_times(
	struct hal_wave *w,
	int n);

/*
 * Get whether a wave stream is reached end-of-stream or not.
 */
bool
hal_is_wave_eos(
	struct hal_wave *w);

/*
 * Get PCM samples from a wave stream.
 */
int
hal_get_wave_samples(
	struct hal_wave *w,
	uint32_t *buf,
	int samples);

/************************
 * Texture Manipulation *
 ************************/

/*
 * Notify an image update.
 *  - This function tells a HAL that an image needs to be uploaded to GPU.
 */
void
hal_notify_image_update(
	struct hal_image *img);

/*
 * Notify an image free.
 *  - This function tells a HAL that an image is no longer used.
 *  - This function must be called from destroy_image() only.
 */
void
hal_notify_image_free(
	struct hal_image *img);

/*
 * Return if RGBA values have to be reversed to BGRA.
 */
#if defined(HAL_TARGET_ANDROID) || \
    defined(HAL_TARGET_WASM) || \
    (defined(HAL_TARGET_LINUX) && (defined(HAL_USE_X11_OPENGL) || defined(HAL_USE_QT)))
#define IS_OPENGL_BYTE_ORDER()	true
#else
#define IS_OPENGL_BYTE_ORDER()	false
#endif

/*************
 * Rendering *
 *************/

/*
 * Render an image to the screen with the "normal" shader pipeline.
 *  - The "normal" shader pipeline renders pixels with alpha blending.
 */
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
void
hal_render_image_melt(
	struct hal_image *src_img,	/* [IN] The source image */
	struct hal_image *rule_img,	/* [IN] The rule image */
	int progress);			/* The progress (0 to 255) */

/*
 * Render an image to the screen as a triangle strip with the "normal" shader pipeline.
 */
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

/*************
 * Lap Timer *
 *************/

/*
 * Reset a lap timer and initializes it with a current time.
 */
void
hal_reset_lap_timer(
	uint64_t *origin);

/*
 * Get a lap time in milliseconds.
 */
uint64_t
hal_get_lap_timer_millisec(
	uint64_t *origin);

/******************
 * Sound Playback *
 ******************/

/*
 * Sound Tracks
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
bool
hal_play_sound(
	int stream,		/* A sound stream index */
	struct hal_wave *w);	/* [IN] A sound object, ownership will be delegated to the callee */

/*
 * Stop playing a sound track.
 */
bool
hal_stop_sound(
	int stream);

/*
 * Set sound volume.
 */
bool
hal_set_sound_volume(
	int stream,
	float vol);

/*
 * Return whether a sound playback for a stream is already finished.
 */
bool
hal_is_sound_finished(
	int stream);

/******************
 * Video Playback *
 ******************/

/*
 * Start playing a video file.
 */
bool
hal_play_video(
	const char *fname,	/* File name */
	bool is_skippable);	/* Allow skip for a unseen video */

/*
 * Stop playing music stream.
 */
void
hal_stop_video(void);

/*
 * Return whether a video playback is running.
 */
bool
hal_is_video_playing(void);

/***********************
 * Window Manipulation *
 ***********************/

/*
 * Return whether the current HAL supports the "full screen mode".
 *  - The "full screen mode" includes the dock-in of some game consoles.
 *  - A HAL can implement the "full screen mode" but it is optional.
 */
bool
hal_is_full_screen_supported(void);

/*
 * Return whether the current HAL is in the "full screen mode".
 */
bool
hal_is_full_screen_mode(void);

/*
 * Enter the full screen mode.
 *  - A HAL can ignore this call.
 */
void
hal_enter_full_screen_mode(void);

/*
 * Leave the full screen mode.
 *  - A HAL can ignore this call.
 */
void
hal_leave_full_screen_mode(void);

/*********************
 * Path Manipulation *
 *********************/

/*
 * Create a save directory if it does not exist.
 */
bool
hal_make_save_directory(void);

/*
 * Creates a real path string from a file name.
 *  - Return value must be freed by callers with free().
 */
char *
hal_make_real_path(const char *fname);

/***********
 * Logging *
 ***********/

/*
 * Note that sound threads cannot use these logging functions.
 */

/*
 * Put a "info" level log with printf formats.
 */
bool
hal_log_info(
	const char *s,
	...);

/*
 * Put a "warn" level log with printf formats.
 */
bool
hal_log_warn(
	const char *s,
	...);

/*
 * Put an "error" level log with printf formats.
 */
bool
hal_log_error(
	const char *s,
	...);

/*
 * Log out-of-memory.
 */
bool
hal_log_out_of_memory(void);

/**********
 * Locale *
 **********/

/*
 * Gets the system language.
 *  - Return value can be:
 *    - "ja": Japanese
 *    - "en": English
 *    - "zh": Simplified Chinese
 *    - "tw": Traditional Chinese
 *    - "fr": French
 *    - "it": Italian
 *    - "es": Spanish (Castellano)
 *    - "de": Deutsch
 *    - "el": Greek
 *    - "ru": Russian
 *    - "other": Other (must fallback to English)
 */
const char *
hal_get_system_language(void);

/****************
 * Touch Screen *
 ****************/

/*
 * Enable/disable message skip by touch move.
 */
void
hal_set_continuous_swipe_enabled(
	bool is_enabled);

/*************
 * Callbacks *
 *************/

/*
 * Callbacks are defined outside the HAL.
 */

/* Keycode. */
enum hal_key_code {
	HAL_KEY_ESCAPE,
	HAL_KEY_RETURN,
	HAL_KEY_SPACE,
	HAL_KEY_TAB,
	HAL_KEY_BACKSPACE,
	HAL_KEY_DELETE,
	HAL_KEY_HOME,
	HAL_KEY_END,
	HAL_KEY_PAGEUP,
	HAL_KEY_PAGEDOWN,
	HAL_KEY_SHIFT,
	HAL_KEY_CONTROL,
	HAL_KEY_ALT,
	HAL_KEY_UP,
	HAL_KEY_DOWN,
	HAL_KEY_LEFT,
	HAL_KEY_RIGHT,
	HAL_KEY_A,
	HAL_KEY_B,
	HAL_KEY_C,
	HAL_KEY_D,
	HAL_KEY_E,
	HAL_KEY_F,
	HAL_KEY_G,
	HAL_KEY_H,
	HAL_KEY_I,
	HAL_KEY_J,
	HAL_KEY_K,
	HAL_KEY_L,
	HAL_KEY_M,
	HAL_KEY_N,
	HAL_KEY_O,
	HAL_KEY_P,
	HAL_KEY_Q,
	HAL_KEY_R,
	HAL_KEY_S,
	HAL_KEY_T,
	HAL_KEY_U,
	HAL_KEY_V,
	HAL_KEY_W,
	HAL_KEY_X,
	HAL_KEY_Y,
	HAL_KEY_Z,
	HAL_KEY_1,
	HAL_KEY_2,
	HAL_KEY_3,
	HAL_KEY_4,
	HAL_KEY_5,
	HAL_KEY_6,
	HAL_KEY_7,
	HAL_KEY_8,
	HAL_KEY_9,
	HAL_KEY_0,
	HAL_KEY_F1,
	HAL_KEY_F2,
	HAL_KEY_F3,
	HAL_KEY_F4,
	HAL_KEY_F5,
	HAL_KEY_F6,
	HAL_KEY_F7,
	HAL_KEY_F8,
	HAL_KEY_F9,
	HAL_KEY_F10,
	HAL_KEY_F11,
	HAL_KEY_F12,
	HAL_KEY_GAMEPAD_UP,
	HAL_KEY_GAMEPAD_DOWN,
	HAL_KEY_GAMEPAD_LEFT,
	HAL_KEY_GAMEPAD_RIGHT,
	HAL_KEY_GAMEPAD_A,
	HAL_KEY_GAMEPAD_B,
	HAL_KEY_GAMEPAD_X,
	HAL_KEY_GAMEPAD_Y,
	HAL_KEY_GAMEPAD_L,
	HAL_KEY_GAMEPAD_R,
	HAL_KEY_MAX,
};

/* Mouse button. */
enum hal_mouse_button {
	HAL_MOUSE_LEFT,
	HAL_MOUSE_RIGHT,
};

/* Analog input. */
enum hal_analog_input {
	HAL_ANALOG_X1,
	HAL_ANALOG_Y1,
	HAL_ANALOG_X2,
	HAL_ANALOG_Y2,
	HAL_ANALOG_L,
	HAL_ANALOG_R,
};

/* Callbacks. */
bool hal_callback_on_event_boot(char **title, int *width, int *height);
bool hal_callback_on_event_start(void);
void hal_callback_on_event_stop(void);
bool hal_callback_on_event_frame(void);
void hal_callback_on_event_key_press(int key);
void hal_callback_on_event_key_release(int key);
void hal_callback_on_event_mouse_press(int button, int x, int y);
void hal_callback_on_event_mouse_release(int button, int x, int y);
void hal_callback_on_event_mouse_move(int x, int y);
void hal_callback_on_event_analog_input(int input, int val);
void hal_callback_on_event_touch_cancel(void);
void hal_callback_on_event_swipe_down(void);
void hal_callback_on_event_swipe_up(void);

/***************************
 * Foreign Language Support *
 ***************************/

#if !defined(HAL_USE_CSHARP) && !defined(HAL_USE_SWIFT)
#define UNSAFEPTR(t) t
#endif

/*
 * Swift Support (Swift Function Pointers)
 */
#if defined(HAL_USE_SWIFT)
#define UNSAFEPTR(t)  t	/* For Swift */
extern void (*wrap_log_info)(UNSAFEPTR(const char *) s);
extern void (*wrap_log_warn)(UNSAFEPTR(const char *) s);
extern void (*wrap_log_error)(UNSAFEPTR(const char *) s);
extern void (*wrap_make_save_directory)(void);
extern void (*wrap_make_real_path)(UNSAFEPTR(const char *) dir, UNSAFEPTR(const char *) fname, UNSAFEPTR(const char *) dst, int len);
extern void (*wrap_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels);
extern void (*wrap_notify_image_free)(int id);
extern void (*wrap_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_rule)(int src_img, int rule_img, int threshold);
extern void (*wrap_render_image_melt)(int src_img, int rule_img, int progress);
extern void (*wrap_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
extern void (*wrap_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin);
extern uint64_t (*wrap_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin);
extern void (*wrap_play_sound)(int stream, UNSAFEPTR(void *) wave);
extern void (*wrap_stop_sound)(int stream);
extern void (*wrap_set_sound_volume)(int stream, float vol);
extern int (*wrap_is_sound_finished)(int stream);
extern bool (*wrap_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable);
extern void (*wrap_stop_video)(void);
extern bool (*wrap_is_video_playing)(void);
extern bool (*wrap_is_full_screen_supported)(void);
extern bool (*wrap_is_full_screen_mode)(void);
extern void (*wrap_enter_full_screen_mode)(void);
extern void (*wrap_leave_full_screen_mode)(void);
extern void (*wrap_get_system_language)(UNSAFEPTR(char *) dst, int len);
extern void (*wrap_set_continuous_swipe_enabled)(bool is_enabled);
extern void (*wrap_free_shared)(UNSAFEPTR(void *) p);
extern bool (*wrap_check_file_exist)(UNSAFEPTR(const char *) file_name);
extern UNSAFEPTR(void *) (*wrap_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len);
extern void (*wrap_open_save_file)(UNSAFEPTR(const char *) file_name);
extern void (*wrap_write_save_file)(int b);
extern void (*wrap_close_save_file)(void);
#endif /* defined(HAL_USE_SWIFT) */

/*
 * C# Support (hal_init_func_table() is called from C#)
 */
#if defined(HAL_USE_CSHARP)

#define UNSAFEPTR(t)  intptr_t	/* For C# */

#ifdef NO_CDECL
#define __cdecl
#endif

void hal_init_func_table(
	void __cdecl (*p_log_info)(UNSAFEPTR(const char *) s),
	void __cdecl (*p_log_warn)(UNSAFEPTR(const char *) s),
	void __cdecl (*p_log_error)(UNSAFEPTR(const char *) s),
	void __cdecl (*p_log_out_of_memory)(void),
	void __cdecl (*p_make_save_directory)(void),
	void __cdecl (*p_make_real_path)(UNSAFEPTR(const char *) fname, UNSAFEPTR(const char *) dst, int len),
	void __cdecl (*p_notify_image_update)(int id, int width, int height, UNSAFEPTR(const uint32_t *) pixels),
	void __cdecl (*p_notify_image_free)(int id),
	void __cdecl (*p_render_image_normal)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_add)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_sub)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_dim)(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_rule)(int src_img, int rule_img, int threshold),
	void __cdecl (*p_render_image_melt)(int src_img, int rule_img, int progress),
	void __cdecl (*p_render_image_3d_normal)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_3d_add)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_3d_sub)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_render_image_3d_dim)(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha),
	void __cdecl (*p_reset_lap_timer)(UNSAFEPTR(uint64_t *) origin),
	uint64_t __cdecl (*p_get_lap_timer_millisec)(UNSAFEPTR(uint64_t *) origin),
	void __cdecl (*p_play_sound)(int stream, UNSAFEPTR(void *) wave),
	void __cdecl (*p_stop_sound)(int stream),
	void __cdecl (*p_set_sound_volume)(int stream, float vol),
	bool __cdecl (*p_is_sound_finished)(int stream),
	bool __cdecl (*p_play_video)(UNSAFEPTR(const char *) fname, bool is_skippable),
	void __cdecl (*p_stop_video)(void),
	bool __cdecl (*p_is_video_playing)(void),
	bool __cdecl (*p_is_full_screen_supported)(void),
	bool __cdecl (*p_is_full_screen_mode)(void),
	void __cdecl (*p_enter_full_screen_mode)(void),
	void __cdecl (*p_leave_full_screen_mode)(void),
	void __cdecl (*p_get_system_language)(UNSAFEPTR(char *) dst, int len),
	void __cdecl (*p_set_continuous_swipe_enabled)(bool is_enabled),
	void __cdecl (*p_free_shared)(UNSAFEPTR(void *) p),
	bool __cdecl (*p_check_file_exist)(UNSAFEPTR(const char *) file_name),
	UNSAFEPTR(void *) __cdecl (*p_get_file_contents)(UNSAFEPTR(const char *) file_name, UNSAFEPTR(int *) len),
	void __cdecl (*p_open_save_file)(UNSAFEPTR(const char *) file_name),
	void __cdecl (*p_write_save_file)(int b),
	void __cdecl (*p_close_save_file)(void)
);

#ifdef NO_CDECL
#undef __cdecl
#endif

#endif /* defined(HAL_USE_CSHARP) */

/***********
 * Helpers *
 ***********/

#if defined(HAL_TARGET_WINDOWS)
const wchar_t *win32_utf8_to_utf16(const char *s);
const char *win32_utf16_to_utf8(const wchar_t *s);
#endif

#ifdef __cplusplus
}
#endif

#endif
