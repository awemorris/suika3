/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Image Management (wrappers of Playfield Engine)
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

#ifndef SUIKA3_IMAGE_H
#define SUIKA3_IMAGE_H

#include <suika3/suika3.h>

struct s3_image {
	int tex_id;
	int width;
	int height;
	int index;
};

/*
 * Initialize the image subsystem.
 */
bool
s3i_init_image(void);

/*
 * Cleanup the image subsystem.
 */
void
s3i_cleanup_image(void);

/*
 * Get the index from a pointer to an image.
 */
int
s3i_image_to_int(
	struct s3_image *img);

/*
 * Get the pointer to image from an index.
 */
struct s3_image *
s3i_int_to_image(
	int index);

#endif
