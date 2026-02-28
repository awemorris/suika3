/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "bg" tag implementation
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
#include "command.h"
#include "conf.h"

#include <string.h>

static uint64_t sw;
static float span;

/*
 * Forward delclarations.
 */
static bool init(void);
static void update(void);
static bool cleanup(void);

/*
 * The "bg" tag implementation.
 */
bool
s3i_tag_bg(
	void *p)
{
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
	static struct s3_image *img, *rule_img;
	const char *fname, *method;
	int fade_method, ofs_x, ofs_y;
	struct s3_fade_desc desc[S3_FADE_DESC_COUNT];

	/* Get the parameters. */
	fname = s3_get_tag_arg_string("file");
	span = s3_get_tag_arg_float("time");
	method = s3_get_tag_arg_string("method");
	ofs_x = s3_get_tag_arg_int("x");
	ofs_y = s3_get_tag_arg_int("y");

 	/* Recognize the rendering method. */
	fade_method = s3_get_fade_method(method);
	if (fade_method == S3_FADE_INVALID) {
		s3_log_error(
			S3_TR("Invalid fade method at file %s line %d."),
			s3_get_tag_file(),
			s3_get_tag_line());
		return false;
	}

	/* If we use a rule file. */
	if (fade_method == S3_FADE_RULE ||
	    fade_method == S3_FADE_MELT) {
		const char *rule;

		/* If the rule file is not specified. */
		if (!s3_check_tag_arg("rule")) {
			s3_log_error(S3_TR("Rule file is missing."));
			s3_log_script_exec_footer();
			return false;
		}

		/* Load a rule image. */
		rule = s3_get_tag_arg_string("rule");
		rule_img = s3_create_image_from_file(rule);
		if (rule_img == NULL) {
			s3_log_script_exec_footer();
			return false;
		}
	} else {
		rule_img = NULL;
	}

	/* If the file name is a color specifier. */
	if (fname[0] == '#') {
		/* Create a colored image. */
		/*
		 * img = s3_create_image_from_color_string(conf_game_width,
		 * 				     conf_game_height,
		 * 				     &fname[1]);
		 */
	} else {
		/* Load an image. */
		img = s3_create_image_from_file(fname);
	}
	if (img == NULL) {
		s3_log_script_exec_footer();
		return false;
	}

	/* Remove the speaking character. */
	s3_set_ch_talking(-1);

	/* Make a fade descriptor. */
	memset(desc, 0, sizeof(desc));
	desc[S3_FADE_DESC_CHB].stay = true;
	desc[S3_FADE_DESC_CHL].stay = true;
	desc[S3_FADE_DESC_CHLC].stay = true;
	desc[S3_FADE_DESC_CHR].stay = true;
	desc[S3_FADE_DESC_CHRC].stay = true;
	desc[S3_FADE_DESC_CHC].stay = true;
	desc[S3_FADE_DESC_CHF].stay = true;
	desc[S3_FADE_DESC_BG].stay = false;
	desc[S3_FADE_DESC_BG].fname = fname;
	desc[S3_FADE_DESC_BG].image = img;
	desc[S3_FADE_DESC_BG].x = ofs_x;
	desc[S3_FADE_DESC_BG].y = ofs_y;
	desc[S3_FADE_DESC_BG].alpha = 255;
	desc[S3_FADE_DESC_BG].scale_x = 1.0f;
	desc[S3_FADE_DESC_BG].scale_y = 1.0f;
	desc[S3_FADE_DESC_BG].center_x = 0;
	desc[S3_FADE_DESC_BG].center_y = 0;
	desc[S3_FADE_DESC_BG].rotate = 0.0f;

	/* Start a fading. */
	if (!s3_start_fade(desc, fade_method, span, rule_img)) {
		s3_log_script_exec_footer();
		return false;
	}

	/* Hide the message box on fading. */
	if (!conf_msgbox_show_on_bg) {
		s3_show_namebox(false);
		s3_show_msgbox(false);
	}

	/* Start repetition. */
	s3_start_command_repetition();

	/* Reset the lap. */
	s3_reset_lap_timer(&sw);

	return true;
}

/* Update. */
static void
update(void)
{
	float lap;

	/* Get the lap time. */
	lap = (float)s3_get_lap_timer_millisec(&sw) / 1000.0f;
	if (lap >= span)
		lap = span;

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
	} else if ((lap >= span)
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

		/* Stop the fading. */
		s3_finish_fade();

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
static bool
cleanup(void)
{
	/* Move to the next command. */
	if (!s3_move_to_next_tag())
		return false;

	return true;
}
