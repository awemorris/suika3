/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Sound HAL for OpenSL ES (Android and OpenHarmony)
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

/* Base */
#include "stratohal/platform.h"

/* OpenSLES */
#include <SLES/OpenSLES.h>
#if defined(HAL_TARGET_ANDROID)
#include <SLES/OpenSLES_Android.h>
#endif
#if defined(HAL_TARGET_OPENHARMONY)
#include <SLES/OpenSLES_OpenHarmony.h>
#endif

/* POSIX */
#include <pthread.h>
#include <sys/types.h>

/* C */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(HAL_TARGET_ANDROID)
#define BUFFERQUEUEITF SLAndroidSimpleBufferQueueItf
#endif
#if defined(HAL_TARGET_OPENHARMONY)
#define BUFFERQUEUEITF SLOHBufferQueueItf
#endif

/*
 * sound Buffer Parameters
 */
#define SAMPLING_RATE		(44100)
#define CHANNELS		(2)
#define FRAME_SIZE		(4)
#define BUF_FRAMES		(SAMPLING_RATE / 4)

/*
 * OpenSL ES Objects
 */
static SLObjectItf engine_object;
static SLEngineItf engine_engine;
static SLObjectItf output_mix_object;
static SLObjectItf bq_player_object[HAL_SOUND_TRACKS];
static SLPlayItf bq_player_play[HAL_SOUND_TRACKS];
static BUFFERQUEUEITF bq_player_buffer_queue[HAL_SOUND_TRACKS];

/* Buffers */
static uint32_t sample_buf[HAL_SOUND_TRACKS][BUF_FRAMES];

/* Mutex objects to synchronize accesses to sample_buf */
static pthread_mutex_t sound_mutex[HAL_SOUND_TRACKS];

/* Input Streams */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Volume Values */
static float volume[HAL_SOUND_TRACKS];

/* Finish Flags */
static bool pre_finish[HAL_SOUND_TRACKS];	 /* Shows if we reached an end-of-stream. */
static bool post_finish[HAL_SOUND_TRACKS];	 /* Shows if we finished a playback of a final buffer. */

/*
 * Forward Declarations
 */
static void play_callback(const BUFFERQUEUEITF bq, void *context);
static void enqueue(int stream);
static void scale_samples(uint32_t *buf, int frames, float vol);

/*
 * Initialize OpenSL ES.
 */
void
init_opensl_es(void)
{
	for (int i = 0; i < HAL_SOUND_TRACKS; i++)
		pthread_mutex_init(&sound_mutex[i], NULL);

	slCreateEngine(&engine_object, 0, NULL, 0, NULL, NULL);
	(*engine_object)->Realize(engine_object, SL_BOOLEAN_FALSE);
	(*engine_object)->GetInterface(engine_object, SL_IID_ENGINE, &engine_engine);

	const SLInterfaceID outids[1] = {SL_IID_PLAYBACKRATE};
	const SLboolean outreq[1] = {SL_BOOLEAN_FALSE};
	(*engine_engine)->CreateOutputMix(engine_engine, &output_mix_object, 1, outids, outreq);
	(*output_mix_object)->Realize(output_mix_object, SL_BOOLEAN_FALSE);

#if defined(HAL_TARGET_ANDROID)
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
#else
	SLDataLocator_BufferQueue loc_bufq = {SL_DATALOCATOR_BUFFERQUEUE, 2};
#endif
	SLDataFormat_PCM format_pcm = {
		SL_DATAFORMAT_PCM,
		2,
		SL_SAMPLINGRATE_44_1,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audio_src = {&loc_bufq, &format_pcm};
	SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, output_mix_object};
	SLDataSink audioSnk = {&loc_outmix, NULL};
    const SLInterfaceID bqids[1] = {SL_IID_BUFFERQUEUE};
	const SLboolean bqreq[1] = {SL_BOOLEAN_TRUE};
	for (int i = 0; i < HAL_SOUND_TRACKS; i++) {
		(*engine_engine)->CreateAudioPlayer(engine_engine, &bq_player_object[i], &audio_src, &audioSnk, 1, bqids, bqreq);
		(*bq_player_object[i])->Realize(bq_player_object[i], SL_BOOLEAN_FALSE);
		(*bq_player_object[i])->GetInterface(bq_player_object[i], SL_IID_PLAY, &bq_player_play[i]);
#if defined(HAL_TARGET_OPENHARMONY)
		(*bq_player_object[i])->GetInterface(bq_player_object[i], SL_IID_OH_BUFFERQUEUE, &bq_player_buffer_queue[i]);
#else
		(*bq_player_object[i])->GetInterface(bq_player_object[i], SL_IID_BUFFERQUEUE, &bq_player_buffer_queue[i]);
#endif
		(*bq_player_buffer_queue[i])->RegisterCallback(bq_player_buffer_queue[i], (void *)play_callback, (void *)(intptr_t)i);
		(*bq_player_play[i])->SetCallbackEventsMask(bq_player_play[i], SL_PLAYEVENT_HEADATEND);
		(*bq_player_play[i])->SetPlayState(bq_player_play[i], SL_PLAYSTATE_PLAYING);
	}
}

