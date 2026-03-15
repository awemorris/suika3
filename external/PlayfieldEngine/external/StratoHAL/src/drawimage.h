/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Image drawing functions
 *  - Included by image.h and SSE source files
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

#include "stratohal/platform.h"
#include <assert.h>

extern bool is_avx2_available;
extern bool is_avx_available;
extern bool is_sse42_available;
extern bool is_sse4_available;
extern bool is_sse3_available;
extern bool is_sse2_available;
extern bool is_sse_available;

bool check_draw_image(struct hal_image *dst_image, int *dst_left, int *dst_top,
		      struct hal_image *src_image, int *width, int *height,
		      int *src_left, int *src_top, int alpha);
void scanline_conversion(float x1, float y1, float tx1, float ty1, float x2, float y2, float tx2, float ty2, float x3, float y3, float tx3, float ty3, float x4, float y4, float tx4, float ty4);

void
DRAW_IMAGE_COPY(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	int x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, 255))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++)
			*(dst_ptr + x) = *(src_ptr + x);
		src_ptr += sw;
		dst_ptr += dw;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_ALPHA(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;
	uint32_t src_pix, dst_pix;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, alpha))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			/* Get the source and destination pixel values. */
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			/* Multiply the alpha value and the source pixel value. */
			src_r = src_a * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * (float)(src_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			*dst_ptr++ = 0xff000000 |
				     ((uint32_t)(src_r + dst_r) << 16) |
				     ((uint32_t)(src_g + dst_g) << 8) |
				     (uint32_t)(src_b + dst_b);
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_GLYPH(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;
	uint32_t src_pix, dst_pix, dst_a_i, alpha_i;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, alpha))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			/* Get the source and destination pixel values. */
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			/* Multiply the alpha value and the source pixel value. */
			src_r = src_a * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * (float)(src_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);
			dst_a_i = hal_get_pixel_a(dst_pix);

			alpha_i = src_a > dst_a ? (uint32_t)(src_a * 255.0f) : dst_a_i;

			/* Store to the destination. */
			*dst_ptr++ = (alpha_i << 24) |
				     ((uint32_t)(src_r + dst_r) << 16) |
				     ((uint32_t)(src_g + dst_g) << 8) |
				     (uint32_t)(src_b + dst_b);
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_EMOJI(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;
	uint32_t src_pix, dst_pix, dst_a_i, alpha_i;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, alpha))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			src_r = src_a * (float)((src_pix) >> 16);
			src_g = src_a * (float)((src_pix) >> 8);
			src_b = src_a * (float)(src_pix & 0xff);

			dst_r = dst_a * (float)((dst_pix) >> 16);
			dst_g = dst_a * (float)((dst_pix) >> 8);
			dst_b = dst_a * (float)(dst_pix & 0xff);
			dst_a_i = hal_get_pixel_a(dst_pix);

			alpha_i = src_a > dst_a ? (uint32_t)(src_a * 255.0f) : dst_a_i;

			*dst_ptr++ = (alpha_i << 24) |
				     ((uint32_t)(src_r + dst_r) << 16) |
				     ((uint32_t)(src_g + dst_g) << 8) |
				     (uint32_t)(src_b + dst_b);
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_ADD(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	float a, src_a;
	uint32_t src_pix, src_r, src_g, src_b;
	uint32_t dst_pix, dst_r, dst_g, dst_b;
	uint32_t add_r, add_g, add_b;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, alpha))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			/* Get the source and destination pixel values. */
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);

			/* Multiply the alpha value and the source pixel value. */
			src_r = (uint32_t)(src_a * ((float)((src_pix >> 16) & 0xff) / 255.0f) * 255.0f);
			src_g = (uint32_t)(src_a * ((float)((src_pix >> 8) & 0xff) / 255.0f) * 255.0f);
			src_b = (uint32_t)(src_a * ((float)(src_pix & 0xff) / 255.0f) * 255.0f);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = (dst_pix >> 16) & 0xff;
			dst_g = (dst_pix >> 8) & 0xff;
			dst_b = dst_pix & 0xff;

			/* Add with saturation. */
			add_r = src_r + dst_r;
			if (add_r > 255)
				add_r = 255;
			add_g = src_g + dst_g;
			if (add_g > 255)
				add_g = 255;
			add_b = src_b + dst_b;
			if (add_b > 255)
				add_b = 255;

			/* Store to the destination. */
			*dst_ptr++ = 0xff000000 |
				     (add_r << 16) |
				     (add_g << 8) |
				     add_b;
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_SUB(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	float a, src_a;
	uint32_t src_pix, src_r, src_g, src_b;
	uint32_t dst_pix, dst_r, dst_g, dst_b;
	uint32_t add_r, add_g, add_b;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, alpha))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			/* Get the source and destination pixel values. */
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);

			/* Multiply the alpha value and the source pixel value. */
			src_r = (uint32_t)(src_a * ((float)((src_pix >> 16) & 0xff) / 255.0f) * 255.0f);
			src_g = (uint32_t)(src_a * ((float)((src_pix >> 8) & 0xff) / 255.0f) * 255.0f);
			src_b = (uint32_t)(src_a * ((float)(src_pix & 0xff) / 255.0f) * 255.0f);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = (dst_pix >> 16) & 0xff;
			dst_g = (dst_pix >> 8) & 0xff;
			dst_b = dst_pix & 0xff;

			/* Add with saturation. */
			add_r = dst_r - src_r;
			if (add_r > 255)
				add_r = 0;
			add_g = dst_g - src_g;
			if (add_g > 255)
				add_g = 0;
			add_b = dst_b - src_b;
			if (add_b > 255)
				add_b = 0;

			/* Store to the destination. */
			*dst_ptr++ = 0xff000000 |
				     (add_r << 16) |
				     (add_g << 8) |
				     add_b;
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_DIM(
	struct hal_image *dst_image,
	int dst_left,
	int dst_top,
	struct hal_image *src_image,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha)
{
	hal_pixel_t * RESTRICT src_ptr, * RESTRICT dst_ptr;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;
	uint32_t src_pix, dst_pix;
	int src_line_inc, dst_line_inc, x, y, sw, dw;

