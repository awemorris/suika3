/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "pencil" tag implementation
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
#include "conf.h"
#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct layer_name {
	const char *name;
	int index;
};

static struct layer_name layer_name[] = {
	{"bg",		S3_LAYER_BG},
	{"bg2",		S3_LAYER_BG},
	{"efb1",	S3_LAYER_EFB1},
	{"efb2",	S3_LAYER_EFB2},
	{"efb3",	S3_LAYER_EFB3},
	{"efb4",	S3_LAYER_EFB4},
	{"chb",		S3_LAYER_CHB},
	{"chl",		S3_LAYER_CHL},
	{"chlc",	S3_LAYER_CHLC},
	{"chr",		S3_LAYER_CHR},
	{"chrc",	S3_LAYER_CHRC},
	{"chc",		S3_LAYER_CHC},
	{"eff1",	S3_LAYER_EFF1},
	{"eff2",	S3_LAYER_EFF2},
	{"eff3",	S3_LAYER_EFF3},
	{"eff4",	S3_LAYER_EFF4},
	{"chf",		S3_LAYER_CHF},
	{"text1",	S3_LAYER_TEXT1},
	{"text2",	S3_LAYER_TEXT2},
	{"text3",	S3_LAYER_TEXT3},
	{"text4",	S3_LAYER_TEXT4},
	{"text5",	S3_LAYER_TEXT5},
	{"text6",	S3_LAYER_TEXT6},
	{"text7",	S3_LAYER_TEXT7},
	{"text8",	S3_LAYER_TEXT8},
};

static int get_layer_index(const char *layer);
static s3_pixel_t parse_color_code(const char *color);

/*
 * The "pencil" tag implementation.
 */
bool
s3i_tag_pencil(
	void *p)
{
	const char *layer;
	int font_type;
	int font_size;
	const char *color;
	int outline_width;
	const char *outline_color;
	int line_margin;
	int char_margin;
	int x;
	int y;
	int width;
	int height;
	const char *text;
	struct s3_image *layer_image;
	int margin_right;
	int margin_bottom;
	int layer_index;
	s3_pixel_t color_p;
	s3_pixel_t outline_color_p;
	struct s3_drawmsg *context;

	UNUSED_PARAMETER(p);

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the arguments. */
	layer = s3_get_tag_arg_string("layer", true, "text1");
	font_type = s3_get_tag_arg_int("font-type", true, S3_FONT_SELECT1);
	font_size = s3_get_tag_arg_int("font-size", true, 16);
	color = s3_get_tag_arg_string("color", true, "#000000");
	outline_width = s3_get_tag_arg_int("outline-width", true, 0);
	outline_color = s3_get_tag_arg_string("outline-color", true, "#ffffff");
	line_margin = s3_get_tag_arg_int("line-margin", true, -1);
	if (line_margin == -1)
		line_margin = font_size;
	char_margin = s3_get_tag_arg_int("char-margin", true, 0);
	x = s3_get_tag_arg_int("x", true, 0);
	y = s3_get_tag_arg_int("y", true, 0);
	width = s3_get_tag_arg_int("width", true, -1);
	height = s3_get_tag_arg_int("height", true, -1);
	text = s3_get_tag_arg_string("text", false, NULL);
	if (text == NULL)
		return false;
	
	layer_index = get_layer_index(layer);
	if (layer_index == -1) {
		s3_log_tag_error(S3_TR("Invalid layer name: %s"), layer);
		return false;
	}

	layer_image = s3_get_layer_image(layer_index);
	if (layer_image == NULL) {
		layer_image = s3_create_image(conf_game_width, conf_game_height);
		if (layer_image == NULL) {
			s3_log_error(S3_TR("Failed to create an image for layer: %s"), layer);
			return false;
		}
		s3_set_layer_image(layer_index, layer_image);
	}

	if (width == -1)
		width = layer_image->width;
	if (height == -1)
		height = layer_image->height;

	margin_right = layer_image->width - x - width;
	if (margin_right < 0)
		margin_right = 0;

	margin_bottom = layer_image->height - y - height;
	if (margin_bottom < 0)
		margin_bottom = 0;

	color_p = parse_color_code(color);
	outline_color_p = parse_color_code(outline_color);

	context = s3_create_drawmsg(
		layer_image,
		text,
		font_type,
		font_size,
		font_size,
		font_size / 4,
		outline_width,
		0,
		0,
		layer_image->width,
		layer_image->height,
		x,
		margin_right,
		y,
		margin_bottom,
		line_margin,
		char_margin,
		color_p,
		outline_color_p,
		0, /* bg_color */
		false, /* fill_bg */
		false, /* dim */
		false, /* ignore_lf */
		false, /* ignore_font */
		false, /* ignore_outline */
		false, /* ignore color */
		false, /* ignore_size */
		false, /* ignore_position */
		false, /* ignore_ruby */
		false, /* ignore_wait */
		NULL,
		false);
		
	s3_draw_message(context, -1);
	s3_destroy_drawmsg(context);

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 1);

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}

static int
get_layer_index(const char *layer)
{
	size_t i;
	size_t n = sizeof(layer_name) / sizeof(struct layer_name);
	for (i = 0; i < n; i++) {
		if (strcmp(layer, layer_name[i].name) == 0)
			return layer_name[i].index;
	}
	return -1;
}

static s3_pixel_t
parse_color_code(const char *color)
{
	unsigned int r, g, b;
	if (sscanf(color, "#%02x%02x%02x", &r, &g, &b) == 3)
		return s3_make_pixel(0xff, r, g, b);

	return 0;
}
