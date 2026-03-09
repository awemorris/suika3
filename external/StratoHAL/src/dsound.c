/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Sound HAL for DirectSound 5.0
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
#include "stratohal/platform.h"

/* Windows */
#include <windows.h>
#define DIRECTSOUND_VERSION 0x0500 /* Windows 98 default */
#include <dsound.h>
#define INITGUID
#include <initguid.h>

/* C */
#include <math.h>
#include <assert.h>

/*
 * Sound Format
 */
#define BIT_DEPTH			(16)
#define SAMPLES_PER_SEC		(44100)
#define CHANNELS			(2)
#define BYTES_PER_SAMPLE	(4)

/*
 * 1sec Ring Buffer Size
 */
#define BUF_SAMPLES			(44100)
#define BUF_BYTES			(SAMPLES_PER_SEC * BIT_DEPTH / 8 * CHANNELS)

/*
 * Buffer Division
 */
#define BUF_AREAS			(4)
#define AREA_SAMPLES		(BUF_SAMPLES / BUF_AREAS)
#define AREA_BYTES			(BUF_BYTES / BUF_AREAS)

/*
 * DirectSound Objects
 */
static LPDIRECTSOUND pDS;
static LPDIRECTSOUNDBUFFER pDSBuffer[HAL_SOUND_TRACKS];
static LPDIRECTSOUNDNOTIFY pDSNotify[HAL_SOUND_TRACKS];
static WAVEFORMATEX wfPrimary;

/*
 * Fill thread.
 */
static HANDLE hEventThread;

/*
 * For thread communication.
 */
static HANDLE hNotifyEvent[HAL_SOUND_TRACKS];
static HANDLE hQuitEvent;

/*
 * Input streams and critical sections for them.
 */
static struct hal_wave *pStream[HAL_SOUND_TRACKS];
static CRITICAL_SECTION	StreamCritical;

/*
 * Finish flags.
 */
static BOOL bFinish[HAL_SOUND_TRACKS];

/*
 * Current update area.
 *  - (-1) for the first time
 */
static int nPosCurArea[HAL_SOUND_TRACKS];

/*
 * Area of the end of a stream.
 *  - (-1) before the end-of-stream
 */
static int nPosEndArea[HAL_SOUND_TRACKS];

/*
 * Flags to show whether touched the EOS area.
 */
static int bLastTouch[HAL_SOUND_TRACKS];

/*
 * Initial volumes.
 */
static float fInitialVol[HAL_SOUND_TRACKS] = {1.0f, 1.0f, 1.0f, 1.0f};

/*
 * Successfully initialized?
 */
static BOOL bInitialized;

/*
 * Forward declaration.
 */
static BOOL CreatePrimaryBuffer();
static BOOL CreateSecondaryBuffers();
static BOOL RestoreBuffers(int nBuffer);
static BOOL PlaySoundBuffer(int nBuffer, struct hal_wave *pStr);
static VOID StopSoundBuffer(int nBuffer);
static BOOL SetBufferVolume(int nBuffer, float Vol);
static BOOL WriteNext(int nBuffer);
static DWORD WINAPI EventThread(LPVOID lpParameter);
static VOID OnNotifyPlayPos(int nBuffer);

/*
 * Initialize DirectSound
 */
BOOL
DSInitialize(
	HWND hWnd)
{
	HRESULT hRet;
	int i;

	/* Get an IDirectSound instance. */
	hRet = CoCreateInstance(&CLSID_DirectSound,
							NULL,
							CLSCTX_INPROC_SERVER,
							&IID_IDirectSound,
							(void **)&pDS);
	if(hRet != S_OK || pDS == NULL)
		return FALSE;

	/* Initialize a COM instance. */
	IDirectSound_Initialize(pDS, NULL);

	/* Set a cooperative level. */
	hRet = IDirectSound_SetCooperativeLevel(pDS, hWnd, DSSCL_PRIORITY);
	if(hRet != DS_OK)
		return FALSE;

	/* Create a primary buffer. */
	if(!CreatePrimaryBuffer())
		return FALSE;

	/* Create a secondary buffer. */
	if(!CreateSecondaryBuffers())
		return FALSE;

	/* Create an event to norify an exit to the event thread. */
	hQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hQuitEvent == NULL)
		return FALSE;

	/* Start the event thread to receive notifications of playback position. */
	hEventThread = CreateThread(NULL, 0, EventThread, NULL, 0, NULL); //t = _beginthread(EventThread, 0, NULL);
	if(hEventThread == NULL)
		return FALSE;

	/* Set volumes that are set before initialization. */
	for(i = 0; i < HAL_SOUND_TRACKS; i++)
		SetBufferVolume(i, fInitialVol[i]);

	bInitialized = TRUE;
	return TRUE;
}

