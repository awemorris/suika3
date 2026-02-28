/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "ch" tag implementation
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * List of parameters for each layer.
 */
struct params {
	int layer;
	const char *file_arg;
	const char *x_arg;
	const char *y_arg;
	const char *alpha_arg;
	const char *scale_x_arg;
	const char *scale_y_arg;
	const char *center_x_arg;
	const char *center_y_arg;
	const char *rotate_arg;
	const char *dim_arg;
} params[] = {
	{
		S3_LAYER_BG,
		"bg",
		"bg-x",
		"bg-y",
		"bg-a",
		"bg-scale-x",
		"bg-scale-y",
		"bg-bg-x",
		"bg-bg-y",
		"bg-rotate",
		"",		/* No "bg-dim" */
	},
	{
		S3_LAYER_CHB,
		"back",
		"back-x",
		"back-y",
		"back-a",
		"back-scale-x",
		"back-scale-y",
		"back-back-x",
		"back-back-y",
		"back-rotate",
		"back-dim",
	},
	{
		S3_LAYER_CHL,
		"left",
		"left-x",
		"left-y",
		"left-a",
		"left-scale-x",
		"left-scale-y",
		"left-left-x",
		"left-left-y",
		"left-rotate",
		"left-dim",
	},
	{
		S3_LAYER_CHLC,
		"left-center",
		"left-center-x",
		"left-center-y",
		"left-center-a",
		"left-center-scale-x",
		"left-center-scale-y",
		"left-center-left-center-x",
		"left-center-left-center-y",
		"left-center-rotate",
		"left-center-dim",
	},
	{
		S3_LAYER_CHR,
		"right",
		"right-x",
		"right-y",
		"right-a",
		"right-scale-x",
		"right-scale-y",
		"right-right-x",
		"right-right-y",
		"right-rotate",
		"right-dim",
	},
	{
		S3_LAYER_CHRC,
		"right-center",
		"right-center-x",
		"right-center-y",
		"right-center-a",
		"right-center-scale-x",
		"right-center-scale-y",
		"right-center-right-center-x",
		"right-center-right-center-y",
		"right-center-rotate",
		"right-center-dim",
	},
	{
		S3_LAYER_CHC,
		"center",
		"center-x",
		"center-y",
		"center-a",
		"center-scale-x",
		"center-scale-y",
		"center-center-x",
		"center-center-y",
		"center-rotate",
		"center-dim",
	},
	{
		S3_LAYER_CHF,
		"face",
		"face-x",
		"face-y",
		"face-a",
		"face-scale-x",
		"face-scale-y",
		"face-face-x",
		"face-face-y",
		"face-rotate",
		"face-dim",
	},
};


static uint64_t sw;
static float span;
static int fade_method;

static bool init(void);
static void update_ch_mapping(const char *fname, int layer);
static void process_frame(void);
static bool cleanup(void);

/*
 * ch command
 */
