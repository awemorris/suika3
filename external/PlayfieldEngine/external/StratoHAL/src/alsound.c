/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for OpenAL 1.1 on Emscripten
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

#include "alsound.h"

#include <AL/al.h>
#include <AL/alc.h>

/*
 * Sampling Rate and Buffer Samples
 */
#define SAMPLING_RATE	(44100)
#define SAMPLES		(SAMPLING_RATE/4)

/*
 * Buffer Count
 */
#define BUFFER_COUNT	(4)

/*
 * Sound Device
 */
static ALCdevice *device;

/*
 * Sound Context
 */
static ALCcontext *context;

/*
 * Buffer
 */
static ALuint buffer[HAL_SOUND_TRACKS][BUFFER_COUNT];

/*
 * Source
 */
static ALuint source[HAL_SOUND_TRACKS];

/*
 * PCM Stream
 */
static struct hal_wave *stream[HAL_SOUND_TRACKS];

/*
 * Finish Flag
 */
static bool finish[HAL_SOUND_TRACKS];

/*
 * Written Area Count When Reached End-of-Stream
 */
static bool remain[HAL_SOUND_TRACKS];

/*
 * Temporary Buffer
 */
static uint32_t tmp_buf[SAMPLES];

/*
 * Initialize OpenAL
 */
bool
init_openal(void)
{
	ALenum error;
	int i;

	/* Open a device. */
	device = alcOpenDevice(NULL);

	/* Create a context. */
	context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);
	alGetError();

	/* Create buffers. */
	for (i = 0; i < HAL_SOUND_TRACKS; i++)
		alGenBuffers(BUFFER_COUNT, buffer[i]);

	/* Create sources. */
	alGenSources(HAL_SOUND_TRACKS, source);
	for (i = 0; i < HAL_SOUND_TRACKS; i++) {
		alSourcef(source[i], AL_GAIN, 1);
		alSource3f(source[i], AL_POSITION, 0, 0, 0);
	}

	return true;
}

/*
 * Cleanup OpenAL.
 */
void
cleanup_openal(void)
{
	/* TODO */
}

/*
 * Start sound playback on a sound track.
 */
bool
hal_play_sound(
	int n,
	struct hal_wave *w)
{
	ALuint buf[BUFFER_COUNT];
	int i, samples;

	/* Stop if playing. */
	if (stream[n] != NULL) {
		alSourceStop(source[n]);
		alSourceUnqueueBuffers(source[n], BUFFER_COUNT, buf);
	}

	/* Retain a stream. */
	stream[n] = w;

	/* Clear a finish flag. */
	finish[n] = false;

	/* Fill buffers. */
	for (i = 0; i < BUFFER_COUNT; i++) {
		samples = hal_get_wave_samples(stream[n], tmp_buf, SAMPLES);
		if (samples < SAMPLES) {
			/* Set a finish flag. */
			finish[n] = true;
		}
		alBufferData(buffer[n][i], AL_FORMAT_STEREO16, tmp_buf,
			     samples * sizeof(uint32_t), SAMPLING_RATE);
	}

	alSourceQueueBuffers(source[n], BUFFER_COUNT, buffer[n]);
	alSourcePlay(source[n]);

	return true;
}

/*
 * Stop a playback.
 */
bool
hal_stop_sound(
	int n)
{
	ALuint buf[BUFFER_COUNT];

	/* Stop if playing. */
	if (stream[n] != NULL) {
		alSourceStop(source[n]);
		alSourceUnqueueBuffers(source[n], BUFFER_COUNT, buf);
		stream[n] = NULL;
	}

	return true;
}

/*
 * Set a volume.
 */
bool
hal_set_sound_volume(
	int n,
	float vol)
{
	alSourcef(source[n], AL_GAIN, vol);

	return true;
}

/*
 * Check if finished.
 */
bool
hal_is_sound_finished(
	int n)
{
	if(finish[n] && remain[n] == 0)
		return true;

	return false;
}

/*
 * Fill sound buffers.
 */
void
fill_sound_buffer(void)
{
	ALuint buf;
	ALint state;
	int n, i, processed, samples;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		/* Resume if dropped due to high loads. */
		alGetSourcei(source[n], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING && !finish[n])
			alSourcePlay(source[n]);

		/* Get a count of processed buffers. */
		alGetSourcei(source[n], AL_BUFFERS_PROCESSED, &processed);
		if (finish[n])
			remain[n] = remain[n] - processed < 0 ? 0 : remain - processed;

		/* For each processed buffer: */
		for (i = 0; i < processed; i++) {
			/* Unqueue a buffer. */
			alSourceUnqueueBuffers(source[n], 1, &buf);

			if (!finish[n]) {
				/* Get PCM samples. */
				samples = hal_get_wave_samples(stream[n], tmp_buf, SAMPLES);
				if (samples < SAMPLES) {
					/* Set a finish flag. */
					remain[n] = i + 1;
					finish[n] = true;
				}
			} else {
				samples = 0;
			}

			/* Write to a buffer. */
			alBufferData(buf, AL_FORMAT_STEREO16, tmp_buf,
				     samples * sizeof(uint32_t),
				     SAMPLING_RATE);

			/* Queue a buffer. */
			alSourceQueueBuffers(source[n], 1, &buf);
		}
	}
}

/*
 * Pause sound.
 */
void
pause_sound(void)
{
	int i;
	for (i = 0; i < HAL_SOUND_TRACKS; i++)
		if (stream[i] != NULL)
			alSourceStop(source[i]);
}

/*
 * Resume sound.
 */
void
resume_sound(void)
{
	int i;
	for (i = 0; i < HAL_SOUND_TRACKS; i++)
		if (stream[i] != NULL)
			alSourcePlay(source[i]);
}
