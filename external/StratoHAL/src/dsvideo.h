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

#ifndef STRATOHAL_DSVIDEO_H
#define STRATOHAL_DSVIDEO_H

#include "stratohal/platform.h"

#include <windows.h>

#define WM_GRAPHNOTIFY	(WM_APP + 13)

BOOL DShowPlayVideo(HWND hWnd, const char *pszFileName, int nOfsX, int nOfsY, int nWidth, int nHeight);
VOID DShowStopVideo(VOID);
BOOL DShowIsVideoPlaying(VOID);
BOOL DShowProcessEvents(VOID);

#endif