bool
s3i_command_ch(
	void *p)
{
	/* Is the first frame? */
	if (!s3_is_in_command_repetition()) {
		/* Initialize a multiple frame execution. */
		if (!init())
			return false;
	}

	/* Process a frame. */
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
	struct s3_fade_desc desc[S3_FADE_DESC_COUNT];
	struct s3_image *rule_img;
	const char *s;
	const char *fade;
	int fade_method;
	int i;

	memset(desc, 0, sizeof(desc));

	/* Load the parameters for each layer. */
	for (i = 0; i < S3_FADE_DESC_COUNT; i++) {
		const int LAYER_INDEX = params[i].layer;
		const char *FILE_ARG = params[i].file_arg;
		const char *X_ARG = params[i].x_arg;
		const char *Y_ARG = params[i].y_arg;
		const char *ALPHA_ARG = params[i].alpha_arg;
		const char *SCALE_X_ARG = params[i].scale_x_arg;
		const char *SCALE_Y_ARG = params[i].scale_y_arg;
		const char *CENTER_X_ARG = params[i].center_x_arg;
		const char *CENTER_Y_ARG = params[i].center_y_arg;
		const char *ROTATE_ARG = params[i].rotate_arg;
		const char *DIM_ARG = params[i].dim_arg;
		bool is_file_specified;

		/* If no arguments specified for this layer. */
		if (!s3_check_tag_arg(FILE_ARG) &&
		    !s3_check_tag_arg(X_ARG) &&
		    !s3_check_tag_arg(Y_ARG) &&
		    !s3_check_tag_arg(ALPHA_ARG) &&
		    !s3_check_tag_arg(SCALE_X_ARG) &&
		    !s3_check_tag_arg(SCALE_Y_ARG) &&
		    !s3_check_tag_arg(CENTER_X_ARG) &&
		    !s3_check_tag_arg(CENTER_Y_ARG) &&
		    !s3_check_tag_arg(ROTATE_ARG) &&
		    !s3_check_tag_arg(DIM_ARG)) {
			desc[i].stay = true;
			continue;
		}

		/* There is a change for this layer. */
		desc[i].stay = false;

		/* Has a file argument? */
		is_file_specified = false;
		if (s3_check_tag_arg(FILE_ARG)) {
			/* There is a file name. */
			s = s3_get_tag_arg_string(FILE_ARG);

			/* Is "none" specified? */
			if (strcmp(s, "none") == 0) {
				/* Erase the existing image. */
				desc[i].fname = NULL;
				desc[i].image = NULL;
			} else {
				/* Load the layer image. */
				desc[i].fname = s;
				desc[i].image = s3_create_image_from_file(s);
				if (desc[i].image == NULL)
					return false;

				/* If a character layer. */
				if (LAYER_INDEX == S3_LAYER_CHB ||
				    LAYER_INDEX == S3_LAYER_CHL ||
				    LAYER_INDEX == S3_LAYER_CHLC ||
				    LAYER_INDEX == S3_LAYER_CHC ||
				    LAYER_INDEX == S3_LAYER_CHRC ||
				    LAYER_INDEX == S3_LAYER_CHR ||
				    LAYER_INDEX == S3_LAYER_CHF) {
					int chpos = s3_layer_to_chpos(LAYER_INDEX);
					/* Load the eye-blink and lip-sync images. */
					if (!s3_load_eye_image_if_exists(chpos, s))
						return false;
					if (!s3_load_lip_image_if_exists(chpos, s))
						return false;

					/* Update the character mapping. */
					update_ch_mapping(s, chpos);
				}

				is_file_specified = true;
			}
		} else {
			/* Not specified, leave the image as is. */
			desc[i].fname = s3_get_layer_file_name(LAYER_INDEX);
			desc[i].image = s3_get_layer_image(LAYER_INDEX);
		}

		/* Has a x position argument? */
		if (s3_check_tag_arg(X_ARG)) {
			s = s3_get_tag_arg_string(X_ARG);

			/* Calc the x value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].x = s3_get_layer_x(LAYER_INDEX);
				desc[i].x += atoi(s + 1);
			} else if (strcmp(s, "keep") == 0) {
				/* Keep. */
				desc[i].x = s3_get_layer_x(LAYER_INDEX);
			} else {
				/* number: Set the value as is. */
				desc[i].x = atoi(s);
			}

			/* Cut off. */
			if (desc[i].x > 255)
				desc[i].x = 255;
			if (desc[i].x < 0)
				desc[i].x = 0;
		} else if (is_file_specified) {
			/* If a file is specified but x is ommited. */
			assert(desc[i].image != NULL);
			switch (LAYER_INDEX) {
			case S3_LAYER_CHB:
			case S3_LAYER_CHC:
				desc[i].x = (conf_game_width - s3_get_image_width(desc[i].image)) / 2;
				break;
			case S3_LAYER_CHL:
				desc[i].x = 0;
				break;
			case S3_LAYER_CHLC:
				desc[i].x = (conf_game_width - s3_get_image_width(desc[i].image)) / 4;
				break;
			case S3_LAYER_CHR:
				desc[i].x = conf_game_width - s3_get_image_width(desc[i].image);
				break;
			case S3_LAYER_CHRC:
			{
				int center = (conf_game_width - s3_get_image_width(desc[i].image)) / 2;
				int right = conf_game_width - s3_get_image_width(desc[i].image) - conf_stage_ch_margin_right;
				desc[i].x = (center + right) / 2;
				break;
			}
			default:
				desc[i].x = 0;
				break;
			}
		} else {
			/* If file and x are both ommited. */
			desc[i].x = s3_get_layer_x(LAYER_INDEX);
		}

		/* Has a y position argument? */
		if (s3_check_tag_arg(Y_ARG)) {
			s = s3_get_tag_arg_string(Y_ARG);

			/* Calc the alpha value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].y = s3_get_layer_y(LAYER_INDEX);
				desc[i].y += atoi(s + 1);
			} else if (strcmp(s, "keep") == 0) {
				/* Keep. */
				desc[i].y = s3_get_layer_y(LAYER_INDEX);
			} else {
				/* number: Set the value as is. */
				desc[i].y = atoi(s);
			}

			/* Cut off. */
			if (desc[i].y > 255)
				desc[i].y = 255;
			if (desc[i].y < 0)
				desc[i].y = 0;
		} else if (is_file_specified) {
			/* If a file is specified and y is ommited. */
			assert(desc[i].image != NULL);
			switch (LAYER_INDEX) {
			case S3_LAYER_CHB:
			case S3_LAYER_CHC:
			case S3_LAYER_CHL:
			case S3_LAYER_CHLC:
			case S3_LAYER_CHR:
			case S3_LAYER_CHRC:
				desc[i].y = conf_game_height - s3_get_image_height(desc[i].image);
				break;
			default:
				desc[i].y = 0;
				break;
			}
		} else {
			/* If file and y are both ommited. */
			desc[i].y = s3_get_layer_y(LAYER_INDEX);
		}

		/* Has an alpha value argument? */
		if (s3_check_tag_arg(ALPHA_ARG)) {
			s = s3_get_tag_arg_string(ALPHA_ARG);

			/* Calc the alpha value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].alpha = s3_get_layer_alpha(LAYER_INDEX);
				desc[i].alpha += atoi(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].alpha = atoi(s);
			}

			/* Cut off. */
			if (desc[i].alpha > 255)
				desc[i].alpha = 255;
			if (desc[i].alpha < 0)
				desc[i].alpha = 0;
		} else if (is_file_specified) {
			/* If ommited. */
			desc[i].alpha = 255;
		} else {
			/* If file and alpha are both ommited. */
			desc[i].alpha = s3_get_layer_alpha(LAYER_INDEX);
		}

		/* Has a scale-x value argument? */
		if (s3_check_tag_arg(SCALE_X_ARG)) {
			s = s3_get_tag_arg_string(SCALE_X_ARG);

			/* Calc the scale-x value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].scale_x = s3_get_layer_scale_x(LAYER_INDEX);
				desc[i].scale_x += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].scale_x = (float)atof(s);
			}
		} else if (is_file_specified) {
			/* If a file is specified and scale-x is ommited. */
			desc[i].scale_x = 1.0f;
		} else {
			/* If file and scale-x are both ommited. */
			desc[i].scale_x = s3_get_layer_scale_x(LAYER_INDEX);
		}

		/* Has a scale-y value argument? */
		if (s3_check_tag_arg(SCALE_Y_ARG)) {
			s = s3_get_tag_arg_string(SCALE_Y_ARG);

			/* Calc the scale-y value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].scale_y = s3_get_layer_scale_y(LAYER_INDEX);
				desc[i].scale_y += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].scale_y = (float)atof(s);
			}
		} else if (is_file_specified) {
			/* If a file is specified and scale-y is ommited. */
			desc[i].scale_y = 1.0f;
		} else {
			/* If file and scale-y are both ommited. */
			desc[i].scale_y = s3_get_layer_scale_y(LAYER_INDEX);
		}

		/* Has a center-x value argument? */
		if (s3_check_tag_arg(CENTER_X_ARG)) {
			s = s3_get_tag_arg_string(CENTER_X_ARG);

			/* Calc the center-x value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].center_x = s3_get_layer_center_x(LAYER_INDEX);
				desc[i].center_x += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].center_x = (float)atof(s);
			}
		} else if (is_file_specified) {
			/* If a file is specified and center-x is ommited. */
			desc[i].center_x = 0;
		} else {
			/* If file and center-x are both ommited. */
			desc[i].center_x = s3_get_layer_center_x(LAYER_INDEX);
		}

		/* Has a center-y value argument? */
		if (s3_check_tag_arg(CENTER_Y_ARG)) {
			s = s3_get_tag_arg_string(CENTER_Y_ARG);

			/* Calc the center-y value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].center_y = s3_get_layer_center_y(LAYER_INDEX);
				desc[i].center_y += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].center_y = (float)atof(s);
			}
		} else if (is_file_specified) {
			/* If a file is specified and center-y is ommited. */
			desc[i].center_y = 0;
		} else {
			/* If file and center-y are both ommited. */
			desc[i].center_y = s3_get_layer_center_y(LAYER_INDEX);
		}

		/* Has a rotate value argument? */
		if (s3_check_tag_arg(ROTATE_ARG)) {
			s = s3_get_tag_arg_string(ROTATE_ARG);

			/* Calc the rotate value. */
			if (*s == 'r') {
				/* Relative. */
				desc[i].rotate = s3_get_layer_rotate(LAYER_INDEX);
				desc[i].rotate += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				desc[i].rotate = (float)atof(s);
			}
		} else if (is_file_specified) {
			/* If a file is specified and rotate is ommited. */
			desc[i].rotate = 0;
		} else {
			/* If file and rotate are both ommited. */
			desc[i].rotate = s3_get_layer_rotate(LAYER_INDEX);
		}

		/* Has a dim value argument? */
		if (LAYER_INDEX == S3_LAYER_CHB ||
		    LAYER_INDEX == S3_LAYER_CHL ||
		    LAYER_INDEX == S3_LAYER_CHLC ||
		    LAYER_INDEX == S3_LAYER_CHC ||
		    LAYER_INDEX == S3_LAYER_CHRC ||
		    LAYER_INDEX == S3_LAYER_CHR ||
		    LAYER_INDEX == S3_LAYER_CHF) {
			if (s3_check_tag_arg(DIM_ARG)) {
				s = s3_get_tag_arg_string(DIM_ARG);

				/* Calc the rotate value. */
				if (strcmp(s, "true") == 0) {
					/* Dim. */
					desc[i].dim = true;
				} else {
					/* No dim. */
					desc[i].dim = false;
				}
			} else if (is_file_specified) {
				/* If a file is specified and rotate is ommited. */
				desc[i].dim = s3_get_ch_dim(s3_layer_to_chpos(LAYER_INDEX));
			} else {
				/* If file and dim are both ommited. */
				desc[i].dim = s3_get_layer_dim(LAYER_INDEX);
			}
		}
	}

	/* Get the fade method. */
	fade = s3_get_tag_arg_string("fade");
	fade_method = s3_get_fade_method(fade);
	if (fade_method == S3_FADE_INVALID) {
		s3_log_error(S3_TR("Invalid fade method \"%s\""), fade_method);
		s3_log_script_exec_footer();
	}

	/* If the fade method is "rule" or "melt". */
	rule_img = NULL;
	if (fade_method == S3_FADE_RULE ||
	    fade_method == S3_FADE_MELT) {
		/* Check for the rule argument. */
		if (!s3_check_tag_arg("rule")) {
			s3_log_error(S3_TR("Rule file is not specified."));
			s3_log_script_exec_footer();
			return false;
		}

		/* Load the rule image. */
		rule_img = s3_create_image_from_file(s);
		if (rule_img == NULL) {
			s3_log_script_exec_footer();
			return false;
		}
	}

	/* Get the transition time. */
	span = s3_get_tag_arg_float("time");

	/* Start a multiple frame behavior. */
	s3_start_command_repetition();

	/* Start the fading. */
	if (!s3_start_fade(desc, fade_method, span, rule_img)) {
		s3_log_script_exec_footer();
		return false;
	}

	/* Start the time measurement. */
	s3_reset_lap_timer(&sw);

	/* Hide the message box. */
	if (!conf_msgbox_show_on_ch) {
		s3_show_namebox(false);
		s3_show_msgbox(false);
	}
	s3_show_click(false);

	return true;
}

/* Update the character and name mapping. */
static void
update_ch_mapping(
	const char *fname,
	int chpos)
{
	int i;

	/* Check if the name is registered. */
	for (i = 0; i < S3_CHARACTER_MAP_COUNT; i++) {
		if (conf_character_name[i] == NULL)
			continue;
		if (conf_character_folder[i] == NULL)
			continue;
		if (fname == NULL)
			continue;
		if (strncmp(conf_character_folder[i],
			    fname,
			    strlen(conf_character_folder[i])) == 0)
			break;
	}
	if (i == S3_CHARACTER_MAP_COUNT)
		i = -1;

	/* Specify a character index for a character position. */
	s3_set_ch_name_mapping(chpos, i);
}

static void
process_frame(void)
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
static bool cleanup(void)
{
	int i;

	/* Restart the eye blink. */
	for (i = 0; i < S3_CH_BASIC_LAYERS; i++)
		s3_reload_eye_anime(i);

	/* Move to the next tag. */
	if (!s3_move_to_next_tag())
		return false;

	return true;
}
