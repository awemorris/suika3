/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Sound HAL for XAudio2
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

extern "C" {
#include "stratohal/platform.h"
#include "xa2sound.h"
};

#include <windows.h>
#include <xaudio2.h>

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//
// Sound Format (same as dsound.c)
//
#define BIT_DEPTH            (16)
#define SAMPLES_PER_SEC      (44100)
#define CHANNELS             (2)
#define BYTES_PER_SAMPLE     (4)

//
// 1sec Ring Buffer Size
//
#define BUF_SAMPLES          (44100)
#define BUF_BYTES            (SAMPLES_PER_SEC * BIT_DEPTH / 8 * CHANNELS)

//
// Buffer Division
//
#define BUF_AREAS            (4)
#define AREA_SAMPLES         (BUF_SAMPLES / BUF_AREAS)
#define AREA_BYTES           (BUF_BYTES / BUF_AREAS)

//
// XAudio2 Objects
//
static IXAudio2*              g_xaudio = NULL;
static IXAudio2MasteringVoice* g_master = NULL;
static IXAudio2SourceVoice*    g_voice[SOUND_TRACKS];

//
// Stream Dtate
//
static struct wave*            g_stream[SOUND_TRACKS];
static CRITICAL_SECTION        g_stream_cs;
static BOOL                    g_finish[SOUND_TRACKS];
static float                   g_initial_vol[SOUND_TRACKS] = {1,1,1,1};
static BOOL                    g_initialized;

//
// Per-track ring buffers in system memory
//
static uint8_t*                g_area_mem[SOUND_TRACKS][BUF_AREAS];
static int                     g_cur_area[SOUND_TRACKS]; /* next area index to fill */
static BOOL                    g_eos_submitted[SOUND_TRACKS];

//
// Voice callback (per track)
//  - XAudio2 calls these on its internal worker thread.
///

struct TrackCallback final : public IXAudio2VoiceCallback
{
    int track = 0;

    explicit TrackCallback(int t = 0) : track(t) {}

    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override;
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
    void STDMETHODCALLTYPE OnStreamEnd() override;
    void STDMETHODCALLTYPE OnBufferStart(void*) override;
    void STDMETHODCALLTYPE OnBufferEnd(void* pContext) override;
    void STDMETHODCALLTYPE OnLoopEnd(void*) override;
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override;
};

static TrackCallback g_cb[SOUND_TRACKS];

//
// Forward
//
static void XA_Stop(int t);
static BOOL XA_Start(int t, struct wave* w);
static void XA_SetVolume(int t, float vol);
static BOOL XA_SubmitNext(int t, BOOL first_submit);

//
// Initialize XAudio2
//
BOOL XA2Initialize(VOID)
{
    HRESULT hr;

    if (g_initialized)
        return TRUE;

    memset(g_voice, 0, sizeof(g_voice));
    memset(g_stream, 0, sizeof(g_stream));
    memset(g_finish, 0, sizeof(g_finish));
    memset(g_eos_submitted, 0, sizeof(g_eos_submitted));
    memset(g_area_mem, 0, sizeof(g_area_mem));
    memset(g_cur_area, 0, sizeof(g_cur_area));

    InitializeCriticalSection(&g_stream_cs);

    hr = XAudio2Create(&g_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr) || g_xaudio == NULL)
        return FALSE;

    hr = g_xaudio->CreateMasteringVoice(&g_master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL, AudioCategory_GameEffects);
    if (FAILED(hr) || g_master == NULL)
        return FALSE;

    // Wave format.
    WAVEFORMATEX wfx;
    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = CHANNELS;
    wfx.nSamplesPerSec = SAMPLES_PER_SEC;
    wfx.wBitsPerSample = BIT_DEPTH;
    wfx.nBlockAlign = (WORD)(wfx.nChannels * (wfx.wBitsPerSample / 8));
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    for (int t = 0; t < SOUND_TRACKS; t++)
    {
        // Allocate area memory.
        for (int a = 0; a < BUF_AREAS; a++)
        {
            g_area_mem[t][a] = (uint8_t*)HeapAlloc(GetProcessHeap(), 0, AREA_BYTES);
            if (!g_area_mem[t][a])
                return FALSE;
            ZeroMemory(g_area_mem[t][a], AREA_BYTES);
        }

        // Init callback.
        g_cb[t].track = t;

        hr = g_xaudio->CreateSourceVoice(
            &g_voice[t],
            &wfx,
            0, // flags
            XAUDIO2_DEFAULT_FREQ_RATIO,
            &g_cb[t],
            NULL,
            NULL);
        if (FAILED(hr) || g_voice[t] == NULL)
            return FALSE;

        // Apply initial volume.
        XA_SetVolume(t, g_initial_vol[t]);
    }

    g_initialized = TRUE;
    return TRUE;
}

//
// Cleanup
//
VOID XA2Cleanup(VOID)
{
    if (!g_initialized)
        return;

    for (int t = 0; t < SOUND_TRACKS; t++)
    {
        if (g_voice[t])
        {
            g_voice[t]->Stop(0);
            g_voice[t]->FlushSourceBuffers();
            g_voice[t]->DestroyVoice();
            g_voice[t] = NULL;
        }
        for (int a = 0; a < BUF_AREAS; a++)
        {
            if (g_area_mem[t][a])
            {
                HeapFree(GetProcessHeap(), 0, g_area_mem[t][a]);
                g_area_mem[t][a] = NULL;
            }
        }
    }

    if (g_master)
    {
        g_master->DestroyVoice();
        g_master = NULL;
    }
    if (g_xaudio)
    {
        g_xaudio->Release();
        g_xaudio = NULL;
    }

    DeleteCriticalSection(&g_stream_cs);

    g_initialized = FALSE;
}

