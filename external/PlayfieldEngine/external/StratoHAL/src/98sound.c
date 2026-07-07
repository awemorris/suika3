/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL PC98 (driver selector)
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
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* Base */
#include <strato/strato.h>

/* Standard C */
#include <math.h>
#include <string.h>
#include <assert.h>

#define SOUND_NONE	0
#define SOUND_SB16	1
#define SOUND_WSS	2

static int sound_driver;

bool sb16_init_sound(void);
void sb16_cleanup_sound(void);
void sb16_sound_poll(void);
bool sb16_play_sound(int n, struct hal_wave *w);
bool sb16_stop_sound(int n);
bool sb16_set_sound_volume(int n, float vol);
bool sb16_is_sound_finished(int n);

bool wss_init_sound(void);
void wss_cleanup_sound(void);
void wss_sound_poll(void);
bool wss_play_sound(int n, struct hal_wave *w);
bool wss_stop_sound(int n);
bool wss_set_sound_volume(int n, float vol);
bool wss_is_sound_finished(int n);

/*
 * Initialize the Sound Blaster 16/98.
 */
bool
init_sound(void)
{
	if (sb16_init_sound()) {
		sound_driver = SOUND_SB16;
		return true;
	}

	if (wss_init_sound()) {
		sound_driver = SOUND_WSS;
		return true;
	}

	return false;
}

/*
 * Cleanup the Sound Blaster 16/98.
 */
void
cleanup_sound(void)
{
	if (sound_driver == SOUND_SB16)
		sb16_cleanup_sound();
	else if (sound_driver == SOUND_SB16)
		wss_cleanup_sound();
}

void
sound_poll(void)
{
	if (sound_driver == SOUND_SB16)
		sb16_sound_poll();
	else if (sound_driver == SOUND_WSS)
		wss_sound_poll();
}

bool
hal_play_sound(
	int n,
	struct hal_wave *w)
{
	if (sound_driver == SOUND_SB16)
		return sb16_play_sound(n, w);
	else if (sound_driver == SOUND_WSS)
		return wss_play_sound(n, w);

	return true;
}

bool
hal_stop_sound(
	int n)
{
	if (sound_driver == SOUND_SB16)
		return sb16_stop_sound(n);
	else if (sound_driver == SOUND_WSS)
		return wss_stop_sound(n);

	return true;
}

bool
hal_set_sound_volume(
	int n,
	float vol)
{
	if (sound_driver == SOUND_SB16)
		return sb16_set_sound_volume(n, vol);
	else if (sound_driver == SOUND_WSS)
		return wss_set_sound_volume(n, vol);

	return true;
}

bool
hal_is_sound_finished(
	int n)
{
	if (sound_driver == SOUND_SB16)
		return sb16_is_sound_finished(n);
	else if (sound_driver == SOUND_WSS)
		return wss_is_sound_finished(n);

	return true;
}
