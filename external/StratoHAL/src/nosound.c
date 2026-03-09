/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Dummy sound HAL
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

#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__sun)

#include "stratohal/platform.h"

/*
 * Initialize sound.
 */
bool init_sound(void)
{
	return true;
}

/*
 * Cleanup sound.
 */
void cleanup_sound(void)
{
}

/*
 * Start sound playback on a stream.
 */
bool play_sound(int n, struct wave *w)
{
	UNUSED_PARAMETER(n);
	UNUSED_PARAMETER(w);
	return true;
}

/*
 * Stop sound playback on a stream.
 */
bool stop_sound(int n)
{
	UNUSED_PARAMETER(n);
	return true;
}

/*
 * Set a sound volume for a stream.
 */
bool set_sound_volume(int n, float vol)
{
	UNUSED_PARAMETER(n);
	UNUSED_PARAMETER(vol);
	return true;
}

/*
 * Check if a sound stream is finished.
 */
bool is_sound_finished(int n)
{
	UNUSED_PARAMETER(n);
	return true;
}

#endif