/*
 * Cleanup DirectSound.
 */
VOID
DSCleanup(VOID)
{
	int i;

	/* Wait for the exit of the event thread. */
	if(hQuitEvent != NULL)
	{
		/* Notify quit.*/
		SetEvent(hQuitEvent);

		/* Wait for quit. */
		if(hEventThread != NULL)
			WaitForSingleObject(hEventThread, 1000*30);

		CloseHandle(hQuitEvent);
		CloseHandle(hEventThread);
	}

	/* Delete a critical section object. */
	DeleteCriticalSection(&StreamCritical);

	/* Release secondary buffers and notification events. */
	for(i=0; i<HAL_SOUND_TRACKS; i++)
	{
		if(pDSNotify[i] != NULL)
		{
			IDirectSoundNotify_Release(pDSNotify[i]);
			pDSNotify[i] = NULL;
		}
		if(pDSBuffer[i] != NULL)
		{
			IDirectSoundBuffer_Release(pDSBuffer[i]);
			pDSBuffer[i] = NULL;
		}
		if(hNotifyEvent[i] != NULL)
		{
			CloseHandle(hNotifyEvent[i]);
			hNotifyEvent[i] = NULL;
		}
	}

	/* Release DirectSound instance. */
	if(pDS != NULL)
	{
		IDirectSound_Release(pDS); 
		pDS = NULL;
	}
}

/*
 * Start playback on a stream.
 */
bool
hal_play_sound(
	int stream,
	struct hal_wave *w)
{
	if (!bInitialized)
		return true;

	assert(pDS != NULL);
	assert(stream >= 0 && stream < HAL_SOUND_TRACKS);
	assert(w != NULL);

	/* Stop if playing. */
	StopSoundBuffer(stream);

	/* Start playing a buffer. */
	PlaySoundBuffer(stream, w);
	return true;
}

/*
 * Stop playing on a stream.
 */
bool
hal_stop_sound(
	int stream)
{
	if (!bInitialized)
		return true;

	assert(pDS != NULL);
	assert(stream >= 0 && stream < HAL_SOUND_TRACKS);

	/* Stop if playing. */
	StopSoundBuffer(stream);

	return true;
}

/*
 * Set a volume on a stream.
 */
bool
hal_set_sound_volume(
	int stream,
	float vol)
{
	if (!bInitialized)
		return true;

	assert(stream >= 0 && stream < HAL_SOUND_TRACKS);

	if (pDS == NULL)
	{
		fInitialVol[stream] = vol;
		return true;
	}

	return SetBufferVolume(stream, vol);
}

/*
 * Check if playback is finished.
 */
bool
hal_is_sound_finished(
	int stream)
{
	if (!bInitialized)
		return true;

    if (bFinish[stream])
        return true;

    return false;
}

/*
 * Create a primary buffer and set a format.
 */
static BOOL
CreatePrimaryBuffer(VOID)
{
	DSBUFFERDESC dsbd;
	LPDIRECTSOUNDBUFFER pDSPrimary;
	HRESULT hRet;

	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	/* Create a primary buffer. */
	hRet = IDirectSound_CreateSoundBuffer(pDS, &dsbd, &pDSPrimary, NULL);
	if(hRet != DS_OK)
		return FALSE;

	/* Set a format. */
	ZeroMemory(&wfPrimary, sizeof(WAVEFORMATEX));
	wfPrimary.wFormatTag = WAVE_FORMAT_PCM;
	wfPrimary.nChannels = CHANNELS;
	wfPrimary.nSamplesPerSec = SAMPLES_PER_SEC;
	wfPrimary.wBitsPerSample = BIT_DEPTH;
	wfPrimary.nBlockAlign = (WORD)(wfPrimary.wBitsPerSample / 8 *
							wfPrimary.nChannels);
	wfPrimary.nAvgBytesPerSec = wfPrimary.nSamplesPerSec *
								wfPrimary.nBlockAlign;
	hRet = IDirectSoundBuffer_SetFormat(pDSPrimary, &wfPrimary);

	/* Release the primary buffer because we don't directly touch it. */
	IDirectSoundBuffer_Release(pDSPrimary);

	return hRet == DS_OK;
}

