/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "callmacro" tag implementation
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

/*
 * The "callmacro" tag implementation.
 */
bool
s3i_tag_callmacro(
	void *p)
{
	const char *file;
	char *file_s;
	const char *name;
	char *name_s;

	UNUSED_PARAMETER(p);

	/* Get the arguments. */
	file = s3_get_tag_arg_string("file", true, NULL);
	name = s3_get_tag_arg_string("name", false, NULL);
	if (name == NULL)
		return false;

	/* Save the arguments. (will be destroyed by a tag file load.) */
	file_s = NULL;
	if (file != NULL) {
		file_s = strdup(file);
		if (file_s == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	}
	name_s = strdup(name);
	if (name_s == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	/* Push the caller point. */
	if (!s3_push_for_call(s3_get_tag_file(), s3_get_tag_index() + 1))
		return false;

	/* Load the tag file. */
	if (file_s != NULL) {
		if (!s3_move_to_tag_file(file_s)) {
			free(file_s);
			free(name_s);
			return false;
		}
	}
	if (!s3_move_to_macro_tag(name)) {
		if (file_s != NULL)
			free(file_s);
		free(name_s);
		return false;
	}
	if (file_s != NULL)
		free(file_s);
	free(name_s);

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 1);

	return true;
}
