/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * Playfield Engine
 * Editor main
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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
#include <shlobj.h>				/* SHGetFolderPath() */
#include <commctrl.h>			/* status bar */
#include <richedit.h>			/* RichEdit */
#include <dwmapi.h>
#include "editor-resource.h"

#include <stdio.h>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED			0x02E0
#endif

/*
 * Constants
 */

/* Window title. */
#define WINDOW_TITLE			L"Playfield Editor"

/* Fonts. */
#define FONT_NAME				L"Consolas"

/* Font size */
#define FONT_SIZE				(12)

/* Colors */
#define COLOR_BG				0x002C2C2D
#define COLOR_FG				0x00FFFFFF
#define COLOR_SELECTED			0x00282828

/* The version string. */
#define VERSION					L"Playfield Editor\nCopyright (C) 2025 Awe Morris"

/* The minimum window size. */
#define DEFAULT_WINDOW_WIDTH	(800)
#define DEFAULT_WINDOW_HEIGHT	(600)

/* Timer */
#define ID_TIMER_FORMAT			(1)

/* Window class names */
static const wchar_t wszWindowClassMainWindow[] = L"OpenNovelMainWindow";

/*
 * Variables
 */

/* Windows objects */
static HWND hWndMain;				/* Main window */
static HWND hWndRichEdit;			/* RichEdit */
static HWND hWndStatus;				/* Status bar */
static HMENU hMenu;					/* Window menu */
static HMENU hMenuPopup;			/* Popup menu */
static HACCEL hAccel;				/* Keyboard accelerator */

/* File. */
static BOOL bFileLoaded;
static wchar_t wszFilePath[1024];
static wchar_t wszDirectoryPath[1024];

/* RichEdit */
static BOOL bChangeEnable;

/* Timestamp. */
static FILETIME ftTimeStamp;

/*
 * Forward Declaration
 */

/* static */
static BOOL InitWindow(HINSTANCE hInstance, int nCmdShow);
static void InitMenu(HWND hWnd);
static BOOL PretranslateMessage(MSG* pMsg);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnCommand(WPARAM wParam, LPARAM lParam);
static void OnSizing(int edge, LPRECT lpRect);
static void OnSize(void);
static void OnNotify(LPNMHDR pNmh);

/* File */
static void OpenPath(const wchar_t *pwszPath);

/* Command Handlers */
static void OnNew(void);
static void OnOpen(void);
static void OnSave(void);
static void OnRun(void);
static void OnHelp(void);

/*
 * Initialization
 */

/*
 * WinMain
 */
