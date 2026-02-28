/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Tag Subsystem
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
#include "tag.h"

#include <playfield/playfield.h>

#include <stdlib.h>
#include <string.h>

/*
 * Initialize the tag subsystem.
 */
bool
s3i_init_tag(void)
{
	if (!pf_move_to_tag_file(S3_PATH_START_TAG))
		return false;

	return true;
}

/*
 * Cleanup the tag subsystem.
 */
void
s3i_cleanup_tag(void)
{
}

/*
 * Move to a tag file.
 */
bool
s3_move_to_tag_file(const char *file)
{
	if (pf_move_to_tag_file(S3_PATH_START_TAG))
		return false;

	return true;
}

/*
 * Move to a next tag.
 */
bool
s3_move_to_next_tag(void)
{
	if (!pf_move_to_next_tag())
		return true;

	return true;
}

/*
 * Move to a tag.
 */
bool
s3_move_to_tag_index(
	int index)
{
	if (pf_move_to_tag_index(index))
		return false;

	return true;
}

/*
 * Move to a label.
 */
bool
s3_move_to_label(
	const char *label)
{
	int i;

	for (i = 0; i <

	/* TODO */
	return false;
}

/*
 * Get the current tag file name.
 */
const char *
s3_get_tag_file(void)
{
	return pf_get_tag_file();
}

/*
 * Get the command index of the current tag.
 */
int
s3_get_tag_index(void)
{
	return pf_get_tag_index();
}

/*
 * Get the line number of the current tag.
 */
int
s3_get_tag_line(void)
{
	return pf_get_tag_line();
}

/*
 * Tag Argument Access
 */

/*
 * Check if a tag argument exists.
 */
bool
s3_check_tag_arg(
	const char *name)
{
	int i, count;
	const char *prop_name;

	count = pf_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = pf_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			return true;
	}

	return false;
}

/*
 * Get an integer tag argument.
 */
int
s3_get_tag_arg_int(
	const char *name)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;
	int val;

	count = pf_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = pf_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found, return the default value. */
		return 0;
	}

	prop_value = pf_get_tag_property_value(i);
	val = atoi(prop_value);

	return val;
}

/*
 * Get a float tag argument.
 */
float
s3_get_tag_arg_float(
	const char *name)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;
	float val;

	count = pf_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = pf_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found, return the default value. */
		return 0;
	}

	prop_value = pf_get_tag_property_value(i);
	val = (float)atof(prop_value);

	return val;
}

/*
 * Get a string tag argument.
 */
const char *
s3_get_tag_arg_string(
	const char *name)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;

	count = pf_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = pf_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found, return the default value. */
		return NULL;
	}

	prop_value = pf_get_tag_property_value(i);

	return prop_value;
}
