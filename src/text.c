/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/*
 * Suika3
 * Text Subsystem
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
#include "text.h"
#include "image.h"
#include "conf.h"

#include <playfield/playfield.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GLYPH_CACHE_SIZE	512

/*
 * Emoji images
 */
static struct s3_image *emoji_image[S3_EMOJI_COUNT];

/*
 * Last codepoint and the cached image.
 */
struct glyph_cache {
	int slot;
	uint32_t codepoint;
	int size;
	s3_pixel_t color;
	int outline_width;
	s3_pixel_t outline_color;
	struct s3_image *image;
	uint64_t lru_time;
};
static struct glyph_cache glyph_cache[GLYPH_CACHE_SIZE];
static uint64_t glyph_cache_time;

/*
 * Context Table
 *  - Contexts are managed in a table.
 *  - This is because to access by indices in the VM.
 */

#define CONTEXT_MAX	256

struct s3_drawmsg *ctx_tbl[CONTEXT_MAX];

/*
 * Forward declarations
 */
static struct s3_image *load_cached_glyph(int slot, uint32_t codepoint, int size, hal_pixel_t color, int outline_width, hal_pixel_t outline_color);
static bool isgraph_extended(const char **mbs, uint32_t *wc);
static int translate_font_type(int font_type);
static bool draw_emoji(struct s3_drawmsg *context, const char *name,
		       int *w, int *h);

/*
 * Initialize the glyph subsystem.
 */
bool
s3i_init_text(void)
{
	int i;

	/* Cleanup for DLL reuse. */
	s3i_cleanup_text();

	/* Load the fonts. */
	for (i = 0; i < S3_FONT_COUNT; i++) {
		if (conf_font_ttf[i] == NULL)
			continue;

		/* Load the font. */
		if (!pf_load_font(i, conf_font_ttf[i]))
			return false;
	}

	/* Load the emojis. */
	for (i = 0; i < S3_EMOJI_COUNT; i++) {
		if (conf_emoji_name[i] != NULL && conf_emoji_image[i] != NULL) {
			emoji_image[i] = s3_create_image_from_file(conf_emoji_image[i]);
			if (emoji_image[i] == NULL)
				return false;
		}
	}

	return true;
}

/*
 * Cleanup the glyph subsystem.
 */
void
s3i_cleanup_text(void)
{
	int i;

	/* Free the glyph cache. */
	for (i = 0; i < GLYPH_CACHE_SIZE; i++) {
		if (glyph_cache[i].image != NULL) {
			s3_destroy_image(glyph_cache[i].image);
			glyph_cache[i].image = NULL;
		}
	}
	memset(glyph_cache, 0, sizeof(glyph_cache));

	/* Free the text draw contexts. */
	for (i = 0; i < CONTEXT_MAX; i++) {
		if (ctx_tbl[i] != NULL) {
			free(ctx_tbl[i]);
			ctx_tbl[i] = NULL;
		}
	}

	/* Free the emojis. */
	for (i = 0; i < S3_EMOJI_COUNT; i++) {
		if (emoji_image[i] != NULL) {
			s3_destroy_image(emoji_image[i]);
			emoji_image[i] = NULL;
		}
	}
}

/*
 * Convert the top character of a utf-8 string to utf-32.
 */
int
s3_utf8_to_utf32(
	const char *mbs,
	uint32_t *wc)
{
	size_t mbslen, octets, i;
	uint32_t ret;

	assert(mbs != NULL);

	/* If a zero-length string. */
	mbslen = strlen(mbs);
	if(mbslen == 0)
		return 0;

	/* Check the first byte and get the numbers of the octets. */
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
		return -1;	/* Error. */

	/* Check if the string has enough length. */
	if (mbslen < octets)
		return -1;	/* mbs is too short. */

	/* Check the 2nd, 3rd, and 4th bytes. */
	for (i = 1; i < octets; i++) {
		if((mbs[i] & 0xc0) != 0x80)
			return -1;	/* Error. */
	}

	/* Compose bytes and get utf-32. */
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

	/* Return the consumed octets. */
	return (int)octets;
}

/*
 * Count the characters of a utf-8 string.
 */
int
s3_count_utf8_chars(
	const char *mbs)
{
	int count;
	int mblen;

	count = 0;
	while (*mbs != '\0') {
		mblen = s3_utf8_to_utf32(mbs, NULL);
		if (mblen == -1)
			return -1;
		count++;
		mbs += mblen;
	}
	return count;
}

/*
 * Get the glyph width.
 */
int
s3_get_glyph_width(
	int font_type,
	int font_size,
	uint32_t codepoint)
{
	struct s3_image *glyph;

	glyph = load_cached_glyph(font_type, codepoint, font_size, 0, 0, 0);
	if (glyph == NULL)
		return 0;

	return glyph->width;
}

/*
 * Get the glyph height.
 */
int
s3_get_glyph_height(int font_type, int font_size, uint32_t codepoint)
{
	struct s3_image *glyph;

	glyph = load_cached_glyph(font_type, codepoint, font_size, 0, 0, 0);
	if (glyph == NULL)
		return 0;

	return glyph->height;
}

/*
 * Get the string width.
 */
int
s3_get_string_width(int font_type, int font_size, const char *mbs)
{
	uint32_t c;
	int mblen, w;

	/* For all characters. */
	w = 0;
	c = 0; /* warning avoidance on gcc 5.3.1 */
	while (*mbs != '\0') {
		/* Skip escape sequences. */
		while (*mbs == '\\') {
			if (*(mbs + 1) == 'n') {
				mbs += 2;
				continue;
			}
			while (*mbs != '\0' && *mbs != '}')
				mbs++;
			mbs++;
		}

		/* Get a codepoint. */
		mblen = s3_utf8_to_utf32(mbs, &c);
		if (mblen == -1)
			return -1;

		/* Get the character width. */
		w += s3_get_glyph_width(font_type, font_size, c);

		/* Move to the next character. */
		mbs += mblen;
	}
	return w;
}

/*
 * 文字列を描画した際の高さを取得する
 */
int
s3_get_string_height(int font_type, int font_size, const char *mbs)
{
	uint32_t c;
	int mblen, h, h_max;

	/* For all characters. */
	h_max = 0;
	c = 0; /* warning avoidance on gcc 5.3.1 */
	while (*mbs != '\0') {
		/* Skip escape sequences. */
		while (*mbs == '\\') {
			if (*(mbs + 1) == 'n') {
				mbs += 2;
				continue;
			}
			while (*mbs != '\0' && *mbs != '}')
				mbs++;
			mbs++;
		}

		/* Get a codepoint. */
		mblen = s3_utf8_to_utf32(mbs, &c);
		if (mblen == -1)
			return -1;

		/* Get the character width. */
		h = s3_get_glyph_height(font_type, font_size, c);
		if (h > h_max)
			h_max = h;

		/* Move to the next character. */
		mbs += mblen;
	}
	return h;
}