int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpszCmd,
	int nCmdShow)
{
	(void *)hPrevInstance;
	(void *)lpszCmd;

	/* Initialize the window. */
	if (!InitWindow(hInstance, nCmdShow))
		return FALSE;

	/* Initialize the menu. */
	InitMenu(hWndMain);

	/* Open a project file if specified in argv[1]. */
	if (__argc >= 2)
		OpenPath(__wargv[1]);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!PretranslateMessage(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

/*
 * Get the system locale.
 */
const char *get_system_locale(void)
{
	DWORD dwLang = GetUserDefaultLCID() & 0x3ff;
	switch (dwLang) {
	case LANG_ENGLISH:
		return "en";
	case LANG_FRENCH:
		return "fr";
	case LANG_GERMAN:
		return "de";
	case LANG_SPANISH:
		return "es";
	case LANG_ITALIAN:
		return "it";
	case LANG_GREEK:
		return "el";
	case LANG_RUSSIAN:
		return "ru";
	case LANG_CHINESE_SIMPLIFIED:
		return "zh";
	case LANG_CHINESE_TRADITIONAL:
		return "tw";
	case LANG_JAPANESE:
		return "ja";
	default:
		break;
	}
	return "other";
}

/* Initialize the window. */
static BOOL InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	wchar_t wszCls[1024];
	WNDCLASSEXW wcex;
	RECT rcWnd, rcClient, rcStatus;
	HFONT hFontFixed;
	int nDpi;
	int nParts[] = {100, 200, -1};
	int nStatusHeight;

	/* Register the window class. */
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)BLACK_BRUSH;
	wcex.lpszClassName  = wszWindowClassMainWindow;
	wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	if (!RegisterClassExW(&wcex))
		return FALSE;

	/* Create the main window. */
	hWndMain = CreateWindowExW(0,
							  wszWindowClassMainWindow,
							  L"Playfield Editor",
							  WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_THICKFRAME,
							  CW_USEDEFAULT,
							  CW_USEDEFAULT,
							  CW_USEDEFAULT,
							  CW_USEDEFAULT,
							  NULL,
							  NULL,
							  hInstance,
							  NULL);
	if (hWndMain == NULL)
		return FALSE;

	nDpi = GetDpiForWindow(hWndMain);
	GetWindowRect(hWndMain, &rcWnd);
	rcWnd.right = MulDiv(DEFAULT_WINDOW_WIDTH, nDpi, 96);
	rcWnd.bottom = MulDiv(DEFAULT_WINDOW_HEIGHT, nDpi, 96);
	MoveWindow(hWndMain, rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, TRUE);

	/* Create the status bar. */
	hWndStatus = CreateWindowEx(0,
								STATUSCLASSNAMEW,
								NULL,
								WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
								0, 0, 0, 0,
								hWndMain,
								(HMENU)1,
								hInstance,
								NULL);

	/* Set the status bar parts. */
	SendMessage(hWndStatus, SB_SETPARTS, 3, (LPARAM)nParts);
	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");
	SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)L"1");
	SendMessage(hWndStatus, SB_SETTEXT, 2, (LPARAM)L"Playfield Script");

	/* Create the RichEdit. */
	LoadLibraryW(L"Msftedit.dll");
	hWndRichEdit = CreateWindowExW(
		0,
		MSFTEDIT_CLASS, /* RichEdit50W */
		L"",
		ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOVSCROLL | ES_NOOLEDRAGDROP,
		rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
		hWndMain,
		(HMENU)ID_RICHEDIT,
		hInstance,
		NULL);
	if (hWndRichEdit == NULL)
	{
		hWndRichEdit = CreateWindowExW(
			0,
			RICHEDIT_CLASS, /* RichEdit30W */
			L"",
			ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOVSCROLL | ES_NOOLEDRAGDROP,
			rcClient.left, rcClient.top, rcClient.right, rcClient.bottom,
			hWndMain,
			(HMENU)ID_RICHEDIT,
			hInstance,
			NULL);
	}
	GetClassNameW(hWndRichEdit, wszCls, sizeof(wszCls) / sizeof(wchar_t));
	if (wcscmp(wszCls, L"RICHEDIT50W") == 0)
	{
		/* No auto-scroll in case of the Windows version of RichEdit, not the Office version. */
		LONG style = GetWindowLong(hWndRichEdit, GWL_STYLE);
		style &= ~ES_AUTOVSCROLL;
		SetWindowLong(hWndRichEdit, GWL_STYLE, style);
	}

	/* Remove the edge from the RichEdit. */
	DWORD dwStyle = GetWindowLongPtr(hWndRichEdit, GWL_EXSTYLE);
	SetWindowLongPtr(hWndRichEdit, GWL_EXSTYLE, dwStyle & ~WS_EX_CLIENTEDGE);

	/* Set the RichEdit styles. */
	SendMessage(hWndRichEdit, EM_SHOWSCROLLBAR, (WPARAM)SB_VERT, (LPARAM)TRUE);
	SendMessage(hWndRichEdit, EM_SETBKGNDCOLOR, (WPARAM)0, (LPARAM)COLOR_BG);
	SendMessage(hWndRichEdit, EM_SETEVENTMASK, 0, ENM_SELCHANGE | ENM_CHANGE);

	/* Set the RichEdit font. */
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
    cf.crTextColor = COLOR_FG;
	cf.yHeight = FONT_SIZE * 20;
	wcscpy(&cf.szFaceName[0], L"Cascadia Code");
    SendMessage(hWndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf);
    SendMessage(hWndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf);

	/* Show the window. */
	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);

	/* Set the dark theme. */
	LoadLibraryW(L"uxtheme.dll");
    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
	if (hUxtheme != NULL)
	{
		HRESULT (*SetWindowTheme)(HWND, LPWSTR, LPVOID) = (HRESULT (*)(HWND, LPWSTR, LPVOID))GetProcAddress(hUxtheme, "SetWindowTheme");
		DWORD (WINAPI *SetPreferredAppMode)(DWORD) = (DWORD (WINAPI *)(DWORD))GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
		void (WINAPI *AllowDarkModeForWindow)(HWND, BOOL) = (void (WINAPI *)(HWND, BOOL))GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));

		if (SetWindowTheme != NULL)
			SetWindowTheme(hWndRichEdit, L"DarkMode_Explorer", NULL);
		if (SetPreferredAppMode)
			SetPreferredAppMode(2 /* ForceDark */);
		if (AllowDarkModeForWindow)
			AllowDarkModeForWindow(hWndMain, TRUE);
	}

	/* Set the dark title bar. */
	LoadLibraryW(L"dwmapi.dll");
    HMODULE hDwmapi = GetModuleHandleW(L"dwmapi.dll");
	if (hDwmapi != NULL)
	{
		HRESULT (*DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD) =
			(HRESULT (*)(HWND, DWORD, LPCVOID, DWORD))GetProcAddress(hDwmapi, "DwmSetWindowAttribute");
		if (DwmSetWindowAttribute != NULL)
		{
			BOOL value = TRUE;
			/* DWMWA_USE_IMMERSIVE_DARK_MODE */
            DwmSetWindowAttribute(hWndMain, 20, &value, sizeof(value));
			DwmSetWindowAttribute(hWndMain, 19, &value, sizeof(value));
		}
	} else
	{
		MessageBoxW(hWndMain,
					L"Cannot load dwmapi.dll",
					L"Playfield Editor",
					MB_OK | MB_ICONEXCLAMATION);
	}

	bChangeEnable = TRUE;
	return TRUE;
}