/*
 * Create a secondary buffer and enable a notification of playback position.
 */
static BOOL
CreateSecondaryBuffers(VOID)
{
	DSBPOSITIONNOTIFY pn[4];
	DSBUFFERDESC dsbd;
	HRESULT hRet;
	int i;

	memset(&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY |	 /* Use position notification */
				   DSBCAPS_GETCURRENTPOSITION2 | /* Accurate position on GetCurrentPositon() */
				   DSBCAPS_GLOBALFOCUS |         /* Play when inactive */
				   DSBCAPS_CTRLVOLUME;           /* Enable volume */
	dsbd.dwBufferBytes = BUF_BYTES;
	dsbd.lpwfxFormat = &wfPrimary;

	for(i=0; i<HAL_SOUND_TRACKS; i++)
	{
		// Create a secondary buffer.
		hRet = IDirectSound_CreateSoundBuffer(pDS, &dsbd, &pDSBuffer[i], NULL);
		if(hRet != DS_OK)
			return FALSE;

		// Prepare for notification.
		hRet = IDirectSoundBuffer_QueryInterface(pDSBuffer[i],
												 &IID_IDirectSoundNotify,
												 (VOID**)&pDSNotify[i]);
		if(hRet != S_OK)
			return FALSE;

		hNotifyEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(hNotifyEvent[i] == NULL)
			return FALSE;

		// Set notification positions. (n * 1/4)
		pn[0].dwOffset = 0;
		pn[0].hEventNotify = hNotifyEvent[i];
		pn[1].dwOffset = AREA_BYTES;
		pn[1].hEventNotify = hNotifyEvent[i];
		pn[2].dwOffset = AREA_BYTES * 2;
		pn[2].hEventNotify = hNotifyEvent[i];
		pn[3].dwOffset = AREA_BYTES * 3;
		pn[3].hEventNotify = hNotifyEvent[i];
		hRet = IDirectSoundNotify_SetNotificationPositions(pDSNotify[i], 4, pn);
		if(hRet != DS_OK)
			return FALSE;
    }

	InitializeCriticalSection(&StreamCritical);
	return TRUE;
}

/*
 * Restore buffers.
 */
static BOOL
RestoreBuffers(int nBuffer)
{
	DWORD dwStatus;
	HRESULT hRet;

	assert(pDSBuffer[nBuffer] != NULL);
	assert(nBuffer >= 0 && nBuffer < HAL_SOUND_TRACKS);

	hRet = IDirectSoundBuffer_GetStatus(pDSBuffer[nBuffer], &dwStatus);
	if(hRet != DS_OK)
		return FALSE;
	if(dwStatus & DSBSTATUS_BUFFERLOST)
    {
		/*
		 * App may not able to get control of DirectSoud at a startup.
		 * Thus, sleep until app gets control.
		 */
		while(1) {
			Sleep(10);
            hRet = IDirectSoundBuffer_Restore(pDSBuffer[nBuffer]);
            if(hRet != DSERR_BUFFERLOST)
				break;
		}
    }
    return TRUE;
}

/*
 * Start playing a buffer.
 */
static BOOL
PlaySoundBuffer(int nBuffer,
				struct hal_wave *pStr)
{
	HRESULT hRet;
	int i;

	assert(pDSBuffer[nBuffer] != NULL);
	assert(pStream[nBuffer] == NULL);
	assert(nBuffer >= 0 && nBuffer < HAL_SOUND_TRACKS);

	/* Restore a buffer if it is lost. */
	if(!RestoreBuffers(nBuffer))
		return FALSE;

	/* Clea a finish flag. */
	bFinish[nBuffer] = FALSE;

	/*
	 * Mutually exclude with the event thread. Use mutual exclusion to
	 * prevent race conditions.  If playback starts immediately after
	 * stopping, a delayed notification before the stop event may
	 * trigger unwanted buffering during playback.
	 */
	EnterCriticalSection(&StreamCritical);
	{
		/* Set a stream. */
		pStream[nBuffer] = pStr;

		/* Clea an end area. */
		nPosEndArea[nBuffer] = -1;
		bLastTouch[nBuffer] = FALSE;

		/* Fill a buffer. */
		nPosCurArea[nBuffer] = 0;
		for(i=0; i<BUF_AREAS; i++)
			WriteNext(nBuffer);
	}
	LeaveCriticalSection(&StreamCritical);

	/* Play a buffer. */
	hRet = IDirectSoundBuffer_Play(pDSBuffer[nBuffer], 0, 0, DSBPLAY_LOOPING);
	if(hRet != DS_OK)
		return FALSE;

	return TRUE;
}

/*
 * Stop playback on a stream.
 */
static VOID
StopSoundBuffer(
	int nBuffer)
{
	assert(pDSBuffer[nBuffer] != NULL);
	assert(nBuffer >= 0 && nBuffer < HAL_SOUND_TRACKS);

	/* Mutually exclude with the event thread. */
	EnterCriticalSection(&StreamCritical);
	{
		/* If the buffer is playing back: */
		if(pStream[nBuffer] != NULL)
		{
			/* Stop the playback. */
			IDirectSoundBuffer_Stop(pDSBuffer[nBuffer]);
			IDirectSoundBuffer_SetCurrentPosition(pDSBuffer[nBuffer], 0);

			/* Clear a current stream. */
			pStream[nBuffer] = NULL;
		}
	}
	LeaveCriticalSection(&StreamCritical);
}

/*
 * Set a buffer volume.
 */
BOOL
SetBufferVolume(
	int nBuffer,
	float Vol)
{
	LONG dB;

	Vol = (Vol > 1.0f) ? 1.0f : Vol;
	Vol = (Vol < 0.0f) ? 0.0f : Vol;

	/*
	 * Get a damping ratio in dB.
	 *  - 0dB(DSBVOLIME_MAX=0) == original
	 *  - -100dB(DSBVOLUME_MIN=-10000) == mute
	 *  - Scale by 100 (it's a 100x fixed point)
     */
	if(Vol <= 0.00001f)
		dB = DSBVOLUME_MIN;
	else
		dB = (LONG)(20.0f * (float)log10(Vol) * 100.0f);

	assert(dB >= DSBVOLUME_MIN && dB <= DSBVOLUME_MAX);

	/* Set a buffer volume. */
	IDirectSoundBuffer_SetVolume(pDSBuffer[nBuffer], (LONG)dB);
	return TRUE;
}

/*
 * Read data from a PCM stream.
 *  - This function is called inside a critical section on the event thread.
 */
static BOOL
WriteNext(
	int nBuffer)
{
	VOID *pBuf[2];
	DWORD dwLockedBytes[2];
	DWORD dwOffset;
	HRESULT hRet;
	int nArea, nSamples;

	assert(nBuffer >= 0 && nBuffer < HAL_SOUND_TRACKS);
	assert(nPosCurArea[nBuffer] >= 0 && nPosCurArea[nBuffer] < BUF_AREAS);

	/* Get an area where playback is finished (and where we write) and increment it. */
	nArea = nPosCurArea[nBuffer];
	nPosCurArea[nBuffer] = (nPosCurArea[nBuffer] + 1) % BUF_AREAS;

	/* Lock the buffer. */
	dwOffset = (DWORD)nArea * AREA_BYTES;
	hRet = IDirectSoundBuffer_Lock(pDSBuffer[nBuffer],
								   dwOffset,
								   AREA_BYTES,
								   &pBuf[0],
								   &dwLockedBytes[0],
								   &pBuf[1],
								   &dwLockedBytes[1],
								   0);
	switch(hRet)
	{
	case DS_OK:
		assert(pBuf[1] == NULL && dwLockedBytes[1] == 0);
		break;
	case DSERR_BUFFERLOST:
		/* Restore the buffer and lock it again. */
		if(!RestoreBuffers(nBuffer))
			return FALSE;
		hRet = IDirectSoundBuffer_Lock(pDSBuffer[nBuffer],
									   dwOffset,
									   AREA_BYTES,
									   &pBuf[0],
									   &dwLockedBytes[0],
									   &pBuf[1],
									   &dwLockedBytes[1],
									   0);
		if (hRet != DS_OK)
			return FALSE;
		break;
	default:
		return FALSE;
	}

	/* If not reached the end-of-stream. */
	if(nPosEndArea[nBuffer] == -1)
	{
		/* Copy from the PCM stream to the buffer. */
		nSamples = hal_get_wave_samples(pStream[nBuffer], (uint32_t *)pBuf[0], AREA_SAMPLES);

		/* If reached the end-of-stream. */
		if(nSamples != AREA_SAMPLES)
		{
			/* Zero clear the remaining of the buffer. */
			ZeroMemory((char*)pBuf[0] + nSamples * BYTES_PER_SAMPLE,
					   (size_t)(AREA_SAMPLES - nSamples) * BYTES_PER_SAMPLE);

			/* Save the position. */
			nPosEndArea[nBuffer] = nArea;
		}
	}
	else 
	{
		/* Zero clear the buffer if reached end-of-stream. */
		ZeroMemory(pBuf[0], AREA_BYTES);
	}

	/* Unlock the buffer. */
	hRet = IDirectSoundBuffer_Unlock(pDSBuffer[nBuffer],
									 pBuf[0],
									 dwLockedBytes[0],
									 pBuf[1],
									 dwLockedBytes[1]);
	if(hRet != DS_OK)
		return FALSE;

	return TRUE;
}

/*
 * Event Thread
 */

/*
 * Main loop of the event thread.
 */
static DWORD WINAPI
EventThread(
	LPVOID lpParameter)
{
	HANDLE hEvents[HAL_SOUND_TRACKS+1];
	DWORD dwResult;
	int i, nBuf;

	UNUSED_PARAMETER(lpParameter);

	/* Create an array for events. */
	for(i=0; i<HAL_SOUND_TRACKS; i++)
		hEvents[i] = hNotifyEvent[i];	/* For playback position. */
	hEvents[HAL_SOUND_TRACKS] = hQuitEvent;	/* For quit event. */

	/* Event wait loop. */
	while(1)
	{
		/* Wait for a notification. */
		dwResult = WaitForMultipleObjects(HAL_SOUND_TRACKS + 1,
										  hEvents,
										  FALSE,
										  INFINITE);
		if(dwResult == WAIT_TIMEOUT || dwResult == WAIT_FAILED)
			continue;
		if(dwResult == WAIT_OBJECT_0 + HAL_SOUND_TRACKS)
			break;		/* hQuitEvent is set. */

		/* Get a buffer index of notification source. */
		nBuf = (int)(dwResult - WAIT_OBJECT_0);
		assert(nBuf >= 0 && nBuf < HAL_SOUND_TRACKS);

		/* Reset the event to non-signal state. */
		ResetEvent(hNotifyEvent[nBuf]);

		/* Mutually exclude with StopSoundBuffer(). */
		EnterCriticalSection(&StreamCritical);
		{
			if(pStream[nBuf] == NULL) {
				/* Playback is stopped as a mutual exclusion. */
			} else {
				/* Update a buffer. */
				OnNotifyPlayPos(nBuf);
			}
		}
		LeaveCriticalSection(&StreamCritical);
	}

	return 0;
}

/*
 * Handler for playback position notification.
 */
static VOID
OnNotifyPlayPos(
	int nBuffer)
{
	DWORD dwPlayPos;
	HRESULT hRet;

	/* If we finished to play the playback-end-area. */
	if(nPosEndArea[nBuffer] != -1 && bLastTouch[nBuffer] &&
	   nPosCurArea[nBuffer] == (nPosEndArea[nBuffer] + 1) % BUF_AREAS)
	{
		/* Stop a playback. */
		IDirectSoundBuffer_Stop(pDSBuffer[nBuffer]);
		IDirectSoundBuffer_SetCurrentPosition(pDSBuffer[nBuffer], 0);

		/* Clear a current stream. */
		pStream[nBuffer] = NULL;

		/* Set a finish flag. */
		bFinish[nBuffer] = TRUE;

		/* Finish a playback. */
		return;
	}

	/* Record bLastTouch. */
	if(nPosEndArea[nBuffer] != -1 &&
	   nPosCurArea[nBuffer] == nPosEndArea[nBuffer])
		bLastTouch[nBuffer] = TRUE;

	/* Get a playback position. */
	hRet = IDirectSoundBuffer_GetCurrentPosition(pDSBuffer[nBuffer], &dwPlayPos, NULL);
	if(hRet != DS_OK)
		return;	/* Failed to get a playback position. */

	/*
	 * If we are on the area to update:
	 *  - This occurs when the first notification.
	 *  - Or, when a delay is larger than a buffer size.
	 *  - We wait for the next notification without updating the buffer.
	 *  - In the delay case, the buffer will be repeated.
	 */
	if(dwPlayPos >= (DWORD)nPosCurArea[nBuffer] * AREA_BYTES &&
	   dwPlayPos < ((DWORD)nPosCurArea[nBuffer] + 1) * AREA_BYTES)
		return;	/* No update. */

	/* Update the buffer. */
	WriteNext(nBuffer);
}