/* Load a glyph */
static struct s3_image *
load_cached_glyph(
	int slot,
	uint32_t codepoint,
	int size,
	s3_pixel_t color,
	int outline_width,
	s3_pixel_t outline_color)
{
	int i, lru_index;
	uint64_t lru_time;

	assert(slot >= 0 && slot < S3_FONT_COUNT);
	assert(size > 0);
	assert(outline_width >= 0);

	slot = translate_font_type(slot);

	/* If cached. */
	lru_index = 0;
	lru_time = (uint64_t)-1;
	for (i = 0; i < GLYPH_CACHE_SIZE; i++) {
		/* If found a top unused entry. */
		if (glyph_cache[i].image == NULL) {
			/* Glyph not found, use this entry. */
			lru_index = i;
			break;
		}

		/* If found a cached glyph. */
		if (glyph_cache[i].slot == slot &&
		    glyph_cache[i].codepoint == codepoint &&
		    glyph_cache[i].size == size &&
		    glyph_cache[i].color == color &&
		    glyph_cache[i].outline_width == outline_width &&
		    glyph_cache[i].outline_color == outline_color &&
		    glyph_cache[i].image != NULL) {
			glyph_cache[i].lru_time = glyph_cache_time++;
			return glyph_cache[i].image;
		}

		/* Otherwise, update the LRU item if it is older. */
		if (glyph_cache[i].lru_time < lru_time) {
			lru_index = i;
			lru_time = glyph_cache[i].lru_time;
		}
	}

	/* Destroy the cached glyph. */
	if (glyph_cache[lru_index].image != NULL) {
		s3_destroy_image(glyph_cache[lru_index].image);
		glyph_cache[lru_index].image = NULL;
	}

	/* Load a glyph. */
	glyph_cache[lru_index].slot = slot;
	glyph_cache[lru_index].codepoint = codepoint;
	glyph_cache[lru_index].size = size;
	glyph_cache[lru_index].color = color;
	glyph_cache[lru_index].outline_width = outline_width;
	glyph_cache[lru_index].outline_color = outline_color;
	glyph_cache[lru_index].lru_time = glyph_cache_time++;
	glyph_cache[lru_index].image = s3_load_glyph_image(slot,
							   codepoint,
							   size,
							   color,
							   outline_width,
							   outline_color);
	if (glyph_cache[lru_index].image == NULL)
		return NULL;

	return glyph_cache[lru_index].image;
}

/* Get an alternative font slot if needed. */
static int
translate_font_type(int font_type)
{
	if (conf_font_ttf[font_type] == NULL)
		return S3_FONT_SELECT1;

	return font_type;
}

/* Check if a supported glyph. */
static bool isgraph_extended(const char **mbs, uint32_t *wc)
{
	int len;

	/* If an English alphabet or signs. */
	if (isgraph((unsigned char)**mbs)) {
		*wc = (unsigned char)**mbs;
		(*mbs)++;
		return true;
	}

	/* Get the codepoint. */
	len = s3_utf8_to_utf32(*mbs, wc);
	if (len < 1)
		return false;
	*mbs += len;

	/* If a Latin alphabet with accents. */
	if (*wc >= 0x00c0 && *wc <= 0x017f)
		return true;

	/* If a Greek alphabet. */
	if (*wc >= 0x0370 && *wc <= 0x3ff)
		return true;

	/* If a Russian alphabet. */
	if (*wc >= 0x410 && *wc <= 0x44f)
		return true;

	/* Not supported. */
	return false;
}

/*
 * Draw a glyph.
 */
bool
s3_draw_glyph(
	struct s3_image *img,
	int font_type,
	int font_size,
	int base_font_size,
	int outline_size,
	int x,
	int y,
	s3_pixel_t color,
	s3_pixel_t outline_color,
	uint32_t codepoint,
	int *ret_w,
	int *ret_h,
	bool is_dim)
{
	struct s3_image *glyph;
	int ofs_y;

	UNUSED_PARAMETER(is_dim);

	/* Load a glyph to the cache slot. */
	glyph = load_cached_glyph(font_type,
				  codepoint,
				  base_font_size,
				  color,
				  outline_size,
				  outline_color);
	if (glyph == NULL)
		return false;

	/* Draw the cached glyph on the destination image. */
	ofs_y = base_font_size - font_size;
	s3_draw_image(img,
		      x,
		      y + ofs_y,
		      glyph,
		      0,
		      0,
		      glyph->width,
		      glyph->height,
		      255,
		      S3_BLEND_GLYPH);

	/* Set the width and height for return. */
	*ret_w = glyph->width;
	*ret_h = glyph->height;

	return true;
}

/*
 * Text layout and drawing
 */

/* Forward declarations. */
static void process_escape_sequence(struct s3_drawmsg *context);
static bool process_escape_sequence_centering(struct s3_drawmsg *context);
static bool process_escape_sequence_rightify(struct s3_drawmsg *context);
static bool process_escape_sequence_leftify(struct s3_drawmsg *context);
static void process_escape_sequence_lf(struct s3_drawmsg *context);
static bool process_escape_sequence_font(struct s3_drawmsg *context);
static bool process_escape_sequence_outline(struct s3_drawmsg *context);
static bool process_escape_sequence_color(struct s3_drawmsg *context);
static bool process_escape_sequence_size(struct s3_drawmsg *context);
static bool process_escape_sequence_wait(struct s3_drawmsg *context);
static bool process_escape_sequence_pen(struct s3_drawmsg *context);
static bool process_escape_sequence_ruby(struct s3_drawmsg *context);
static bool process_escape_sequence_emoticon(struct s3_drawmsg *context);
static bool process_escape_sequence_background(struct s3_drawmsg *context);
static bool process_escape_sequence_line_margin(struct s3_drawmsg *context);
static bool process_escape_sequence_left_top_margins(struct s3_drawmsg *context);
static bool search_for_end_of_escape_sequence(const char **msg);
static bool do_word_wrapping(struct s3_drawmsg *context);
static int get_en_word_width(struct s3_drawmsg *context);
static uint32_t convert_tategaki_char(uint32_t wc);
static bool is_tategaki_punctuation(uint32_t wc);
static bool process_bottom_space(struct s3_drawmsg *context, int glyph_width, int glyph_height);
static bool process_lf(struct s3_drawmsg *context, uint32_t c, int glyph_width, int glyph_height, uint32_t c_next, int next_glyph_width, int next_glyph_height);
static bool is_gyomatsu_kinsoku(uint32_t c);
static bool is_gyoto_kinsoku(uint32_t c);
static bool is_small_kana(uint32_t wc);

/*
 * Initialize a message drawing context.
 */
