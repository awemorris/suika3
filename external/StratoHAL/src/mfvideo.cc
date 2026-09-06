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

#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <evr.h>
#include <stdio.h>

extern "C" {
#include "mfvideo.h"
};

//
// Dynamic API Loading
//

typedef HRESULT (WINAPI *PFN_MFStartup)(ULONG Version, DWORD dwFlags);
typedef HRESULT (WINAPI *PFN_MFShutdown)(void);
typedef HRESULT (WINAPI *PFN_MFCreateAttributes)(IMFAttributes **ppMFAttributes, UINT32 cInitialSize);
typedef HRESULT (WINAPI *PFN_MFCreateMediaSession)(IMFAttributes *pConfiguration, IMFMediaSession **ppMediaSession);
typedef HRESULT (WINAPI *PFN_MFCreateSourceResolver)(IMFSourceResolver **ppISourceResolver);
typedef HRESULT (WINAPI *PFN_MFCreateTopology)(IMFTopology **ppTopo);
typedef HRESULT (WINAPI *PFN_MFCreateTopologyNode)(MF_TOPOLOGY_TYPE NodeType, IMFTopologyNode **ppNode);
typedef HRESULT (WINAPI *PFN_MFCreateVideoRendererActivate)(HWND hwndVideo, IMFActivate **ppActivate);
typedef HRESULT (WINAPI *PFN_MFCreateAudioRendererActivate)(IMFActivate **ppActivate);
typedef HRESULT (WINAPI *PFN_MFGetService)(IUnknown *punkObject, REFGUID guidService, REFIID riid, LPVOID *ppvObject);

struct api {
    HMODULE h_mfplat;
    HMODULE h_mf;
    HMODULE h_evr;

    PFN_MFStartup                     MFStartup;
    PFN_MFCreateAttributes            MFCreateAttributes;
    PFN_MFCreateMediaSession          MFCreateMediaSession;
    PFN_MFCreateSourceResolver        MFCreateSourceResolver;
    PFN_MFCreateTopology              MFCreateTopology;
    PFN_MFCreateTopologyNode          MFCreateTopologyNode;
    PFN_MFCreateVideoRendererActivate MFCreateVideoRendererActivate;
    PFN_MFCreateAudioRendererActivate MFCreateAudioRendererActivate;
    PFN_MFGetService                  MFGetService;
    PFN_MFShutdown                    MFShutdown;
} api;

static const GUID MY_GUID_NULL = {0};

//
// Safe Release
//

template<class T>
static void SafeRelease(T*& p)
{
	if (p)
	{
		p->Release();
		p = nullptr;
	}
}

//
// Player State
//
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
static HRESULT EnsureStartup();
static HRESULT CreateSession();
static HRESULT CreateSourceFromURL(const wchar_t* url, IMFMediaSource** ppSource);
static HRESULT BuildTopology(IMFMediaSource* pSource, HWND hwnd, IMFTopology** ppTopology);
static HRESULT AcquireVideoDisplay();
static HRESULT ApplyVideoRect();
static void DetachVideoWindow();
static VOID Cleanup(bool keepMFStartup);
static HRESULT AnsiToWideACP(const char* s, wchar_t** outWide);