void
sl_pause_sound(void)
{
	for (int i = 0; i < HAL_SOUND_TRACKS; i++) {
		if (bq_player_play[i] != NULL)
			(*bq_player_play[i])->SetPlayState(bq_player_play[i], SL_PLAYSTATE_STOPPED);
	}
}

void
sl_resume_sound(void)
{
	for (int i = 0; i < HAL_SOUND_TRACKS; i++) {
		if (bq_player_play[i] != NULL)
			(*bq_player_play[i])->SetPlayState(bq_player_play[i], SL_PLAYSTATE_PLAYING);
	}
}

static void
play_callback(BUFFERQUEUEITF bq, void *context)
{
	int stream = (intptr_t)context;

	pthread_mutex_lock(&sound_mutex[stream]);
	enqueue(stream);
	pthread_mutex_unlock(&sound_mutex[stream]);
}

static void
enqueue(int stream)
{
	if (wave[stream] == NULL)
		return;

	/* Set a post-finish flag if an end-of-stream was detected in a previous filling. */
	if (pre_finish[stream]) {
		wave[stream] = NULL;
		post_finish[stream] = true;
		return;
	}

	/* Get PCM samples. */
	int read_samples = hal_get_wave_samples(wave[stream], sample_buf[stream], BUF_FRAMES);

	/* Fill the remaining samples by zeros for a case where we have reached an end-of-stream. */
	if (read_samples < BUF_FRAMES)
		memset(sample_buf[stream] + read_samples, 0, (size_t)(BUF_FRAMES - read_samples) * FRAME_SIZE);

	/* Scale samples by a volume value. */
	scale_samples(sample_buf[stream], BUF_FRAMES, volume[stream]);

	/* Write the buffer. */
	(*bq_player_buffer_queue[stream])->Enqueue(bq_player_buffer_queue[stream], sample_buf[stream], read_samples * FRAME_SIZE);

	/* Set a pre-finish flag if we reached an end-of-stream. */
	if (hal_is_wave_eos(wave[stream]))
		pre_finish[stream] = true;
}

static void
scale_samples(uint32_t *buf, int frames, float vol)
{
	float scale;
	uint32_t frame;
	int32_t il, ir;	/* intermediate L/R */
	int16_t sl, sr;	/* source L/R*/
	int16_t dl, dr;	/* destination L/R */
	int i;

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

/*
 * HAL
 */

bool
hal_play_sound(
	int stream,
	struct hal_wave *w)
{
	if (bq_player_play[stream] == NULL)
		return true;

	pthread_mutex_lock(&sound_mutex[stream]);
	{
		wave[stream] = w;
		pre_finish[stream] = false;
		post_finish[stream] = false;
		enqueue(stream);
	}
	pthread_mutex_unlock(&sound_mutex[stream]);

	return true;
}

bool
hal_stop_sound(
	int stream)
{
	if (bq_player_play[stream] == NULL)
		return true;

	pthread_mutex_lock(&sound_mutex[stream]);
	{
		wave[stream] = NULL;
		pre_finish[stream] = false;
		post_finish[stream] = false;
	}
	pthread_mutex_unlock(&sound_mutex[stream]);

	return true;
}

bool
hal_set_sound_volume(
	int stream,
	float vol)
{
	volume[stream] = vol;

	__sync_synchronize();

	return true;
}

bool
hal_is_sound_finished(
	int stream)
{
	__sync_synchronize();

	if (!post_finish[stream])
	    return false;

	return true;
}
