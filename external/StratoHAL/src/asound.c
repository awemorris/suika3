/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for ALSA
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

#if defined(__linux__)

/* Base */
#include "stratohal/platform.h"

/* Standard C */
#include <math.h>

/* POSIX */
#include <pthread.h>

/* ALSA */
#include <alsa/asoundlib.h>

/*
 * Format
 */
#define SAMPLING_RATE	(44100)
#define CHANNELS	(2)
#define DEPTH		(16)
#define FRAME_SIZE	(4)

/*
 * Sound Buffer Config
 */
#define BUF_FRAMES		(SAMPLING_RATE / 8)
#define BUF_SIZE		(BUF_FRAMES * FRAME_SIZE)
#define PERIODS			(4)
#define PERIOD_FRAMES		(BUF_FRAMES / PERIODS)
#define PERIOD_SIZE		(BUF_SIZE / PERIODS)
#define PERIOD_FRAMES_PAD	((PERIOD_SIZE + 63) / 64 * 64 - PERIOD_SIZE)

/*
 * Stream Data
 */

/* ALSA Devices */
static snd_pcm_t *pcm[HAL_SOUND_TRACKS];

/* Input Streams */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Sound Threads */
static pthread_t thread[HAL_SOUND_TRACKS];

/* Mutex Objects (mutually exclude between the main thread and a sound thread) */
static pthread_mutex_t mutex[HAL_SOUND_TRACKS];

/* Exit Requst */
static bool exit_req;

/* Buffers */
static uint32_t period_buf[HAL_SOUND_TRACKS][PERIOD_FRAMES + PERIOD_FRAMES_PAD];

/* Volume Values */
static float volume[HAL_SOUND_TRACKS];

/* Finish Flags */
static bool finish[HAL_SOUND_TRACKS];

/*
 * Forward Declarations
 */
static bool init_pcm(int n);
static void *sound_thread(void *p);
static bool playback_period(int n);
static void scale_samples(uint32_t *buf, int frames, float vol);

/*
 * Initialize ALSA.
 */
bool
init_sound(void)
{
	int n, ret;

	exit_req = false;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		/* Initialize per stream data. */
		pcm[n] = NULL;
		wave[n] = NULL;
		volume[n] = 1.0f;
		finish[n] = false;

		/* Initialize a device. */
		if (!init_pcm(n))
			return false;

		/* Create a mutex object. */
		pthread_mutex_init(&mutex[n], NULL);

		/* Start a sound thread. */
		ret = pthread_create(&thread[n], NULL, sound_thread, (void *)(intptr_t)n);
		if (ret != 0) {
			pthread_mutex_unlock(&mutex[n]);
			return false;
		}
	}

	return true;
}

/*
 * Cleanup ALSA.
 */
void
cleanup_sound(void)
{
	void *p1;
	int n;

	exit_req = true;

	for (n = 0; n < HAL_SOUND_TRACKS; n++) {
		if (pcm[n] == NULL)
			continue;

		hal_stop_sound(n);

		/* Wait for an exit of the sound thread. */
		pthread_join(thread[n], &p1);

		/* Close a device. */
		if (pcm[n] != NULL)
			snd_pcm_close(pcm[n]);

		/* Destroy a mutex. */
		pthread_mutex_destroy(&mutex[n]);
	}

	/* Free caches for Valgrind check. */
	snd_config_update_free_global();
}

/*
 * Start sound playback on a stream.
 */
bool
hal_play_sound(
	int n,
	struct hal_wave *w)
{
	assert(n < HAL_SOUND_TRACKS);
	assert(w != NULL);

	/* If ALSA is not available, just return. */
	if (pcm[n] == NULL)
		return true;

	pthread_mutex_lock(&mutex[n]);
	{
		/* Drop the current buffer. */
		snd_pcm_drop(pcm[n]);

		/* Set a PCM stream. */
		wave[n] = w;

		/* Reset a finish flag. */
		finish[n] = false;
	}
	pthread_mutex_unlock(&mutex[n]);

	return true;
}

/*
 * Stop sound playback on a stream.
 */
bool
hal_stop_sound(
	int n)
{
	assert(n < HAL_SOUND_TRACKS);

	/* If ALSA is not available, just return. */
	if (pcm[n] == NULL)
		return true;

	pthread_mutex_lock(&mutex[n]);
	{
		if (wave[n] != NULL) {
			/* Cancel playback status. */
			wave[n] = NULL;

			/* Stop an in-flight buffer. */
			snd_pcm_drop(pcm[n]);
		}
	}
	pthread_mutex_unlock(&mutex[n]);

	return true;
}

/*
 * Set a sound volume for a stream.
 */
bool
hal_set_sound_volume(
	int n,
	float vol)
{
	assert(n < HAL_SOUND_TRACKS);
	assert(vol >= 0 && vol <= 1.0f);

	volume[n] = vol;

	/* For relaxed consistencies. Not needed for x86 and arm processors. */
	__sync_synchronize();

	return true;
}

/*
 * Check if a sound stream is finished.
 */
bool
hal_is_sound_finished(
	int n)
{
	/* If ALSA is not available, just return. */
	if (pcm[n] == NULL)
		return true;

	/* For relaxed consistencies. Not needed for x86 and arm processors. */
	__sync_synchronize();

	if (!finish[n])
		return false;

	return true;
}