//
// Initialize.
//
BOOL MFVInit(VOID)
{
    ZeroMemory(&api, sizeof(api));

    api.h_mfplat = LoadLibraryA("mfplat.dll");
    api.h_mf     = LoadLibraryA("mf.dll");
    api.h_evr    = LoadLibraryA("evr.dll");
    if (!api.h_mfplat || !api.h_mf || !api.h_evr)
	{
		hal_log_error("err0");
        return FALSE;
	}

    api.MFStartup = (PFN_MFStartup)GetProcAddress(api.h_mfplat, "MFStartup");
    api.MFCreateAttributes = (PFN_MFCreateAttributes)GetProcAddress(api.h_mfplat, "MFCreateAttributes");
    api.MFCreateMediaSession = (PFN_MFCreateMediaSession)GetProcAddress(api.h_mf, "MFCreateMediaSession");
    api.MFCreateSourceResolver = (PFN_MFCreateSourceResolver)GetProcAddress(api.h_mf, "MFCreateSourceResolver");
    api.MFCreateTopology = (PFN_MFCreateTopology)GetProcAddress(api.h_mf, "MFCreateTopology");
    api.MFCreateTopologyNode = (PFN_MFCreateTopologyNode)GetProcAddress(api.h_mf, "MFCreateTopologyNode");
    api.MFCreateVideoRendererActivate = (PFN_MFCreateVideoRendererActivate)GetProcAddress(api.h_mf, "MFCreateVideoRendererActivate");
    api.MFCreateAudioRendererActivate = (PFN_MFCreateAudioRendererActivate)GetProcAddress(api.h_mf, "MFCreateAudioRendererActivate");
    api.MFGetService = (PFN_MFGetService)GetProcAddress(api.h_mf, "MFGetService");
    api.MFShutdown = (PFN_MFShutdown)GetProcAddress(api.h_mfplat, "MFShutdown");
    if (api.MFStartup == NULL)
	{
		hal_log_error("err1-1");
	}
	if (api.MFCreateAttributes == NULL)
	{
		hal_log_error("err1-2");
        return FALSE;
	}
	if (api.MFCreateMediaSession == NULL)
	{
		hal_log_error("err1-3");
        return FALSE;
	}
	if (api.MFCreateSourceResolver == NULL)
	{
		hal_log_error("err1-4");
        return FALSE;
	}
	if (api.MFCreateTopology == NULL)
	{
		hal_log_error("err1-5");
        return FALSE;
	}
    if (api.MFCreateTopologyNode == NULL)
	{
		hal_log_error("err1-6");
        return FALSE;
	}
	if (api.MFCreateVideoRendererActivate == NULL)
	{
		hal_log_error("err1-7");
        return FALSE;
	}
    if (api.MFCreateAudioRendererActivate == NULL)
	{
		hal_log_error("err1-8");
        return FALSE;
	}
    if (api.MFGetService == NULL)
	{
		hal_log_error("err1-9");
        return FALSE;
	}
    if (api.MFShutdown == NULL)
	{
		hal_log_error("err1-10");
        return FALSE;
	}

    HRESULT hr = EnsureStartup();
    if (FAILED(hr))
	{
		hal_log_error("err2");
        return FALSE;
	}

    return TRUE;
}

BOOL MFVPlayVideo(HWND hWnd, const char* pszFileName, int nOfsX, int nOfsY, int nWidth, int nHeight)
{
    if (!hWnd || !pszFileName || nWidth <= 0 || nHeight <= 0)
        return FALSE;

    HRESULT hr = S_OK;

    // Stop previous playback if any.
    MFVStopVideo();

    hr = EnsureStartup();
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

    hr = CreateSession();
    if (SUCCEEDED(hr))
		hr = CreateSourceFromURL(wfile, &g.source);
    if (SUCCEEDED(hr))
		hr = BuildTopology(g.source, hWnd, &g.topology);
    if (SUCCEEDED(hr))
		hr = g.session->SetTopology(0, g.topology);

    CoTaskMemFree(wfile);

    if (FAILED(hr))
	{
        Cleanup(false);
        return FALSE;
    }

	if (g.hCloseEvent)
		ResetEvent(g.hCloseEvent);

    // Start: session will actually start after MESessionTopologyStatus(READY) etc.
    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    hr = g.session->Start(&MY_GUID_NULL, &varStart);
    PropVariantClear(&varStart);
    if (FAILED(hr))
	{
        Cleanup(false);
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

	for (int i = 0; i < 20; i++)
	{
		MFVProcessEvents();
		Sleep(1);
	}

    return TRUE;
}

VOID MFVStopVideo(VOID)
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
                MFVProcessEvents();
                DWORD w = WaitForSingleObject(g.hCloseEvent, 5);
                if (w == WAIT_OBJECT_0)
					break;
            }
        }
    }

    Cleanup(true);
}

BOOL MFVIsVideoPlaying(VOID)
{
    return (g.playing || g.closing) ? TRUE : FALSE;
}

BOOL MFVProcessEvents(VOID)
{
    if (!g.session)
        return FALSE;

    bool handledAny = false;
    bool needCleanup = false;
    HWND hwndToRepaint = nullptr;

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
            g.playing = false;

        switch (met) {
        case MESessionTopologyStatus:
        {
            UINT32 topoStatus = 0;
            pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &topoStatus);
            if (topoStatus == MF_TOPOSTATUS_READY)
                ApplyVideoRect();
            break;
        }
        case MESessionStarted:
		{
            ApplyVideoRect();
            break;
		}
        case MESessionEnded:
		{
            g.playing = false;
            g.closing = true;
            if (g.session)
                g.session->Close();
            break;
		}
        case MESessionClosed:
		{
            g.playing = false;
            g.closing = false;
            hwndToRepaint = g.hwnd;
            needCleanup = true;
            if (g.hCloseEvent)
                SetEvent(g.hCloseEvent);
            break;
		}
        default:
            break;
        }

        SafeRelease(pEvent);

        if (needCleanup)
            break;
    }

    if (needCleanup)
	{
        Cleanup(true);
        if (hwndToRepaint)
		{
            InvalidateRect(hwndToRepaint, NULL, TRUE);
            UpdateWindow(hwndToRepaint);
        }
    }

    return handledAny ? TRUE : FALSE;
}