struct s3_drawmsg *
s3_create_drawmsg(
	struct s3_image *image,
	const char *msg,
	int font,
	int font_size,
	int base_font_size,
	int ruby_size,
	int outline_size,
	int pen_x,
	int pen_y,
	int area_width,
	int area_height,
	int left_margin,
	int right_margin,
	int top_margin,
	int bottom_margin,
	int line_margin,
	int char_margin,
	s3_pixel_t color,
	s3_pixel_t outline_color,
	s3_pixel_t bg_color,
	bool fill_bg,
	bool is_dimming,
	bool ignore_linefeed,
	bool ignore_font,
	bool ignore_outline,
	bool ignore_color,
	bool ignore_size,
	bool ignore_position,
	bool ignore_ruby,
	bool ignore_wait,
	void (*inline_wait_hook)(float),
	bool use_tategaki)
{
	struct s3_drawmsg *context;
	int i;

	for (i = 0; i < CONTEXT_MAX; i++) {
		if (ctx_tbl[i] == NULL)
			break;
	}
	if (i == CONTEXT_MAX) {
		s3_log_tag_error(S3_TR("Too many drawmsg contexts."));
		return false;
	}

	ctx_tbl[i] = malloc(sizeof(struct s3_drawmsg));
	if (ctx_tbl[i] == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	ctx_tbl[i]->index = i;

	context = ctx_tbl[i];
	context->image = image;
	context->msg = msg;
	context->font = font;
	context->font_size = font_size;
	context->base_font_size = base_font_size;
	context->ruby_size = ruby_size;
	context->outline_size = outline_size;
	context->pen_x = pen_x;
	context->pen_y = pen_y;
	context->area_width = area_width;
	context->area_height = area_height;
	context->left_margin = left_margin;
	context->right_margin = right_margin;
	context->top_margin = top_margin;
	context->bottom_margin = bottom_margin;
	context->line_margin = line_margin;
	context->char_margin = char_margin;
	context->default_color = color;
	context->color = color;
	context->outline_color = outline_color;
	context->is_dimming = is_dimming;
	context->ignore_linefeed = ignore_linefeed;
	context->ignore_font = ignore_font;
	context->ignore_outline = ignore_outline;
	context->ignore_color = ignore_color;
	context->ignore_size = ignore_size;
	context->ignore_position = ignore_position;
	context->ignore_ruby = ignore_ruby;
	context->ignore_wait = ignore_wait;
	context->bg_color = bg_color;
	context->fill_bg = fill_bg;
	context->inline_wait_hook = inline_wait_hook;
	context->use_tategaki = use_tategaki;
	context->bg_color = s3_make_pixel((uint32_t)255, (uint32_t)0, (uint32_t)0, (uint32_t)0);

	/* The first character is treated as after-space. */
	context->runtime_is_after_space = true;

	/* Set line-top.. */
	context->runtime_is_line_top = true;

	/* "no-beginning-of-line" rule. */
	context->runtime_is_gyoto_kinsoku = false;
	context->runtime_is_gyoto_kinsoku_second = false;

	/* Set zeros. */
	context->runtime_is_inline_wait = false;
	context->runtime_ruby_x = 0;
	context->runtime_ruby_y = 0;

	/* Is quoted? */
	if (s3_is_quoted_serif(msg) && !use_tategaki)
		context->left_margin += s3_get_glyph_width(font, font_size, U32C('　', 0x3000));

	return context;
}

/*
 * Destroy a message drawing context.
 */
void
s3_destroy_drawmsg(
	struct s3_drawmsg *context)
{
	int index;

	index = context->index;
	free(ctx_tbl[index]);
	ctx_tbl[index] = NULL;
}

/*
 * Get drawmsg index.
 */
int
s3i_drawmsg_to_int(
	struct s3_drawmsg *context)
{
	return context->index;
}

/*
 * Lookup drawmsg by index.
 */
struct s3_drawmsg *
s3i_int_to_drawmsg(
	int index)
{
	return ctx_tbl[index];
}

/*
 * Count the remaining characters excluding escape sequences.
 */
int
s3_count_drawmsg_chars(
	struct s3_drawmsg *context,
	int *width)
{
	const char *msg;
	uint32_t wc;
	int count, mblen;

	if (width != NULL)
		*width = 0;

	count = 0;
	msg = context->msg;
	while (*msg) {
		/* Skip the top escape sequences. */
		while (*msg == '\\') {
			switch (*(msg + 1)) {
			/*
			 * 1-byte escape sequences.
			 */
			case 'c':	/* Centering */
			case 'r':	/* Rightify */
			case 'l':	/* Leftify */
				msg += 2;
				break;
			case 'n':	/* Return */
				msg += 2;
				if (width != NULL)
					return count;
				break;
			/*
			 * Escape sequences with a block.
			 */
			case 'f':	/* Font */
			case 'o':	/* Outline */
			case '#':	/* Color */
			case '@':	/* Size */
			case 'w':	/* Inline Wait */
			case 'p':	/* Pen Move */
			case 'e':	/* Emoji */
			case '^':	/* Ruby */
			case 'L':	/* Line Margin */
			case 'M':	/* Rect Margin */
			case 'k':	/* Back Color */
				if (!search_for_end_of_escape_sequence(&msg))
					return count;
				break;
			default:
				/* Unknown escape sequence. */
				return count;
			}
		}
		if (*msg == '\0')
			break;

		/* Get the next codepoint. */
		mblen = s3_utf8_to_utf32(msg, &wc);
		if (mblen == -1)
			break;

		if (width != NULL)
			*width += s3_get_glyph_width(context->font, context->font_size, wc);

		msg += mblen;
		count++;
	}

	return count;
}

/* Increment the pointer to the end of the current escape sequence. */
static bool search_for_end_of_escape_sequence(const char **msg)
{
	const char *s;
	int len;

	s = *msg;
	len = 0;
	while (*s != '\0' && *s != '}') {
		s++;
		len++;
	}
	if (*s == '\0')
		return false;

	*msg += len + 1;
	return true;
}

/*
 * Draw characters in a message up to (char count) characters.
 */
int
s3_draw_message(
	struct s3_drawmsg *context,	/* a drawing context. */
	int char_count)				/* characters to draw. */
{
	uint32_t wc = 0;
	uint32_t wc_next = 0;
	int i, mblen;
	int glyph_width, glyph_height, next_glyph_width, next_glyph_height, ofs_x, ofs_y;
	int ret_width = 0, ret_height = 0;

	context->font = translate_font_type(context->font);

	if (char_count == -1)
		char_count = s3_count_drawmsg_chars(context, NULL);

	/* For all characters. */
	for (i = 0; i < char_count; i++) {
		if (*context->msg == '\0')
			break;

		/* Skip all of the heading escape sequences. */
		process_escape_sequence(context);
		if (context->runtime_is_inline_wait) {
			context->runtime_is_inline_wait = false;
			return i;
		}

		/* Do word wrapping. */
		if (!do_word_wrapping(context))
			return i;

		/* Get the codepoint. */
		mblen = s3_utf8_to_utf32(context->msg, &wc);
		if (mblen == -1) {
			/* Invalid utf-8 sequence. */
			return -1;
		}

		/* Skip one character for the end-of-line kinsoku. */
		if (s3_utf8_to_utf32(context->msg + mblen, &wc_next) == -1)
			wc_next = 0;

		/* Convert a tategaki glyph. */
		if (context->use_tategaki) {
			wc = convert_tategaki_char(wc);
			wc_next = convert_tategaki_char(wc_next);
		}

		/* Get the glyph width and height. */
		glyph_width = s3_get_glyph_width(context->font, context->font_size, wc);
		glyph_height = s3_get_glyph_height(context->font, context->font_size, wc);
		next_glyph_width = s3_get_glyph_width(context->font, context->font_size, wc_next);
		next_glyph_height = s3_get_glyph_height(context->font, context->font_size, wc_next);

		/* Check for bottom space. */
		if (!process_bottom_space(context, glyph_width, glyph_height))
			return i;

		/* If the right space is not enough, do a line feed. */
		if (!process_lf(context, wc, glyph_width, glyph_height, wc_next, next_glyph_width, next_glyph_height))
			return i;

		/* Calculate the offsets of the small hiragara or katakana. */
		if (context->use_tategaki && is_small_kana(wc)) {
			/* FIXME: Need adjustments. */
			ofs_x = context->font_size / 10;
			ofs_y = -context->font_size / 6;
		} else {
			ofs_x = 0;
			ofs_y = 0;
		}

		/* Fill the background if needed. */
		if (context->fill_bg) {
			s3_fill_image_rect(context->image,
					   context->pen_x,
					   context->pen_y,
					   context->font_size,
					   context->line_margin,
					   context->bg_color);
		}

		/* Draw the glyph. */
		s3_draw_glyph(context->image,
			      context->font,
			      context->font_size,
			      context->base_font_size,
			      context->outline_size,
			      context->pen_x + ofs_x,
			      context->pen_y + ofs_y,
			      context->color,
			      context->outline_color,
			      wc,
			      &ret_width,
			      &ret_height,
			      context->is_dimming);

		/* Update the pen position for ruby. */
		if (!context->use_tategaki) {
			context->runtime_ruby_x = context->pen_x;
			context->runtime_ruby_y = context->pen_y -
				context->ruby_size;
		} else {
			context->runtime_ruby_x = context->pen_x + ret_width;
			context->runtime_ruby_y = context->pen_y;
		}

		/* Move to the next character. */
		context->msg += mblen;
		if (!context->use_tategaki) {
			context->pen_x += glyph_width + context->char_margin;
		} else {
			if (is_tategaki_punctuation(wc))
				context->pen_y += context->font_size;
			else
				context->pen_y += glyph_height;
			context->pen_y += context->char_margin;
		}
	}

	/* Process all the tailing escape sequences. */
	process_escape_sequence(context);
	if (context->runtime_is_inline_wait)
		context->runtime_is_inline_wait = false;

	/* Return the count of the drawn characters. */
	return i;
}

/* Do word wrapping. */
static bool do_word_wrapping(struct s3_drawmsg *context)
{
	if (context->use_tategaki)
		return true;

	if (context->runtime_is_after_space) {
		if (context->pen_x + get_en_word_width(context) >=
		    context->area_width - context->right_margin) {
			if (context->ignore_linefeed)
				return false;

			context->pen_y += context->line_margin;
			context->pen_x = context->left_margin;
		}
	}

	context->runtime_is_after_space = *context->msg == ' ';

	return true;
}

/* Return the word width if it's alphabets, otherwise 0. */
static int get_en_word_width(struct s3_drawmsg *context)
{
	const char *m;
	uint32_t wc;
	int width;

	m = context->msg;
	width = 0;
	while (isgraph_extended(&m, &wc))
		width += s3_get_glyph_width(context->font, context->font_size, wc);

	return width;
}

/* Check for the bottom space. */
static bool
process_bottom_space(
	struct s3_drawmsg *context,
	int glyph_width,
	int glyph_height)
{
	if (!context->use_tategaki) {
		/* If the bottom space is not enough. */
		if (context->pen_y + glyph_height >= context->area_height - context->bottom_margin)
			return false;
	} else {
		/* If the left space is not enough. */
		if (context->pen_x - glyph_width < context->left_margin)
			return false;
	}

	return true;
}

/* Do line feed if the rigth space is not enough. */
static bool process_lf(
	struct s3_drawmsg *context,
	uint32_t c,
	int glyph_width,
	int glyph_height,
	uint32_t c_next,
	int next_glyph_width,
	int next_glyph_height)
{
	bool line_top, gyoto_second;

	line_top = context->runtime_is_line_top;
	gyoto_second = context->runtime_is_gyoto_kinsoku_second;
	context->runtime_is_line_top = false;
	context->runtime_is_gyoto_kinsoku_second = false;

	/* If we detected a gyoto kinsoku on the previous character. */
	if (context->runtime_is_gyoto_kinsoku) {
		/* Cancel gyoto kinsoku. */
		context->runtime_is_gyoto_kinsoku = false;
		context->runtime_is_gyoto_kinsoku_second = true;

		/* End drawing if we inore LF. */
		if (context->ignore_linefeed && context->line_margin == 0)
			return false;

		/* Do not line feed. */
		return true;
	}

	if (!context->use_tategaki) {
		int limit = context->area_width - context->right_margin;

		/* If the right space is not enough. */
		if (context->pen_x + glyph_width + context->char_margin >= limit) {
			/* Do not line feed if c is a gyoto kinsoku character. */
			if (is_gyoto_kinsoku(c) && !line_top && !gyoto_second)
				return true;

			/* End drawing if we ignore LF. */
			if (context->ignore_linefeed && context->line_margin == 0)
				return false;

			/* Do line feed. */
			context->pen_y += context->line_margin;
			context->pen_x = context->left_margin;
			context->runtime_is_line_top = true;
			if (context->pen_y + context->line_margin >= context->area_height)
				return false; /* Finish drawing. */
		} else {
			/* If the right space is enough. */
			if (line_top) {
				/* Do not line feed if line top. */
			} else if (is_gyomatsu_kinsoku(c)) {
				/*
				 * Do not line feed if c is a gyomatsu
				 * kinsoku character.
				 */
			} else if (is_gyoto_kinsoku(c_next) &&
				   context->pen_x + glyph_width +
				   context->char_margin + next_glyph_width +
				   context->char_margin >= limit) {
				/*
				 * Do not line feed if c_next is a
				 * gyoto kinsoku character and the
				 * right space is not enough.
				 * Set a flag not to line feed at the next character.
				 */
				context->runtime_is_gyoto_kinsoku = true;
			}
		}
	} else {
		int limit = context->area_height - context->bottom_margin;

		/* If the bottom space is not enough. */
		if (context->pen_y + glyph_height + context->char_margin >= limit) {
			/* Do not line feed if c is a gyoto kinsoku character. */
			if (is_gyoto_kinsoku(c) && !line_top && !gyoto_second)
				return true;

			/* End drawing if we ignore LF. */
			if (context->ignore_linefeed && context->line_margin == 0)
				return false;

			/* Do line feed. */
			context->pen_x -= context->line_margin;
			context->pen_y = context->top_margin;
			context->runtime_is_line_top = true;
			if (context->pen_x + context->line_margin < 0)
				return false; /* Finish drawing. */
		} else {
			/* If the bottom space is enough. */
			if (line_top) {
				/* Do not line feed if at the line top. */
			} else if (is_gyomatsu_kinsoku(c)) {
				/* Do not line feed if c is a gyomatsu kinsoku character. */
			} else if (is_gyoto_kinsoku(c_next) &&
				   context->pen_y + glyph_height + context->char_margin + next_glyph_height + context->char_margin >= limit) {
				/*
				 * Do not line feed if c_next if a
				 * gyoto kinsoku character and the
				 * bottom space is not enough.
				 * Set a flag not to line feed at the next
				 * character.
				 */
				context->runtime_is_gyoto_kinsoku = true;
			}
		}
	}

	/* Do not line feed */
	return true;
}

/* Check if "no-end-of-line" ruled character. */
static bool is_gyomatsu_kinsoku(uint32_t c)
{
	switch (c) {
	case '(':
	case '[':
	case '{':
	case U32C('（', 0xff08):
	case U32C('︵', 0xfe35):
	case U32C('｛', 0xff5b):
	case U32C('︷', 0xfe37):
	case U32C('「', 0x300c):
	case U32C('﹁', 0xfe41):
	case U32C('『', 0x300e):
	case U32C('﹃', 0xfe43):
	case U32C('【', 0x3010):
	case U32C('︻', 0xfe3b):
	case U32C('［', 0xff3b):
	case U32C('﹇', 0xfe47):
	case U32C('〔', 0x3014):
	case U32C('︹', 0xfe39):
	case U32C('〘', 0x3018):
	case U32C('〖', 0x3016):
	case U32C('《', 0x300a):
	case U32C('︽', 0xfe3d):
	case U32C('〈', 0x3008):
	case U32C('〈', 0x2329):
	case U32C('｟', 0xff5f):
	case U32C('«', 0x00ab):
	case U32C('〝', 0x301d):
	case U32C('‘', 0x2018):
	case U32C('“', 0x201c):
		return true;
	default:
		break;
	}
	return false;
}

/* Check if "no-beginning-of-line" ruled character. */
static bool is_gyoto_kinsoku(uint32_t c)
{
	switch (c) {
	case ' ':
	case ',':
	case '.':
	case '!':
	case '?':
	case ':':
	case ';':
	case ')':
	case ']':
	case '}':
	case '/':
	case U32C('？', 0xff1f):
	case U32C('、', 0x3001):
	case U32C('︑', 0xfe11):
	case U32C('，', 0xff0c):
	case U32C('︐', 0xfe10):
	case U32C('。', 0x3002):
	case U32C('︒', 0xfe12):
	case U32C('〕', 0x3015):
	case U32C('〉', 0x3009):
	case U32C('》', 0x300b):
	case U32C('」', 0x300d):
	case U32C('』', 0x300f):
	case U32C('】', 0x3011):
	case U32C('〙', 0x3019):
	case U32C('〗', 0x3017):
	case U32C('︘', 0xfe18):
	case U32C('〟', 0x301f):
	case U32C('’', 0x2019):
	case U32C('”', 0x201d):
	case U32C('｠', 0xff60):
	case U32C('»', 0x00bb):
	case U32C('ゝ', 0x309d):
	case U32C('ゞ', 0x309e):
	case U32C('‐', 0x002d):
	case U32C('–', 0x2013):
	case U32C('ー', 0x30fc):
	case U32C('丨', 0x4e28):
	case U32C('︙', 0xfe19):
	case U32C('︰', 0xfe30):
	case U32C('ァ', 0x30a1):
	case U32C('ィ', 0x30a3):
	case U32C('ゥ', 0x30a5):
	case U32C('ェ', 0x30a7):
	case U32C('ォ', 0x30a9):
	case U32C('ッ', 0x30c3):
	case U32C('ャ', 0x30e3):
	case U32C('ュ', 0x30e5):
	case U32C('ョ', 0x30e7):
	case U32C('ヮ', 0x30ee):
	case U32C('ヵ', 0x30f5):
	case U32C('ヶ', 0x30f6):
	case U32C('ぁ', 0x3041):
	case U32C('ぃ', 0x3043):
	case U32C('ぅ', 0x3045):
	case U32C('ぇ', 0x3047):
	case U32C('ぉ', 0x3049):
	case U32C('っ', 0x3063):
	case U32C('ゃ', 0x3083):
	case U32C('ゅ', 0x3085):
	case U32C('ょ', 0x3087):
	case U32C('ゎ', 0x308e):
	case U32C('ゕ', 0x3095):
	case U32C('ゖ', 0x3096):
	case U32C('ㇰ', 0x31f0):
	case U32C('ㇱ', 0x31f1):
	case U32C('ㇲ', 0x31f2):
	case U32C('ㇳ', 0x31f3):
	case U32C('ㇴ', 0x31f4):
	case U32C('ㇵ', 0x31f5):
	case U32C('ㇶ', 0x31f6):
	case U32C('ㇷ', 0x31f7):
	case U32C('ㇸ', 0x31f8):
	case U32C('ㇹ', 0x31f9):
	case U32C('゚', 0x309a):
	case U32C('ㇺ', 0x31fa):
	case U32C('ㇻ', 0x31fb):
	case U32C('ㇼ', 0x31fc):
	case U32C('ㇽ', 0x31fd):
	case U32C('ㇾ', 0x31fe):
	case U32C('ㇿ', 0x31ff):
	case U32C('々', 0x3005):
	case U32C('〻', 0x303b):
	case U32C('゠', 0x30a0):
	case U32C('〜', 0x301c):
	case U32C('～', 0xff5e):
	case U32C('‼', 0x203c):
	case U32C('⁇', 0x2047):
	case U32C('⁈', 0x2048):
	case U32C('⁉', 0x2049):
	case U32C('・', 0x30fb):
		return true;
	default:
		break;
	}
	return false;
}

/* Convert a character for tategaki. */
static uint32_t convert_tategaki_char(uint32_t wc)
{
	switch (wc) {
	case U32C('、', 0x3001): return U32C('︑', 0xfe11);
	case U32C('，', 0xff0c): return U32C('︐', 0xfe10);
	case U32C('。', 0x3002): return U32C('︒', 0xfe12);
	case U32C('（', 0xff08): return U32C('︵', 0xfe35);
	case U32C('）', 0xff09): return U32C('︶', 0xfe36);
	case U32C('｛', 0xff5b): return U32C('︷', 0xfe37);
	case U32C('｝', 0xff5d): return U32C('︸', 0xfe38);
	case U32C('「', 0x300c): return U32C('﹁', 0xfe41);
	case U32C('」', 0x300d): return U32C('﹂', 0xfe42);
	case U32C('『', 0x300e): return U32C('﹃', 0xfe43);
	case U32C('』', 0x300f): return U32C('﹄', 0xfe44);
	case U32C('【', 0x3010): return U32C('︻', 0xfe3b);
	case U32C('】', 0x3011): return U32C('︼', 0xfe3c);
	case U32C('［', 0xff3b): return U32C('﹇', 0xfe47);
	case U32C('］', 0xff3d): return U32C('﹈', 0xfe48);
	case U32C('〔', 0x3014): return U32C('︹', 0xfe39);
	case U32C('〕', 0x3015): return U32C('︺', 0xfe3a);
	case U32C('…', 0x2026): return U32C('︙', 0xfe19);
	case U32C('‥', 0x2025): return U32C('︰', 0xfe30);
	case U32C('ー', 0x30fc): return U32C('丨', 0x4e28);
	case U32C('─', 0x2500): return U32C('丨', 0x4e28);
	case U32C('〈', 0x3008): return U32C('︿', 0xfe3f);
	case U32C('〈', 0x2329): return U32C('︿', 0xfe3f);
	case U32C('〉', 0x3009): return U32C('⟩', 0xfe40);
	case U32C('〉', 0x232a): return U32C('⟩', 0xfe40);
	case U32C('《', 0x300a): return U32C('︽', 0xfe3d);
	case U32C('》', 0x300b): return U32C('︾', 0xfe3e);
	case U32C('〖', 0x3016): return U32C('︗', 0xfe17);
	case U32C('〗', 0x3017): return U32C('︘', 0xfe18);
	default:
		break;
	}
	return wc;
}

/* Check if a character is a tategaki punctuation. */
static bool is_tategaki_punctuation(uint32_t wc)
{
	switch (wc) {
	case U32C('︑', 0xfe11): return true;
	case U32C('︐', 0xfe10): return true;
	case U32C('︒', 0xfe12): return true;
	case U32C('︵', 0xfe35): return true;
	case U32C('︶', 0xfe36): return true;
	case U32C('︷', 0xfe37): return true;
	case U32C('︸', 0xfe38): return true;
	case U32C('﹁', 0xfe41): return true;
	case U32C('﹂', 0xfe42): return true;
	case U32C('﹃', 0xfe43): return true;
	case U32C('﹄', 0xfe44): return true;
	case U32C('︻', 0xfe3b): return true;
	case U32C('︼', 0xfe3c): return true;
	case U32C('﹇', 0xfe47): return true;
	case U32C('﹈', 0xfe48): return true;
	case U32C('︹', 0xfe39): return true;
	case U32C('︺', 0xfe3a): return true;
	case U32C('︙', 0xfe19): return true;
	case U32C('︰', 0xfe30): return true;
	case U32C('丨', 0x4e28): return true;
	default:
		break;
	}
	return false;
}

/* Check if a character is a small hiragana or katakana. */
static bool is_small_kana(uint32_t wc)
{
	switch (wc) {
	case U32C('ぁ', 0x3041): return true;
	case U32C('ぃ', 0x3043): return true;
	case U32C('ぅ', 0x3045): return true;
	case U32C('ぇ', 0x3047): return true;
	case U32C('ぉ', 0x3049): return true;
	case U32C('っ', 0x3063): return true;
	case U32C('ゃ', 0x3083): return true;
	case U32C('ゅ', 0x3085): return true;
	case U32C('ょ', 0x3087): return true;
	case U32C('ゎ', 0x308e): return true;
	case U32C('ゕ', 0x3095): return true;
	case U32C('ゖ', 0x3096): return true;
	case U32C('ァ', 0x30a1): return true;
	case U32C('ィ', 0x30a3): return true;
	case U32C('ゥ', 0x30a5): return true;
	case U32C('ェ', 0x30a7): return true;
	case U32C('ォ', 0x30a9): return true;
	case U32C('ッ', 0x30c3): return true;
	case U32C('ャ', 0x30e3): return true;
	case U32C('ュ', 0x30e5): return true;
	case U32C('ョ', 0x30e7): return true;
	case U32C('ヮ', 0x30ee): return true;
	case U32C('ヵ', 0x30f5): return true;
	case U32C('ヶ', 0x30f6): return true;
	default: break;
	}
	return false;
}

/*
 * Check if a character is an escape sequence character.
 */
bool s3_is_escape_sequence_char(char c)
{
	switch (c) {
	case 'c':	/* Centering */
	case 'r':	/* Rightify */
	case 'l':	/* Leftify */
	case 'n':	/* Line Feed */
	case 'f':	/* Font */
	case 'o':	/* Outline */
	case '#':	/* Color */
	case '@':	/* Size */
	case 'w':	/* Inline Wait */
	case 'p':	/* Pen Move */
	case 'e':	/* Emoji */
	case '^':	/* Ruby */
	case 'L':	/* Line Margin */
	case 'M':	/* Rect Margin */
	case 'k':	/* Back Color */
		return true;
	default:
		break;
	}

	return false;
}

/* Process all the heading escape sequence. */
static void process_escape_sequence(struct s3_drawmsg *context)
{
	char c;
	int adjust;

	/* Continue while the top is an escape sequence. */
	adjust = 0;
	while (*context->msg == '\\' || *context->msg == '\n') {
		/* Emulate '\n' by "\\n" */
		if (*context->msg == '\n') {
			c = 'n';
			adjust = -1;
		} else {
			c = *(context->msg + 1);
			adjust = 0;
		}

		switch (c) {
		/*
		 * One character escape sequences.
		 */
		case 'c':
			/* Centering */
			if (!process_escape_sequence_centering(context))
				return; /* Invalid */
			break;
		case 'r':
			/* Rightify */
			if (!process_escape_sequence_rightify(context))
				return; /* Invalid */
			break;
		case 'l':
			/* Leftify */
			if (!process_escape_sequence_leftify(context))
				return; /* Invalid */
			break;
		case 'n':
			/* Line Feed */
			process_escape_sequence_lf(context);
			break;
		/*
		 * Blocked escape sequences.
		 */
		case 'f':
			/* Font */
			if (!process_escape_sequence_font(context))
				return; /* Invalid */
			break;
		case 'o':
			/* Outline */
			if (!process_escape_sequence_outline(context))
				return; /* Invalid */
			break;
		case '#':
			/* Color */
			if (!process_escape_sequence_color(context))
				return; /* Invalid */
			break;
		case '@':
			/* Size */
			if (!process_escape_sequence_size(context))
				return; /* Invalid */
			break;
		case 'w':
			/* Inline Wait */
			if (!process_escape_sequence_wait(context))
				return; /* Invalid */
			break;
		case 'p':
			/* Pen Move */
			if (!process_escape_sequence_pen(context))
				return; /* Invalid */
			break;
		case 'e':
			/* Emoji */
			if (!process_escape_sequence_emoticon(context))
				return; /* Invalid */
			break;
		case '^':
			/* ルビ */
			if (!process_escape_sequence_ruby(context))
				return; /* Invalid */
			break;
		case 'k':
			/* Back Color */
			if (!process_escape_sequence_background(context))
				return; /* Invalid */
			break;
		case 'L':
			/* Line Margin */
			if (!process_escape_sequence_line_margin(context))
				return; /* Invalid */
			break;
		case 'M':
			/* Rect Margins */
			if (!process_escape_sequence_left_top_margins(context))
				return; /* Invalid */
			break;
		default:
			return; /* Invalid */
		}

		/* For '\n'. */
		context->msg += adjust;
	}
}

/* Process the line feed escape sequence. ("\\n") */
static void process_escape_sequence_lf(struct s3_drawmsg *context)
{
	if (context->ignore_linefeed) {
		context->msg += 2;
		return;
	}

	if (!context->use_tategaki) {
		context->pen_y += context->line_margin;
		context->pen_x = context->left_margin;
	} else {
		context->pen_x -= context->line_margin;
		context->pen_y = context->top_margin;
	}
	context->msg += 2;
}

/* Process the font escape sequence. ("\\f{X}") */
static bool process_escape_sequence_font(struct s3_drawmsg *context)
{
	char font_type;
	const char *p;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'f');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (strlen(p + 3) < 6)
		return false;

	/* Check for '}'. */
	if (*(p + 4) != '}')
		return false;

	if (!context->ignore_font) {
		/* Read the font type. */
		font_type = *(p + 3);
		switch (font_type) {
		case '1':
			context->font = S3_FONT_SELECT1;
			break;
		case '2':
			context->font = S3_FONT_SELECT2;
			break;
		case '3':
			context->font = S3_FONT_SELECT3;
			break;
		case '4':
			context->font = S3_FONT_SELECT4;
			break;
		default:
			break;
		}
	}

	/* "\\f{" + "X" + "}" */
	context->msg += 3 + 1 + 1;
	return true;
}

