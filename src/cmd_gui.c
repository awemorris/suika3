/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "gui" tag implementation
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

static bool init(void);
static bool cleanup(void);

/*
 * The "gui" tag implementation.
 */
bool
s3i_tag_gui(
	void *p)
{
	/* Initialize for the first frame. */
	if (!s3_is_in_command_repetition()) {
		/* Initialize. */
		if (!init())
			return false;

		/* GUI execution is handled by the main loop. */
		return true;
	}

	/*
	 * Will reach here when the GUI is finished.
	 * (But never come here when loaded or went back to title.)
	 */
	if (!cleanup())
		return false;

	return true;
}

/* Initialize. */
static bool
init(void)
{
	const char *file;

	/* Get the GUI file name. */
	file = s3_get_tag_arg_string("file");

	/* Create a thumbnail for when saved. */
	s3_draw_stage_to_thumb();

	/* Load the GUI file. */
	if (!s3_load_gui_file(file, false)) {
		s3_log_script_exec_footer();
		return false;
	}

	/* Stop the auto and skip modes. */
	if (s3_is_auto_mode()) {
		s3_stop_auto_mode();
		s3_show_automode_banner(false);
	}
	if (s3_is_skip_mode()) {
		s3_stop_skip_mode();
		s3_show_skipmode_banner(false);
	}

	/* Start the multiple frame execution. */
	s3_start_command_repetition();

	/* Start the GUI. */
	s3_start_gui();

	return true;
}

/* Cleanup. */
static bool
cleanup(void)
{
	const char *label;
	bool ret;

	ret = true;

	/* Stop the multiple frame execution. */
	s3_stop_command_repetition();

	/* If a button with a label was selected. */
	label = s3_get_gui_result_label();
	if (label != NULL) {
		ret = s3_move_to_label_tag(label);
		s3_stop_gui();
		return ret;
	}

	/* If canceled.	 */
	ret = s3_move_to_next_tag();
	s3_stop_gui();

	return ret;
}
