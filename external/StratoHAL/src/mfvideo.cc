/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Video playback HAL for Media Foundation
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

#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <evr.h>
#include <strsafe.h>

extern "C" {
#include "mfvideo.h"
};

//#pragma comment(lib, "mf.lib")
//#pragma comment(lib, "mfplat.lib")
//#pragma comment(lib, "mfuuid.lib")
//#pragma comment(lib, "mfreadwrite.lib")
//#pragma comment(lib, "evr.lib")

template<class T>
static void SafeRelease(T*& p)
{
	if (p)
	{
		p->Release(); p = nullptr;
	}
}

static inline HRESULT HR(HRESULT hr)
{
	return hr;
}

struct MFPlayerState
{
    bool inited = false;
    bool playing = false;
    bool closing = false;

    HWND hwnd = nullptr;
    RECT dstRect = { 0,0,0,0 };

    IMFMediaSession* session = nullptr;
    IMFMediaSource* source = nullptr;
    IMFTopology* topology = nullptr;
    IMFVideoDisplayControl* videoDisplay = nullptr;

    HANDLE hCloseEvent = nullptr;
};
static MFPlayerState g;

// Forward declaraton.
static HRESULT MFEnsureStartup();
static HRESULT MFCreateSession();
static HRESULT MFCreateSourceFromURL(const wchar_t* url, IMFMediaSource** ppSource);
static HRESULT MFBuildTopology(IMFMediaSource* pSource, HWND hwnd, IMFTopology** ppTopology);
static HRESULT MFApplyVideoRect();
static VOID MFCleanup(bool keepMFStartup);
static HRESULT AnsiToWideACP(const char* s, wchar_t** outWide);

BOOL MFPlayVideo(HWND hWnd, const char* pszFileName, int nOfsX, int nOfsY, int nWidth, int nHeight)
{
    if (!hWnd || !pszFileName || nWidth <= 0 || nHeight <= 0)
        return FALSE;

    HRESULT hr = S_OK;

    // Stop previous playback if any.
    MFStopVideo();

    hr = MFEnsureStartup();
    if (FAILED(hr))
	    return FALSE;

    g.hwnd = hWnd;
    g.dstRect.left = nOfsX;
    g.dstRect.top = nOfsY;
    g.dstRect.right = nOfsX + nWidth;
    g.dstRect.bottom = nOfsY + nHeight;

    wchar_t* wfile = nullptr;
    hr = AnsiToWideACP(pszFileName, &wfile);
    if (FAILED(hr))
		return FALSE;

    hr = MFCreateSession();
    if (SUCCEEDED(hr))
		hr = MFCreateSourceFromURL(wfile, &g.source);
    if (SUCCEEDED(hr))
		hr = MFBuildTopology(g.source, hWnd, &g.topology);
    if (SUCCEEDED(hr))
		hr = g.session->SetTopology(0, g.topology);

    CoTaskMemFree(wfile);

    if (FAILED(hr))
	{
        MFCleanup(false);
        return FALSE;
    }

    // Start: session will actually start after MESessionTopologyStatus(READY) etc.
    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    hr = g.session->Start(&GUID_NULL, &varStart);
    PropVariantClear(&varStart);
    if (FAILED(hr)) {
        MFCleanup(false);
        return FALSE;
    }

	// optimistic; will be corrected by events if it ends/errors.
    g.playing = true;

    // Send WM_SIZE and WM_PAINT to adjust viewport.
    RECT rc{};
    GetClientRect(hWnd, &rc);
    SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
    InvalidateRect(g.hwnd, nullptr, TRUE);
    UpdateWindow(g.hwnd);

    return TRUE;
}

VOID MFStopVideo(VOID)
{
    if (!g.inited)
		return;

    if (g.session)
	{
        // Ask session to stop and close. We will wait for MESessionClosed.
        g.playing = false;
        g.closing = true;

        // Stop can fail if not started yet; ignore best-effort.
        g.session->Stop();
        g.session->Close();

        // Pump events until closed.
		// Avoid deadlock by also allowing caller to call MFProcessEvent,
        // but here we wait a bit to ensure resources are released.
        if (g.hCloseEvent)
		{
            // Give it some time; if you don't want to block, remove this wait and rely on MFProcessEvent().
            for (int i = 0; i < 200; ++i)
			{
                MFProcessEvents();
                DWORD w = WaitForSingleObject(g.hCloseEvent, 5);
                if (w == WAIT_OBJECT_0)
					break;
            }
        }
    }

    MFCleanup(false);
}

