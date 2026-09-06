/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/*
 * Playfield Engine
 * Glyph HAL for PC98 Font ROM
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

#include <strato/strato.h>
#include "98glyph.h"

#include <string.h>
#include <assert.h>

#include <conio.h> /* inp, outp */

/*
 * Forward declaration
 */
static uint16_t unicode_to_jis(uint32_t unicode_codepoint);
static uint8_t *get_pc98_font(uint16_t jis_code);
static void wait_vsync(void);

/*
 * Load a glyph data. (TTF)
 */
HAL_DLL
bool
hal_load_glyph_data(
	int slot,
	const uint8_t *data,
	size_t len)
{
	UNUSED_PARAMETER(slot);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(len);

	assert(slot >= 0 && slot < HAL_GLYPH_DATA_COUNT);
	assert(data != NULL);
	assert(len > 0);

	return true;
}

/*
 * Destroy a glyph data.
 */
HAL_DLL
void
hal_destroy_glyph_data(
	int slot)
{
	UNUSED_PARAMETER(slot);

	assert(slot >= 0 && slot < HAL_GLYPH_DATA_COUNT);
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
HAL_DLL
int
hal_get_glyph_width(
	int slot,
	int size,
	uint32_t codepoint)
{
	if (codepoint < 0x80)
		return 8;

	return 16;
}

/*
 * Get a height for a character.
 */
HAL_DLL
int
hal_get_glyph_height(
	int slot,
	int size,
	uint32_t codepoint)
{
	return 16;
}

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
	int *height)
{
	assert(width != NULL);
	assert(height != NULL);

	if (codepoint < 0x100)
		*width = 8;
	else
		*width = 16;

	*height = 16;
}

/*
 * Get a width for a string.
 */
HAL_DLL
int
hal_get_string_width(
	int slot,
	int font_size,
	const char *mbs)
{
	uint32_t c;
	int mblen, w;

	/* Calc for each character. */
	w = 0;
	c = 0;
	while (*mbs != '\0') {
		/* Get a character. */
		mblen = hal_utf8_to_utf32(mbs, &c);
		if (mblen == -1)
			return -1;

		/* Get a character width. */
		w += hal_get_glyph_width(slot, font_size, c);

		/* Move to a next character. */
		mbs += mblen;
	}
	return w;
}

/*
 * Get a height for a string.
 */
HAL_DLL
int
hal_get_string_height(
	int slot,
	int font_size,
	const char *mbs)
{
	return 16;
}

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
	int *height)
{
	*width = hal_get_string_width(slot, size, mbs);
	if (*width > 0)
		*height = 16;
	else
		*height = 0;
}

/*
 * Draw a character.
 */
HAL_DLL
bool
hal_draw_glyph(
	struct hal_image *img,
	int slot,
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
	uint16_t jis_code;
	uint8_t *font;
	int font_x, font_y, width;

	jis_code = unicode_to_jis(codepoint);
	if (jis_code == 0) {
		*ret_w = 0;
		*ret_h = 0;
		return false;
	}

	font = get_pc98_font(jis_code);
	width = ((jis_code >> 8) == 0x20) ? 8 : 16;

	for (font_y = 0; font_y < 16; font_y++) {
		int py = y + font_y;
		if (py < 0 || py >= img->height)
			continue;
		for (font_x = 0; font_x < width; font_x++) {
			int px = x + font_x;
			int byte_pos, is_on;
			if (px < 0 || px >= img->width)
				continue;
			byte_pos = (width == 16) ? (font_y * 2 + (font_x / 8)) : font_y;
			is_on = font[byte_pos] & (0x80 >> (font_x % 8));
			if (is_on)
				img->pixels[py * img->width + px] = 0xffffffff;
		}
	}

	*ret_w = width;
	*ret_h = 16;

	return true;
}

/*
 * Read PC-98 CGROM via the CG window (A4000h).
 */
static uint8_t *
get_pc98_font(uint16_t jis_code)
{
	static uint8_t font_buf[32];
	volatile uint8_t *cg = (volatile uint8_t *)0xa4000;
	int i;
	uint8_t ku  = (uint8_t)(jis_code >> 8);
	uint8_t ten = (uint8_t)(jis_code & 0xff);
	uint8_t is_gaiji_or_symbol = 0;

	memset(font_buf, 0, sizeof(font_buf));

	if (ku >= 0x29 && ku <= 0x2f)
		is_gaiji_or_symbol = 1;
	else if (ku >= 0x76 && ku <= 0x7f)
		is_gaiji_or_symbol = 1;

	wait_vsync();

	outp(0x68, 0x0b);

	if (ku == 0x20) {
		outp(0xa1, 0x00);
		outp(0xa3, ten);
		outp(0xa5, 0x00);

		for (i = 0; i < 16; i++)
			font_buf[i] = cg[i * 2 + 1];
	} else if (!is_gaiji_or_symbol) {
		outp(0xa1, ten);
		outp(0xa3, (uint8_t)(ku - 0x20));
		outp(0xa5, 0x00);

		for (i = 0; i < 32; i++)
			font_buf[i] = cg[i];
	} else {
		outp(0xa1, ten);
		outp(0xa3, (uint8_t)(ku - 0x20));

		outp(0xa5, 0x20);
		for (i = 0; i < 16; i++)
			font_buf[i * 2 + 0] = cg[i * 2 + 1];

		outp(0xa5, 0x00);
		for (i = 0; i < 16; i++)
			font_buf[i * 2 + 1] = cg[i * 2 + 1];
	}

	outp(0x68, 0x0a);

	return font_buf;
}

/* Convert Unicode to PC-98 CGROM JIS code. */
static uint16_t
unicode_to_jis(uint32_t unicode_codepoint)
{
	size_t lo, hi, mid;
	uint16_t u;

	if (unicode_codepoint > 0xffff)
		return 0;

	u = (uint16_t)unicode_codepoint;

	/* ANK / ASCII */
	if (u < 0x80)
		return (uint16_t)(0x2000 | u);

	/* Half Kana (U+FF61-FF9F -> ANK 0xA1-0xDF) */
	if (u >= 0xff61 && u <= 0xff9f)
		return (uint16_t)(0x2000 | (0xa1 + (u - 0xff61)));

	/* Unicode -> JIS X 0208 */
	lo = 0;
	hi = U2J_TABLE_SIZE;
	while (lo < hi) {
		mid = (lo + hi) / 2;
		if (u2j_table[mid].ucs < u)
			lo = mid + 1;
		else
			hi = mid;
	}
	if (lo < U2J_TABLE_SIZE && u2j_table[lo].ucs == u)
		return u2j_table[lo].jis;

	return 0;
}

/* Wait for VSYNC start. I/O 0060h bit 5 = VERTICAL SYNC. */
static void
wait_vsync(void)
{
    while (inp(0x60) & 0x20)
        ;
    while (!(inp(0x60) & 0x20))
        ;
}
