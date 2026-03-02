/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "volume" tag implementation
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

/*
 * The "volume" tag implementation.
 */
bool
s3i_tag_volume(
	void *p)
{
	const char *track;
	float vol;
	float span;

	/* Update the tag values by variable values. */
	s3_evaluate_tag();

	if (vol < 0)
		vol = 0;
	if (vol > 1.0f)
		vol = 1.0f;

	if (strcmp(track, "bgm") == 0) {
		s3_set_mixer_volume(S3_TRACK_BGM, vol, span);
	} else if (strcmp(track, "voice") == 0) {
		s3_set_mixer_volume(S3_TRACK_VOICE, vol, span);
	} else if (strcmp(track, "se") == 0) {
		s3_set_mixer_volume(S3_TRACK_SE, vol, span);
	} else {
		/* Incorrect stream name */
		s3_log_tag_error(S3_TR("Invalid track \"%s\" specified."), track);
		return false;
	}

	/* Move to the next tag. */
	return s3_move_to_next_tag();
}
