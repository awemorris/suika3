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
	{"bg-fo",	S3_LAYER_BG_FO},
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

	{"chl", LAYER_CHL},
	{"chlc", LAYER_CHL},
	{"chr", LAYER_CHR},
	{"chrc", LAYER_CHRC},
	{"chc", LAYER_CHC},
	{"effect1", LAYER_EFFECT1},
	{"effect2", LAYER_EFFECT2},
	{"effect3", LAYER_EFFECT3},
	{"effect4", LAYER_EFFECT4},
	{"text1", LAYER_TEXT1},
	{"text2", LAYER_TEXT2},
	{"text3", LAYER_TEXT3},
	{"text4", LAYER_TEXT4},
	{"text5", LAYER_TEXT5},
	{"text6", LAYER_TEXT6},
	{"text7", LAYER_TEXT7},
	{"text8", LAYER_TEXT8},
};

/*
 * The "layer" tag implementation.
 */
bool
s3i_tag_bgm(
	void *p)
{
	const char *file;
	bool once;
	bool loop, stop;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the arguments. */
	file = s3_get_tag_arg_string("file", false, NULL);
	once = s3_get_tag_arg_bool("once", true, false);

	/* Check if stop. */
	if (strcmp(file, "stop") == 0 ||
	    strcmp(file, "none") == 0)
		stop = true;
	else
		stop = false;

	if (!stop) {
		/* Play a sound file. */
		if (!s3_set_mixer_input_file(S3_TRACK_BGM, file, once ? false : true))
			return false;
	} else {
		/* Stop the sound. */
		if (!s3_set_mixer_input_file(S3_TRACK_BGM, NULL, false))
			return false;
	}

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 0);

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}
