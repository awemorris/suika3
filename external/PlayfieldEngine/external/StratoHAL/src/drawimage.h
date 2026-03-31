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
void scanline_conversion(float x1, float y1, float tx1, float ty1,
			 float x2, float y2, float tx2, float ty2,
			 float x3, float y3, float tx3, float ty3,
			 float x4, float y4, float tx4, float ty4);
void scanline_conversion_2(float x1, float y1, float tx1, float ty1,
			   float x2, float y2, float tx2, float ty2,
			   float x3, float y3, float tx3, float ty3,
			   float x4, float y4, float tx4, float ty4);

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
	int sw, sh, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    (float)src_left,
			    (float)src_top,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    (float)src_top,
			    (float)x3,
			    (float)y3,
			    (float)src_left,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	sh = src_image->height;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		int min_x, max_x;
		float div, tx, ty, tx_inc, ty_inc;

		min_x = scbuf1[y].sc_min_x;
		max_x = scbuf1[y].sc_max_x;
		if (min_x == INT_MAX)
			continue;
		if (max_x == INT_MIN)
			continue;

		tx = scbuf1[y].sc_min_tx;
		ty = scbuf1[y].sc_min_ty;
		div = (float)max_x - (float)min_x;
		if (div != 0) {
			tx_inc = (scbuf1[y].sc_max_tx - scbuf1[y].sc_min_tx) / div;
			ty_inc = (scbuf1[y].sc_max_ty - scbuf1[y].sc_min_ty) / div;
		} else {
			tx_inc = 0;
			ty_inc = 0;
		}

		for (x = min_x; x <= max_x; x++) {
			/* Clip by destination. */
			if (x < 0)
				continue;
			if (x >= dw)
				break;

			/* Wrap texture sampling. */
			if (tx < 0)
				tx = 0;
			if (tx >= sw)
				tx = (float)sw - 1;
			if (ty < 0)
				ty = 0;
			if (ty >= sh)
				ty = (float)sh - 1;

			/* Sample the texture. */
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Get the destination pixel value for blending. */
			dst_pix	= dst_pixel[y * dw + x];

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
	int sw, sh, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	uint32_t src_r, src_g, src_b,  dst_r, dst_g, dst_b;
	float a, src_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    (float)src_left,
			    (float)src_top,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    (float)src_top,
			    (float)x3,
			    (float)y3,
			    (float)src_left,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	sh = src_image->height;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		int min_x, max_x;
		float div, tx, ty, tx_inc, ty_inc;

		min_x = scbuf1[y].sc_min_x;
		max_x = scbuf1[y].sc_max_x;
		if (min_x == INT_MAX)
			continue;
		if (max_x == INT_MIN)
			continue;
		if (min_x < 0)
			min_x = 0;
		if (max_x >= dw)
			max_x = dw - 1;

		tx = scbuf1[y].sc_min_tx;
		ty = scbuf1[y].sc_min_ty;
		div = (float)scbuf1[y].sc_max_x - (float)scbuf1[y].sc_min_x;
		if (div != 0) {
			tx_inc = (scbuf1[y].sc_max_tx - scbuf1[y].sc_min_tx) / div;
			ty_inc = (scbuf1[y].sc_max_ty - scbuf1[y].sc_min_ty) / div;
		} else {
			tx_inc = 0;
			ty_inc = 0;
		}

		for (x = min_x; x < max_x; x++) {
			/* Clip by destination. */
			if (x < 0)
				continue;
			if (x >= dw)
				break;

			/* Wrap texture sampling. */
			if (tx < 0)
				tx = 0;
			if (tx >= sw)
				tx = (float)sw - 1;
			if (ty < 0)
				ty = 0;
			if (ty >= sh)
				ty = (float)sh - 1;

			/* Sample the texture. */
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Get the destination pixel value for blending. */
			dst_pix	= dst_pixel[y * dw + x];

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
	int sw, sh, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	uint32_t add_r, add_g, add_b;
	uint32_t src_r, src_g, src_b,  dst_r, dst_g, dst_b;
	float a, src_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    (float)src_left,
			    (float)src_top,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    (float)src_top,
			    (float)x3,
			    (float)y3,
			    (float)src_left,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	sh = src_image->height;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		int min_x, max_x;
		float div, tx, ty, tx_inc, ty_inc;

		min_x = scbuf1[y].sc_min_x;
		max_x = scbuf1[y].sc_max_x;
		if (min_x == INT_MAX)
			continue;
		if (max_x == INT_MIN)
			continue;
		if (min_x < 0)
			min_x = 0;
		if (max_x >= dw)
			max_x = dw - 1;

		tx = scbuf1[y].sc_min_tx;
		ty = scbuf1[y].sc_min_ty;
		div = (float)scbuf1[y].sc_max_x - (float)scbuf1[y].sc_min_x;
		if (div != 0) {
			tx_inc = (scbuf1[y].sc_max_tx - scbuf1[y].sc_min_tx) / div;
			ty_inc = (scbuf1[y].sc_max_ty - scbuf1[y].sc_min_ty) / div;
		} else {
			tx_inc = 0;
			ty_inc = 0;
		}

		for (x = min_x; x < max_x; x++) {
			/* Clip by destination. */
			if (x < 0)
				continue;
			if (x >= dw)
				break;

			/* Wrap texture sampling. */
			if (tx < 0)
				tx = 0;
			if (tx >= sw)
				tx = (float)sw - 1;
			if (ty < 0)
				ty = 0;
			if (ty >= sh)
				ty = (float)sh - 1;

			/* Sample the texture. */
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Get the destination pixel value for blending. */
			dst_pix	= dst_pixel[y * dw + x];

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
	int sw, sh, dw, dst_y_max;
	uint32_t *dst_pixel, *src_pixel;
	uint32_t src_pix, dst_pix;
	float a, src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a;

	scanline_conversion((float)x1,
			    (float)y1,
			    (float)src_left,
			    (float)src_top,
			    (float)x2,
			    (float)y2,
			    (float)src_width,
			    (float)src_top,
			    (float)x3,
			    (float)y3,
			    (float)src_left,
			    (float)src_height,
			    (float)x4,
			    (float)y4,
			    (float)src_width,
			    (float)src_height);			    

	dw = dst_image->width;
	sw = src_image->width;
	sh = src_image->height;
	dst_pixel = dst_image->pixels;
	src_pixel = src_image->pixels;
	dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
	a = (float)alpha / 255.0f;

	for (y = 0; y < dst_y_max; y++) {
		int min_x, max_x;
		float div, tx, ty, tx_inc, ty_inc;

		min_x = scbuf1[y].sc_min_x;
		max_x = scbuf1[y].sc_max_x;
		if (min_x == INT_MAX)
			continue;
		if (max_x == INT_MIN)
			continue;
		if (min_x < 0)
			min_x = 0;
		if (max_x >= dw)
			max_x = dw - 1;

		tx = scbuf1[y].sc_min_tx;
		ty = scbuf1[y].sc_min_ty;
		div = (float)scbuf1[y].sc_max_x - (float)scbuf1[y].sc_min_x;
		if (div != 0) {
			tx_inc = (scbuf1[y].sc_max_tx - scbuf1[y].sc_min_tx) / div;
			ty_inc = (scbuf1[y].sc_max_ty - scbuf1[y].sc_min_ty) / div;
		} else {
			tx_inc = 0;
			ty_inc = 0;
		}

		for (x = min_x; x < max_x; x++) {
			/* Clip by destination. */
			if (x < 0)
				continue;
			if (x >= dw)
				break;

			/* Wrap texture sampling. */
			if (tx < 0)
				tx = 0;
			if (tx >= sw)
				tx = (float)sw - 1;
			if (ty < 0)
				ty = 0;
			if (ty >= sh)
				ty = (float)sh - 1;

			/* Sample the texture. */
			src_pix	= src_pixel[(int)ty * sw + (int)tx];

			/* Get the destination pixel value for blending. */
			dst_pix	= dst_pixel[y * dw + x];

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

void
DRAW_IMAGE_CROSS(
	struct hal_image *dst_image,
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	int src1_left,
	int src1_top,
	int src2_left,
	int src2_top,
	int alpha)
{
	int x, y;
	float alpha_f;

	alpha_f = (float)alpha / 255.0f;

	for(y = 0; y < dst_image->height; y++) {
		int src1_y, src2_y;

		src1_y = y - src1_top;
		src2_y = y - src2_top;

		for(x = 0; x < dst_image->width; x++) {
			hal_pixel_t src1_pix, src2_pix, dst_pix;
			hal_pixel_t src_r, src_g, src_b;
			hal_pixel_t dst_r, dst_g, dst_b;
			float src1_a, src2_a, dst_a;
			int src1_x, src2_x;

			src1_x = x - src1_left;
			src2_x = x - src2_left;

			/* Get the source and destination pixel values. */
			if (src1_x >= 0 && src1_x < src1_image->width &&
			    src1_y >= 0 && src1_y < src1_image->height)
				src1_pix = src1_image->pixels[src1_y * src1_image->width + src1_x];
			else
				src1_pix = 0;
			if (src2_x >= 0 && src2_x < src2_image->width &&
			    src2_y >= 0 && src2_y < src2_image->height)
				src2_pix = src2_image->pixels[src2_y * src2_image->width + src2_x];
			else
				src2_pix = 0;
			dst_pix	= dst_image->pixels[y * dst_image->width + x];

			/* Calc alpha values. */
			src1_a = alpha_f * ((float)hal_get_pixel_a(src1_pix) / 255.0f);
			src2_a = (1.0f - alpha_f) * ((float)hal_get_pixel_a(src1_pix) / 255.0f);
			dst_a = 1.0f - (src1_a + src2_a);

			/* Multiply the alpha value and the source pixel value. */
			src_r = src1_a * (float)((src1_pix >> 16) & 0xff) +
				src2_a * (float)((src2_pix >> 16) & 0xff);
			src_g = src1_a * (float)((src1_pix >> 8) & 0xff) +
				src2_a * (float)((src2_pix >> 8) & 0xff);
			src_b = src1_a * (float)(src1_pix & 0xff) +
				src2_a * (float)(src2_pix & 0xff);

			/* Multiply the alpha value and the destination pixel value. */
			dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
			dst_g = dst_a * (float)((dst_pix >> 8) & 0xff);
			dst_b = dst_a * (float)(dst_pix & 0xff);

			/* Store to the destination. */
			dst_image->pixels[y * dst_image->width + x] =
				0xff000000 |
				((uint32_t)(src_r + dst_r) << 16) |
				((uint32_t)(src_g + dst_g) << 8) |
				(uint32_t)(src_b + dst_b);
		}
	}

	hal_notify_image_update(dst_image);
}

void
DRAW_IMAGE_3D_CROSS(
        struct hal_image *dst_image,
        struct hal_image *src1_image,
        struct hal_image *src2_image,
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
        int x, y;
        int dw, sw1, sh1, sw2, sh2, dst_y_max;
        uint32_t *dst_pixel, *src1_pixel, *src2_pixel;
        float alpha_f;

        scanline_conversion(src1_x1,
			    src1_y1,
			    0.0f,
			    0.0f,
                            src1_x2,
			    src1_y2,
			    (float)src1_image->width,
			    0.0f,
                            src1_x3,
			    src1_y3,
			    0.0f,
			    (float)src1_image->height,
                            src1_x4,
			    src1_y4,
			    (float)src1_image->width,
			    (float)src1_image->height);

        scanline_conversion_2(src2_x1,
			      src2_y1,
			      0.0f,
			      0.0f,
			      src2_x2,
			      src2_y2,
			      (float)src2_image->width,
			      0.0f,
			      src2_x3,
			      src2_y3,
			      0.0f,
			      (float)src2_image->height,
			      src2_x4,
			      src2_y4,
			      (float)src2_image->width,
			      (float)src2_image->height);

        dw = dst_image->width;
        sw1 = src1_image->width;  sh1 = src1_image->height;
        sw2 = src2_image->width;  sh2 = src2_image->height;
        dst_pixel = dst_image->pixels;
        src1_pixel = src1_image->pixels;
        src2_pixel = src2_image->pixels;
        
        dst_y_max = (dst_image->height > SC_LINES) ? SC_LINES : dst_image->height;
        alpha_f = (float)alpha / 255.0f;

        for (y = 0; y < dst_y_max; y++) {
                int min_x1 = scbuf1[y].sc_min_x,  max_x1 = scbuf1[y].sc_max_x;
                int min_x2 = scbuf2[y].sc_min_x, max_x2 = scbuf2[y].sc_max_x;

                float tx1 = scbuf1[y].sc_min_tx,  ty1 = scbuf1[y].sc_min_ty;
                float tx2 = scbuf2[y].sc_min_tx, ty2 = scbuf2[y].sc_min_ty;
                float tx1_inc = 0, ty1_inc = 0, tx2_inc = 0, ty2_inc = 0;

                if (min_x1 != INT_MAX && max_x1 != INT_MIN) {
                        float div1 = (float)max_x1 - (float)min_x1;
                        if (div1 != 0) {
                                tx1_inc = (scbuf1[y].sc_max_tx - scbuf1[y].sc_min_tx) / div1;
                                ty1_inc = (scbuf1[y].sc_max_ty - scbuf1[y].sc_min_ty) / div1;
                        }
                }

                if (min_x2 != INT_MAX && max_x2 != INT_MIN) {
                        float div2 = (float)max_x2 - (float)min_x2;
                        if (div2 != 0) {
                                tx2_inc = (scbuf2[y].sc_max_tx - scbuf2[y].sc_min_tx) / div2;
                                ty2_inc = (scbuf2[y].sc_max_ty - scbuf2[y].sc_min_ty) / div2;
                        }
                }

                int start_x = INT_MAX, end_x = INT_MIN;
                if (min_x1 != INT_MAX)
			start_x = (min_x1 < start_x) ? min_x1 : start_x;
                if (min_x2 != INT_MAX)
			start_x = (min_x2 < start_x) ? min_x2 : start_x;
                if (max_x1 != INT_MIN)
			end_x = (max_x1 > end_x) ? max_x1 : end_x;
                if (max_x2 != INT_MIN)
			end_x = (max_x2 > end_x) ? max_x2 : end_x;
                if (start_x == INT_MAX || end_x == INT_MIN)
                        continue;

                for (x = start_x; x <= end_x; x++) {
                        uint32_t src1_pix = 0, src2_pix = 0;

                        if (min_x1 != INT_MAX && x >= min_x1 && x <= max_x1) {
                                if (x >= 0 && x < dw) {
                                        float stx = tx1, sty = ty1;
                                        if (stx < 0)
						stx = 0;
					if (stx >= sw1)
						stx = (float)sw1 - 1;
                                        if (sty < 0)
						sty = 0;
					if (sty >= sh1)
						sty = (float)sh1 - 1;
                                        src1_pix = src1_pixel[(int)sty * sw1 + (int)stx];
                                }
                                tx1 += tx1_inc;
                                ty1 += ty1_inc;
                        }

                        if (min_x2 != INT_MAX && x >= min_x2 && x <= max_x2) {
                                if (x >= 0 && x < dw) {
                                        float stx = tx2, sty = ty2;
                                        if (stx < 0)
						stx = 0;
					if (stx >= sw2)
						stx = (float)sw2 - 1;
                                        if (sty < 0)
						sty = 0;
					if (sty >= sh2)
						sty = (float)sh2 - 1;
                                        src2_pix = src2_pixel[(int)sty * sw2 + (int)stx];
                                }
                                tx2 += tx2_inc;
                                ty2 += ty2_inc;
                        }

                        if (x < 0 || x >= dw)
				continue; /* Out of range. */
                        if ((src1_pix >> 24) == 0 && (src2_pix >> 24) == 0)
				continue; /* alpha = 0 */

                        uint32_t dst_pix = dst_pixel[y * dw + x];

                        float src1_a = alpha_f * ((float)hal_get_pixel_a(src1_pix) / 255.0f);
                        float src2_a = (1.0f - alpha_f) * ((float)hal_get_pixel_a(src2_pix) / 255.0f);
                        float dst_a = 1.0f - (src1_a + src2_a);

                        float src_r = src1_a * (float)((src1_pix >> 16) & 0xff) +
				      src2_a * (float)((src2_pix >> 16) & 0xff);
                        float src_g = src1_a * (float)((src1_pix >> 8)  & 0xff) +
				      src2_a * (float)((src2_pix >> 8)  & 0xff);
                        float src_b = src1_a * (float)(src1_pix & 0xff) +
				      src2_a * (float)(src2_pix & 0xff);

                        float dst_r = dst_a * (float)((dst_pix >> 16) & 0xff);
                        float dst_g = dst_a * (float)((dst_pix >> 8)  & 0xff);
                        float dst_b = dst_a * (float)(dst_pix & 0xff);

                        dst_pixel[y * dw + x] = 0xff000000 |
                                                ((uint32_t)(src_r + dst_r) << 16) |
                                                ((uint32_t)(src_g + dst_g) << 8) |
                                                (uint32_t)(src_b + dst_b);
                }
        }

	hal_notify_image_update(dst_image);
}