/* Process the outline escape sequence. ("\\o{X}") */
static bool process_escape_sequence_outline(struct s3_drawmsg *context)
{
	const char *p;
	char size_spec[8];
	int i;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'o');

	/* Check for '{'. */
	if (*(p + 2) != '{') {
		s3_log_out_of_memory();
		return false;
	}

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the size. */
	for (i = 0; i < (int)sizeof(size_spec) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		size_spec[i] = *(p + 3 + i);
	}
	size_spec[i] = '\0';

	if (!context->ignore_outline)
		context->outline_size = atoi(size_spec);

	/* "\\o{" + "xxx" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the color escape sequence. ("\\#{RRGGBB}") */
static bool process_escape_sequence_color(struct s3_drawmsg *context)
{
	char color_code[7];
	const char *p;
	uint32_t r, g, b;
	int rgb;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == '#');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* \#{default} */
	if (*(p + 3) == 'd' &&
	    *(p + 4) == 'e' &&
	    *(p + 5) == 'f' &&
	    *(p + 6) == 'a' &&
	    *(p + 7) == 'u' &&
	    *(p + 8) == 'l' &&
	    *(p + 9) == 't' &&
	    *(p + 10) == '}') {
		context->color = context->default_color;
		context->msg += 3 + 7 + 1;
		return true;
	}

	/* If too short. */
	if (strlen(p + 3) < 6)
		return false;

	/* Check for '}'. */
	if (*(p + 9) != '}')
		return false;

	if (!context->ignore_color) {
		/* Read the color code. */
		memcpy(color_code, p + 3, 6);
		color_code[6] = '\0';
		rgb = 0;
		sscanf(color_code, "%x", &rgb);
		r = (rgb >> 16) & 0xff;
		g = (rgb >> 8) & 0xff;
		b = rgb & 0xff;
		context->color = s3_make_pixel(0xff, r, g, b);
	}

	/* "\\#{" + "RRGGBB" + "}" */
	context->msg += 3 + 6 + 1;
	return true;
}

