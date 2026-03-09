/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/*
 * Playfield Engine
 * Glyph HAL for OpenGL
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

#include "stratohal/platform.h"

#include <ctype.h>
#include <assert.h>

/*
 * FreeType2 headers
 */

#include <ft2build.h>

#include FT_FREETYPE_H
#include <freetype/ftstroke.h>

/*
 * The scale constant
 */
#define SCALE	(64)

/*
 * FreeType2 objects
 */
static FT_Library library;
static FT_Face face[HAL_GLYPH_DATA_COUNT];
static FT_Byte *file_content[HAL_GLYPH_DATA_COUNT];

/*
 * Forward declarations
 */
static bool draw_glyph_with_outline(
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
static bool draw_glyph_without_outline(
	struct hal_image *img,
	int font_type,
	int font_size,
	int base_font_size,
	int x,
	int y,
	hal_pixel_t color,
	uint32_t codepoint,
	int *ret_w,
	int *ret_h,
	bool is_dim);
static void draw_glyph_func(
	unsigned char * RESTRICT font,
	int font_width,
	int font_height,
	int margin_left,
	int margin_top,
	hal_pixel_t * RESTRICT image,
	int image_width,
	int image_height,
	int image_x,
	int image_y,
	hal_pixel_t color);
static void draw_glyph_dim_func(
	unsigned char * RESTRICT font,
	int font_width,
	int font_height,
	int margin_left,
	int margin_top,
	hal_pixel_t * RESTRICT image,
	int image_width,
	int image_height,
	int image_x,
	int image_y,
	hal_pixel_t color);

/*
 * Initialize the font render.
 */
bool
hal_init_glyph(void)
{
#ifdef USE_DLL
	cleanup_glyph();
#endif
	return true;
}

/*
 * Cleanup the font renderer.
 */
void
hal_cleanup_glyph(void)
{
	int i;

	for (i = 0; i < HAL_GLYPH_DATA_COUNT; i++)
		hal_destroy_glyph_data(i);

	if (library != NULL) {
		FT_Done_FreeType(library);
		library = NULL;
	}
}

/*
 * Load a font file.
 */
bool
hal_load_glyph_data(
	int index,
	const uint8_t *data,
	size_t len)
{
	FT_Error err;

	assert(index >= 0 && index < HAL_GLYPH_DATA_COUNT);

	/* Initialize FreeType2 if required. */
	if (library == NULL) {
		err = FT_Init_FreeType(&library);
		if (err != 0) {
			hal_log_error("FT_Init_FreeType() failed.");
			return false;
		}
	}

	/* Destroy a font if required. */
	hal_destroy_glyph_data(index);

	/* Allocate a buffer to hold the data. */
	file_content[index] = malloc(len);
	if (file_content[index] == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	/* Copy the data. */
	memcpy(file_content[index], data, len);

	/* Load the content font. */
	err = FT_New_Memory_Face(library,
				 file_content[index],
				 (FT_Long)len,
				 0,
				 &face[index]);
	if (err != 0) {
		hal_log_error("Failed to load a font file.");
		return false;
	}

	/* Preload font glyphs. */
	hal_get_glyph_width(index, 8, 'A');

	return true;
}

/*
 * Destroy a font.
 */
void
hal_destroy_glyph_data(
	int index)
{
	if (face[index] != NULL) {
		FT_Done_Face(face[index]);
		face[index] = NULL;
	}
	if (file_content[index] != NULL) {
		free(file_content[index]);
		file_content[index] = NULL;
	}
}

/*
 * Get a top character of a utf-8 string as a utf-32.
 */
int
hal_utf8_to_utf32(
	const char *mbs,
	uint32_t *wc)
{
	size_t mbslen, octets, i;
	uint32_t ret;

	assert(mbs != NULL);

	/* If mbs is empty. */
	mbslen = strlen(mbs);
	if(mbslen == 0)
		return 0;

	/* Check the first byte, get an octet count. */
	if (mbs[0] == '\0')
		octets = 0;
	else if ((mbs[0] & 0x80) == 0)
		octets = 1;
	else if ((mbs[0] & 0xe0) == 0xc0)
		octets = 2;
	else if ((mbs[0] & 0xf0) == 0xe0)
		octets = 3;
	else if ((mbs[0] & 0xf8) == 0xf0)
		octets = 4;
	else
		return -1;	/* Not suppoerted. */

	/* Check the mbs length. */
	if (mbslen < octets)
		return -1;	/* mbs is too short. */

	/* Check for 2-4 bytes. */
	for (i = 1; i < octets; i++) {
		if((mbs[i] & 0xc0) != 0x80)
			return -1;	/* Non-understandable */
	}

	/* Compose a utf-32 character. */
	switch (octets) {
	case 0:
		ret = 0;
		break;
	case 1:
		ret = (uint32_t)mbs[0];
		break;
	case 2:
		ret = (uint32_t)(((mbs[0] & 0x1f) << 6) |
				 (mbs[1] & 0x3f));
		break;
	case 3:
		ret = (uint32_t)(((mbs[0] & 0x0f) << 12) |
				 ((mbs[1] & 0x3f) << 6) |
				 (mbs[2] & 0x3f));
		break;
	case 4:
		ret = (uint32_t)(((mbs[0] & 0x07) << 18) |
				 ((mbs[1] & 0x3f) << 12) |
				 ((mbs[2] & 0x3f) << 6) |
				 (mbs[3] & 0x3f));
		break;
	default:
		/* never come here */
		assert(0);
		return -1;
	}

	/* Store the result. */
	if(wc != NULL)
		*wc = ret;

	/* Return the octet count. */
	return (int)octets;
}

/*
 * Get a characters of a utf-8 string.
 */
int
hal_count_utf8_chars(
	const char *mbs)
{
	int count;
	int mblen;

	count = 0;
	while (*mbs != '\0') {
		mblen = hal_utf8_to_utf32(mbs, NULL);
		if (mblen == -1)
			return -1;
		count++;
		mbs += mblen;
	}
	return count;
}

/*
 * Get a width for a character.
 */
int
hal_get_glyph_width(
	int slot,
	int size,
	uint32_t codepoint)
{
	int w, h;

	w = h = 0;

	hal_draw_glyph(NULL,
		       slot,
		       size,
		       size,
		       0,
		       0,
		       0,
		       0,
		       0,
		       codepoint, &w, &h, false);

	return w;
}

/*
 * Get a height for a character.
 */
int
hal_get_glyph_height(
	int slot,
	int size,
	uint32_t codepoint)
{
	int w, h;

	w = h = 0;

	hal_draw_glyph(NULL,
		       slot,
		       size,
		       size,
		       0,
		       0,
		       0,
		       0,
		       0,
		       codepoint,
		       &w,
		       &h,
		       false);

	return h;
}

/*
 * Get a width for a character.
 */
void
hal_get_glyph_width_and_height(
	int slot,
	int size,
	uint32_t codepoint,
	int *width,
	int *height)
{
	int w, h;

	w = h = 0;

	hal_draw_glyph(NULL,
		       slot,
		       size,
		       size,
		       0,
		       0,
		       0,
		       0,
		       0,
		       codepoint, &w, &h, false);

	*width = w;
	*height = h;
}

/*
 * Get a width for a string.
 */
int
hal_get_string_width(
	int font_index,
	int font_size,
	const char *mbs)
{
	uint32_t c;
	int mblen, w;

	/* Calc for each character. */
	w = 0;
	c = 0; /* warning avoidance on gcc 5.3.1 */
	while (*mbs != '\0') {
		/* Get a character. */
		mblen = hal_utf8_to_utf32(mbs, &c);
		if (mblen == -1)
			return -1;

		/* Get a character width. */
		w += hal_get_glyph_width(font_index, font_size, c);

		/* Move to a next character. */
		mbs += mblen;
	}
	return w;
}

/*
 * Get a height for a string.
 */
int
hal_get_string_height(
	int font_type,
	int font_size,
	const char *mbs)
{
	uint32_t c;
	int mblen, w;

	/* Calc for each character.s */
	w = 0;
	c = 0; /* warning avoidance on gcc 5.3.1 */
	while (*mbs != '\0') {
		/* Get a character. */
		mblen = hal_utf8_to_utf32(mbs, &c);
		if (mblen == -1)
			return -1;

		/* Get a character height. */
		w += hal_get_glyph_height(font_type, font_size, c);

		/* Move to a next character. */
		mbs += mblen;
	}
	return w;
}

/*
 * Get a width and a height for a string.
 */
void
hal_get_string_width_and_height(
	int slot,
	int size,
	const char *mbs,
	int *width,
	int *height)
{
	uint32_t c;
	int mblen, w, h;

	/* Calc for each character. */
	w = 0;
	h = 0;
	c = 0; /* warning avoidance on gcc 5.3.1 */
	while (*mbs != '\0') {
		int cw, ch;

		/* Get a character. */
		mblen = hal_utf8_to_utf32(mbs, &c);
		if (mblen == -1) {
			*width = 0;
			*height = 1;
			return;
		}

		/* Get a character width. */
		hal_get_glyph_width_and_height(slot, size, c, &cw, &ch);
		w += cw;
		if (ch > h)
			h = ch;

		/* Move to a next character. */
		mbs += mblen;
	}

	*width = w;
	*height = h;
}

/*
 * Drawing
 */

/*
 * Draw a glyph.
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
	bool is_dim)
{
	if (outline_size == 0) {
		if (!draw_glyph_without_outline(img, font_index, font_size,
						base_font_size, x, y, color,
						codepoint, ret_w, ret_h, is_dim))
			return false;
	} else {
		if (!draw_glyph_with_outline(img, font_index, font_size,
					     base_font_size, outline_size,
					     x, y, color, outline_color,
					     codepoint, ret_w, ret_h, is_dim))
			return false;
	}

	return true;
}

/* Draw a glyph with outline. */
static bool
draw_glyph_with_outline(
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
	bool is_dim)
{
	FT_Stroker stroker;
	FT_UInt glyphIndex;
	FT_Glyph glyph;
	FT_BitmapGlyph bitmapGlyph;
	int descent;

	/* Draw inner outline. */
	FT_Stroker_New(library, &stroker);
	FT_Stroker_Set(stroker, outline_size * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
	glyphIndex = FT_Get_Char_Index(face[font_index], codepoint);
	FT_Load_Glyph(face[font_index], glyphIndex, FT_LOAD_DEFAULT);
	FT_Get_Glyph(face[font_index]->glyph, &glyph);
	FT_Glyph_StrokeBorder(&glyph, stroker, true, true);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
	bitmapGlyph = (FT_BitmapGlyph)glyph;
	if (img != NULL) {
		if (!is_dim) {
			draw_glyph_func(bitmapGlyph->bitmap.buffer,
					(int)bitmapGlyph->bitmap.width,
					(int)bitmapGlyph->bitmap.rows,
					bitmapGlyph->left,
					font_size - bitmapGlyph->top,
					img->pixels,
					img->width,
					img->height,
					x,
					y - (font_size - base_font_size),
					outline_color);
		} else {
			draw_glyph_dim_func(bitmapGlyph->bitmap.buffer,
					    (int)bitmapGlyph->bitmap.width,
					    (int)bitmapGlyph->bitmap.rows,
					    bitmapGlyph->left,
					    font_size - bitmapGlyph->top,
					    img->pixels,
					    img->width,
					    img->height,
					    x,
					    y - (font_size - base_font_size),
					    outline_color);
		}
	}
	FT_Done_Glyph(glyph);
	FT_Stroker_Done(stroker);

	/* Draw outer outline. */
	FT_Stroker_New(library, &stroker);
	FT_Stroker_Set(stroker, outline_size * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
	glyphIndex = FT_Get_Char_Index(face[font_index], codepoint);
	FT_Load_Glyph(face[font_index], glyphIndex, FT_LOAD_DEFAULT);
	FT_Get_Glyph(face[font_index]->glyph, &glyph);
	FT_Glyph_StrokeBorder(&glyph, stroker, false, true);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
	bitmapGlyph = (FT_BitmapGlyph)glyph;
	if (img != NULL) {
		if (!is_dim) {
			draw_glyph_func(bitmapGlyph->bitmap.buffer,
					(int)bitmapGlyph->bitmap.width,
					(int)bitmapGlyph->bitmap.rows,
					bitmapGlyph->left,
					font_size - bitmapGlyph->top,
					img->pixels,
					img->width,
					img->height,
					x,
					y - (font_size - base_font_size),
					outline_color);
		} else {
			draw_glyph_dim_func(bitmapGlyph->bitmap.buffer,
					    (int)bitmapGlyph->bitmap.width,
					    (int)bitmapGlyph->bitmap.rows,
					    bitmapGlyph->left,
					    font_size - bitmapGlyph->top,
					    img->pixels,
					    img->width,
					    img->height,
					    x,
					    y - (font_size - base_font_size),
					    outline_color);
		}
	}
	descent = (int)(face[font_index]->glyph->metrics.height / SCALE) -
		  (int)(face[font_index]->glyph->metrics.horiBearingY / SCALE);
	*ret_w = (int)face[font_index]->glyph->advance.x / SCALE;
	*ret_h = font_size + descent + 2;
	FT_Done_Glyph(glyph);
	FT_Stroker_Done(stroker);
	if (img == NULL)
		return true;

	/* Draw body. */
	glyphIndex = FT_Get_Char_Index(face[font_index], codepoint);
	FT_Load_Glyph(face[font_index], glyphIndex, FT_LOAD_DEFAULT);
	FT_Get_Glyph(face[font_index]->glyph, &glyph);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
	bitmapGlyph = (FT_BitmapGlyph)glyph;
	if (img != NULL) {
		if (!is_dim) {
			draw_glyph_func(bitmapGlyph->bitmap.buffer,
					(int)bitmapGlyph->bitmap.width,
					(int)bitmapGlyph->bitmap.rows,
					bitmapGlyph->left,
					font_size - bitmapGlyph->top,
					img->pixels,
					img->width,
					img->height,
					x,
					y - (font_size - base_font_size),
					color);
		} else {
			draw_glyph_dim_func(bitmapGlyph->bitmap.buffer,
					    (int)bitmapGlyph->bitmap.width,
					    (int)bitmapGlyph->bitmap.rows,
					    bitmapGlyph->left,
					    font_size - bitmapGlyph->top,
					    img->pixels,
					    img->width,
					    img->height,
					    x,
					    y - (font_size - base_font_size),
					    color);
		}
	}

	/* End drawing. */
	FT_Done_Glyph(glyph);

	/* Update a GPU texture. */
	if (img != NULL)
		hal_notify_image_update(img);

	return true;
}

/* Draw glyph without outline. */
static bool
draw_glyph_without_outline(
	struct hal_image *img,
	int font_index,
	int font_size,
	int base_font_size,
	int x,
	int y,
	hal_pixel_t color,
	uint32_t codepoint,
	int *ret_w,
	int *ret_h,
	bool is_dim)
{
	FT_Error err;
	int descent;

	/* Set a font size. */
	err = FT_Set_Pixel_Sizes(face[font_index], 0, (FT_UInt)font_size);
	if (err != 0) {
		hal_log_error("FT_Set_Pixel_Sizes() failed.");
		return false;
	}

	/* Get a character as a grayscaled image. */
	err = FT_Load_Char(face[font_index], codepoint, FT_LOAD_RENDER);
	if (err != 0) {
		hal_log_error("FT_Load_Char() failed.");
		return false;
	}

	/* Draw. */
	if (img != NULL) {
		if (!is_dim) {
			draw_glyph_func(face[font_index]->glyph->bitmap.buffer,
					(int)face[font_index]->glyph->bitmap.width,
					(int)face[font_index]->glyph->bitmap.rows,
					face[font_index]->glyph->bitmap_left,
					font_size - face[font_index]->glyph->bitmap_top,
					img->pixels,
					img->width,
					img->height,
					x,
					y - (font_size - base_font_size),
					color);
		} else {
			draw_glyph_dim_func(face[font_index]->glyph->bitmap.buffer,
					    (int)face[font_index]->glyph->bitmap.width,
					    (int)face[font_index]->glyph->bitmap.rows,
					    face[font_index]->glyph->bitmap_left,
					    font_size - face[font_index]->glyph->bitmap_top,
					    img->pixels,
					    img->width,
					    img->height,
					    x,
					    y - (font_size - base_font_size),
					    color);
		}
	}

	/* Get a descent. */
	descent = (int)(face[font_index]->glyph->metrics.height / SCALE) -
		  (int)(face[font_index]->glyph->metrics.horiBearingY / SCALE);

	/* Get a width and a height. */
	*ret_w = (int)face[font_index]->glyph->advance.x / SCALE;
	*ret_h = font_size + descent;

	/* Update a GPU texture. */
	if (img != NULL)
		hal_notify_image_update(img);

	return true;
}

/* Check if a supported alphabet. */
bool
hal_isgraph_extended(
	const char **mbs,
	uint32_t *wc)
{
	int len;

	/* If a character is a ASCII character. */
	if (isgraph((unsigned char)**mbs)) {
		*wc = (unsigned char)**mbs;
		(*mbs)++;
		return true;
	}

	/* Get a utf-32 character. */
	len = hal_utf8_to_utf32(*mbs, wc);
	if (len < 1)
		return false;
	*mbs += len;

	/* If a Latin alphabet with an accent. */
	if (*wc >= 0x00c0 && *wc <= 0x017f)
		return true;

	/* If a Greek alphabet. */
	if (*wc >= 0x0370 && *wc <= 0x3ff)
		return true;

	/* If a Cyrillic alphabet. */
	if (*wc >= 0x410 && *wc <= 0x44f)
		return true;

	/* Other language. */
	return false;
}

/* Draw a glyph to an image. */
static void
draw_glyph_func(
	unsigned char *font,
	int font_width,
	int font_height,
	int margin_left,
	int margin_top,
	hal_pixel_t * RESTRICT image,
	int image_width,
	int image_height,
	int image_x,
	int image_y,
	hal_pixel_t color)
{
	unsigned char *src_ptr, src_pix;
	hal_pixel_t *dst_ptr, dst_pix, dst_a2;
	float color_r, color_g, color_b;
	float src_a, src_r, src_g, src_b;
	float dst_a, dst_r, dst_g, dst_b;
	int image_real_x, image_real_y;
	int font_real_x, font_real_y;
	int font_real_width, font_real_height;
	int px, py;

	/* Do clipping. */
	if (image_x + margin_left + font_width < 0)
		return;
	if (image_x + margin_left >= image_width)
		return;
	if (image_y + margin_top + font_height < 0)
		return;
	if (image_y + margin_top > image_height)
		return;

	/* Do clipping. */
	image_real_x = image_x + margin_left;
	image_real_y = image_y + margin_top;
	font_real_x = 0;
	font_real_y = 0;
	font_real_width = font_width;
	font_real_height = font_height;
	if (image_real_x < 0) {
		font_real_x -= image_real_x;
		font_real_width += image_real_x;
		image_real_x = 0;
	}
	if (image_real_x + font_real_width >= image_width) {
		font_real_width -= (image_real_x + font_real_width) -
				   image_width;
	}
	if (image_real_y < 0) {
		font_real_y -= image_real_y;
		font_real_height += image_real_y;
		image_real_y = 0;
	}
	if (image_real_y + font_real_height >= image_height) {
		font_real_height -= (image_real_y + font_real_height) -
				    image_height;
	}

	/* Draw. */
	color_r = (float)hal_get_pixel_r(color);
	color_g = (float)hal_get_pixel_g(color);
	color_b = (float)hal_get_pixel_b(color);
	dst_ptr = image + image_real_y * image_width + image_real_x;
	src_ptr = font + font_real_y * font_width + font_real_x;
	for (py = font_real_y; py < font_real_y + font_real_height; py++) {
		for (px = font_real_x; px < font_real_x + font_real_width; px++) {
			/* Get a source pixel. */
			src_pix = *src_ptr++;

			/* Multiply the source alpha to the color. */
			src_a = (float)src_pix / 255.0f;
			dst_a = 1.0f - src_a;
			src_r = src_a * color_r;
			src_g = src_a * color_g;
			src_b = src_a * color_b;

			/* Get a destination pixel. */
			dst_pix	= *dst_ptr;
			dst_r  = dst_a * (float)hal_get_pixel_r(dst_pix);
			dst_g  = dst_a * (float)hal_get_pixel_g(dst_pix);
			dst_b  = dst_a * (float)hal_get_pixel_b(dst_pix);
			dst_a2 = src_pix + hal_get_pixel_a(dst_pix);
			if (dst_a2 > 255)
				dst_a2 = 255;

			/* Compose and store. */
			*dst_ptr++ = hal_make_pixel((uint32_t)dst_a2,
						    (uint32_t)(src_r + dst_r),
						    (uint32_t)(src_g + dst_g),
						    (uint32_t)(src_b + dst_b));
		}
		dst_ptr += image_width - font_real_width;
		src_ptr += font_width - font_real_width;
	}
}

/* Draw a glyph to an image. */
static void
draw_glyph_dim_func(
	unsigned char *font,
	int font_width,
	int font_height,
	int margin_left,
	int margin_top,
	hal_pixel_t * RESTRICT image,
	int image_width,
	int image_height,
	int image_x,
	int image_y,
	hal_pixel_t color)
{
	unsigned char *src_ptr;
	hal_pixel_t *dst_ptr;
	int image_real_x, image_real_y;
	int font_real_x, font_real_y;
	int font_real_width, font_real_height;
	int px, py;

	/* Do clipping. */
	if (image_x + margin_left + font_width < 0)
		return;
	if (image_x + margin_left >= image_width)
		return;
	if (image_y + margin_top + font_height < 0)
		return;
	if (image_y + margin_top > image_height)
		return;

	/* Do clipping. */
	image_real_x = image_x + margin_left;
	image_real_y = image_y + margin_top;
	font_real_x = 0;
	font_real_y = 0;
	font_real_width = font_width;
	font_real_height = font_height;
	if (image_real_x < 0) {
		font_real_x -= image_real_x;
		font_real_width += image_real_x;
		image_real_x = 0;
	}
	if (image_real_x + font_real_width >= image_width) {
		font_real_width -= (image_real_x + font_real_width) -
				   image_width;
	}
	if (image_real_y < 0) {
		font_real_y -= image_real_y;
		font_real_height += image_real_y;
		image_real_y = 0;
	}
	if (image_real_y + font_real_height >= image_height) {
		font_real_height -= (image_real_y + font_real_height) -
				    image_height;
	}

	color = hal_make_pixel(255,
			       hal_get_pixel_r(color),
			       hal_get_pixel_g(color),
			       hal_get_pixel_b(color));

	/* Draw. */
	dst_ptr = image + image_real_y * image_width + image_real_x;
	src_ptr = font + font_real_y * font_width + font_real_x;
	for (py = font_real_y; py < font_real_y + font_real_height; py++) {
		for (px = font_real_x; px < font_real_x + font_real_width; px++) {
			/* Draw if non-zero. */
			if (*src_ptr++ == 0)
				dst_ptr++;
			else
				*dst_ptr++ = color;
		}
		dst_ptr += image_width - font_real_width;
		src_ptr += font_width - font_real_width;
	}
}