BOOL MFIsVideoPlaying(VOID)
{
    return g.playing ? TRUE : FALSE;
}

BOOL MFProcessEvents(VOID)
{
    if (!g.session)
		return FALSE;

    bool handledAny = false;

    while (true) {
        IMFMediaEvent* pEvent = nullptr;
        HRESULT hr = g.session->GetEvent(MF_EVENT_FLAG_NO_WAIT, &pEvent);
        if (hr == MF_E_NO_EVENTS_AVAILABLE)
			break;
        if (FAILED(hr))
			break;

        handledAny = true;

        MediaEventType met = MEUnknown;
        pEvent->GetType(&met);

        HRESULT hrStatus = S_OK;
        pEvent->GetStatus(&hrStatus);
        if (FAILED(hrStatus))
		{
            // Something went wrong: stop.
            g.playing = false;
        }

        switch (met) {
        case MESessionTopologyStatus:
        {
            UINT32 topoStatus = 0;
            pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &topoStatus);
            if (topoStatus == MF_TOPOSTATUS_READY)
			{
                // Now we can query video display service and apply destination rect.
                MFApplyVideoRect();
            }
            break;
        }
        case MESessionStarted:
            MFApplyVideoRect();
            break;
        case MESessionEnded:
            // Reached end-of-stream.
            g.playing = false;
            g.session->Stop();
            break;

        case MESessionClosed:
            // Close completed.
            g.playing = false;
            if (g.hCloseEvent)
				SetEvent(g.hCloseEvent);
            break;
        default:
            break;
        }

        SafeRelease(pEvent);
    }

    return handledAny ? TRUE : FALSE;
}

static HRESULT MFEnsureStartup()
{
    if (g.inited)
		return S_OK;

    HRESULT hr = S_OK;

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE)
	{
        // Already initialized.
        hr = S_OK;
    }
    if (FAILED(hr))
		return hr;

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
		return hr;

    g.hCloseEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g.hCloseEvent)
		return HRESULT_FROM_WIN32(GetLastError());

    g.inited = true;
    return S_OK;
}

static HRESULT MFCreateSession()
{
    SafeRelease(g.session);

    IMFAttributes* pAttrs = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttrs, 1);
    if (SUCCEEDED(hr))
        pAttrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    hr = MFCreateMediaSession(pAttrs, &g.session);
    SafeRelease(pAttrs);
    return hr;
}

static HRESULT MFCreateSourceFromURL(const wchar_t* url, IMFMediaSource** ppSource)
{
    if (!url || !ppSource)
		return E_INVALIDARG;

    *ppSource = nullptr;

    IMFSourceResolver* pResolver = nullptr;
    IUnknown* pUnkSource = nullptr;

    MF_OBJECT_TYPE objType = MF_OBJECT_INVALID;

    HRESULT hr = MFCreateSourceResolver(&pResolver);
    if (SUCCEEDED(hr))
	{
        hr = pResolver->CreateObjectFromURL(url,
											MF_RESOLUTION_MEDIASOURCE,
											nullptr,
											&objType,
											&pUnkSource);
    }
    if (SUCCEEDED(hr))
        hr = pUnkSource->QueryInterface(IID_PPV_ARGS(ppSource));

    SafeRelease(pUnkSource);
    SafeRelease(pResolver);
    return hr;
}

