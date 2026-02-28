/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Tag file
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
 * Copyright (c) 1996-2024 Keiichi Tabata
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

#ifndef PLAYFIELD_TAG_H
#define PLAYFIELD_TAG_H

#include "playfield/playfield.h"

/*
 * Maximum properties in a tag.
 */
#define PROP_MAX	128

/*
 * Command struct.
 */
struct pfi_tag {
	char *tag_name;
	int prop_count;
	char *prop_name[PROP_MAX];
	char *prop_value[PROP_MAX];
	int line;
};

/* Initialize the tag system. */
void
pfi_init_tag(void);

/* Cleanup the tag system. */
void
pfi_cleanup_tag(void);

/*
 * Load a tag file.
 */
bool
pfi_load_tag_file(
	const char *file);

/*
 * Get the tag file name.
 */
const char *
pfi_get_tag_file_name(void);

/*
 * Get the tag count.
 */
int
pfi_get_tag_count(void);

/* Get the command index of the current tag. */
int
pfi_get_tag_index(void);

/* Get the tag line. */
int
pfi_get_tag_line(void);

/* Get the current tag. */
struct pfi_tag *
pfi_get_current_tag(void);

/* Move to the next tag. */
bool
pfi_move_to_next_tag(void);

/* Move to a tag by index. */
bool
pfi_move_to_tag_index(
	int index);

#endif
