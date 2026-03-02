/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Audio Mixer (wrappers for Playfield Engine)
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
#include "mixer.h"
#include "conf.h"

#include <playfield/playfield.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Is playing on the track. */
static bool is_playing[S3_MIXER_TRACKS];

/* Is fading on the track. */
static bool is_fading[S3_MIXER_TRACKS];

/* Current volume in the track. */
static float vol_cur[S3_MIXER_TRACKS];

/* Start volume of the fading on the track. */
static float vol_start[S3_MIXER_TRACKS];

/* End volume of the fading on the track. */
static float vol_end[S3_MIXER_TRACKS];

/* Fading time on the track. */
static float vol_span[S3_MIXER_TRACKS];

/* Start time of the fading on the track. */
static uint64_t sw[S3_MIXER_TRACKS];

/* Volume value to be written to the local save data. */
static float vol_local[S3_MIXER_TRACKS];

/* Master volume. */
static float vol_master;

/* Global volume of the track. */
static float vol_global[S3_MIXER_TRACKS];

/* Per-character volume. */
static float vol_character[S3_CH_VOL_SLOTS];

/* Which character to apply the volume. */
static int ch_vol_index;

/* Track file name. */
static char *track_file_name[S3_MIXER_TRACKS];

/*
 * Initialize the mixer subsystem.
 */
bool
s3i_init_mixer(void)
{
	int track;

	vol_master = 1.0f;
	vol_global[S3_TRACK_BGM] = conf_sound_vol_bgm;
	vol_global[S3_TRACK_VOICE] = conf_sound_vol_voice;
	vol_global[S3_TRACK_SE] = conf_sound_vol_se;
	vol_global[S3_TRACK_SYS] = conf_sound_vol_se;

	for (track = 0; track < S3_MIXER_TRACKS; track++) {
		is_playing[track] = false;
		vol_cur[track] = 1.0f;
		vol_local[track] = 1.0f;
		is_fading[track] = false;
		if (track_file_name[track] != NULL) {
			free(track_file_name[track]);
			track_file_name[track] = NULL;
		}
		pf_stop_sound(track);
		pf_set_sound_volume(track, vol_master * vol_global[track]);
	}

	for (track = 0; track < S3_CH_VOL_SLOTS; track++)
		vol_character[track] = 1.0f;

	ch_vol_index = S3_CH_VOL_SLOT_DEFAULT;

	return true;
}

/*
 * Cleanup the mixer subsystem.
 */
void
s3i_cleanup_mixer(void)
{
	int track;

	for (track = 0; track < S3_MIXER_TRACKS; track++) {
		pf_stop_sound(track);
		if (is_playing[track])
			is_playing[track] = false;
		if (track_file_name[track] != NULL) {
			free(track_file_name[track]);
			track_file_name[track] = NULL;
		}
	}
}

/*
 * Get the SE file name. (only when loopback-playing)
 */
const char *
s3_get_track_file_name(
	int track)
{
	return track_file_name[track];
}

/*
 * Play a sound file on a mixer track.
 */
bool
s3_set_mixer_input_file(
	int track,
	const char *file,
	bool is_looped)
{
	assert(track < S3_MIXER_TRACKS);

	if (!s3_check_file_exists(file)) {
		s3_log_tag_error(S3_TR("File \"%s\" not exist."), file);
		return false;
	}

	if (is_playing[track]) {
		pf_stop_sound(track);
		is_playing[track] = false;
	}

	if (track_file_name[track] != NULL) {
		free(track_file_name[track]);
		track_file_name[track] = NULL;
	}

	if (file != NULL && strcmp(file, "") != 0) {
		if (!pf_play_sound(track, file)) {
			s3_log_tag_error(S3_TR("Cannot play sound file \"%s\"."), file);
			return false;
		}

		is_playing[track] = true;

		if (is_looped) {
			track_file_name[track] = strdup(file);
			if (track_file_name[track] == NULL) {
				s3_log_out_of_memory();
				return false;
			}
		}
	}

	return true;
}

/*
 * Set the volume for a mixer track.
 */
