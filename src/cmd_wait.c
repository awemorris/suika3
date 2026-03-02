/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "wait" tag implementation
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

static float span;
static uint64_t sw;
static bool show_sysbtn;
static bool show_msgbox;
static bool show_namebox;

/*
 * The "click" tag implementation.
 */
bool
s3i_tag_wait(
	void *p)
{
	/* Perform initialization on the first invocation. */
	if (!s3_is_in_command_repetition()) {
		/* Get the arguments. */
		span = s3_get_tag_arg_float("time", false, 0);
		show_sysbtn = s3_get_tag_arg_bool("showsysbtn", true, false);
		show_msgbox = s3_get_tag_arg_bool("showmsgbox", true, false);
		show_namebox = s3_get_tag_arg_bool("shownamebox", true, false);

		/* Hide the message and name boxes. */
		s3_show_sysbtn(show_sysbtn);
		s3_show_msgbox(show_msgbox);
		s3_show_namebox(show_namebox);

		/* Start measuring elapsed time */
		s3_reset_lap_timer(&sw);

		/* Enter command repetition state */
		s3_start_command_repetition();
	}

	/*
	 * End the repetition when user input is detected,
	 * or when a certain amount of time has elapsed in auto mode.
	 */
	if ((float)s3_get_lap_timer_millisec(&sw) / 1000.0f >= span ||
	    (s3_is_skip_mode() && !s3_is_non_interruptible()) ||
	    (!s3_is_auto_mode() && !s3_is_non_interruptible() &&
	     (s3_is_control_key_pressed() || s3_is_return_key_pressed() ||
	      s3_is_down_key_pressed() || s3_is_mouse_left_clicked()))) {
		s3_stop_command_repetition();
		return s3_move_to_next_tag();
	}

	/* Continue executing this command. */
	return true;
}
