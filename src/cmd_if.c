/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "if" tag implementation
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

static const char *get_var_value(const char *var_name);

/*
 * The "if" tag implementation.
 */
bool
s3i_tag_if(
	void *p)
{
	const char *lhs;
	const char *op;
	const char *rhs;
	bool cond;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	/* Get the condition. */
	if (!s3_check_tag_arg("lhs")) {
		s3_log_error(S3_TR("No LHS specified."));
		return false;
	}
	if (!s3_check_tag_arg("rhs")) {
		s3_log_error(S3_TR("No RHS specified."));
		return false;
	}
	if (!s3_check_tag_arg("op")) {
		s3_log_error(S3_TR("No operator specified."));
		return false;
	}
	lhs = s3_get_tag_arg_string("lhs");
	op = s3_get_tag_arg_string("op");
	rhs = s3_get_tag_arg_string("rhs");

	/* Compare. */
	if (strcmp(op, "==") == 0) {
		cond = strcmp(lhs, rhs) == 0 ? true : false;
	} else if (strcmp(op, "!=") == 0) {
		cond = strcmp(lhs, rhs) != 0 ? true : false;
	} else if (strcmp(op, ">") == 0) {
		cond = atof(lhs) > atof(rhs) ? true : false;
	} else if (strcmp(op, ">=") == 0) {
		cond = atof(lhs) >= atof(rhs) ? true : false;
	} else if (strcmp(op, "<") == 0) {
		cond = atof(lhs) < atof(rhs) ? true : false;
	} else if (strcmp(op, "<=") == 0) {
		cond = atof(lhs) <= atof(rhs) ? true : false;
	} else {
		cond = false;
	}

	/* Set the continue flag to run also the next tag. */
	s3_set_vm_int("s3Continue", 0);

	/* If condition doesn't meet.  */
	if (!cond) {
		if (!s3_move_to_endif_tag()) {
			s3_log_script_exec_footer();
			return false;
		}
		return true;
	}

	/* Move to the next tag if condition met.. */
	return s3_move_to_next_tag();
}
