/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "set" tag implementation
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
#include <math.h>
#include <assert.h>

/*
 * The "set" tag implementation.
 */
bool
s3i_tag_set(
	void *p)
{
	const char *name;
	const char *value;

	UNUSED_PARAMETER(p);

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the variable name. */
	name = s3_get_tag_arg_string("name", false, NULL);

	/* Get the variable value. */
	value = s3_get_tag_arg_string("value", false, NULL);
	if (value != NULL) {
		/* Simply assign the variable. */
		if (!s3_set_variable_string(name, value))
			return false;
	} else {
		/* Calculate. */
		const char *value1, *value2, *op;

		value1 = s3_get_tag_arg_string("value1", false, NULL);
		if (value1 == NULL)
			return false;

		op = s3_get_tag_arg_string("op", false, NULL);
		if (op == NULL)
			return false;

		value2 = s3_get_tag_arg_string("value2", false, NULL);
		if (value2 == NULL)
			return false;

		if (strcmp(op, "+") == 0) {
			float v1 = (float)atof(value1);
			float v2 = (float)atof(value2);
			float v3 = v1 + v2;
			if (fabs(v3 - round(v3)) < 1e-9) {
				if (!s3_set_variable_int(name, (int)v3))
					return false;
			} else {
				if (!s3_set_variable_float(name, v3))
					return false;
			}
		} else if (strcmp(op, "-") == 0) {
			float v1 = (float)atof(value1);
			float v2 = (float)atof(value2);
			float v3 = v1 - v2;
			if (fabs(v3 - round(v3)) < 1e-9) {
				if (!s3_set_variable_int(name, (int)v3))
					return false;
			} else {
				if (!s3_set_variable_float(name, v3))
					return false;
			}
		} else if (strcmp(op, "*") == 0) {
			float v1 = (float)atof(value1);
			float v2 = (float)atof(value2);
			float v3 = v1 * v2;
			if (fabs(v3 - round(v3)) < 1e-9) {
				if (!s3_set_variable_int(name, (int)v3))
					return false;
			} else {
				if (!s3_set_variable_float(name, v3))
					return false;
			}
		} else if (strcmp(op, "/") == 0) {
			float v1 = (float)atof(value1);
			float v2 = (float)atof(value2);
			float v3 = v1 / v2;
			if (fabs(v3 - round(v3)) < 1e-9) {
				if (!s3_set_variable_int(name, (int)v3))
					return false;
			} else {
				if (!s3_set_variable_float(name, v3))
					return false;
			}
		} else if (strcmp(op, "//") == 0) {
			int v1 = atoi(value1);
			int v2 = atoi(value2);
			int v3 = v1 / v2;
			if (!s3_set_variable_int(name, v3))
				return false;
		} else if (strcmp(op, "%") == 0) {
			int v1 = atoi(value1);
			int v2 = atoi(value2);
			int v3 = v1 % v2;
			if (!s3_set_variable_int(name, v3))
				return false;
		}
	}

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 0);

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}
