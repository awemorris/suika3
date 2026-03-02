/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "load" tag implementation
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
 * The "load" tag implementation.
 */
bool
s3i_tag_load(
	void *p)
{
	const char *file;
	const char *label;
	char *file_save;
	char *label_save;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the file and label. */
	file = s3_get_tag_arg_string("file", false, NULL);
	label = s3_get_tag_arg_string("label", true, NULL);

	/* Save the strings. (due to destruction) */
	file_save = strdup(file);
	if (file_save == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	if (label != NULL) {
		label_save = strdup(label);
		if (label_save == NULL) {
			free(file_save);
			s3_log_out_of_memory();
			return false;
		}
	} else {
		label_save = NULL;
	}

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 0);

	/* Jump to the file. */
	if (!s3_move_to_tag_file(file_save)) {
		free(file_save);
		if (label_save != NULL)
			free(label_save);
		return false;
	}

	/* Jump to the label. */
	if (label_save != NULL) {
		if (!s3_move_to_label_tag(label_save)) {
			free(file_save);
			free(label_save);
			return false;
		}
	}

	free(file_save);
	free(label_save);

	return true;
}