/* Initialize the menu. */
static VOID InitMenu(HWND hWnd)
{
	HMENU hMenuFile = CreatePopupMenu();
	HMENU hMenuRun = CreatePopupMenu();
	HMENU hMenuAPI = CreatePopupMenu();
	HMENU hMenuExport = CreatePopupMenu();
	HMENU hMenuHelp = CreatePopupMenu();
    MENUITEMINFOW mi;
	UINT nOrder;

	/* Also use the menu as popup menu. */
	hMenuPopup = hMenuAPI;

	/* Create the main menu. */
	hMenu = CreateMenu();

	/* Prepare for the first level. */
	ZeroMemory(&mi, sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mi.fType = MFT_STRING;
	mi.fState = MFS_ENABLED;

	/* File */
	nOrder = 0;
	mi.hSubMenu = hMenuFile;
	mi.dwTypeData = L"File";
	InsertMenuItemW(hMenu, nOrder++, TRUE, &mi);

	/* Run */
	mi.hSubMenu = hMenuRun;
	mi.dwTypeData = L"Run";
	InsertMenuItemW(hMenu, nOrder++, TRUE, &mi);

	/* Helpを作成する */
	mi.hSubMenu = hMenuHelp;
	mi.dwTypeData = L"Help";
	InsertMenuItemW(hMenu, nOrder++, TRUE, &mi);

	/* Prepare for the second level. */
	mi.fMask = MIIM_TYPE | MIIM_ID;

	/*
	 * File
	 */

	/* New File */
	mi.wID = ID_NEW;
	mi.dwTypeData = L"New File\tCtrl+N";
	InsertMenuItemW(hMenuFile, nOrder++, TRUE, &mi);

	/* Open File */
	mi.wID = ID_OPEN;
	mi.dwTypeData = L"Open File\tCtrl+O";
	InsertMenuItemW(hMenuFile, nOrder++, TRUE, &mi);

	/* Save File */
	mi.wID = ID_SAVE;
	mi.dwTypeData = L"Save File\tCtrl+S";
	InsertMenuItemW(hMenuFile, nOrder++, TRUE, &mi);

	/* Quit */
	mi.wID = ID_QUIT;
	mi.dwTypeData = L"Quit\tCtrl+Q";
	InsertMenuItemW(hMenuFile, nOrder++, TRUE, &mi);

	/*
	 * Run
	 */

	/* Run Game */
	nOrder = 0;
	mi.wID = ID_RUN;
	mi.dwTypeData = L"Run Game\tCtrl+R";
	InsertMenuItemW(hMenuRun, nOrder++, TRUE, &mi);

	/*
	 * Help
	 */
	
	/* Version */
	nOrder = 0;
	mi.wID = ID_HELP;
	mi.dwTypeData = L"Version";
	InsertMenuItemW(hMenuHelp, nOrder++, TRUE, &mi);

	/* メインメニューをセットする */
	SetMenu(hWnd, hMenu);
}

/*
 * Translate a message before calling TranslateMessage().
 *  - TRUE: Don't process the message.
 *  - FALSE: Process the message.
 */
static BOOL PretranslateMessage(MSG *pMsg)
{
	static BOOL bShiftDown;
	static BOOL bControlDown;

	/* Clear the modifier states. */
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KILLFOCUS)
	{
		bShiftDown = FALSE;
		bControlDown = FALSE;
		return FALSE;
	}

	/* Save the shift state. */
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_SHIFT)
	{
		bShiftDown = TRUE;
		return FALSE;
	}
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KEYUP && pMsg->wParam == VK_SHIFT)
	{
		bShiftDown = FALSE;
		return FALSE;
	}

	/* Save the control state. */
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_CONTROL)
	{
		bControlDown = TRUE;
		return FALSE;
	}
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KEYUP && pMsg->wParam == VK_CONTROL)
	{
		bControlDown = FALSE;
		return FALSE;
	}

	/* Don't paste non-text content. */
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_PASTE)
	{
		if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			SendMessage(hWndRichEdit, EM_PASTESPECIAL, CF_UNICODETEXT, 0);
			return FALSE;
		}

		/* Don't send this message. */
		return TRUE;
	}

	/* Process a key down. */
	if (pMsg->hwnd == hWndRichEdit && pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_TAB:
			if (!bShiftDown && !bControlDown)
			{
				//OnTab();

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'O':
			if (bControlDown)
			{
				bControlDown = FALSE;
				PostMessage(hWndMain, WM_COMMAND, (WPARAM)ID_OPEN, 0);

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'S':
			if (bControlDown)
			{
				bControlDown = FALSE;
				PostMessage(hWndMain, WM_COMMAND, (WPARAM)ID_SAVE, 0);

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'Q':
			if (bControlDown)
			{
				bControlDown = FALSE;
				PostMessage(hWndMain, WM_COMMAND, (WPARAM)ID_QUIT, 0);

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'R':
			if (bControlDown)
			{
				bControlDown = FALSE;
				PostMessage(hWndMain, WM_COMMAND, (WPARAM)ID_RUN, 0);

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'N':
			if (bControlDown)
			{
				bControlDown = FALSE;
				PostMessage(hWndMain, WM_COMMAND, (WPARAM)ID_NEW, 0);

				/* Don't send this message. */
				return TRUE;
			}
			break;
		case 'V':
			if (bControlDown)
			{
				/* Paste only text. */
				if (IsClipboardFormatAvailable(CF_UNICODETEXT))
					return FALSE;

				/* Don't send this message. */
				return TRUE;
			}
		default:
			break;
		}
	}

	/* Send this message. */
	return FALSE;
}

/* Window Procedure */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_SIZE:
		OnSize();
		return 0;
	case WM_NOTIFY:
		OnNotify((LPNMHDR)lParam);
		return 0;
	case WM_COMMAND:
		OnCommand(wParam, lParam);
		return 0;
	case WM_DPICHANGED:
		SendMessage(hWndMain, WM_SIZE, 0, 0);
		return 0;
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

/* WM_SIZE */
static void OnSize(void)
{
	RECT rcClient, rcStatus;

	/* Get the sizes. */
	GetClientRect(hWndMain, &rcClient);
	SendMessage(hWndStatus, SB_GETRECT, (WPARAM)0, (LPARAM)&rcStatus);

	/* Move the status bar. */
	SendMessage(hWndStatus, WM_SIZE, 0, 0);

	/* Move the staus bar. */
	rcClient.bottom -= (rcStatus.bottom - rcStatus.top + 2);
	MoveWindow(hWndRichEdit, rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE);
	InvalidateRect(hWndMain, NULL, TRUE);
}

/* WM_NOTIFY */
static void OnNotify(LPNMHDR pNmh)
{
	if (pNmh->hwndFrom == hWndRichEdit && pNmh->code == EN_SELCHANGE)
	{
		SELCHANGE *sc = (SELCHANGE *)pNmh;
		int charIndex = sc->chrg.cpMin;
		int lineIndex = SendMessage(hWndRichEdit, EM_EXLINEFROMCHAR, 0, charIndex);
		int lineNumber = lineIndex + 1;
		wchar_t buf[64];

		swprintf(buf, 64, L"Line: %d", lineNumber);
		SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)buf);
		return;
	}
}

/* WM_COMMAND */
static void OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT nID;
	UINT nNotify;

	(void *)lParam;

	nID = LOWORD(wParam);
	nNotify = (WORD)(wParam >> 16) & 0xFFFF;

	if (nID == ID_RICHEDIT && nNotify == EN_CHANGE)
	{
		if (bChangeEnable)
			SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Changed");
		return;
	}

	switch(nID)
	{
	case ID_NEW:
		OnNew();
		break;
	case ID_OPEN:
		OnOpen();
		break;
	case ID_SAVE:
		OnSave();
		break;
	case ID_QUIT:
		DestroyWindow(hWndMain);
		break;
	case ID_RUN:
		OnRun();
		break;
	case ID_HELP:
		OnHelp();
		break;
	default:
		break;
	}
}