/* Initialize a device for a stream. */
static bool
init_pcm(
	int n)
{
	/* Open a device. */
	int ret;
	ret = snd_pcm_open(&pcm[n], "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		hal_log_error("snd_pcm_open() failed.");
		return false;
	}

	/* Set the format (44.1kHz, stereo, 16-bit signed little endian) */
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	snd_pcm_hw_params_alloca(&params);
	ret = snd_pcm_hw_params_any(pcm[n], params);
	if (ret < 0) {
		hal_log_error("snd_pcm_hw_params_any() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_access(pcm[n], params,
					 SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		hal_log_error("snd_pcm_hw_params_set_access() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_format(pcm[n], params, SND_PCM_FORMAT_S16_LE) < 0) {
		hal_log_error("snd_pcm_hw_params_set_format() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_rate(pcm[n], params, SAMPLING_RATE, 0) < 0) {
		hal_log_error("snd_pcm_hw_params_set_rate() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_channels(pcm[n], params, 2) < 0) {
		hal_log_error("snd_pcm_hw_params_set_channels() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_periods(pcm[n], params, PERIODS, 0) < 0) {
		hal_log_error("snd_pcm_hw_params_set_periods() failed.");
		return false;
	}
	if (snd_pcm_hw_params_set_buffer_size(pcm[n], params, BUF_FRAMES) < 0) {
		frames = BUF_FRAMES;
		if (snd_pcm_hw_params_set_buffer_size_near(pcm[n], params, &frames) < 0) {
			hal_log_error("snd_pcm_hw_params_set_buffer_size_near() failed.");
			return false;
		}
	}
	if (snd_pcm_hw_params(pcm[n], params) < 0) {
		hal_log_error("snd_pcm_hw_params() failed.");
		return false;
	}

	return true;
}

/*
 * Sound Threads
 */

/* The entrypoint of a per-device sound thread. */
static void *sound_thread(void *p)
{
	int n = (int)(intptr_t)p;

	while (!exit_req) {
		/* Run a playback loop. */
		while (playback_period(n)) {
#if defined(__linux__)
			/*
			 * [Important]
			 *  - We execute context-switch explicitly on Linux by sleep(0).
			 *  - If we do not call sleep(0), the main thread stops.
			 *  - We have confirmed this issue on Linux 4.4.0 x86_64.
			 *  - As we understand, sched_yield() does not solve this issue.
			 */
			sleep(0);
#elif defined(__NetBSD__)
			/*
			 * [Important]
			 *  - We execute context-switch explicitly on NetBSD by sched_yield().
			 *  - If we do not call sched_yield(), the main thread cannot acquire a mutex.
			 *  - We have confirmed this issue on NetBSD 9.1 amd64.
			 *  - As we understand, sleep(0) does not solve this issue.
			 */
			sched_yield();
#endif
		}
	}

	return (void *)0;
}

/* Write to a buffer. */
static bool playback_period(int n)
{
	int size;

	pthread_mutex_lock(&mutex[n]);
	{
		/* Return false if the main thread stopped a playback. */
		if (wave[n] == NULL) {
			pthread_mutex_unlock(&mutex[n]);
			return false;
		}

		/* Get PCM samples. */
		size = hal_get_wave_samples(wave[n], period_buf[n], PERIOD_FRAMES);

		/* Fill the remaining samples by zeros for a case where we have reached an end-of-stream. */
		if (size < PERIOD_FRAMES)
			memset(period_buf[n] + size, 0, (size_t)(PERIOD_FRAMES - size) * FRAME_SIZE);

		/* Scale samples by a volume value. */
		scale_samples(period_buf[n], PERIOD_FRAMES, volume[n]);

		/* Write to the device (repeat while under-running) */
		while (snd_pcm_writei(pcm[n], period_buf[n], PERIOD_FRAMES) < 0)
			snd_pcm_prepare(pcm[n]);

		/* Return false if we reached an end-of-stream. */
		if (hal_is_wave_eos(wave[n])) {
			wave[n] = NULL;
			finish[n] = true;
			pthread_mutex_unlock(&mutex[n]);
			return false;
		}
	}
	pthread_mutex_unlock(&mutex[n]);

	return true;
}

/* Apply a volume value. */
static void scale_samples(uint32_t *buf, int frames, float vol)
{
	float scale;
	uint32_t frame;
	int32_t il, ir;	/* intermediate L/R */
	int16_t sl, sr;	/* source L/R*/
	int16_t dl, dr;	/* destination L/R */
	int i;

	/* For relaxed consistencies. Not needed for x86 and arm processors. */
	__sync_synchronize();

	/* Convert a scale factor to an exponential value. */
	scale = (powf(10.0f, vol) - 1.0f) / (10.0f - 1.0f);

	/* Scale samples. */
	for (i = 0; i < frames; i++) {
		frame = buf[i];

		sl = (int16_t)(uint16_t)frame;
		sr = (int16_t)(uint16_t)(frame >> 16);

		il = (int)(sl * scale);
		ir = (int)(sr * scale);

		il = il > 32767 ? 32767 : il;
		il = il < -32768 ? -32768 : il;
		ir = ir > 32767 ? 32767 : ir;
		ir = ir < -32768 ? -32768 : ir;

		dl = (int16_t)il;
		dr = (int16_t)ir;

		buf[i] = ((uint32_t)(uint16_t)dl) | (((uint32_t)(uint16_t)dr) << 16);
	}
}

#endif /* defined(__linux__) */