	if (!check_draw_image(dst_image, &dst_left, &dst_top, src_image, &width, &height, &src_left, &src_top, 255))
		return;

	sw = src_image->width;
	dw = dst_image->width;
	src_ptr = src_image->pixels + sw * src_top + src_left;
	dst_ptr = dst_image->pixels + dw * dst_top + dst_left;
	src_line_inc = sw - width;
	dst_line_inc = dw - width;
	a = (float)alpha / 255.0f;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			/* Get the source and destination pixel values. */
			src_pix	= *src_ptr++;
			dst_pix	= *dst_ptr;

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			/* Multiply 0.5 x alpha value and the source pixel value. */
			src_r = src_a * 0.5f * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * 0.5f * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * 0.5f * (float)(src_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			*dst_ptr++ = 
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);
		}
		src_ptr += src_line_inc;
		dst_ptr += dst_line_inc;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_RULE(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	hal_pixel_t * RESTRICT rule_ptr;
	int x, y, dw, sw, rw, w, dh, sh, rh, h;

	assert(dst_image != NULL);
	assert(src_image != NULL);
	assert(rule_image != NULL);

	/* Calc the width. */
	dw = dst_image->width;
	sw = src_image->width;
	rw = rule_image->width;
	w = dw;
	if (sw < w)
		w = sw;
	if (rw < w)
		w = rw;
	
	/* Calc the height. */
	dh = dst_image->height;
	sh = src_image->height;
	rh = rule_image->height;
	h = dh;
	if (sh < h)
		h = sh;
	if (rh < h)
		h = rh;

	/* Draw. */
	dst_ptr = dst_image->pixels;
	src_ptr = src_image->pixels;
	rule_ptr = rule_image->pixels;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
			if (hal_get_pixel_b(*(rule_ptr + x)) <= (unsigned char)threshold)
				*(dst_ptr + x) = *(src_ptr + x);
		dst_ptr += dw;
		src_ptr += sw;
		rule_ptr += rw;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_MELT(
	struct hal_image *dst_image,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
	hal_pixel_t * RESTRICT src_ptr;
	hal_pixel_t * RESTRICT dst_ptr;
	hal_pixel_t * RESTRICT rule_ptr;
	hal_pixel_t src_pix, dst_pix, rule_pix;
	float src_a, src_r, src_g, src_b, dst_a, dst_r, dst_g, dst_b, rule_a;
	int x, y, dw, sw, rw, w, dh, sh, rh, h;

	assert(dst_image != NULL);
	assert(src_image != NULL);
	assert(rule_image != NULL);

	/* Calc the width. */
	dw = dst_image->width;
	sw = src_image->width;
	rw = rule_image->width;
	w = dw;
	if (sw < w)
		w = sw;
	if (rw < w)
		w = rw;
	
	/* Calc the height. */
	dh = dst_image->height;
	sh = src_image->height;
	rh = rule_image->height;
	h = dh;
	if (sh < h)
		h = sh;
	if (rh < h)
		h = rh;

	/* Draw. */
	dst_ptr = dst_image->pixels;
	src_ptr = src_image->pixels;
	rule_ptr = rule_image->pixels;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			/* Get a source pixel. */
			src_pix = src_ptr[x];

			/* Get a destination pixel. */
			dst_pix = dst_ptr[x];

			/* Get a rule pixel. */
			rule_pix = rule_ptr[x];

			/* Calc alpha. */
			rule_a = (float)hal_get_pixel_b(rule_pix) / 255.0f;
			src_a = 2.0f * ((float)threshold / 255.0f) - rule_a;
			src_a = src_a < 0 ? 0 : src_a;
			src_a = src_a > 1.0f ? 1.0f : src_a;
			dst_a = 1.0f - src_a;

			/* Multiply alpha. */
			src_r = src_a * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * (float)(src_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			dst_ptr[x] =
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);
		}
		dst_ptr += dw;
		src_ptr += sw;
		rule_ptr += rw;
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_SCALE(
	struct hal_image *dst_image,
	int virtual_dst_width,
	int virtual_dst_height,
	int virtual_dst_left,
	int virtual_dst_top,
	struct hal_image *src_image)
{
	hal_pixel_t * RESTRICT dst_ptr;
	hal_pixel_t * RESTRICT src_ptr;
	float scale_x, scale_y;
	hal_pixel_t src_pix, dst_pix;
	float src_a, src_r, src_g, src_b, dst_a, dst_r, dst_g, dst_b;
	int real_dst_width, real_dst_height;
	int real_src_width, real_src_height;
	int real_draw_left, real_draw_top, real_draw_width, real_draw_height;
	int virtual_x, virtual_y;
	int i, j;

	assert(dst_image != NULL);
	assert(src_image != NULL);

	/* Get the destination size. */
	real_dst_width = dst_image->width;
	real_dst_height = dst_image->height;

	/* Calc the scale. */
	scale_x = (float)real_dst_width / (float)virtual_dst_width;
	scale_y = (float)real_dst_height / (float)virtual_dst_height;

	/* Get the source size. */
	real_src_width = src_image->width;
	real_src_height = src_image->height;

	/* Calc the destination position and size. */
	real_draw_left = (int)((float)virtual_dst_left * scale_x);
	real_draw_top = (int)((float)virtual_dst_top * scale_y);
	real_draw_width = (int)((float)real_src_width * scale_x);
	real_draw_height = (int)((float)real_src_height * scale_y);

	/* Get the pixel pointes. */
	dst_ptr = dst_image->pixels;
	src_ptr = src_image->pixels;

	/* Draw. */
	for (i = real_draw_top; i < real_draw_top + real_draw_height; i++) {
		/* Clip by a destination Y. */
		if (i < 0)
			continue;
		if (i >= real_dst_height)
			continue;

		/* Calc a source Y, and clip. */
		virtual_y = (int)((float)i / scale_y) - virtual_dst_top;
		if (virtual_y < 0)
			continue;
		if (virtual_y >= real_src_height)
			continue;

		for (j = real_draw_left; j < real_draw_left + real_draw_width;
		     j++) {
			/* Clip by a destination X. */
			if (j < 0)
				continue;
			if (j >= real_dst_width)
				continue;

			/* Calc a source X, and clip. */
			virtual_x = (int)((float)j / scale_x) - virtual_dst_left;
			if (virtual_x < 0)
				continue;
			if (virtual_x >= real_src_width)
				continue;

			/* Get a source pixel. */
			src_pix = src_ptr[real_src_width * virtual_y + virtual_x];

			/* Get a destination pixel. */
			dst_pix = dst_ptr[real_dst_width * i + j];

			/* Calc alpha. */
			src_a = (float)hal_get_pixel_a(src_pix) / 255.0f;
			dst_a = 1.0f - src_a;

			/* Multiply alpha to a source pixel. */
			src_r = src_a * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * (float)(src_pix & 0xff);

			/* Multiply alpha to a destination pixel. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			dst_ptr[real_dst_width * i + j] =
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);
		}
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_3D_ALPHA(
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
	int alpha)
{
	int x, y;
	int sw, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    0,
			    0,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    0,
			    (float)x3,
			    (float)y3,
			    0,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		int min_x, max_x;
		float tx, ty, tx_inc, ty_inc;

		min_x = sc_min_x[y];
		max_x = sc_max_x[y];
		if (min_x == INT_MAX)
			continue;
		if (max_x == INT_MIN)
			continue;
		if (min_x < 0)
			min_x = 0;
		if (max_x > dw)
			max_x = dw;

		tx = sc_min_tx[y];
		ty = sc_min_ty[y];
		tx_inc = (sc_max_tx[y] - sc_min_tx[y]) / (sc_max_x[y] - sc_min_x[y]);
		ty_inc = (sc_max_ty[y] - sc_min_ty[y]) / (sc_max_x[y] - sc_min_x[y]);

		for (x = min_x; x <= max_x; x++) {
			/* Get the source and destination pixel values. */
			dst_pix	= dst_pixel[y * dw + x];
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			/* Multiply the alpha value and the source pixel value. */
			src_r = src_a * (float)((src_pix >> 16) & 0xff);
			src_g = src_a * (float)((src_pix >> 8) & 0xff);
			src_b = src_a * (float)(src_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			dst_pixel[y * dw + x] =
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);

			tx += tx_inc;
			ty += ty_inc;
		}
	}
}

void
DRAW_IMAGE_3D_ADD(
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
	int alpha)
{
	int x, y;
	int sw, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	uint32_t src_r, src_g, src_b,  dst_r, dst_g, dst_b;
	float a, src_a, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    0,
			    0,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    0,
			    (float)x3,
			    (float)y3,
			    0,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		float tx = sc_min_tx[y];
		float ty = sc_min_ty[y];
		float tx_inc = (sc_max_tx[y] - sc_min_tx[y]) / (sc_max_x[y] - sc_min_x[y]);
		float ty_inc = (sc_max_ty[y] - sc_min_ty[y]) / (sc_max_x[y] - sc_min_x[y]);

		for (x = sc_min_x[y]; x < sc_max_x[y]; x++) {
			/* Get the source and destination pixel values. */
			dst_pix	= dst_pixel[y * dw + x];
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);

			/* Multiply the alpha value and the source pixel value. */
			src_r = (uint32_t)(src_a * ((float)((src_pix >> 16) & 0xff) / 255.0f) * 255.0f);
			src_g = (uint32_t)(src_a * ((float)((src_pix >> 8) & 0xff) / 255.0f) * 255.0f);
			src_b = (uint32_t)(src_a * ((float)(src_pix & 0xff) / 255.0f) * 255.0f);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = (dst_pix >> 16) & 0xff;
			dst_g = (dst_pix >> 8) & 0xff;
			dst_b = dst_pix & 0xff;

			/* Add with saturation. */
			add_r = src_r + dst_r;
			if (add_r > 255)
				add_r = 255;
			add_g = src_g + dst_g;
			if (add_g > 255)
				add_g = 255;
			add_b = src_b + dst_b;
			if (add_b > 255)
				add_b = 255;

			/* Store to the destination. */
			dst_pixel[y * dw + x] =
				0xff000000 |
				(add_r << 16) |
				(add_g << 8) |
				add_b;

			tx += tx_inc;
			ty += ty_inc;
		}
	}
}