static HRESULT EnsureStartup()
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

    hr = api.MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr))
		return hr;

    g.hCloseEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g.hCloseEvent)
		return HRESULT_FROM_WIN32(GetLastError());

    g.inited = true;
    return S_OK;
}

static HRESULT CreateSession()
{
    SafeRelease(g.session);

    IMFAttributes* pAttrs = nullptr;
    HRESULT hr = api.MFCreateAttributes(&pAttrs, 1);
    if (SUCCEEDED(hr))
        pAttrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    hr = api.MFCreateMediaSession(pAttrs, &g.session);
    SafeRelease(pAttrs);
    return hr;
}

static HRESULT CreateSourceFromURL(const wchar_t* url, IMFMediaSource** ppSource)
{
    if (!url || !ppSource)
		return E_INVALIDARG;

    *ppSource = nullptr;

    IMFSourceResolver* pResolver = nullptr;
    IUnknown* pUnkSource = nullptr;

    MF_OBJECT_TYPE objType = MF_OBJECT_INVALID;

    HRESULT hr = api.MFCreateSourceResolver(&pResolver);
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
    //BOOL fSelected = FALSE;
    //HRESULT hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);

    if (!pTopology || !pSource || !pPD || !pSD)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    IMFTopologyNode* pSourceNode = nullptr;
    IMFTopologyNode* pOutputNode = nullptr;

    IMFMediaTypeHandler* pHandler = nullptr;
    GUID major = MY_GUID_NULL;

    IMFActivate* pSinkActivate = nullptr;

    // Source node
    hr = api.MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
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
            hr = api.MFCreateVideoRendererActivate(hwnd, &pSinkActivate);
        }
		else if (major == MFMediaType_Audio)
		{
            hr = api.MFCreateAudioRendererActivate(&pSinkActivate);
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
		hr = api.MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
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

static HRESULT BuildTopology(IMFMediaSource* pSource, HWND hwnd, IMFTopology** ppTopology)
{
    if (!pSource || !ppTopology)
		return E_INVALIDARG;

    *ppTopology = nullptr;

    IMFPresentationDescriptor* pPD = nullptr;
    IMFTopology* pTopology = nullptr;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (SUCCEEDED(hr))
		hr = api.MFCreateTopology(&pTopology);

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

static HRESULT AcquireVideoDisplay()
{
    if (g.videoDisplay)
        return S_OK;

    if (!g.session)
        return E_FAIL;

    return api.MFGetService(
        g.session,
        MR_VIDEO_RENDER_SERVICE,
        IID_PPV_ARGS(&g.videoDisplay));
}

static HRESULT ApplyVideoRect()
{
    HRESULT hr = AcquireVideoDisplay();
    if (FAILED(hr))
        return hr;

    hr = g.videoDisplay->SetVideoWindow(g.hwnd);
    if (FAILED(hr))
        return hr;

    RECT rc;
    GetClientRect(g.hwnd, &rc);
    g.videoDisplay->SetVideoPosition(NULL, &rc);
    g.videoDisplay->RepaintVideo();

    hr = g.videoDisplay->SetAspectRatioMode(MFVideoARMode_PreservePicture);
    if (FAILED(hr))
        return hr;

    return g.videoDisplay->SetVideoPosition(nullptr, &g.dstRect);
}

static void DetachVideoWindow()
{
    if (g.videoDisplay) {
        RECT rc = { 0, 0, 0, 0 };
        g.videoDisplay->SetVideoPosition(nullptr, &rc);
        g.videoDisplay->SetVideoWindow(NULL);
        // g.videoDisplay->RepaintVideo(); // If needed.
    }
}

static void Cleanup(bool keepMFStartup)
{
    DetachVideoWindow();

    SafeRelease(g.videoDisplay);

    if (g.source) {
        g.source->Shutdown();
        SafeRelease(g.source);
    }
    if (g.session) {
        g.session->Shutdown();
        SafeRelease(g.session);
    }
    if (g.topology) {
        SafeRelease(g.topology);
    }

    g.playing = false;
    g.closing = false;
    g.hwnd = nullptr;

    if (!keepMFStartup) {
        if (g.hCloseEvent) {
            CloseHandle(g.hCloseEvent);
            g.hCloseEvent = nullptr;
        }
        if (g.inited) {
            api.MFShutdown();
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

    if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, w, len))
    {
        CoTaskMemFree(w);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *outWide = w;
    return S_OK;
}