bool play_sound(int stream, struct wave *w)
{
    if (!g_initialized)
        return true;

    assert(stream >= 0 && stream < SOUND_TRACKS);
    assert(w != NULL);

    XA_Stop(stream);
    return XA_Start(stream, w) ? true : false;
}

bool stop_sound(int stream)
{
    if (!g_initialized)
        return true;

    assert(stream >= 0 && stream < SOUND_TRACKS);
    XA_Stop(stream);
    return true;
}

bool set_sound_volume(int stream, float vol)
{
    if (!g_initialized)
    {
        if (stream >= 0 && stream < SOUND_TRACKS)
            g_initial_vol[stream] = vol;
        return true;
    }

    assert(stream >= 0 && stream < SOUND_TRACKS);
    XA_SetVolume(stream, vol);
    return true;
}

bool is_sound_finished(int stream)
{
    if (!g_initialized)
        return true;
    return g_finish[stream] ? true : false;
}

static void XA_SetVolume(int t, float vol)
{
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;

    /* XAudio2 uses linear gain (1.0 = original). */
    if (g_voice[t])
        g_voice[t]->SetVolume(vol, XAUDIO2_COMMIT_NOW);
}

static void XA_Stop(int t)
{
    EnterCriticalSection(&g_stream_cs);
    {
        g_stream[t] = NULL;
        g_finish[t] = FALSE;
        g_eos_submitted[t] = FALSE;
        g_cur_area[t] = 0;

        if (g_voice[t])
        {
            g_voice[t]->Stop(0);
            g_voice[t]->FlushSourceBuffers();
        }
    }
    LeaveCriticalSection(&g_stream_cs);
}

static BOOL XA_Start(int t, struct wave* w)
{
    assert(g_voice[t] != NULL);

    EnterCriticalSection(&g_stream_cs);
    {
        g_stream[t] = w;
        g_finish[t] = FALSE;
        g_eos_submitted[t] = FALSE;
        g_cur_area[t] = 0;

        /* pre-submit 4 buffers */
        for (int i = 0; i < BUF_AREAS; i++)
        {
            if (!XA_SubmitNext(t, TRUE))
                break;
        }

        g_voice[t]->Start(0, XAUDIO2_COMMIT_NOW);
    }
    LeaveCriticalSection(&g_stream_cs);

    return TRUE;
}

// Fill one AREA and submit it.
static BOOL XA_SubmitNext(int t, BOOL first_submit)
{
    (void)first_submit;

    if (!g_voice[t])
        return FALSE;

    if (g_stream[t] == NULL)
        return FALSE;

    if (g_eos_submitted[t])
        return FALSE; // No more submits after EOS.

    int area = g_cur_area[t];
    g_cur_area[t] = (g_cur_area[t] + 1) % BUF_AREAS;

    uint8_t* dst = g_area_mem[t][area];
    assert(dst != NULL);

    // Read PCM frames.
    int nSamples = get_wave_samples(g_stream[t], (uint32_t*)dst, AREA_SAMPLES);

    XAUDIO2_BUFFER buf;
    memset(&buf, 0, sizeof(buf));
    buf.pAudioData = dst;
    buf.AudioBytes = AREA_BYTES;
    buf.pContext = (void*)(uintptr_t)area; // Optional

    if (nSamples != AREA_SAMPLES)
    {
        /* zero pad remaining */
        size_t written = (size_t)nSamples * BYTES_PER_SAMPLE;
        if (written < AREA_BYTES)
            ZeroMemory(dst + written, AREA_BYTES - written);

        // Mark EOS on this buffer.
        buf.Flags = XAUDIO2_END_OF_STREAM;
        g_eos_submitted[t] = TRUE;
    }

    HRESULT hr = g_voice[t]->SubmitSourceBuffer(&buf, NULL);
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//
// Callbacks
//

void STDMETHODCALLTYPE TrackCallback::OnVoiceProcessingPassStart(UINT32)
{
}

void STDMETHODCALLTYPE TrackCallback::OnVoiceProcessingPassEnd()
{
}

void STDMETHODCALLTYPE TrackCallback::OnStreamEnd()
{
    int t = track;

    EnterCriticalSection(&g_stream_cs);
    {
        // Stream finished.
        g_stream[t] = NULL;
        g_finish[t] = TRUE;

        if (g_voice[t])
        {
            g_voice[t]->Stop(0);
            g_voice[t]->FlushSourceBuffers();
        }
    }
    LeaveCriticalSection(&g_stream_cs);
}

void STDMETHODCALLTYPE TrackCallback::OnBufferStart(void*)
{
}

void STDMETHODCALLTYPE TrackCallback::OnBufferEnd(void* context)
{
    (void)context;

    int t = track;

    // Keep critical section short.
    EnterCriticalSection(&g_stream_cs);
    {
        if (g_stream[t] != NULL && !g_eos_submitted[t])
        {
            XA_SubmitNext(t, FALSE);
        }
    }
    LeaveCriticalSection(&g_stream_cs);
}

void STDMETHODCALLTYPE TrackCallback::OnLoopEnd(void*)
{
}

void STDMETHODCALLTYPE TrackCallback::OnVoiceError(void*, HRESULT)
{
}
