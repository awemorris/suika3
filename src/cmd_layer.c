/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "layer" tag implementation
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2026 The Suika3 Community
 * Copyright (c) 2025-2026 The Playfield Engine Project
 * Copyright (c) 2025-2026 The NoctVM Project
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 2016-2024 The Suika2 Development Team
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is derived from the codebase of Playfield Engine, NoctLang,
 * Suika2, Suika Studio, Wind Game Lib, and 98/AT Game Lib.
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct layer_name_map {
	const char *name;
	int index;
};
static struct layer_name_map layer_name_map[] = {
	{"bg",		S3_LAYER_BG},
	{"bg2",		S3_LAYER_BG2},
	{"efb1",	S3_LAYER_EFB1},
	{"efb2",	S3_LAYER_EFB2},
	{"efb3",	S3_LAYER_EFB3},
	{"efb4",	S3_LAYER_EFB4},
	{"chb",		S3_LAYER_CHB},
	{"chb-eye",	S3_LAYER_CHB_EYE},
	{"chb-lip",	S3_LAYER_CHB_LIP},
	{"chb-fo",	S3_LAYER_CHB_FO},
	{"chl",		S3_LAYER_CHL},
	{"chl-eye",	S3_LAYER_CHL_EYE},
	{"chl-lip",	S3_LAYER_CHL_LIP},
	{"chl-fo",	S3_LAYER_CHL_FO},
	{"chlc",	S3_LAYER_CHLC},
	{"chlc-eye",	S3_LAYER_CHLC_EYE},
	{"chlc-lip",	S3_LAYER_CHLC_LIP},
	{"chlc-fo",	S3_LAYER_CHLC_FO},
	{"chr",		S3_LAYER_CHR},
	{"chr-eye",	S3_LAYER_CHR_EYE},
	{"chr-lip",	S3_LAYER_CHR_LIP},
	{"chr-fo",	S3_LAYER_CHR_FO},
	{"chrc",	S3_LAYER_CHRC},
	{"chrc-eye",	S3_LAYER_CHRC_EYE},
	{"chrc-lip",	S3_LAYER_CHRC_LIP},
	{"chrc-fo",	S3_LAYER_CHRC_FO},
	{"chc",		S3_LAYER_CHC},
	{"chc-eye",	S3_LAYER_CHC_EYE},
	{"chc-lip",	S3_LAYER_CHC_LIP},
	{"chc-fo",	S3_LAYER_CHC_FO},
	{"eff1",	S3_LAYER_EFF1},
	{"eff2",	S3_LAYER_EFF2},
	{"eff3",	S3_LAYER_EFF3},
	{"eff4",	S3_LAYER_EFF4},
	/* msgbox is not accessible. */
	/* namebox is not accessible. */
	/* choose1-idle is not accessible. */
	/* choose1-hover is not accessible. */
	/* ... */
	/* choose8-idle is not accessible. */
	/* choose8-hover is not accessible. */
	{"chf",		S3_LAYER_CHF},
	{"chf-eye",	S3_LAYER_CHF_EYE},
	{"chf-lip",	S3_LAYER_CHF_LIP},
	{"chf-fo",	S3_LAYER_CHF_FO},
	/* click is not accessible. */
	/* auto is not accessible. */
	/* skip is not accessible. */
	{"text1",	S3_LAYER_TEXT1},
	{"text2",	S3_LAYER_TEXT2},
	{"text3",	S3_LAYER_TEXT3},
	{"text4",	S3_LAYER_TEXT4},
	{"text5",	S3_LAYER_TEXT5},
	{"text6",	S3_LAYER_TEXT6},
	{"text7",	S3_LAYER_TEXT7},
	{"text8",	S3_LAYER_TEXT8},
	/* gui1 is not accessible. */
	/* ... */
	/* gui32 is not accessible. */
};

static int name_to_layer(const char *name);

/*
 * The "layer" tag implementation.
 */
bool
s3i_tag_layer(
	void *p)
{
	const char *name;
	const char *file;
	int x, y, alpha;
	float scale_x, scale_y, center_x, center_y, rotate;
	int layer;
	struct s3_image *img;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the arguments. */
	name = s3_get_tag_arg_string("name", false, NULL);
	file = s3_get_tag_arg_string("file", false, NULL);
	x = s3_get_tag_arg_int("x", true, 0);
	y = s3_get_tag_arg_int("y", true, 0);
	alpha = s3_get_tag_arg_int("alpha", true, 255);
	scale_x = s3_get_tag_arg_float("scale-x", true, 1.0f);
	scale_y = s3_get_tag_arg_float("scale-y", true, 1.0f);
	center_x = s3_get_tag_arg_int("center-x", true, 0);
	center_y = s3_get_tag_arg_int("center-y", true, 0);
	rotate = s3_get_tag_arg_float("rotate", true, 0);

	/* Get the layer index from the layer name. */
	layer = name_to_layer(name);
	if (layer == -1)
		return false;

	/* For when erase a layer. */
	if (strcmp(file, "none") == 0)
		file = NULL;

	/* If an image is specified. */
	img = NULL;
	if (file != NULL) {
		/* Load the image. */
		img = s3_create_image_from_file(file);
		if (img == NULL)
			return false;
	}

	/* Set the layer parameters. */
	if (!s3_set_layer_file_name(layer, file))
		return false;
	s3_set_layer_image(layer, img);
	s3_set_layer_position(layer, x, y);
	s3_set_layer_alpha(layer, alpha);
	s3_set_layer_scale(layer, scale_x, scale_y);
	s3_set_layer_center(layer, center_x, center_y);
	s3_set_layer_rotate(layer, rotate);

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 0);

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}

static int
name_to_layer(const char *name)
{
	int i;

	for (i = 0;
	     i < (int)(sizeof(layer_name_map) / sizeof(struct layer_name_map));
	     i++) {
		if (strcmp(layer_name_map[i].name, name) == 0)
			return layer_name_map[i].index;
	}

	s3_log_tag_error(S3_TR("Invalid layer name \"%s\"."), name);
	return -1;
}
