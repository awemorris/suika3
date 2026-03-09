/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for UNIX and BSD variants
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

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun)

#include "stratohal/platform.h"

#if defined(__FreeBSD__)
#include <sys/soundcard.h>
#elif defined(__NetBSD__)
#include <sys/audioio.h>
#elif defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/audioio.h>
#elif defined(__sun)
#include <sys/audioio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

/*
 * Device
 */
#if defined(__FreeBSD__)
#define DEVICE		"/dev/dsp0"
#elif defined(__NetBSD__)
#define DEVICE		"/dev/audio0"
#elif defined(__OpenBSD__)
#define DEVICE		"/dev/audio0"
#elif defined(__sun)
#define DEVICE		"/dev/audio"
#endif

/*
 * Format
 */
#define SAMPLING_RATE	44100
#define CHANNELS	2
#define DEPTH		16
#define FRAME_SIZE	4

/*
 * Sound Buffer Config
 */
#define TMP_SAMPLES	512

/* File Descriptor */
static int dsp_fd;

/* Input Streams */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Sound Thread */
static pthread_t thread;

/* Mutex Object  */
static pthread_mutex_t mutex;

/* Exit Requst */
static bool exit_req;

/* Volume Values */
static float volume[HAL_SOUND_TRACKS];

/* Finish Flags */
static bool finish[HAL_SOUND_TRACKS];

/* Forward Declaration */
static void *sound_thread(void *p);
static bool fill_buffer(void);
static void mul_add_pcm(uint32_t *dst, uint32_t *src, float vol, int samples);

/*
 * Initialize sound.
 */
bool
init_sound(void)
{
	int i, ret;

	exit_req = false;

	/* Open /dev/audio0. */
	dsp_fd = open(DEVICE, 2);
	if (dsp_fd < 0) {
		hal_log_error("Failed to open " DEVICE);
		return false;
	}

#if defined(__FreeBSD__)
	unsigned int val;

	/* Set the format. */
	val = AFMT_S16_LE;
	if (ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &val)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}

	/* Set the sampling rate. */
	val = SAMPLING_RATE;
	if (ioctl(dsp_fd, SNDCTL_DSP_SPEED, &val)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}

	/* Set the channels. */
	val = CHANNELS;
	if (ioctl(dsp_fd, SNDCTL_DSP_STEREO, &val)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}

	/* Set the fragment size. */
	val = (2 << 16) | 11; 	/* blocks=2, size=2^11=2048byte */
	if (ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &val)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}
#elif defined(__NetBSD__)
	/* Set the sampling rate. */
	struct audio_info param;
	if (ioctl(dsp_fd, AUDIO_GETINFO, &param)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}
	param.play.sample_rate = SAMPLING_RATE;
	param.play.channels = CHANNELS;
	param.play.precision = DEPTH;
	param.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	param.play.buffer_size = TMP_SAMPLES * FRAME_SIZE;
	param.play.samples = TMP_SAMPLES;
	if (ioctl(dsp_fd, AUDIO_SETINFO, &param)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}
#elif defined(__OpenBSD__)
	/* Set the sampling rate. */
	struct audio_swpar param;
	param.sig = 1;
	param.le = 1;
	param.bits = DEPTH;
	param.bps = FRAME_SIZE;
	param.msb = 0;
	param.rate = SAMPLING_RATE;
	param.pchan = 2;
	param.rchan = 0;
	param.nblks = TMP_SAMPLES;
	param.round = TMP_SAMPLES * FRAME_SIZE;
	if (ioctl(dsp_fd, AUDIO_SETPAR, &param)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}
#elif defined(__sun)
	audio_info_t info;
	AUDIO_INITINFO(&info);
	info.play.sample_rate = SAMPLING_RATE;
	info.play.channels = 2;
	info.play.precision = DEPTH;
	info.play.encoding = AUDIO_ENCODING_LINEAR;
	if (ioctl(dsp_fd, AUDIO_SETINFO, &info)) {
		hal_log_error("ioctl() failed on " DEVICE);
		return false;
	}
#endif

	/* Clear volumes and flags. */
	for (i = 0; i < HAL_SOUND_TRACKS; i++) {
		volume[i] = 1.0f;
		finish[i] = false;
	}

	/* Start a sound thread. */
	ret = pthread_create(&thread, NULL, sound_thread, 0);
	if (ret != 0) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	return true;
}

/*
 * Cleanup sound.
 */
