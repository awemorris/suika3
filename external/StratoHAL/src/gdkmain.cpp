/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * Playfield Engine
 * Main code for the Microsoft Game Development Kit (GDK)
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

//
// SCID (from MicrosoftGame.config)
// 
#define SCID	"PUT-YOUR-SCID-HERE"

// HAL
extern "C" {
#include "stratohal/platform.h"
#include "gdkfile.h"
#include "d3drender.h"
#include "xa2sound.h"
#include "gigamepad.h"
};

// Windows/GDK
#include <windows.h>
#include <XGameRuntime.h>
#include <XUser.h>
#include <appmodel.h>
#include <appnotify.h>

// Standard C
#include <stdio.h>
#include <assert.h>
#include <signal.h>

//
// Constants
//

// Screen size.
#define SCREEN_WIDTH			(1920)
#define SCREEN_HEIGHT			(1080)

// Buffer size for the window title.
#define TITLE_BUF_SIZE	(1024)

// Millisecond of a frame.
#define FRAME_MILLI		(16)

// Size of a log line.
#define LOG_BUF_SIZE	(4096)

// Buffer size of UTF-8/UTF-16 conversion.
#define CONV_MESSAGE_SIZE	(65536)

// Window class names.
static const wchar_t wszWindowClassMain[] = L"AppMain";

// Window title.
static wchar_t wszTitle[] = L"GDK Window";

//
// Variables
//

// GDK globals.
XUserHandle g_gdk_user = nullptr;
const char *g_gdk_scid = SCID;
XTaskQueueHandle g_queue = nullptr;

/* Windows object. */
static HWND hWndMain;

/* Frame start time. */
static LARGE_INTEGER startTime;

/* Running? */
static BOOL bRunning;

/* RunFrame() allows rendering? */
static BOOL bRunFrameAllow;

/* Viewport. */
static int nViewportOffsetX;
static int nViewportOffsetY;
static int nViewportWidth;
static int nViewportHeight;

/* Scale factor for mouse coordinates. */
static float fMouseScale;

/* Is video playing? */
static BOOL bDShowMode;

/* Is video skippable by a click? */
static BOOL bDShowSkippable;

/*
 * App suspend/resume (PLM)
 *
 * On Xbox (and on GDK titles on PC), the OS can quiesce (suspend) the title.
 * RegisterAppStateChangeNotification lets us flush state and stop doing heavy
 * work while quiesced.
 */
static PAPPSTATE_REGISTRATION g_appStateReg;
static volatile LONG g_isQuiesced;

/* Performance counter. */
static LARGE_INTEGER g_qpcFreq;

/*
 * Init Info
 */
static char *pszWindowTitle;
static int nWindowWidth;
static int nWindowHeight;

/*
 * Forward Declaration
 */

/* static */
static BOOL InitApp(HINSTANCE hInstance, int nCmdShow);
static void CleanupApp(void);
static BOOL InitTaskQueue(void);
static BOOL InitGdkUser(void);
static BOOL IsRunningPackaged();
void InitTimer(void);
static BOOL InitWindow(HINSTANCE hInstance, int nCmdShow);
static void GameLoop(void);
static BOOL RunFrame(void);
static BOOL SyncEvents(void);
static BOOL WaitForNextFrame(void);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int ConvertKeyCode(int nVK);
static void OnPaint(HWND hWnd);
static void OnCommand(WPARAM wParam, LPARAM lParam);
static void OnSizing(int edge, LPRECT lpRect);
static void OnSize(void);
static void UpdateScreenOffsetAndScale(int nClientWidth, int nClientHeight);
static VOID CALLBACK AppStateChangeRoutine(BOOLEAN Quiesced, PVOID Context);

/*
 * WinMain
 */
int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpszCmd,
	int nCmdShow)
{
	int nRet = 1;

	UNUSED_PARAMETER(hPrevInstance);
	UNUSED_PARAMETER(lpszCmd);

	if (FAILED(XGameRuntimeInitialize()))
		return 1;

	/* Main process. */
	do {
		/* Do the lower layer initialization. */
		if (!InitApp(hInstance, nCmdShow))
			break;

		/* Do a start call. */
		if (!on_event_start())
			break;

		/* Run the main loop. */
		GameLoop();

		/* Do a stop call.. */
		on_event_stop();

		/* Cleanup */
		CleanupApp();

		nRet = 0;
	} while (0);
	if (nRet != 0)
		CleanupApp();

	if (g_gdk_user)
    {
        XUserCloseHandle(g_gdk_user);
        g_gdk_user = NULL;
    }

	XGameRuntimeUninitialize();

	return nRet;
}