void
DRAW_IMAGE_3D_SUB(
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
	int alpha)
{
	int x, y;
	int sw, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	uint32_t src_r, src_g, src_b,  dst_r, dst_g, dst_b;
	float a, src_a, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    0,
			    0,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    0,
			    (float)x3,
			    (float)y3,
			    0,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		float tx = sc_min_tx[y];
		float ty = sc_min_ty[y];
		float tx_inc = (sc_max_tx[y] - sc_min_tx[y]) / (sc_max_x[y] - sc_min_x[y]);
		float ty_inc = (sc_max_ty[y] - sc_min_ty[y]) / (sc_max_x[y] - sc_min_x[y]);

		for (x = sc_min_x[y]; x < sc_max_x[y]; x++) {
			/* Get the source and destination pixel values. */
			dst_pix	= dst_pixel[y * dw + x];
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);

			/* Multiply the alpha value and the source pixel value. */
			src_r = (uint32_t)(src_a * ((float)hal_get_pixel_r(src_pix) / 255.0f) * 255.0f);
			src_g = (uint32_t)(src_a * ((float)hal_get_pixel_g(src_pix) / 255.0f) * 255.0f);
			src_b = (uint32_t)(src_a * ((float)hal_get_pixel_b(src_pix) / 255.0f) * 255.0f);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = hal_get_pixel_r(dst_pix);
			dst_g = hal_get_pixel_g(dst_pix);
			dst_b = hal_get_pixel_b(dst_pix);

			/* Add with saturation. */
			add_r = dst_r - src_r;
			if (add_r > 255)
				add_r = 0;
			add_g = dst_g - src_g;
			if (add_g > 255)
				add_g = 0;
			add_b = dst_b - src_b;
			if (add_b > 255)
				add_b = 0;

			/* Store to the destination. */
			dst_pixel[y * dw + x] =
				0xff000000 |
				(add_r << 16) |
				(add_g << 8) |
				add_b;

			tx += tx_inc;
			ty += ty_inc;
		}
	}
}

