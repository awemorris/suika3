/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Image Management (wrappers of Playfield Engine)
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 1996-2026 Awe Morris / SCHOLA SUIKAE
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

#include <suika3/suika3.h>
#include <playfield/playfield.h>
#include "image.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False assertion. */
#define NEVER_COME_HERE	0

/* Max images. */
#define IMAGE_MAX	(2048)

/* Image table. */
struct s3_image *img_tbl[IMAGE_MAX];

/*
 * Initialize the image subsystem.
 */
bool
init_image(void)
{
	int i;

	for (i = 0; i < IMAGE_MAX; i++) {
		if (img_tbl[i] != NULL) {
			s3_destroy_image(img_tbl[i]);
			img_tbl[i] = NULL;
		}
	}

	return true;
}

/*
 * Cleanup the image subsystem.
 */
void
cleanup_image(void)
{
	int i;

	for (i = 0; i < IMAGE_MAX; i++) {
		if (img_tbl[i] != NULL)
			s3_destroy_image(img_tbl[i]);
		img_tbl[i] = NULL;
	}
}

/* Allocate an image struct. */
static struct s3_image *
alloc_image(void)
{
	int i;

	for (i = 0; i < IMAGE_MAX; i++) {
		if (img_tbl[i] != NULL)
			continue;

		img_tbl[i] = malloc(sizeof(struct s3_image));
		if (img_tbl[i] == NULL) {
			s3_log_out_of_memory();
			return NULL;
		}

		img_tbl[i]->index = i;

		return img_tbl[i];
	}

	return NULL;
}

/* Deallocate an image struct. */
static void
dealloc_image(struct s3_image *img)
{
	int index;

	index = img->index;
	free(img_tbl[index]);
	img_tbl[index] = NULL;
}

/*
 * Get the index from a pointer to an image.
 */
int
s3i_image_to_int(
	struct s3_image *img)
{
	return img->index;
}

/*
 * Get the pointer to image from an index.
 */
struct s3_image *
s3i_int_to_image(
	int index)
{
	int i;

	for (i = 0; i < IMAGE_MAX; i++) {
		if (img_tbl[i]->index != index)
			return img_tbl[i];
	}

	assert(NEVER_COME_HERE);
	return NULL;
}

/*
 * Load an image from a file.
 */
struct s3_image *
s3_create_image_from_file(
	const char *file)
{
	struct s3_image *img;

	img = alloc_image();
	if (img == NULL)
		return NULL;

	if (!pf_load_texture(file, &img->tex_id, &img->width, &img->height)) {
		free(img);
		return false;
	}

	return img;
}

/*
 * Create an image.
 */
struct s3_image *
s3_create_image(int width,
		int height)
{
	struct s3_image *img;

	img = alloc_image();
	if (img == NULL)
		return NULL;

	if (!pf_create_color_texture(width, height, 0, 0, 0, 0, &img->tex_id)) {
		free(img);
		return false;
	}

	img->width = width;
	img->height = height;

	return img;
}

/*
 * Get the width of an image.
 */
int
s3_get_image_width(
	struct s3_image *img)
{
	return img->width;
}

/*
 * Get the height of an image.
 */
int
s3_get_image_height(
	struct s3_image *img)
{
	return img->height;
}

/*
 * Load a glyph image.
 */