/* Process the size escape sequence. ("\\@{XXX}") */
static bool process_escape_sequence_size(struct s3_drawmsg *context)
{
	char size_spec[8];
	const char *p;
	int i, size;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == '@');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the size. */
	for (i = 0; i < (int)sizeof(size_spec) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		size_spec[i] = *(p + 3 + i);
	}
	size_spec[i] = '\0';

	if (!context->ignore_size) {
		/* Convert the size to an integer. */
		size = 0;
		if (size_spec[0] == '!') {
			sscanf(&size_spec[1], "%d", &size);

			/* Change the base size. */
			context->base_font_size = size;
		} else {
			sscanf(&size_spec[0], "%d", &size);
		}

		/* Change the font size. */
		context->font_size = size;
	}

	/* "\\@{" + "xxx" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the inline wait escape sequence. ("\\w{f.f}") */
static bool process_escape_sequence_wait(struct s3_drawmsg *context)
{
	char time_spec[16];
	const char *p;
	float wait_time;
	int i;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'w');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the time. */
	for (i = 0; i < (int)sizeof(time_spec) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		time_spec[i] = *(p + 3 + i);
	}
	time_spec[i] = '\0';

	if (!context->ignore_wait) {
		/* Convert the time to a float. */
		sscanf(time_spec, "%f", &wait_time);

		/* Pricess a wait. */
		context->runtime_is_inline_wait = true;
		context->inline_wait_hook(wait_time);
	}

	/* "\\w{" + "f.f" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the pen move escape sequence. ("\\p{x,y}") */