void
DRAW_IMAGE_3D_DIM(
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
	int alpha)
{
	int x, y;
	int sw, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    0,
			    0,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    0,
			    (float)x3,
			    (float)y3,
			    0,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		float tx = sc_min_tx[y];
		float ty = sc_min_ty[y];
		float tx_inc = (sc_max_tx[y] - sc_min_tx[y]) / (sc_max_x[y] - sc_min_x[y]);
		float ty_inc = (sc_max_ty[y] - sc_min_ty[y]) / (sc_max_x[y] - sc_min_x[y]);

		for (x = sc_min_x[y]; x < sc_max_x[y]; x++) {
			/* Get the source and destination pixel values. */
			dst_pix	= dst_pixel[y * dw + x];
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Calc alpha values. */
			src_a = a * ((float)hal_get_pixel_a(src_pix) / 255.0f);
			dst_a = 1.0f - src_a;

			/* Multiply 0.5 x alpha value and the source pixel value. */
			src_r = src_a * 0.5f * (float)hal_get_pixel_r(src_pix);
			src_g = src_a * 0.5f * (float)hal_get_pixel_g(src_pix);
			src_b = src_a * 0.5f * (float)hal_get_pixel_b(src_pix);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)hal_get_pixel_r(dst_pix);
			dst_g = dst_a * (float)hal_get_pixel_g(dst_pix);
			dst_b = dst_a * (float)hal_get_pixel_b(dst_pix);

			/* Store to the destination. */
			dst_pixel[y * dw + x] =
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);

			tx += tx_inc;
			ty += ty_inc;
		}
	}
}
