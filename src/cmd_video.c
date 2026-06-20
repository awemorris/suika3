/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "video" tag implementation
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

#include <string.h>

static bool is_skippable;

static const char *prop_names[] = {
	"file",
	"skippable",
	NULL,
};

/*
 * Forward delclarations.
 */
static bool init(void);
static void update(void);
static bool cleanup(void);

/*
 * The "video" tag implementation.
 */
bool
s3i_tag_video(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_is_in_command_repetition())
		if (!init())
			return false;

	update();

	if (!s3_is_in_command_repetition())
		if (!cleanup())
			return false;

	return true;
}

/* Initialize. */
static bool
init(void)
{
	const char *file;

	/* Check properties. */
	if (!s3i_check_tag_properties(prop_names))
		return false;

	/* Get the parameters. */
	file = s3_get_tag_arg_string("file", false, NULL);
	if (file == NULL)
		return false;
	is_skippable = s3_get_tag_arg_bool("skippable", true, false);

	/* Play the video. */
	if (!s3_play_video(file, is_skippable)) {
		s3_log_tag_error(S3_TR("Cannot play video file %s."), file);
		return false;
	}

	/* Start repetition. */
	s3_start_command_repetition();

	return true;
}

/* Update. */
static void
update(void)
{
	if (!s3_is_video_playing()) {
		/* Stop the repetition. */
		s3_stop_command_repetition();
		return;
	}

	if (is_skippable &&
	    !s3_is_non_interruptible() &&
	     (s3_is_control_key_pressed() ||
	      s3_is_return_key_pressed() ||
	      s3_is_mouse_left_clicked() ||
	      s3_is_mouse_right_clicked() ||
	      s3_is_down_key_pressed())) {
		/* Stop the video. */
		s3_stop_video();

		/* Stop the repetition. */
		s3_stop_command_repetition();
	}
}

/* Cleanup. */
static bool
cleanup(void)
{
	/* Move to the next command. */
	if (!s3_move_to_next_tag())
		return false;

	return true;
}