// Initialize the app.
static BOOL
InitApp(
	HINSTANCE hInstance,
	int nCmdShow)
{
	HRESULT hResult;

	// Initialize COM.
	hResult = CoInitialize(0);
	if (FAILED(hResult))
		return FALSE;

	// Create a window.
	if (!InitWindow(hInstance, nCmdShow))
		return FALSE;

	// Initialize the task queue.
	if (!InitTaskQueue())
		return FALSE;

	// Initialize the user.
	InitGdkUser();

	// Initialize the performance counter.
	InitTimer();

	// Register for OS suspend/resume (quiesce) notifications.
	g_appStateReg = NULL;
	InterlockedExchange(&g_isQuiesced, 0);
	int status = RegisterAppStateChangeNotification(AppStateChangeRoutine, NULL, &g_appStateReg);
	if (status != 0)
	{
		log_error("RegisterAppStateChangeNotification() failed.");
		return FALSE;
	}

	// Initialize the pakcage.
	init_file();

	// Do a boot callback.
	if (!on_event_boot(&pszWindowTitle, &nWindowWidth, &nWindowHeight))
		return FALSE;

	// Initialize the graphics HAL.
	if (!D3D12Initialize(hWndMain, nWindowWidth, nWindowHeight))
	{
		log_info(S_TR("Failed to initialize the graphics."));
		return FALSE;
	}

	// Correct the aspect ratio.
	UpdateScreenOffsetAndScale(SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize GameInput.
	GameInputInitialize();

	// Initialize the sound HAL.
	if (!XA2Initialize())
	{
	    log_error(S_TR("Failed to initialize the sound."));
	    return FALSE;
	}

	return TRUE;
}

/* Cleanup the app. */
static void
CleanupApp(void)
{
	// Cleanup the graphics HAL.
	D3D12Cleanup();

	// Cleanup GameInput.
	GameInputCleanup();
	
	// Cleanup the sound HAL.
	XA2Cleanup();
}

/* Initialize the poerformance counter. */
void
InitTimer(void)
{
    QueryPerformanceFrequency(&g_qpcFreq);
}


/* Create a window. */
static BOOL
InitWindow(
	HINSTANCE hInstance,
	int nCmdShow)
{
	WNDCLASSEXW wcex;
	DWORD dwStyle;

#if defined(TARGET_GDK_WINDOWS)
	// Enable High-DPI to avoid mismatch of the sizes of the back buffer and the window.
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

	/* Register a window class. */
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize			= sizeof(WNDCLASSEXW);
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName  = wszWindowClassMain;
	if (!RegisterClassExW(&wcex))
		return FALSE;

	/* Select window styles. */
	dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_OVERLAPPED;

	/* Create the main window. */
	hWndMain = CreateWindowExW(0, wszWindowClassMain, wszTitle, dwStyle,
							   0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
							   NULL, NULL, hInstance, NULL);
	if (hWndMain == NULL)
	{
		log_error("CreateWindowEx() failed.");
		return FALSE;
	}

	/* Show the window. */
	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);

	/* Get a frame start time. */
	QueryPerformanceCounter(&startTime);
	for(int i = 0; i < 10; i++)
		WaitForNextFrame();

	return TRUE;
}

static int
InitTaskQueue()
{
    if (g_queue) return 1;

    HRESULT hr = XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &g_queue);

    if (FAILED(hr) || !g_queue) {
        printf("XTaskQueueCreate failed hr=0x%08X\n", hr);
        return 0;
    }
    return 1;
}

/* Initialize the GDK user. */
static BOOL
InitGdkUser(void)
{
	if (!IsRunningPackaged())
		return TRUE;

    XAsyncBlock async{};
    async.queue = g_queue;

    HRESULT hr = XUserAddAsync(XUserAddOptions::AddDefaultUserAllowingUI, &async);
    printf("XUserAddAsync hr=0x%08X\n", hr);
    if (FAILED(hr))
		return FALSE;

    hr = XAsyncGetStatus(&async, true);
    printf("XAsyncGetStatus hr=0x%08X\n", hr);
    if (FAILED(hr))
		return FALSE;

    hr = XUserAddResult(&async, &g_gdk_user);
    printf("XUserAddResult hr=0x%08X user=%p\n", hr, g_gdk_user);
    if (FAILED(hr) || g_gdk_user == nullptr)
		return FALSE;

    return TRUE;
}