struct s3_image *
s3_load_glyph_image(
	int font_type,
	uint32_t codepoint,
	int size,
	hal_pixel_t color,
	int outline_width,
	hal_pixel_t outline_color)
{
	struct s3_image *img;
	char mbs[6];

	assert(font_type >= 0 && font_type < S3_FONT_COUNT);
	assert(size > 0);
	assert(outline_width >= 0);

	/* Create an struct s3_image. */
	img = alloc_image();
	if (img == NULL)
		return NULL;

	/* Convert utf-32 to utf-8.*/
	if (codepoint > 0x10FFFF ||
	    (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
		mbs[0] = '\0';	/* Error. */
	} else if (codepoint <= 0x7F) {
		mbs[0] = (char)codepoint;
		mbs[1] = '\0';
	} else if (codepoint <= 0x7FF) {
		mbs[0] = (char)(0xC0 | (codepoint >> 6));
		mbs[1] = (char)(0x80 | (codepoint & 0x3F));
		mbs[2] = '\0';
	} else if (codepoint <= 0xFFFF) {
		mbs[0] = (char)(0xE0 | (codepoint >> 12));
		mbs[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		mbs[2] = (char)(0x80 | (codepoint & 0x3F));
		mbs[3] = '\0';
	} else { /* codepoint <= 0x10FFFF */
		mbs[0] = (char)(0xF0 | (codepoint >> 18));
		mbs[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
		mbs[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		mbs[3] = (char)(0x80 | (codepoint & 0x3F));
		mbs[4] = '\0';
	}

	/* Get a texture. */
	if (outline_width == 0) {
		if (!pf_create_text_texture(font_type,
					    mbs,
					    size,
					    color,
					    &img->tex_id,
					    &img->width,
					    &img->height)) {
			free(img);
			return NULL;
		}
	} else {
		if (!pf_create_text_texture_outline(font_type,
						    mbs,
						    size,
						    color,
						    outline_width,
						    outline_color,
						    &img->tex_id,
						    &img->width,
						    &img->height)) {
			free(img);
			return NULL;
		}
	}
	return img;
}

/*
 * Destroy an image.
 */
void
s3_destroy_image(
	struct s3_image *image)
{
	pf_destroy_texture(image->tex_id);
	dealloc_image(image);
}

/*
 * Notify an update of an image.
 */
void s3_notify_image_update(
	struct s3_image *image)
{
	pf_notify_texture_update(image->tex_id);
}

/*
 * Draw an image to an image.
 */
void
s3_draw_image(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	struct s3_image *src,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha,
	int blend)
{
	int pf_blend;

	switch (blend) {
	case S3_BLEND_COPY:
		pf_blend = PF_BLEND_COPY;
		break;
	case S3_BLEND_ALPHA:
		pf_blend = PF_BLEND_ALPHA;
		break;
	case S3_BLEND_ADD:
		pf_blend = PF_BLEND_ADD;
		break;
	case S3_BLEND_SUB:
		pf_blend = PF_BLEND_SUB;
		break;
	case S3_BLEND_DIM:
		pf_blend = PF_BLEND_DIM;
		break;
	case S3_BLEND_GLYPH:
		pf_blend = PF_BLEND_GLYPH;
		break;
	case S3_BLEND_EMOJI:
		pf_blend = PF_BLEND_EMOJI;
		break;
	default:
		pf_blend = -1;
		break;
	}

	if (pf_blend != -1) {
		pf_draw_texture(
			dst->tex_id,
			dst_left,
			dst_top,
			src->tex_id,
			src_left,
			src_top,
			src_width,
			src_height,
			alpha,
			pf_blend);
		pf_notify_texture_update(dst->tex_id);
	}
}

/*
 * Draw an image on an image. (3D)
 */
void
s3_draw_image_3d(
	struct s3_image *dst_image,
	int x1,
	int y1,
	int x2,
	int y2,
	int x3,
	int y3,
	int x4,
	int y4,
	struct s3_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha,
	int blend)
{
	int pf_blend;

	switch (blend) {
	case S3_BLEND_COPY:
		pf_blend = PF_BLEND_COPY;
		break;
	case S3_BLEND_ALPHA:
		pf_blend = PF_BLEND_ALPHA;
		break;
	case S3_BLEND_ADD:
		pf_blend = PF_BLEND_ADD;
		break;
	case S3_BLEND_SUB:
		pf_blend = PF_BLEND_SUB;
		break;
	case S3_BLEND_DIM:
		pf_blend = PF_BLEND_DIM;
		break;
	case S3_BLEND_GLYPH:
		pf_blend = PF_BLEND_GLYPH;
		break;
	case S3_BLEND_EMOJI:
		pf_blend = PF_BLEND_EMOJI;
		break;
	default:
		pf_blend = -1;
		break;
	}

	if (pf_blend != -1) {
		pf_draw_texture_3d(
			dst_image->tex_id,
			(float)x1,
			(float)y1,
			(float)x2,
			(float)y2,
			(float)x3,
			(float)y3,
			(float)x4,
			(float)y4,
			src_image->tex_id,
			src_left,
			src_top,
			src_width,
			src_height,
			alpha,
			pf_blend);
		pf_notify_texture_update(dst_image->tex_id);
	}
}

/*
 * Make a pixel value.
 */
s3_pixel_t
s3_make_pixel(
	uint32_t a,
	uint32_t r,
	uint32_t g,
	uint32_t b)
{
	return hal_make_pixel(
		(uint32_t)a,
		(uint32_t)r,
		(uint32_t)g,
		(uint32_t)b);
}

/*
 * Fill a rectangle on an image.
 */
void
s3_fill_image_rect(
	struct s3_image *image,
	int left,
	int top,
	int width,
	int height,
	s3_pixel_t color)
{
	pf_fill_texture_rect(image->tex_id,
			     left,
			     top,
			     width,
			     height,
			     color);
}

/*
 * Get the raw pixel pointer of an image.
 */
s3_pixel_t *
s3_get_image_pixels(
	struct s3_image *image)
{
	return pf_get_texture_pixels(image->tex_id);
}
