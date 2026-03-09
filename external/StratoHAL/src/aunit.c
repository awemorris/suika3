/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Sound HAL for Apple AudioUnit
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

#include "stratohal/platform.h"
#include "aunit.h"

#include <AudioUnit/AudioUnit.h>

#include <pthread.h>

/* Sound format. */
#define SAMPLING_RATE   (44100)
#define CHANNELS        (2)
#define DEPTH           (16)
#define FRAME_SIZE      (4)

/* Temporary samples. */
#define TMP_SAMPLES     (512)

/* Audio Unit instance. */
static AudioUnit au;

/* For synchronization to the callback thread. */
static pthread_mutex_t mutex;

/* Input streams. */
static struct hal_wave *wave[HAL_SOUND_TRACKS];

/* Volumes. */
static float volume[HAL_SOUND_TRACKS];

/* Playback finish flags. */
static bool finish[HAL_SOUND_TRACKS];

/* Temporary sample buffer. */
static uint32_t tmpBuf[TMP_SAMPLES];

/* Flag to show whether initialized or not. */
static bool isInitialized;

/* Flag to show whether started or not. */
static bool isPlaying;

/* Forward declaration. */
static bool create_audio_unit(void);
static void destroy_audio_unit(void);
static OSStatus callback(void *inRef,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,
                         AudioBufferList *ioData);
static void mul_add_pcm(uint32_t *dst, uint32_t *src, float vol, int samples);

/*
 * Initialize Audio Unit.
 */
bool
init_aunit(void)
{
    int n;
    bool ret;

    /* Create Audio Unit. */
    if(!create_audio_unit())
        return false;

    /* Initialize the mutex. */
    pthread_mutex_init(&mutex, NULL);

    /* Initialize the volumes. */
    for (n = 0; n < HAL_SOUND_TRACKS; n++)
        volume[n] = 1.0f;

    /* Set initialized. */
    isInitialized = true;

    /* Star audio playback. */
    ret = AudioOutputUnitStart(au) == noErr;
    if (!ret)
        hal_log_error("AudioOutputUnitStart() failed.");
    else
        isPlaying = true;

    return ret;
}

/* Create Audio Unit. */
static bool
create_audio_unit(void)
{
    AudioComponentDescription cd;
    AudioComponent comp;
    AURenderCallbackStruct cb;
    AudioStreamBasicDescription streamFormat;

    /* Get an audio component. */
    cd.componentType = kAudioUnitType_Output;
#ifdef HAL_TARGET_IOS
    cd.componentSubType = kAudioUnitSubType_RemoteIO;
#else
    cd.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
    cd.componentManufacturer = kAudioUnitManufacturer_Apple;
    cd.componentFlags = 0;
    cd.componentFlagsMask = 0;
    comp = AudioComponentFindNext(NULL, &cd);
    if(comp == NULL) {
        hal_log_error("AudioComponentFindNext() failed.");
        return false;
    }
    if(AudioComponentInstanceNew(comp, &au) != noErr) {
        hal_log_error("AudioComponentInstanceNew() failed.");
        return false;
    }
    if(AudioUnitInitialize(au) != noErr) {
        hal_log_error("AudioUnitInitialize() failed.");
        return false;
    }

    /* Set a callback. */
    cb.inputProc = callback;
    cb.inputProcRefCon = NULL;
    if(AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback,
                            kAudioUnitScope_Input, 0, &cb,
                            sizeof(AURenderCallbackStruct)) != noErr) {
        hal_log_error("AudioUnitSetProperty() failed.");
        return false;
    }

    /* Set a format. */
    streamFormat.mSampleRate = 44100.0;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger |
                                kAudioFormatFlagIsPacked;
    streamFormat.mBitsPerChannel = 16;
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerFrame = 4;
    streamFormat.mBytesPerPacket = 4;
    streamFormat.mReserved = 0;
    if(AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Input, 0, &streamFormat,
                            sizeof(streamFormat)) != noErr) {
        hal_log_error("AudioUnitSetProperty() failed.");
        return false;
    }

    return true;
}

/*
 * Cleanup Audio Unit.
 */
void
cleanup_aunit(void)
{
    if(isInitialized) {
        /* Stop playback by destroying Audio Unit. */
        destroy_audio_unit();

        /* Destroy the mutex. */
        pthread_mutex_destroy(&mutex);
    }
}

/* Destroy Audio Unit. */
static void
destroy_audio_unit(void)
{
    AudioOutputUnitStop(au);
    AudioUnitUninitialize(au);
    AudioComponentInstanceDispose(au);
}