static BOOL
IsRunningPackaged()
{
    UINT32 len = 0;
    LONG rc = GetCurrentPackageFullName(&len, nullptr);

    if (rc == ERROR_INSUFFICIENT_BUFFER)
        return true;
    if (rc == ERROR_SUCCESS)
        return true;
    if (rc == APPMODEL_ERROR_NO_PACKAGE)
        return false;

    return false;
}

/* Run the game loop. */
static void
GameLoop(void)
{
	BOOL bBreak;

	/* Set the running flag. */
	bRunning = TRUE;

	/* Allow redraw on WM_PAINT. */
	bRunFrameAllow = TRUE;

	/* Game Loop: */
	bBreak = FALSE;
	while (!bBreak)
	{
		/* Process events. */
		if (!SyncEvents())
			break;	/* "Close" pressed. */

		/* Sleep until a next frame. */
		if (!WaitForNextFrame())
			break;	/* "Close" pressed. */

		/* Get a frame start time. */
		QueryPerformanceCounter(&startTime);

		/* Run a frame. */
		if (!RunFrame())
			bBreak = TRUE;
	}

	/* Clear the running flag. */
	bRunning = FALSE;
}

/* Run a frame. */
static BOOL
RunFrame(void)
{
	BOOL bRet;

	/* If not ready. */
	if (!bRunFrameAllow)
		return TRUE;

	/* If a video is showing. */
	if(bDShowMode)
	{
		/* Process events. */
		if(!SyncEvents())
			return FALSE;

		/* Do a frame callback. */
		if(!on_event_frame())
			return FALSE;

		return TRUE;
	}

	/* Update the gamepad input. */
	GameInputUpdate();

	/* Start rendering. */
	D3D12StartFrame();

	/* Do a frame callback. */
	bRet = TRUE;
	if(!on_event_frame())
	{
		/* Reached an exit. */
		bRet = FALSE;
		bRunFrameAllow = FALSE;
	}

	/* End rendering. */
	D3D12EndFrame();

	return bRet;
}

/* Process events on the queue. */
static BOOL
SyncEvents(void)
{
	/* DWORD dwStopWatchPauseStart; */
	MSG msg;

	/* Process events. */
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return FALSE;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return TRUE;
}

/* Sleep until a next frame. */
static BOOL
WaitForNextFrame(void)
{
	LARGE_INTEGER now;
	double elapsedMs;

	/* Process events and sleep until a next frame time. */
	while (true) {
		/* Process events if exit. */
		if(!SyncEvents())
			return FALSE;

		/* Get a lap time. */
        QueryPerformanceCounter(&now);
        elapsedMs =
            (double)(now.QuadPart - startTime.QuadPart) * 1000.0 /
            (double)g_qpcFreq.QuadPart;

		/* Break if reached a next frame time. */
		if(elapsedMs >= FRAME_MILLI) {
			startTime = now;
			break;
		}

		/* Sleep. */
		Sleep(0);
	}

	return TRUE;
}

