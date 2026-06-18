/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "anime" tag implementation
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static bool is_async;
static bool is_showsysbtn;
static bool is_showmsgbox;
static bool is_shownamebox;
static bool is_layer_used[S3_STAGE_LAYERS];
static uint64_t sw;

static bool init(void);
static void process_frame(void);
static bool cleanup(void);

/*
 * The "anime" tag implementation.
 */
bool
s3i_tag_anime(
	void *p)
{
	UNUSED_PARAMETER(p);

	/* Is the first frame? */
	if (!s3_is_in_command_repetition()) {
		/* Initialize a multiple frame execution. */
		if (!init())
			return false;
	}

	/* Process a frame. */
	if (s3_is_in_command_repetition())
		process_frame();

	/* Is finished? */
	if (!s3_is_in_command_repetition()) {
		/* Cleanup the multiple frame execution. */
		if (!cleanup())
			return false;
	}

	return true;
}

/*
 * Initialize the command execution.
 */
static bool
init(void)
{
	const char *file;
	const char *reg_name;

	/* If stop. */
	if (s3_get_tag_arg_bool("stop", true, false)) {
		reg_name = s3_get_tag_arg_string("register", false, NULL);
		if (reg_name == NULL)
			return false;

		s3_unregister_anime(reg_name);
		return true;
	}

	/* Get the arguments. */
	file = s3_get_tag_arg_string("file", false, NULL);
	if (file == NULL)
		return false;
	is_async = s3_get_tag_arg_bool("async", true, false);
	is_showsysbtn = s3_get_tag_arg_bool("showsysbtn", true, false);
	is_showmsgbox = s3_get_tag_arg_bool("showmsgbox", true, false);
	is_shownamebox = s3_get_tag_arg_bool("shownamebox", true, false);
	reg_name = s3_get_tag_arg_string("register", true, NULL);

	/* Start the animes. */
	if (!s3_load_anime_from_file(file, reg_name, is_layer_used))
		return false;

	if (!is_async) {
		/* Start a multiple frame behavior. */
		s3_start_command_repetition();

		/* Start the time measurement. */
		s3_reset_lap_timer(&sw);
	}

	return true;
}

static void
process_frame(void)
{
	bool is_anime_running;
	int i;

	/* Check if aniems running. */
	is_anime_running = s3_is_anime_running_with_layer_mask(is_layer_used);

	/* Process inputs. */
	if (s3_is_auto_mode() &&
	    (s3_is_control_key_pressed() || s3_is_return_key_pressed() ||
	     s3_is_mouse_left_clicked() || s3_is_down_key_pressed())) {
		/* Stop the auto mode by input. */
		s3_stop_auto_mode();
		s3_show_automode_banner(false);

		/* Stop the repetition. */
		s3_stop_command_repetition();

		/* Stop the fading. */
		s3_finish_fade();
	} else if (s3_is_skip_mode() &&
		   (s3_is_control_key_pressed() ||
		    s3_is_return_key_pressed() ||
		    s3_is_mouse_left_clicked() ||
		    s3_is_down_key_pressed())) {
		/* Stop the skip mode by input.  */
		s3_stop_skip_mode();
		s3_show_skipmode_banner(false);

		/* Stop the repetition. */
		s3_stop_command_repetition();

		/* Stop the fading. */
		s3_finish_fade();
	} else if (!is_anime_running
		   ||
		   s3_is_skip_mode()
		   ||
		   (!s3_is_non_interruptible() &&
		    (s3_is_control_key_pressed() ||
		     s3_is_return_key_pressed() ||
		     s3_is_mouse_left_clicked() ||
		     s3_is_down_key_pressed()))) {
		/*
		 * In case of:
		 *  - Elapsed time exceeds a certain value.
		 *  - In skip mode.
		 *  - Omitted by input.
		 */

		/* Stop the repetition. */
		s3_stop_command_repetition();

		/* Stop the anime. */
		for (i = 0; i < S3_STAGE_LAYERS; i++) {
			if (is_layer_used[i])
				s3_clear_layer_anime_sequence(i);
		}

		/* Stop the skip mode or auto mode if there is input.*/
		if (s3_is_control_key_pressed() ||
		    s3_is_return_key_pressed() ||
		    s3_is_mouse_left_clicked() ||
		    s3_is_down_key_pressed()) {
			if (s3_is_skip_mode()) {
				s3_stop_skip_mode();
				s3_show_skipmode_banner(false);
			} else if (s3_is_auto_mode()) {
				s3_stop_auto_mode();
				s3_show_automode_banner(false);
			}
		}
	}
}

/* Cleanup. */
static bool cleanup(void)
{
	/* Move to the next tag. */
	if (!s3_move_to_next_tag())
		return false;

	return true;
}