/*
 * Start playback on a stream.
 */
bool
hal_play_sound(
    int stream,
    struct hal_wave *w)
{
    bool ret;

    ret = true;
    pthread_mutex_lock(&mutex);
    {
        /* Set a stream. */
        wave[stream] = w;

        /* Clear a finish flag. */
        finish[stream] = false;

        /* Start playback if not playing. */
        if(!isPlaying) {
            ret = AudioOutputUnitStart(au) == noErr;
            if (!ret)
                hal_log_error("AudioOutputUnitStart() failed.");
            isPlaying = true;
        }
    }
    pthread_mutex_unlock(&mutex);

    return ret;
}

/*
 * Stop playback on a stream.
 */
bool
hal_stop_sound(
    int stream)
{
    bool ret;

    ret = true;
    pthread_mutex_lock(&mutex);
    {
        /* Clear a current stream. */
        wave[stream] = NULL;

        /* Set a finish flag. */
        finish[stream] = true;
    }
    pthread_mutex_unlock(&mutex);

    return ret;
}

/*
 * Set a volume of a stream.
 */
bool
hal_set_sound_volume(
    int stream,
    float vol)
{
    /*
     * pthread_mutex_lock(&mutex);
     * {
     */

    volume[stream] = vol;

    /* 
     * }
     * pthread_mutex_unlock(&mutex);
     */

    return true;
}

/*
 * Check if playback is finished on a stream.
 */
bool
hal_is_sound_finished(
    int stream)
{
    if (finish[stream])
        return true;

    return false;
}

/*
 * Callback Thread
 */

/* Callback function. */
static OSStatus
callback(
    void *inRef,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData)
{
    uint32_t *samplePtr;
    int stream, ret, remain, readSamples;

    UNUSED_PARAMETER(inRef);
    UNUSED_PARAMETER(ioActionFlags);
    UNUSED_PARAMETER(inTimeStamp);
    UNUSED_PARAMETER(inBusNumber);

    /* Zero clear the buffer. */
    samplePtr = (uint32_t *)ioData->mBuffers[0].mData;
    memset(samplePtr, 0, sizeof(uint32_t) * inNumberFrames);

    pthread_mutex_lock(&mutex);
    {
        remain = (int)inNumberFrames;
        while (remain > 0) {
            /* Determine a sample count to read once. */
            readSamples = remain > TMP_SAMPLES ? TMP_SAMPLES : remain;

            /* For each stream: */
            for (stream = 0; stream < HAL_SOUND_TRACKS; stream++) {
                /* If not playing on the stream, ignore. */
                if (wave[stream] == NULL)
                    continue;

                /* Get samples from a input stream. */
                ret = hal_get_wave_samples(wave[stream], tmpBuf, readSamples);

                /* If reached the end of the stream: */
                if(ret < readSamples) {
                    /* Zero clear the remaining. */
                    memset(tmpBuf + ret, 0, (size_t)(readSamples - ret) * sizeof(uint32_t));

                    /* Clear a current stream. */
                    wave[stream] = NULL;

                    /* Set a finish flag. */
                    finish[stream] = true;
                }

                /* Do mixing. */
                mul_add_pcm(samplePtr, tmpBuf, volume[stream], readSamples);
            }

            /* Increment the position. */
            samplePtr += readSamples;
            remain -= readSamples;
        }
    }
    pthread_mutex_unlock(&mutex);

    return noErr;
}

/*
 * Pause playback.
 */
void
pause_sound(void)
{
    pthread_mutex_lock(&mutex);
    {
        if(isPlaying) {
            AudioOutputUnitStop(au);
            isPlaying = false;
        }
    }
    pthread_mutex_unlock(&mutex);
}

/*
 * Resume playback.
 */
void
resume_sound(void)
{
    pthread_mutex_lock(&mutex);
    {
        if(!isPlaying) {
            AudioOutputUnitStart(au);
            isPlaying = true;
        }
    }
    pthread_mutex_unlock(&mutex);
}

/* Do mixing. */
static void
mul_add_pcm(uint32_t *dst, uint32_t *src, float vol, int samples)
{
    float scale;
    int i;
    int32_t il, ir; /* intermediate L/R */
    int16_t sl, sr; /* source L/R*/
    int16_t dl, dr; /* destination L/R */

    /* Convert a volume to a exponential scale factor. */
    scale = (powf(10.0f, vol) - 1.0f) / (10.0f - 1.0f);

    /* Compose each sample. */
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