/* Window procedure. */
static LRESULT CALLBACK
WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	int kc;

	switch(message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_LBUTTONDOWN:
		if (bRunning)
		{
			on_event_mouse_press(MOUSE_LEFT,
								 (int)((float)(LOWORD(lParam) - nViewportOffsetX) / fMouseScale),
								 (int)((float)(HIWORD(lParam) - nViewportOffsetY) / fMouseScale));
		}
		return 0;
	case WM_LBUTTONUP:
		if (bRunning)
		{
			on_event_mouse_release(MOUSE_LEFT,
								   (int)((float)(LOWORD(lParam) - nViewportOffsetX) / fMouseScale),
								   (int)((float)(HIWORD(lParam) - nViewportOffsetY) / fMouseScale));
		}
		return 0;
	case WM_RBUTTONDOWN:
		if (bRunning)
		{
			on_event_mouse_press(MOUSE_RIGHT,
								 (int)((float)(LOWORD(lParam) - nViewportOffsetX) / fMouseScale),
								 (int)((float)(HIWORD(lParam) - nViewportOffsetY) / fMouseScale));
		}
		return 0;
	case WM_RBUTTONUP:
		if (bRunning)
		{
			on_event_mouse_release(MOUSE_RIGHT,
								   (int)((float)(LOWORD(lParam) - nViewportOffsetX) / fMouseScale),
								   (int)((float)(HIWORD(lParam) - nViewportOffsetY) / fMouseScale));
		}
		return 0;
	case WM_KEYDOWN:
		/* Exclude auto repeat cases. */
		if((HIWORD(lParam) & 0x4000) != 0)
			return 0;

		if (bRunning)
		{
			kc = ConvertKeyCode((int)wParam);
			if(kc != -1)
				on_event_key_press(kc);
		}
		return 0;
	case WM_KEYUP:
		if (bRunning)
		{
			kc = ConvertKeyCode((int)wParam);
			if(kc != -1)
				on_event_key_release(kc);
		}
		return 0;
	case WM_MOUSEMOVE:
		if (bRunning)
		{
			on_event_mouse_move((int)((float)(LOWORD(lParam) - nViewportOffsetX) / fMouseScale),
								(int)((float)(HIWORD(lParam) - nViewportOffsetY) / fMouseScale));
		}
		return 0;
	case WM_MOUSEWHEEL:
		if (bRunning)
		{
			if((int)(short)HIWORD(wParam) > 0)
			{
				on_event_key_press(KEY_UP);
				on_event_key_release(KEY_UP);
			}
			else if((int)(short)HIWORD(wParam) < 0)
			{
				on_event_key_press(KEY_DOWN);
				on_event_key_release(KEY_DOWN);
			}
		}
		return 0;
	case WM_KILLFOCUS:
		if (bRunning)
		{
			on_event_key_release(KEY_CONTROL);
		}
		return 0;
	case WM_PAINT:
		OnPaint(hWnd);
		return 0;
	case WM_COMMAND:
		OnCommand(wParam, lParam);
		return 0;
	default:
		break;
	}

	/* Chain the system WndProc. */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/* Convert a key code. */
static int
ConvertKeyCode(
	int nVK)
{
	switch(nVK)
	{
	case VK_ESCAPE:
		return KEY_ESCAPE;
	case VK_RETURN:
		return KEY_RETURN;
	case VK_SPACE:
		return KEY_SPACE;
	case VK_TAB:
		return KEY_TAB;
	case VK_BACK:
		return KEY_BACKSPACE;
	case VK_DELETE:
		return KEY_DELETE;
	case VK_HOME:
		return KEY_HOME;
	case VK_END:
		return KEY_END;
	case VK_PRIOR:
		return KEY_PAGEUP;
	case VK_NEXT:
		return KEY_PAGEDOWN;
	case VK_SHIFT:
		return KEY_SHIFT;
	case VK_CONTROL:
		return KEY_CONTROL;
	case VK_MENU:
		return KEY_ALT;
	case VK_UP:
		return KEY_UP;
	case VK_DOWN:
		return KEY_DOWN;
	case VK_LEFT:
		return KEY_LEFT;
	case VK_RIGHT:
		return KEY_RIGHT;
	case 'A':
		return KEY_A;
	case 'B':
		return KEY_B;
	case 'C':
		return KEY_C;
	case 'D':
		return KEY_D;
	case 'E':
		return KEY_E;
	case 'F':
		return KEY_F;
	case 'G':
		return KEY_G;
	case 'H':
		return KEY_H;
	case 'I':
		return KEY_I;
	case 'J':
		return KEY_J;
	case 'K':
		return KEY_K;
	case 'L':
		return KEY_L;
	case 'M':
		return KEY_M;
	case 'N':
		return KEY_N;
	case 'O':
		return KEY_O;
	case 'P':
		return KEY_P;
	case 'Q':
		return KEY_Q;
	case 'R':
		return KEY_R;
	case 'S':
		return KEY_S;
	case 'T':
		return KEY_T;
	case 'U':
		return KEY_U;
	case 'V':
		return KEY_V;
	case 'W':
		return KEY_W;
	case 'X':
		return KEY_X;
	case 'Y':
		return KEY_Y;
	case 'Z':
		return KEY_Z;
	case '1':
		return KEY_1;
	case '2':
		return KEY_2;
	case '3':
		return KEY_3;
	case '4':
		return KEY_4;
	case '5':
		return KEY_5;
	case '6':
		return KEY_6;
	case '7':
		return KEY_7;
	case '8':
		return KEY_8;
	case '9':
		return KEY_9;
	case '0':
		return KEY_0;
	case VK_F1:
		return KEY_F1;
	case VK_F2:
		return KEY_F2;
	case VK_F3:
		return KEY_F3;
	case VK_F4:
		return KEY_F4;
	case VK_F5:
		return KEY_F5;
	case VK_F6:
		return KEY_F6;
	case VK_F7:
		return KEY_F7;
	case VK_F8:
		return KEY_F8;
	case VK_F9:
		return KEY_F9;
	case VK_F10:
		return KEY_F10;
	case VK_F11:
		return KEY_F11;
	case VK_F12:
		return KEY_F12;
	default:
		break;
	}
	return -1;
}

