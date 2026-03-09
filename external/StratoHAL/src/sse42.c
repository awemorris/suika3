/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * SSE4.2 version of the image processing functions.
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
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
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if defined(USE_SSE_DISPATCH)

#define DRAW_IMAGE_COPY		draw_image_copy_sse42
#define DRAW_IMAGE_ALPHA	draw_image_alpha_sse42
#define DRAW_IMAGE_GLYPH	draw_image_glyph_sse42
#define DRAW_IMAGE_ADD		draw_image_add_sse42
#define DRAW_IMAGE_DIM		draw_image_dim_sse42
#define DRAW_IMAGE_RULE		draw_image_rule_sse42
#define DRAW_IMAGE_MELT		draw_image_melt_sse42
#define DRAW_IMAGE_SCALE	draw_image_scale_sse42

#include "drawimage.h"

#endif