void
s3_set_mixer_volume(
	int track,
	float vol,
	float span)
{
	assert(track < S3_MIXER_TRACKS);
	assert(vol >= 0 && vol <= 1.0f);

	if (span > 0) {
		is_fading[track] = true;
		vol_start[track] = vol_cur[track];
		vol_end[track] = vol;
		vol_span[track] = span;
		vol_local[track] = vol;
		pf_reset_lap_timer(&sw[track]);
	} else {
		is_fading[track] = false;
		vol_cur[track] = vol;
		vol_local[track] = vol;
		pf_set_sound_volume(track, vol_global[track] * vol * vol_master);
	}

	/* TODO: completely separate SE/SYS */
	if (track == S3_TRACK_SE)
		s3_set_mixer_volume(S3_TRACK_SYS, vol, span);
}

/*
 * Get the volume for a mixer track.
 */
float
s3_get_mixer_volume(
	int track)
{
	assert(track < S3_MIXER_TRACKS);

	return vol_local[track];
}

/*
 * Set the master volume.
 */
void
s3_set_master_volume(
	float vol)
{
	int i;

	assert(vol >= 0 && vol <= 1.0f);

	vol_master = vol;

	for (i = 0; i < S3_MIXER_TRACKS; i++)
		pf_set_sound_volume(i, vol_global[i] * vol_cur[i] * vol_master);
}

/*
 * Get the master volume.
 */
float
s3_get_master_volume(void)
{
	return vol_master;
}

/*
 * Set the global volume for a track.
 */
void
s3_set_mixer_global_volume(
	int track,
	float vol)
{
	assert(track < S3_MIXER_TRACKS);
	assert(vol >= 0 && vol <= 1.0f);

	vol_global[track] = vol;

	pf_set_sound_volume(track, vol_global[track] * vol_cur[track] * vol_master);

	/* TODO: completely separate SE/SYS */
	if (track == S3_TRACK_SE)
		s3_set_mixer_global_volume(S3_TRACK_SYS, vol);
}

/*
 * Get the global volume of a track.
 */
float
s3_get_mixer_global_volume(
	int track)
{
	assert(track < S3_MIXER_TRACKS);

	return vol_global[track];
}

/*
 * Set the character volume.
 */
void
s3_set_character_volume(
	int ch_index,
	float vol)
{
	assert(ch_index >= 0 && ch_index < S3_CH_VOL_SLOTS);

	vol_character[ch_index] = vol;

	ch_vol_index = ch_index;
	vol = vol_global[S3_TRACK_VOICE] * vol_local[S3_TRACK_VOICE] *
	      vol_character[ch_vol_index] * vol_master;
	pf_set_sound_volume(S3_TRACK_VOICE, vol);
}

/*
 * Get the character volume.
 */
float
s3_get_character_volume(
	int ch_index)
{
	assert(ch_index >= 0 && ch_index < S3_CH_VOL_SLOTS);

	return vol_character[ch_index];
}

/*
 * Apply a character volume to the VOICE track.
 */
void
s3_apply_character_volume(
	int ch)
{
	float vol;

	assert(ch >= 0 && ch < S3_CH_VOL_SLOTS);

	ch_vol_index = ch;

	vol = vol_global[S3_TRACK_VOICE] * vol_local[S3_TRACK_VOICE] *
	      vol_character[ch_vol_index] * vol_master;

	pf_set_sound_volume(S3_TRACK_VOICE, vol);
}

/*
 * Check if the track playback is finished.
 */
bool
s3_is_mixer_sound_finished(
	int track)
{
	if (pf_is_sound_finished(track))
		return true;

	return false;
}

/*
 * Process sound fading.
 */
void process_sound_fading(void)
{
	int track;
	float lap, vol;

	for (track = 0; track < S3_MIXER_TRACKS; track++) {
		if (!is_fading[track])
			continue;

		/* Get the lap time. */
		lap = (float)pf_get_lap_timer_millisec(&sw[track]) / 1000.0f;
		if (lap >= vol_span[track]) {
			lap = vol_span[track];
			is_fading[track] = false;
		}

		/* Decide the volume to be set. */
		vol = vol_start[track] * (1.0f - lap / vol_span[track]) +
		      vol_end[track] * (lap / vol_span[track]);

		/* If a per-character volume is applied. */
		if (track == S3_TRACK_VOICE && ch_vol_index != -1)
			vol *= vol_character[ch_vol_index];

		/* Set the volume. */
		vol_cur[track] = vol;
		pf_set_sound_volume(track, vol_global[track] * vol_cur[track] * vol_master);
	}
}