/* WM_PAINT */
static void
OnPaint(
	HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;

	hDC = BeginPaint(hWnd, &ps);
	RunFrame();
	EndPaint(hWnd, &ps);

	UNUSED_PARAMETER(hDC);
}

/* WM_COMMAND */
static void
OnCommand(
	WPARAM wParam,
	LPARAM lParam)
{
	UINT nID;

	UNUSED_PARAMETER(lParam);

	nID = LOWORD(wParam);
}

/* Calculate screen offsets and scales. */
static void
UpdateScreenOffsetAndScale(
	int nClientWidth,
	int nClientHeight)
{
	float fAspect, fUseWidth, fUseHeight;

	/* Calc the aspect ratio of the game. */
	fAspect = (float)nWindowHeight / (float)nWindowWidth;

	/* Set the height temporarily with "width-first". */
    fUseWidth = (float)nClientWidth;
    fUseHeight = fUseWidth * fAspect;
    fMouseScale = (float)nClientWidth / (float)nWindowWidth;

	/* If height is not enough, determine width with "height-first". */
    if(fUseHeight > (float)nClientHeight)
	{
        fUseHeight = (float)nClientHeight;
        fUseWidth = (float)nClientHeight / fAspect;
        fMouseScale = (float)nClientHeight / (float)nWindowHeight;
    }

	/* Calc the viewport origin. */
	nViewportOffsetX = (int)((((float)nClientWidth - fUseWidth) / 2.0f) + 0.5);
	nViewportOffsetY = (int)((((float)nClientHeight - fUseHeight) / 2.0f) + 0.5);

	/* Save the viewport size. */
	nViewportWidth = (int)fUseWidth;
	nViewportHeight = (int)fUseHeight;

	/* Update the screen offset and scale for drawing subsystem. */
	D3D12ResizeWindow(nClientWidth, nClientHeight, nViewportOffsetX, nViewportOffsetY, nViewportWidth, nViewportHeight, fMouseScale);
}

static VOID CALLBACK
AppStateChangeRoutine(
	BOOLEAN Quiesced,
	PVOID Context)
{
	UNUSED_PARAMETER(Context);
	InterlockedExchange(&g_isQuiesced, Quiesced ? 1 : 0);

	/*
	 * Keep the callback lightweight: just flip flags.
	 * Heavy work should be done on the main thread.
	 */
	if (Quiesced)
		bRunFrameAllow = FALSE;
	else
		bRunFrameAllow = TRUE;
}

/*
 * HAL
 */

/*
 * Show an INFO log.
 */
bool
hal_log_info(
	const char *s,
	...)
{
	va_list ap;

	va_start(ap, s);
	printf(s, ap);
	va_end(ap);

	return true;
}

/*
 * Show a WARN log.
 */
bool
hal_log_warn(
	const char *s,
	...)
{
	va_list ap;

	va_start(ap, s);
	printf(s, ap);
	va_end(ap);

	return true;
}

/*
 * Show an ERROR log.
 */
bool
hal_log_error(
	const char *s,
	...)
{
	va_list ap;

	va_start(ap, s);
	printf(s, ap);
	va_end(ap);

	return true;
}

/*
 * Show an Out-of-memory error.
 */
bool
hal_log_out_of_memory(void)
{
	log_error(S_TR("Out of memory."));
	return true;
}

/*
 * Convert a utf-8 string to a utf-17 string.
 */