static bool process_escape_sequence_pen(struct s3_drawmsg *context)
{
	char pos_spec[32];
	const char *p;
	int i, pen_x, pen_y;
	bool separator_found;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'p');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the position. */
	separator_found = false;
	for (i = 0; i < (int)sizeof(pos_spec) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		if (*(p + 3 + i) == ',')
			separator_found = true;
		pos_spec[i] = *(p + 3 + i);
	}
	pos_spec[i] = '\0';
	if (!separator_found)
		return false;

	if (!context->ignore_position) {
		/* Convert to integers. */
		sscanf(pos_spec, "%d,%d", &pen_x, &pen_y);

		/* Update the pen. */
		context->pen_x = pen_x;
		context->pen_y = pen_y;
	}

	/* "\\p{" + "x,y" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the ruby escape sequence. ("\\^{ruby}") */
static bool process_escape_sequence_ruby(struct s3_drawmsg *context)
{
	char ruby[64];
	const char *p;
	uint32_t wc = 0;
	int i, mblen, ret_w, ret_h;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == '^');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the ruby. */
	for (i = 0; i < (int)sizeof(ruby) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		ruby[i] = *(p + 3 + i);
	}
	ruby[i] = '\0';

	/* \^{ + ruby[] + } */
	context->msg += 3 + i + 1;

	if (context->ignore_ruby)
		return true;

	/* Draw the ruby. */
	p = ruby;
	while (*p) {
		mblen = s3_utf8_to_utf32(p, &wc);
		if (mblen == -1)
			return false;

		s3_draw_glyph(context->image,
			      context->font,
			      context->ruby_size,
			      context->ruby_size,
			      context->outline_size,
			      context->runtime_ruby_x,
			      context->runtime_ruby_y,
			      context->color,
			      context->outline_color,
			      wc,
			      &ret_w,
			      &ret_h,
			      context->is_dimming);

		if (!context->use_tategaki)
			context->runtime_ruby_x += ret_w;
		else
			context->runtime_ruby_y += ret_h;

		p += mblen;
	}

	return true;
}