void
cleanup_sound(void)
{
	void *p;

	/* If /dev/dsp is not available, just return. */
	if (dsp_fd <= 0)
		return;

	exit_req = true;

	/* Wait for an exit of the sound thread. */
	if (thread != 0)
		pthread_join(thread, &p);

	/* Close the file handle. */
	close(dsp_fd);
	dsp_fd = 0;
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

	/* If /dev/dsp is not available, just return. */
	if (dsp_fd <= 0)
		return true;

	pthread_mutex_lock(&mutex);
	{
		/* Set a PCM stream. */
		wave[n] = w;

		/* Reset a finish flag. */
		finish[n] = false;
	}
	pthread_mutex_unlock(&mutex);

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

	/* If /dev/dsp is not available, just return. */
	if (dsp_fd <= 0)
		return true;

	pthread_mutex_lock(&mutex);
	{
		if (wave[n] != NULL) {
			/* Cancel playback status. */
			wave[n] = NULL;
		}
	}
	pthread_mutex_unlock(&mutex);

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
	/* If /dev/dsp is not available, just return. */
	if (dsp_fd <= 0)
		return true;

	/* For relaxed consistencies. Not needed for x86 and arm processors. */
	__sync_synchronize();

	if (!finish[n])
		return false;

	return true;
}

/*
 * Sound Thread
 */

/* The entrypoint of the sound thread. */
static void *
sound_thread(void *p)
{
	UNUSED_PARAMETER(p);

	while (!exit_req)
		fill_buffer();

	return (void *)0;
}

/* Write to the buffer. */
static bool
fill_buffer(void)
{
	static uint32_t period_buf[TMP_SAMPLES];
	static uint32_t channel_buf[TMP_SAMPLES];
	int stream, ret;

	memset(period_buf, 0, sizeof(uint32_t) * TMP_SAMPLES);
	memset(channel_buf, 0, sizeof(uint32_t) * TMP_SAMPLES);

	/* For each stream. */
	for (stream = 0; stream < HAL_SOUND_TRACKS; stream++) {
		pthread_mutex_lock(&mutex);
		{
			/* Don't get samples if the stream is not in playback. */
			if (wave[stream] == NULL) {
				pthread_mutex_unlock(&mutex);
				continue;
			}

			/* Get samples from an input wave stream. */
			ret = hal_get_wave_samples(wave[stream], channel_buf, TMP_SAMPLES);

			/* If we reached the end-of-stream. */
			if(ret < TMP_SAMPLES) {
				/* Zero-clear the remaining. */
				memset(channel_buf + ret, 0, (size_t)(TMP_SAMPLES - ret) * sizeof(uint32_t));

				/* Unset the input wave stream. */
				wave[stream] = NULL;

				/* Set the finish flag. */
				finish[stream] = true;
			}
		}
		pthread_mutex_unlock(&mutex);

		/* Do mixing. */
		mul_add_pcm(period_buf, channel_buf, volume[stream], TMP_SAMPLES);
	}

	write(dsp_fd, period_buf, sizeof(uint32_t) * TMP_SAMPLES);

	return true;
}

static void
mul_add_pcm(
	uint32_t *dst,
	uint32_t *src,
	float vol,
	int samples)
{
    float scale;
    int i;
    int32_t il, ir; /* intermediate L/R */
    int16_t sl, sr; /* source L/R*/
    int16_t dl, dr; /* destination L/R */

    /* Get an exponential scale factor. */
    scale = (powf(10.0f, vol) - 1.0f) / (10.0f - 1.0f);

    /* Synthesis each sample. */
    for (i = 0; i < samples; i++) {
        dl = (int16_t)(uint16_t)dst[i];
        dr = (int16_t)(uint16_t)(dst[i] >> 16);

        sl = (int16_t)(uint16_t)src[i];
        sr = (int16_t)(uint16_t)(src[i] >> 16);

        il = (int32_t)dl + (int32_t)(sl * scale);
        ir = (int32_t)dr + (int32_t)(sr * scale);

        il = il > 32767 ? 32767 : il;
        il = il < -32768 ? -32768 : il;
        ir = ir > 32767 ? 32767 : ir;
        ir = ir < -32768 ? -32768 : ir;

        dst[i] = ((uint32_t)(uint16_t)(int16_t)il) |
                 (((uint32_t)(uint16_t)(int16_t)ir) << 16);
    }
}

#endif /* defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun) */