const wchar_t *
win32_utf8_to_utf16(
	const char *utf8_message)
{
	static wchar_t wszMessage[CONV_MESSAGE_SIZE];

	const uint8_t *s;
	uint16_t *d;

	if (utf8_message == NULL)
		return L"";

	/* Don't use MultiByteToWideChar() as Windows 2000 doesn't support CP_UTF8. */
	s = (const uint8_t *)utf8_message;
	d = (uint16_t *)wszMessage;
	while (*s) {
		if (((*s) & 0x80) == 0) {
			/* 1-byte */
			*d++ = (uint16_t)((*s) & 0x7f);
			s += 1;
		} else if (((*s) & 0xe0) == 0xc0) {
			/* 2-byte */
			*d++ = (uint16_t)(
					  (((*(s + 0)) & 0x1f) << 6) |
					  (((*(s + 1)) & 0x3f) << 0)
				   );
			s += 2;
		} else if (((*s) & 0xf0) == 0xe0) {
			/* 3-byte */
			*d++ = (uint16_t)(
					  (((*(s + 0)) & 0x0f) << 12) |
					  (((*(s + 1)) & 0x3f) << 6) |
					  (((*(s + 2)) & 0x3f) << 0)
				   );
			s += 3;
		} else if (((*s) & 0xf8) == 0xf0) {
			/* 4-byte */
			uint32_t cp, hi, lo;
			cp = (uint32_t)(
					(((*(s + 0)) & 0x07) << 18) |
					(((*(s + 1)) & 0x3f) << 12) |
					(((*(s + 2)) & 0x3f) << 6) |
					(((*(s + 3)) & 0x3f) << 0)
				 );
			cp -= 0x10000;
			hi = 0xd800 + (uint32_t)((cp >> 10) & 0x3ff);
			lo = 0xdc00 + (uint32_t)((cp & 0x3ff));
			*d++ = (uint16_t)hi;
			*d++ = (uint16_t)lo;
			s += 4;
		} else {
			/* error */
			return L"";
		}
	}

	*d = L'\0';

	return wszMessage;
}

/*
 * Convert a utf-16 string to a utf-8 string.
 */
const char *
win32_utf16_to_utf8(
	const wchar_t *utf16_message)
{
	static char szMessage[CONV_MESSAGE_SIZE];

	const uint16_t *s;
	uint8_t *d;

	if (utf16_message == NULL)
		return "";

	/* Don't use MultiByteToWideChar() as Windows 2000 doesn't support CP_UTF8. */
	s = (const uint16_t *)utf16_message;
	d = (uint8_t *)szMessage;
	while (*s) {
		uint32_t cp;

		/* FIXME: support surrogate-pair */
		cp = *s++;

		if (cp <= 0x7f) {
			/* 1-byte */
			*d++ = cp & 0x7f;
		} else if (cp <= 0x7ff) {
			/* 2-byte */
			*d++ = (uint8_t)(0xc0 | ((cp >> 6) & 0x1f));
			*d++ = (uint8_t)(0x80 | ((cp >> 0) & 0x3f));
		} else if (cp <= 0xffff) {
			/* 3-byte */
			*d++ = (uint8_t)(0xe0 | ((cp >> 12) & 0x0f));
			*d++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3f));
			*d++ = (uint8_t)(0x80 | ((cp >> 0) & 0x3f));
		} else if (cp <= 0x1fffff) {
			/* 4-byte */
			*d++ = (uint8_t)(0xf0 | ((cp >> 18) & 0x07));
			*d++ = (uint8_t)(0x80 | ((cp >> 12) & 0x3f));
			*d++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3f));
			*d++ = (uint8_t)(0x80 | ((cp >> 0) & 0x3f));
		} else {
			/* error */
			return "";
		}
	}

	*d = '\0';

	return szMessage;
}

/*
 * Create a save directory.
 */
bool
hal_make_save_directory(void)
{
	return true;
}

/*
 * Get a real path of a file.
 */
char *
hal_make_real_path(const char *fname)
{
	return NULL;
}

/*
 * Reset a lap timer.
 */
void
hal_reset_lap_timer(
	uint64_t *origin)
{
	*origin = GetTickCount64();
}

/*
 * Get a timer lap.
 */
uint64_t
hal_get_lap_timer_millisec(
	uint64_t *origin)
{
	uint64_t cur = GetTickCount64();
	return (uint64_t)(cur - *origin);
}

/*
 * Play a video.
 */