/* Process the emoji escape sequence. ("\\e{emoji}") */
static bool process_escape_sequence_emoticon(struct s3_drawmsg *context)
{
	char name[256];
	const char *p;
	int i, ret_w, ret_h;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'e');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the emoji name. */
	for (i = 0; i < (int)sizeof(name) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		name[i] = *(p + 3 + i);
	}
	name[i] = '\0';

	/* \e{ + name[] + } */
	context->msg += 3 + i + 1;

	/* Draw the emoji. */
	ret_w = 0;
	ret_h = 0;
	if (!draw_emoji(context, name, &ret_w, &ret_h))
		return false;

	if (!context->use_tategaki)
		context->pen_x += ret_w;
	else
		context->pen_y += ret_h;

	return true;
}

/* Process the line margin escape sequence. ("\\L{n}") */
static bool process_escape_sequence_line_margin(struct s3_drawmsg *context)
{
	char digits[32];
	const char *p;
	int i;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'L');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the size. */
	for (i = 0; i < (int)sizeof(digits) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		digits[i] = *(p + 3 + i);
	}
	digits[i] = '\0';

	/* Update the line margin. */
	context->line_margin = 0;
	sscanf(digits, "%d", &context->line_margin);

	/* "\\L{" + "xxx" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the rect margin escape sequence. ("\\M{x,y}") */
