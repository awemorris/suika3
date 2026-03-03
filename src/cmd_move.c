/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "move" tag implementation
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

/*
 * List of parameters for each layer.
 */
struct params {
	int layer;
	const char *x_arg;
	const char *y_arg;
	const char *alpha_arg;
	const char *scale_x_arg;
	const char *scale_y_arg;
	const char *center_x_arg;
	const char *center_y_arg;
	const char *rotate_arg;
	const char *dim_arg;
};
static struct params params[] = {
	{
		S3_LAYER_BG,
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
static bool is_async;
static bool has_layer_anime[sizeof(params) / sizeof(struct params)];

static bool init(void);
static void update_ch_mapping(const char *fname, int layer);
static void process_frame(void);
static bool cleanup(void);

/*
 * The "move" tag implementation.
 */
bool
s3i_tag_move(
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
	const char *s;
	int i;
	int accel;

	/* Get the animation time. */
	span = s3_get_tag_arg_float("time", true, 0);

	/* Get the accel type. */
	s = s3_get_tag_arg_string("accel", true, "uniform");
	accel = s3_get_accel_method(s);
	if (accel == S3_ANIME_ACCEL_INVALID)
		return false;

	/* Load the parameters for each layer. */
	for (i = 0; i < sizeof(params) / sizeof(struct params); i++) {
		const int LAYER_INDEX = params[i].layer;
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
		if (!s3_check_tag_arg(X_ARG) &&
		    !s3_check_tag_arg(Y_ARG) &&
		    !s3_check_tag_arg(ALPHA_ARG) &&
		    !s3_check_tag_arg(SCALE_X_ARG) &&
		    !s3_check_tag_arg(SCALE_Y_ARG) &&
		    !s3_check_tag_arg(CENTER_X_ARG) &&
		    !s3_check_tag_arg(CENTER_Y_ARG) &&
		    !s3_check_tag_arg(ROTATE_ARG) &&
		    !s3_check_tag_arg(DIM_ARG)) {
			continue;
		}

		has_layer_anime[i] = true;

		s3_new_anime_sequence(LAYER_INDEX);
		s3_add_anime_sequence_property_f("start", 0);
		s3_add_anime_sequence_property_f("end", span);
		s3_add_anime_sequence_property_i("accel", accel);

		/* Has a x position argument? */
		if (s3_check_tag_arg(X_ARG)) {
			int x;

			s = s3_get_tag_arg_string(X_ARG, false, NULL);

			/* Calc the x value. */
			if (*s == 'r') {
				/* Relative. */
				x = s3_get_layer_x(LAYER_INDEX);
				x += atoi(s + 1);
			} else if (strcmp(s, "keep") == 0) {
				/* Keep. */
				x = s3_get_layer_x(LAYER_INDEX);
			} else {
				/* number: Set the value as is. */
				x = atoi(s);
			}

			s3_add_anime_sequence_property_i("from-x", s3_get_layer_x(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-x", x);
		} else {
			int x;
			x = s3_get_layer_x(LAYER_INDEX);
			s3_add_anime_sequence_property_i("from-x", x);
			s3_add_anime_sequence_property_i("to-x", x);
		}

		/* Has a y position argument? */
		if (s3_check_tag_arg(Y_ARG)) {
			int y;

			s = s3_get_tag_arg_string(Y_ARG, false, NULL);

			/* Calc the alpha value. */
			if (*s == 'r') {
				/* Relative. */
				y = s3_get_layer_y(LAYER_INDEX);
				y += atoi(s + 1);
			} else if (strcmp(s, "keep") == 0) {
				/* Keep. */
				y = s3_get_layer_y(LAYER_INDEX);
			} else {
				/* number: Set the value as is. */
				y = atoi(s);
			}

			s3_add_anime_sequence_property_i("from-y", s3_get_layer_y(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-y", y);
		} else {
			int y;
			y = s3_get_layer_y(LAYER_INDEX);
			s3_add_anime_sequence_property_i("from-y", y);
			s3_add_anime_sequence_property_i("to-y", y);
		}

		/* Has an alpha value argument? */
		if (s3_check_tag_arg(ALPHA_ARG)) {
			int alpha;

			s = s3_get_tag_arg_string(ALPHA_ARG, false, NULL);

			/* Calc the alpha value. */
			if (*s == 'r') {
				/* Relative. */
				alpha = s3_get_layer_alpha(LAYER_INDEX);
				alpha += atoi(s + 1);
			} else {
				/* number: Set the value as is. */
				alpha = atoi(s);
			}

			/* Cut off. */
			if (alpha > 255)
				alpha = 255;
			if (alpha < 0)
				alpha = 0;

			s3_add_anime_sequence_property_i("from-a", s3_get_layer_alpha(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-a", alpha);
		} else {
			int alpha;
			alpha = s3_get_layer_alpha(LAYER_INDEX);
			s3_add_anime_sequence_property_i("from-a", alpha);
			s3_add_anime_sequence_property_i("to-a", alpha);
		}

		/* Has a scale-x value argument? */
		if (s3_check_tag_arg(SCALE_X_ARG)) {
			float scale_x;

			s = s3_get_tag_arg_string(SCALE_X_ARG, false, NULL);

			/* Calc the scale-x value. */
			if (*s == 'r') {
				/* Relative. */
				scale_x = s3_get_layer_scale_x(LAYER_INDEX);
				scale_x += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				scale_x = (float)atof(s);
			}

			s3_add_anime_sequence_property_f("from-scale-x", s3_get_layer_scale_x(LAYER_INDEX));
			s3_add_anime_sequence_property_f("to-scale-x", scale_x);
		} else {
			float scale_x;
			scale_x = s3_get_layer_scale_x(LAYER_INDEX);
			s3_add_anime_sequence_property_f("from-scale-x", scale_x);
			s3_add_anime_sequence_property_f("to-scale-x", scale_x);
		}

		/* Has a scale-y value argument? */
		if (s3_check_tag_arg(SCALE_Y_ARG)) {
			float scale_y;

			s = s3_get_tag_arg_string(SCALE_Y_ARG, false, NULL);

			/* Calc the scale-y value. */
			if (*s == 'r') {
				/* Relative. */
				scale_y = s3_get_layer_scale_y(LAYER_INDEX);
				scale_y += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				scale_y = (float)atof(s);
			}

			s3_add_anime_sequence_property_f("from-scale-y", s3_get_layer_scale_y(LAYER_INDEX));
			s3_add_anime_sequence_property_f("to-scale-y", scale_y);
		} else {
			float scale_y;
			scale_y = s3_get_layer_scale_y(LAYER_INDEX);
			s3_add_anime_sequence_property_f("from-scale-y", scale_y);
			s3_add_anime_sequence_property_f("to-scale-y", scale_y);
		}

		/* Has a center-x value argument? */
		if (s3_check_tag_arg(CENTER_X_ARG)) {
			int center_x;

			s = s3_get_tag_arg_string(CENTER_X_ARG, false, NULL);

			/* Calc the center-x value. */
			if (*s == 'r') {
				/* Relative. */
				center_x = s3_get_layer_center_x(LAYER_INDEX);
				center_x += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				center_x = (float)atof(s);
			}

			s3_add_anime_sequence_property_i("from-center-x", s3_get_layer_center_x(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-center-x", center_x);
		} else {
			int center_x;
			center_x = s3_get_layer_center_x(LAYER_INDEX);
			s3_add_anime_sequence_property_i("from-center-x", center_x);
			s3_add_anime_sequence_property_i("to-center-x", center_x);
		}

		/* Has a center-y value argument? */
		if (s3_check_tag_arg(CENTER_Y_ARG)) {
			int center_y;

			s = s3_get_tag_arg_string(CENTER_Y_ARG, false, NULL);

			/* Calc the center-y value. */
			if (*s == 'r') {
				/* Relative. */
				center_y = s3_get_layer_center_y(LAYER_INDEX);
				center_y += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				center_y = (float)atof(s);
			}

			s3_add_anime_sequence_property_i("from-center-y", s3_get_layer_center_y(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-center-y", center_y);
		} else {
			int center_y;
			center_y = s3_get_layer_center_y(LAYER_INDEX);
			s3_add_anime_sequence_property_i("from-center-y", center_y);
			s3_add_anime_sequence_property_i("to-center-y", center_y);
		}

		/* Has a rotate value argument? */
		if (s3_check_tag_arg(ROTATE_ARG)) {
			float rotate;

			s = s3_get_tag_arg_string(ROTATE_ARG, false, NULL);

			/* Calc the rotate value. */
			if (*s == 'r') {
				/* Relative. */
				rotate = s3_get_layer_rotate(LAYER_INDEX);
				rotate += (float)atof(s + 1);
			} else {
				/* number: Set the value as is. */
				rotate = (float)atof(s);
			}

			s3_add_anime_sequence_property_i("from-rotate", s3_get_layer_rotate(LAYER_INDEX));
			s3_add_anime_sequence_property_i("to-rotate", rotate);
		} else {
			float rotate;
			rotate = s3_get_layer_rotate(LAYER_INDEX);
			s3_add_anime_sequence_property_f("from-rotate", rotate);
			s3_add_anime_sequence_property_f("to-rotate", rotate);
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
				bool dim;

				dim = s3_get_tag_arg_bool(DIM_ARG, false, NULL);

				s3_set_layer_dim(LAYER_INDEX, dim);
			}
		}
	}

	/* Check if asynchronous. */
	is_async = s3_get_tag_arg_bool("async", true, false);

	/* Start the animes. */
	for (i = 0; i < sizeof(params) / sizeof(struct params); i++)
		s3_start_layer_anime(params[i].layer);

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
	float lap;
	int i;

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

		/* Stop the anime. */
		for (i = 0; i < sizeof(params) / sizeof(struct params); i++)
			s3_clear_layer_anime_sequence(params[i].layer);

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

	/* Move to the next tag. */
	if (!s3_move_to_next_tag())
		return false;

	return true;
}
