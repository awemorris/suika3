/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Common routines
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

#include "common.h"
#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * Load a file content.
 */
bool
pfi_load_file(
	const char *file,
	char **buf,
	size_t *size)
{
	struct hal_rfile *f;
	size_t file_size, read_size;

	assert(buf != NULL);

	if (!hal_open_rfile(file, &f)) {
		hal_log_error(PF_TR("Cannot open file \"%s\"."), file);
		return false;
	}

	if (!hal_get_rfile_size(f, &file_size)) {
		hal_log_error(PF_TR("Cannot get the size of file \"%s\"."), file);
		return false;
	}

	if (buf != NULL) {
		*buf = malloc(file_size + 1);
		if (*buf == NULL) {
			hal_log_out_of_memory();
			return false;
		}

		if (!hal_read_rfile(f, *buf, file_size, &read_size)) {
			hal_log_error(PF_TR("Cannot read file \"%s\"."), file);
			free(*buf);
			return false;
		}

		(*buf)[file_size] = '\0';
	}

	hal_close_rfile(f);

	if (size != NULL)
		*size = file_size;

	return true;
}