static bool process_escape_sequence_left_top_margins(struct s3_drawmsg *context)
{
	char margin_spec[32];
	const char *p;
	int i, left, top;
	bool separator_found;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'M');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* If too short. */
	if (*(p + 3) == '\0') {
		s3_log_out_of_memory();
		return false;
	}

	/* Read the margin. */
	separator_found = false;
	for (i = 0; i < (int)sizeof(margin_spec) - 1; i++) {
		if (*(p + 3 + i) == '\0')
			return false;
		if (*(p + 3 + i) == '}')
			break;
		if (*(p + 3 + i) == ',')
			separator_found = true;
		margin_spec[i] = *(p + 3 + i);
	}
	margin_spec[i] = '\0';
	if (!separator_found)
		return false;

	if (!context->ignore_position) {
		/* Convert the margin to integers. */
		sscanf(margin_spec, "%d,%d", &left, &top);

		/* Update the margin and the pen. */
		if (!context->use_tategaki) {
			context->left_margin = left;
			context->top_margin = top;
			context->pen_x = left;
			context->pen_y = top;
		} else {
			context->right_margin = left;
			context->top_margin = top;
			context->pen_x = left;
			context->pen_y = top;
		}
	}

	/* "\\M{" + "x,y" + "}" */
	context->msg += 3 + i + 1;
	return true;
}

/* Process the centering escape sequence. ("\\c") */
static bool process_escape_sequence_centering(struct s3_drawmsg *context)
{
	int width;

	s3_count_drawmsg_chars(context, &width);

	if (!context->use_tategaki)
		context->pen_x = (context->area_width - width) / 2;
	else
		context->pen_y = (context->area_height - width) / 2;

	context->msg += 2;
	return true;
}

/* Process the rightify escape sequence. ("\\r") */
static bool process_escape_sequence_rightify(struct s3_drawmsg *context)
{
	int width;

	s3_count_drawmsg_chars(context, &width);

	if (!context->use_tategaki)
		context->pen_x = context->area_width - context->right_margin - width - 1;
	else
		context->pen_y = context->area_height - context->bottom_margin - width;

	context->msg += 2;
	return true;
}

/* Process the leftify escape sequence. ("\\l") */
static bool process_escape_sequence_leftify(struct s3_drawmsg *context)
{
	int width;

	s3_count_drawmsg_chars(context, &width);

	if (!context->use_tategaki)
		context->pen_x = context->left_margin;
	else
		context->pen_y = context->top_margin;

	context->msg += 2;
	return true;
}

/* Process the back color escape sequence. ("\\k{RRGGBB}") */
static bool process_escape_sequence_background(struct s3_drawmsg *context)
{
	char color_code[7];
	const char *p;
	uint32_t r, g, b;
	int rgb;

	p = context->msg;
	assert(*p == '\\');
	assert(*(p + 1) == 'k');

	/* Check for '{'. */
	if (*(p + 2) != '{')
		return false;

	/* \k{off} */
	if (*(p + 3) == 'o' &&
	    *(p + 4) == 'f' &&
	    *(p + 5) == 'f' &&
	    *(p + 6) == '}') {
		context->fill_bg = false;
		context->msg += 3 + 3 + 1;
		return true;
	}

	/* If too short. */
	if (strlen(p + 3) < 6)
		return false;

	/* Check for '}'. */
	if (*(p + 9) != '}')
		return false;

	if (!context->ignore_color) {
		/* Read the color. */
		memcpy(color_code, p + 3, 6);
		color_code[6] = '\0';
		rgb = 0;
		sscanf(color_code, "%x", &rgb);
		r = (rgb >> 16) & 0xff;
		g = (rgb >> 8) & 0xff;
		b = rgb & 0xff;
		context->bg_color = s3_make_pixel(0xff, r, g, b);
		context->fill_bg = true;
	}

	/* "\\k{" + "RRGGBB" + "}" */
	context->msg += 3 + 6 + 1;
	return true;
}

/*
 * Get a pen position.
 */
void
s3_get_drawmsg_pen_position(
	struct s3_drawmsg *context,
	int *pen_x,
	int *pen_y)
{
	*pen_x = context->pen_x;
	*pen_y = context->pen_y;
}

/*
 * Emoji
 */

/* Draw an emoticon. */
static bool
draw_emoji(
	struct s3_drawmsg *context,
	const char *name,
	int *w,
	int *h)
{
	int i;

	for (i = 0; i < S3_EMOJI_COUNT; i++) {
		if (conf_emoji_name[i] == NULL || conf_emoji_image[i] == NULL)
			continue;
		if (strcmp(name, conf_emoji_name[i]) == 0)
			break;
	}
	if (i == S3_EMOJI_COUNT)
		return false;

	if (emoji_image[i] == NULL)
		return false;

	*w = emoji_image[i]->width;
	*h = emoji_image[i]->height;

	context->runtime_is_line_top = false;
	if (!context->use_tategaki) {
		int limit = context->area_width - context->right_margin;

		/* If the right space is not enough. */
		if (context->pen_x + *w + context->char_margin >= limit) {
			/* If we ifnore LF. */
			if (context->ignore_linefeed && context->line_margin == 0)
				return false; /* Finish drawing. */

			/* Do line feed. */
			context->pen_y += context->line_margin;
			context->pen_x = context->left_margin;
			context->runtime_is_line_top = true;
			if (context->pen_y + context->line_margin >= context->area_height)
				return false; /* Finish drawing. */
		}
	} else {
		int limit = context->area_height - context->bottom_margin;

		/* If the bottom space is not enough. */
		if (context->pen_y + *h + context->char_margin >= limit) {
			/* If we ignore LF. */
			if (context->ignore_linefeed && context->line_margin == 0)
				return false; /* Finish drawing. */

			/* Do line feed. */
			context->pen_x -= context->line_margin;
			context->pen_y = context->top_margin;
			context->runtime_is_line_top = true;
			if (context->pen_x + context->line_margin < 0)
				return false; /* Finish drawing. */
		}
	}

	s3_draw_image(context->image,
		      context->pen_x,
		      context->pen_y,
		      emoji_image[i],
		      0,
		      0,
		      emoji_image[i]->width,
		      emoji_image[i]->height,
		      255,
		      S3_BLEND_EMOJI);

	if (!context->use_tategaki)
		*w += context->char_margin;
	else
		*h += context->char_margin;

	return true;
}

/*
 * Check if a message is a quoted serif.
 */
bool
s3_is_quoted_serif(
	const char *msg)
{
	UNUSED_PARAMETER(msg);

	/* XXX: may be removed. */
	return false;
}

/*
 * Ignore inline wait.
 */
void
s3_set_drawmsg_ignore_inline_wait(
	struct s3_drawmsg *context)
{
	context->ignore_wait = true;
}
