/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Seen Subsystem
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined(S3_TARGET_PC98) || defined(S3_TARGET_PCAT)
#define SEEN_COUNT		128
#else
#define SEEN_COUNT		S3_TAG_MAX
#endif

/*
 * Seen flag.
 *  - Each flag indicates a text or a choose.
 *  - For a text, a flag must be 0 (unseen) or 1 (seen).
 *  - For a choose, the bit N of a flag indicates whether the option N was chosen (1) or not (0).
 */
static uint8_t seen_flag[SEEN_COUNT / 8];

/* Is initialized. */
static bool is_initialized;

/* Forward declarations. */
static const char *hash(const char *file);
static char hex(int c);

/*
 * Initialize the seen subsystem.
 */
bool
s3i_init_seen(void)
{
	is_initialized = false;
	memset(seen_flag, 0, sizeof(seen_flag));

	return true;
}

/*
 * Cleanup the seen subsystem.
 */
void
s3i_cleanup_seen(void)
{
	if (is_initialized) {
		is_initialized = false;
		memset(seen_flag, 0, sizeof(seen_flag));
	}
}

/*
 * Load the seen file for the current tag file.
 */
bool
s3_load_seen(void)
{
	char key[128];
	size_t file_size;

	/* Clear all flags. */
	memset(seen_flag, 0, sizeof(seen_flag));

	/* Get the save data key. */
	snprintf(key, sizeof(key), "s-%s", hash(s3_get_tag_file()));

	/* Read the save data. */
	if (!s3_read_save_data(key, &seen_flag, sizeof(seen_flag), &file_size))
		return false;

	return true;
}

/*
 * Save the seen file for the current tag file.
 */
bool
s3_save_seen(void)
{
	char key[128];

	/* Get the save data key. */
	snprintf(key, sizeof(key), "s-%s", hash(s3_get_tag_file()));

	/* Write the save data. */
	if (!s3_write_save_data(key, &seen_flag, sizeof(seen_flag)))
		return false;

	return true;
}

/*
 * Get the seen flags for the current tag.
 */
int
s3_get_seen_flags(void)
{
	int index;

	index = s3_get_tag_index();
	if (index < SEEN_COUNT)
		return seen_flag[index];
	return false;
}

/*
 * Set the seen flags for the current tag.
 */
void
s3_set_seen_flags(int flag)
{
	int index;

	index = s3_get_tag_index();
	if (index < SEEN_COUNT)
		seen_flag[index] = (uint8_t)flag;
}

/* Get a hash string from a tag file name. */
static const char *
hash(const char *file)
{
	static char h[129];
	int len, i;

	len = (int)strlen(file);
	if (len > 128)
		len = 128;

	memset(h, 0, sizeof(h));

	for (i = 0; i < len; i++) {
		h[i * 2] = hex(file[i] >> 4);
		h[i * 2 + 1] = hex(file[i] & 0x0f);
	}

	return h;
}

/* Get a hex code. */
static char
hex(int c)
{
	assert(c >= 0 && c <= 15);

	if (c >= 0 && c <= 9)
		return (char)('0' + (char)c);
	else
		return (char)('a' + (char)(c - 10));
}
