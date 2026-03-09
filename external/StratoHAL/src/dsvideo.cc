/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Video playback HAL for DirectShow
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
#include "dsvideo.h"
#include <assert.h>
};

#include <dshow.h>

#define PATH_SIZE	(1024)

// DirectShow Interface
static IGraphBuilder *pBuilder;
static IMediaControl *pControl;
static IVideoWindow *pWindow;
static IMediaEventEx *pEvent;

static BOOL bPlaying;

static BOOL DisplayRenderFileErrorMessage(HRESULT hr);

//
// Play a video.
//
BOOL
DShowPlayVideo(
	HWND hWnd,
	const char *pszFileName,
	int nOfsX,
	int nOfsY,
	int nWidth,
	int nHeight)
{
	HRESULT hRes;

	// Get an instance of IGraphBuilder.
	hRes = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
							IID_IGraphBuilder, (void**)&pBuilder);
	if(hRes != S_OK || !pBuilder)
	{
		hal_log_error("CoCreateInstance() failed.");
		return FALSE;
	}

	// Generate a filter graph.
	WCHAR wFileName[PATH_SIZE];
	mbstowcs(wFileName, pszFileName, PATH_SIZE);
	HRESULT hr = pBuilder->RenderFile(wFileName, NULL);
	if(!DisplayRenderFileErrorMessage(hr))
		return FALSE;

	// Specify an owner window.
	pBuilder->QueryInterface(IID_IVideoWindow, (void **)&pWindow);
	if(pBuilder == NULL)
	{
		hal_log_error("IGraphBuilder::QueryInterface() failed.");
		return FALSE;
	}
	pWindow->put_Owner((OAHWND)hWnd);
	pWindow->put_MessageDrain((OAHWND)hWnd);
	pWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	pWindow->SetWindowPosition(nOfsX, nOfsY, nWidth, nHeight);

	// Set hWnd to get events.
	pBuilder->QueryInterface(IID_IMediaEventEx, (void**)&pEvent);
	if(pEvent == NULL)
	{
		hal_log_error("IGraphBuilder::QueryInterface() failed.");
		return FALSE;
	}
	pEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);

	// Play a file.
	pBuilder->QueryInterface(IID_IMediaControl, (void **)&pControl);
	if(pControl == NULL)
	{
		hal_log_error("IGraphBuilder::QueryInterface() failed.");
		return FALSE;
	}
	hRes = pControl->Run();
	if (FAILED(hRes))
	{
		hal_log_error("IMediaControl::Run() failed.");
		return FALSE;
	}

	bPlaying = TRUE;

	return TRUE;
}

static BOOL
DisplayRenderFileErrorMessage(
	HRESULT hr)
{
	switch(hr)
	{
	case S_OK:
	case VFW_S_DUPLICATE_NAME:
		break;
	case VFW_S_AUDIO_NOT_RENDERED:
	case VFW_S_PARTIAL_RENDER:
	case VFW_S_VIDEO_NOT_RENDERED:
		hal_log_error("Unsupported codec.");
		return FALSE;
	case E_ABORT:
	case E_FAIL:
	case E_INVALIDARG:
	case E_OUTOFMEMORY:
	case E_POINTER:
	case VFW_E_CANNOT_CONNECT:
	case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
	case VFW_E_CANNOT_RENDER:
		hal_log_error("Video runtime error.");
		return FALSE;
	case VFW_E_INVALID_FILE_FORMAT:
		hal_log_error("Invalid video file format.");
		return FALSE;
	case VFW_E_NOT_FOUND:
		hal_log_error("File not found.");
		return FALSE;
	case VFW_E_UNKNOWN_FILE_TYPE:
		hal_log_error("Unknown video file type.");
		return FALSE;
	case VFW_E_UNSUPPORTED_STREAM:
		hal_log_error("Unsupported video stream.");
		return FALSE;
	default:
		hal_log_error("Unknown video error.");
		return FALSE;
	}

	return TRUE;
}

//
// Stop a video playback.
//
VOID
DShowStopVideo(VOID)
{
	if(pControl != NULL)
	{
		pControl->Stop();

		if(pBuilder)
		{
			pBuilder->Release();
			pBuilder = NULL;
		}
		if(pWindow)
		{
			pWindow->put_Visible(OAFALSE);
			pWindow->put_Owner((OAHWND)NULL);
			pWindow->Release();
			pWindow = NULL;
		}
		if(pControl)
		{
			pControl->Release();
			pControl = NULL;
		}
		if(pEvent)
		{
			pEvent->Release();
			pEvent = NULL;
		}
	}

	bPlaying = FALSE;
}

//
// Check if a video is playing back.
//
BOOL
DShowIsVideoPlaying(VOID)
{
	return bPlaying;
}

//
// Process events (Called from WndProc)
//
BOOL
DShowProcessEvents(VOID)
{
	assert(pEvent != NULL);

	// Get a playback complete event. (Don't process other events.)
	LONG code;
	LONG_PTR p, q;
	BOOL bContinue = TRUE;
	while(SUCCEEDED(pEvent->GetEvent(&code, &p, &q, 0)))
	{
		switch(code)
		{
		case EC_COMPLETE:
			// Playback complete event.
			bContinue = FALSE;
			pEvent->SetNotifyWindow((OAHWND)NULL, 0, 0);
			break;
		}

		pEvent->FreeEventParams(code, p, q);
	}

	// Release resources if a playback is completed.
	if(!bContinue)
	{
		if(pBuilder)
		{
			pBuilder->Release();
			pBuilder = NULL;
		}
		if(pWindow)
		{
			pWindow->put_Visible(OAFALSE);
			pWindow->put_Owner((OAHWND)NULL);
			pWindow->Release();
			pWindow = NULL;
		}
		if(pControl)
		{
			pControl->Release();
			pControl = NULL;
		}
		if(pEvent)
		{
			pEvent->Release();
			pEvent = NULL;
		}

		bPlaying = FALSE;
	}

	// TRUE for continue, FALSE for exit.
	return bContinue;
}