static HRESULT AddBranchForStream(
    IMFTopology* pTopology,
    IMFMediaSource* pSource,
    IMFPresentationDescriptor* pPD,
    IMFStreamDescriptor* pSD,
    HWND hwnd)
{
    BOOL fSelected = FALSE;
    HRESULT hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);

    IMFTopologyNode* pSourceNode = nullptr;
    IMFTopologyNode* pOutputNode = nullptr;

    IMFMediaTypeHandler* pHandler = nullptr;
    GUID major = GUID_NULL;

    IMFActivate* pSinkActivate = nullptr;

    // Source node
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
    if (SUCCEEDED(hr))
		hr = pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
    if (SUCCEEDED(hr))
		hr = pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
    if (SUCCEEDED(hr))
		hr = pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
    if (SUCCEEDED(hr))
		hr = pTopology->AddNode(pSourceNode);

    // Determine stream type
    if (SUCCEEDED(hr))
		hr = pSD->GetMediaTypeHandler(&pHandler);
    if (SUCCEEDED(hr))
		hr = pHandler->GetMajorType(&major);

    // Output node + renderer activate
    if (SUCCEEDED(hr))
	{
        if (major == MFMediaType_Video)
		{
            hr = MFCreateVideoRendererActivate(g.hwnd, &pSinkActivate);
        }
		else if (major == MFMediaType_Audio)
		{
            hr = MFCreateAudioRendererActivate(&pSinkActivate);
        }
        else
		{
            hr = S_FALSE;
        }
    }
    if (hr == S_FALSE) {
        SafeRelease(pHandler);
        SafeRelease(pSourceNode);
        return S_OK;
    }

    if (SUCCEEDED(hr))
		hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
    if (SUCCEEDED(hr))
		hr = pOutputNode->SetObject(pSinkActivate);
    if (SUCCEEDED(hr))
		hr = pOutputNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
    if (SUCCEEDED(hr))
		hr = pOutputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    if (SUCCEEDED(hr))
		hr = pTopology->AddNode(pOutputNode);
    if (SUCCEEDED(hr))
		hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);

    SafeRelease(pSinkActivate);
    SafeRelease(pHandler);
    SafeRelease(pOutputNode);
    SafeRelease(pSourceNode);
    return hr;
}

static HRESULT MFBuildTopology(IMFMediaSource* pSource, HWND hwnd, IMFTopology** ppTopology)
{
    if (!pSource || !ppTopology)
		return E_INVALIDARG;

    *ppTopology = nullptr;

    IMFPresentationDescriptor* pPD = nullptr;
    IMFTopology* pTopology = nullptr;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (SUCCEEDED(hr))
		hr = MFCreateTopology(&pTopology);

    if (SUCCEEDED(hr))
	{
        DWORD streamCount = 0;
        hr = pPD->GetStreamDescriptorCount(&streamCount);

        for (DWORD i = 0; SUCCEEDED(hr) && i < streamCount; ++i) {
            BOOL fSelected = FALSE;
            IMFStreamDescriptor* pSD = nullptr;
            hr = pPD->GetStreamDescriptorByIndex(i, &fSelected, &pSD);
            if (SUCCEEDED(hr) && fSelected)
                hr = AddBranchForStream(pTopology, pSource, pPD, pSD, hwnd);
            SafeRelease(pSD);
        }
    }

    if (SUCCEEDED(hr))
	{
        *ppTopology = pTopology;
        (*ppTopology)->AddRef();
    }

    SafeRelease(pTopology);
    SafeRelease(pPD);
    return hr;
}

static HRESULT MFApplyVideoRect()
{
    IMFVideoDisplayControl* vdc = nullptr;

    HRESULT hr = MFGetService(g.session,
							  MR_VIDEO_RENDER_SERVICE,
							  IID_PPV_ARGS(&vdc));
    if (FAILED(hr))
		return hr;

    vdc->SetVideoWindow(g.hwnd);
    vdc->SetAspectRatioMode(MFVideoARMode_PreservePicture);
    hr = vdc->SetVideoPosition(nullptr, &g.dstRect);
    vdc->Release();
    return hr;
}

static void MFCleanup(bool keepMFStartup)
{
    SafeRelease(g.videoDisplay);

    if (g.source)
        g.source->Shutdown();
    SafeRelease(g.source);

    if (g.session)
	{
        // If not closed yet, try to close
        g.session->Shutdown();
    }
    SafeRelease(g.session);

    SafeRelease(g.topology);

    g.playing = false;
    g.closing = false;
    g.hwnd = nullptr;
    g.dstRect = { 0,0,0,0 };

    if (!keepMFStartup)
	{
        if (g.hCloseEvent)
		{
            CloseHandle(g.hCloseEvent);
            g.hCloseEvent = nullptr;
        }
        if (g.inited)
		{
            MFShutdown();
            CoUninitialize();
            g.inited = false;
        }
    }
}

static HRESULT AnsiToWideACP(const char* s, wchar_t** outWide)
{
    if (!s || !outWide)
	    return E_INVALIDARG;

    *outWide = nullptr;

    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0)
	    return HRESULT_FROM_WIN32(GetLastError());

    wchar_t* w = (wchar_t*)CoTaskMemAlloc(sizeof(wchar_t) * (size_t)len);
    if (!w)
	    return E_OUTOFMEMORY;

    if (!MultiByteToWideChar(CP_ACP, 0, s, -1, w, len))
    {
        CoTaskMemFree(w);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *outWide = w;
    return S_OK;
}