/* ID_NEW */
static void OnNew(void)
{
	bFileLoaded = FALSE;
	wcscpy(wszFilePath, L"");
	wcscpy(wszDirectoryPath, L"");

	/* Set to the RichEdit. */
	SetWindowTextW(hWndRichEdit, L"");

	/* Set the status. */
	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

	/* Set the window title. */
	SetWindowText(hWndMain, WINDOW_TITLE);
}

/* ID_OPEN */
static void OnOpen(void)
{
	static wchar_t wszPath[256];
	static wchar_t wszTitle[1024];
	OPENFILENAMEW ofn;
	BOOL bRet;
	FILE *fp;
	UINT nFileSize;
	char *pszBuf;
	wchar_t *pwszBuf;
	UINT nWideLen;

	/* Open a file dialog. */
	ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
	wcscpy(&wszPath[0], L"main.pf");
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = wszPath;
	ofn.nMaxFile = sizeof(wszPath) / sizeof(wchar_t);
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.lpstrFilter = L"Playfield Script Files\0*.pf\0\0";
	ofn.lpstrDefExt = L".pf";

	/* This will set the working directory. */
	bRet = GetOpenFileNameW(&ofn);
	if (!bRet)
		return;

	/* If no file was selected. */
	if(ofn. lpstrFile[0] == L'\0')
		return;

	/* Get the file. */
	wcscpy(wszFilePath, wszPath);

	/* Get the directory. */
	GetCurrentDirectory(sizeof(wszDirectoryPath) / sizeof(wchar_t), wszDirectoryPath);

	/* Load the file. */
	fp = _wfopen(wszPath, L"rb");
	if (fp == NULL)
	{
		MessageBox(hWndMain, L"Cannot open the file.", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* Get the file size. */
	ftell(fp);
	fseek(fp, 0, SEEK_END);
	nFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Make a buffer. */
	pszBuf = calloc(nFileSize + 1, 1);
	if (pszBuf == NULL)
	{
		fclose(fp);
		return;
	}

	/* Read from the file. */
	if (fread(pszBuf, nFileSize, 1, fp) != 1)
	{
		fclose(fp);
		free(pszBuf);
		MessageBox(hWndMain, L"Cannot read from the file.", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* Get the wide-string length. */
	nWideLen = MultiByteToWideChar(CP_UTF8, 0, pszBuf, -1, NULL, 0);

	/* Make a buffer. */
	pwszBuf = calloc((nWideLen + 1) * sizeof(wchar_t), 1);
	if (pwszBuf == NULL)
	{
		fclose(fp);
		free(pszBuf);
		return;
	}

	/* Convert. */
	MultiByteToWideChar(CP_UTF8, 0, pszBuf, -1, pwszBuf, nWideLen);

	/* Set to the RichEdit. */
	SetWindowTextW(hWndRichEdit, pwszBuf);

	fclose(fp);
	free(pszBuf);
	free(pwszBuf);

	bFileLoaded = TRUE;

	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

	swprintf(wszTitle, sizeof(wszTitle) / sizeof(wchar_t), L"%ls - %ls", WINDOW_TITLE, wszFilePath);
	SetWindowText(hWndMain, wszTitle);
}

static void OpenPath(const wchar_t *pwszPath)
{
	wchar_t *pSep;
	FILE *fp;
	UINT nFileSize, nWideLen;
	char *pszBuf;
	wchar_t *pwszBuf;
	wchar_t wszTitle[1024];

	/* Get the file. */
	wcscpy(wszFilePath, pwszPath);

	/* Get the directory. */
	pSep = wcsrchr(pwszPath, L'\\');
	if (pSep == NULL)
		return;
	*(pSep + 1) = L'\0';
	wcscpy(wszDirectoryPath, pwszPath);

	/* Load the file. */
	fp = _wfopen(wszFilePath, L"rb");
	if (fp == NULL)
	{
		MessageBox(hWndMain, L"Cannot open the file.", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* Get the file size. */
	ftell(fp);
	fseek(fp, 0, SEEK_END);
	nFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Make a buffer. */
	pszBuf = calloc(nFileSize + 1, 1);
	if (pszBuf == NULL)
	{
		fclose(fp);
		return;
	}

	/* Read from the file. */
	if (fread(pszBuf, nFileSize, 1, fp) != 1)
	{
		fclose(fp);
		free(pszBuf);
		MessageBox(hWndMain, L"Cannot read from the file.", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/* Get the wide-string length. */
	nWideLen = MultiByteToWideChar(CP_UTF8, 0, pszBuf, -1, NULL, 0);

	/* Make a buffer. */
	pwszBuf = calloc((nWideLen + 1) * sizeof(wchar_t), 1);
	if (pwszBuf == NULL)
	{
		fclose(fp);
		free(pszBuf);
		return;
	}

	/* Convert. */
	MultiByteToWideChar(CP_UTF8, 0, pszBuf, -1, pwszBuf, nWideLen);

	/* Set to the RichEdit. */
	SetWindowTextW(hWndRichEdit, pwszBuf);

	fclose(fp);
	free(pszBuf);
	free(pwszBuf);

	bFileLoaded = TRUE;

	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

	swprintf(wszTitle, sizeof(wszTitle) / sizeof(wchar_t), L"%ls - %ls", WINDOW_TITLE, wszFilePath);
	SetWindowText(hWndMain, wszTitle);
}

/* ID_SAVE */
static void OnSave(void)
{
	static wchar_t wszTitle[1024];
	GETTEXTLENGTHEX gtl;
	GETTEXTEX gt;
	UINT nChars, nBytes;
	wchar_t *pwszBuf;
	char *pszBuf;
	FILE *fp;
	int i;

	if (!bFileLoaded)
	{
		static wchar_t wszPath[1024];
		OPENFILENAMEW ofn;
		BOOL bRet;

		/* Open a file dialog. */
		ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
		wcscpy(&wszPath[0], L"main.pf");
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = wszPath;
		ofn.nMaxFile = sizeof(wszPath) / sizeof(wchar_t);
		ofn.Flags = OFN_OVERWRITEPROMPT;
		ofn.lpstrFilter = L"Playfield Script Files\0*.pf\0\0";
		ofn.lpstrDefExt = L".pf";

		/* This will set the working directory. */
		bRet = GetSaveFileNameW(&ofn);
		if (!bRet)
			return;

		/* If no file was selected. */
		if(ofn. lpstrFile[0] == L'\0')
			return;

		/* Get the file. */
		wcscpy(wszFilePath, wszPath);

		/* Get the directory. */
		GetCurrentDirectory(sizeof(wszDirectoryPath) / sizeof(wchar_t), wszDirectoryPath);

		bFileLoaded = TRUE;
	}

	/* Get the text length. */
	gtl.flags = GTL_DEFAULT;
	gtl.codepage = 1200; /* Unicode */
	nChars = SendMessage(hWndRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);

	/* Make a wide-string buffer. */
	pwszBuf = calloc((nChars + 1) * sizeof(wchar_t), 1);
	if (pwszBuf == NULL)
		return;

	/* Get the text. */
	gt.flags = GT_DEFAULT;
	gt.codepage = 1200; /* Unicode */
	gt.cb = (nChars + 1) * sizeof(wchar_t);
	SendMessage(hWndRichEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)pwszBuf);

	/* Conver CR to LF. */
	for (i = 0; i < nChars; i++)
	{
		if (pwszBuf[i] == L'\r')
			pwszBuf[i] = L'\n';
	}

	/* Get the UTF-8 size. */
	nBytes = WideCharToMultiByte(CP_UTF8, 0, pwszBuf, -1, NULL, 0, NULL, NULL);

	/* Make a multibyte-string buffer. */
	pszBuf = calloc(nBytes + 1, 1);
	if (pszBuf == NULL)
	{
		free(pwszBuf);
		return;
	}

	/* Convert to UTF-8. */
	WideCharToMultiByte(CP_UTF8, 0, pwszBuf, -1, pszBuf, nBytes + 1, NULL, NULL);

	/* Write to file. */
	fp = _wfopen(wszFilePath, L"wb");
	if (fp == NULL)
	{
		MessageBox(hWndMain, L"Cannot open the file.", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		free(pwszBuf);
		free(pszBuf);
		return;
	}
	fprintf(fp, "%s", pszBuf);
	fclose(fp);
	free(pwszBuf);
	free(pszBuf);

	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

	swprintf(wszTitle, sizeof(wszTitle) / sizeof(wchar_t), L"%ls - %ls", WINDOW_TITLE, wszFilePath);
	SetWindowText(hWndMain, wszTitle);
}

static void OnRun(void)
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	wchar_t wszPath[MAX_PATH], *pSep;

	if (wcscmp(wszDirectoryPath, L"") == 0)
		return;

	/* Get the EXE path. */
	GetModuleFileName(NULL, wszPath, MAX_PATH);
	pSep = wcsrchr(wszPath, L'\\');
	if (pSep != NULL)
		*(pSep + 1) = L'\0';
	wcscat(wszPath, L"playfield.exe");

	/* Run a process. */
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	CreateProcessW(wszPath,	/* lpApplication */
				   NULL,	/* lpCommandLine */
				   NULL,	/* lpProcessAttribute */
				   NULL,	/* lpThreadAttributes */
				   FALSE,	/* bInheritHandles */
				   NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP,
				   NULL,	/* lpEnvironment */
				   wszDirectoryPath,
				   &si,
				   &pi);
	if (pi.hProcess != NULL)
		CloseHandle(pi.hThread);
	if (pi.hProcess != NULL)
		CloseHandle(pi.hProcess);
}

/* ID_VERSION */
static void OnHelp(void)
{
	MessageBox(hWndMain, VERSION, WINDOW_TITLE, MB_OK | MB_ICONINFORMATION);
}
