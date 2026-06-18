/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "config" tag implementation
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
 * The "config" tag implementation.
 */
bool
s3i_tag_config(
	void *p)
{
	const char *key;
	const char *val;

	UNUSED_PARAMETER(p);

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the arguments. */
	key = s3_get_tag_arg_string("name", false, NULL);
	if (key == NULL)
		return false;
	val = s3_get_tag_arg_string("value", false, NULL);
	if (val == NULL)
		return false;

	/* Update the config. */
	if (!s3_set_config(key, val)) {
		s3_log_tag_error(S3_TR("Conifg \"%s\" not found."), key);
		return false;
	}

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 1);

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}
