/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "volume" tag implementation
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
#include "tag.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const char *prop_names[] = {
	"track",
	"vol",
	"span",
	NULL,
};

/*
 * The "volume" tag implementation.
 */
bool
s3i_tag_volume(
	void *p)
{
	const char *track;
	float vol;
	float span;

	UNUSED_PARAMETER(p);

	/* Check properties. */
	if (!s3i_check_tag_properties(prop_names))
		return false;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	track = s3_get_tag_arg_string("track", false, NULL);
	if (track == NULL)
		return false;
	vol = s3_get_tag_arg_float("vol", false, -1.0f);
	if (vol == -1.0f)
		return false;
	span = s3_get_tag_arg_float("span", true, 0.0f);

	if (vol < 0)
		vol = 0;
	if (vol > 1.0f)
		vol = 1.0f;

	if (strcmp(track, "bgm") == 0) {
		s3_set_mixer_volume(S3_TRACK_BGM, vol, span);
	} else if (strcmp(track, "voice") == 0) {
		s3_set_mixer_volume(S3_TRACK_VOICE, vol, span);
	} else if (strcmp(track, "se") == 0) {
		s3_set_mixer_volume(S3_TRACK_SE, vol, span);
	} else {
		/* Incorrect stream name */
		s3_log_tag_error(S3_TR("Invalid track \"%s\" specified."), track);
		return false;
	}

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}