bool
hal_play_video(
	const char *fname,
	bool is_skippable)
{
	return false;
}

/*
 * Stop a video.
 */
void
hal_stop_video(void)
{
}

/*
 * Check if a video is playing back.
 */
bool
hal_is_video_playing(void)
{
	return false;
}

/*
 * Check if the full screen mode is supported.
 */
bool
hal_is_full_screen_supported()
{
	return true;
}

/*
 * Check if in the full screen mode.
 */
bool
hal_is_full_screen_mode(void)
{
	return true;
}

/*
 * Enter the full screen mode.
 */
void
hal_enter_full_screen_mode(void)
{
}

/*
 * Leave the full screen mode.
 */
void
hal_leave_full_screen_mode(void)
{
}

/*
 * Get a system language.
 */
const char *
hal_get_system_language(void)
{
	return "en";
}

/* Not used in Windows. */
void
hal_set_continuous_swipe_enabled(
	bool is_enabled)
{
	UNUSED_PARAMETER(is_enabled);
}


void notify_image_update(struct image *img)
{
	D3D12NotifyImageUpdate(img);
}

void notify_image_free(struct image *img)
{
	D3D12NotifyImageFree(img);
}

void
render_image_normal(
	int dst_left,				/* The X coordinate of the screen */
	int dst_top,				/* The Y coordinate of the screen */
	int dst_width,				/* The width of the destination rectangle */
	int dst_height,				/* The width of the destination rectangle */
	struct image *src_image,	/* [IN] an image to be rendered */
	int src_left,				/* The X coordinate of a source image */
	int src_top,				/* The Y coordinate of a source image */
	int src_width,				/* The width of the source rectangle */
	int src_height,				/* The height of the source rectangle */
	int alpha)					/* The alpha value (0 to 255) */
{
	D3D12RenderImageNormal(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_add(
	int dst_left,				/* The X coordinate of the screen */
	int dst_top,				/* The Y coordinate of the screen */
	int dst_width,				/* The width of the destination rectangle */
	int dst_height,				/* The width of the destination rectangle */
	struct image *src_image,	/* [IN] an image to be rendered */
	int src_left,				/* The X coordinate of a source image */
	int src_top,				/* The Y coordinate of a source image */
	int src_width,				/* The width of the source rectangle */
	int src_height,				/* The height of the source rectangle */
	int alpha)					/* The alpha value (0 to 255) */
{
	D3D12RenderImageAdd(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_sub(
	int dst_left,				/* The X coordinate of the screen */
	int dst_top,				/* The Y coordinate of the screen */
	int dst_width,				/* The width of the destination rectangle */
	int dst_height,				/* The width of the destination rectangle */
	struct image *src_image,	/* [IN] an image to be rendered */
	int src_left,				/* The X coordinate of a source image */
	int src_top,				/* The Y coordinate of a source image */
	int src_width,				/* The width of the source rectangle */
	int src_height,				/* The height of the source rectangle */
	int alpha)					/* The alpha value (0 to 255) */
{
	D3D12RenderImageSub(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_dim(
	int dst_left,				/* The X coordinate of the screen */
	int dst_top,				/* The Y coordinate of the screen */
	int dst_width,				/* The width of the destination rectangle */
	int dst_height,				/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] an image to be rendered */
	int src_left,				/* The X coordinate of a source image */
	int src_top,				/* The Y coordinate of a source image */
	int src_width,				/* The width of the source rectangle */
	int src_height,				/* The height of the source rectangle */
	int alpha)					/* The alpha value (0 to 255) */
{
	D3D12RenderImageDim(dst_left, dst_top, dst_width, dst_height, src_image, src_left, src_top, src_width, src_height, alpha);
}

void render_image_rule(struct image *src_image, struct image *rule_image, int threshold)
{
	D3D12RenderImageRule(src_image, rule_image, threshold);
}

void render_image_melt(struct image *src_image, struct image *rule_image, int progress)
{
	D3D12RenderImageMelt(src_image, rule_image, progress);
}

void
render_image_3d_normal(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	D3D12RenderImage3DNormal(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_3d_add(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	D3D12RenderImage3DAdd(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_3d_sub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	D3D12RenderImage3DSub(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
}

void
render_image_3d_dim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	D3D12RenderImage3DDim(x1, y1, x2, y2, x3, y3, x4, y4, src_image, src_left, src_top, src_width, src_height, alpha);
}
