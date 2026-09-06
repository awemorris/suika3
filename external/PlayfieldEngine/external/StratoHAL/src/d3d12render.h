/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Direct3D 12 Graphics HAL
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

#ifndef STRATOHAL_D3D12RENDER_H
#define STRATOHAL_D3D12RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <strato/strato.h>

#include <windows.h>

BOOL D3D12Initialize(HWND hWnd, int nWidth, int nHeight);
BOOL D3D12InitializeForUWP(IUnknown* pNativeWindow, int nWidth, int nHeight);
VOID D3D12Cleanup(void);
BOOL D3D12ResizeWindow(int nScreenWidth, int nScreenHeight, int nOffsetX, int nOffsetY, int nViewportWidth, int nViewportHeight, float scale);
VOID D3D12StartFrame(void);
VOID D3D12EndFrame(void);
VOID D3D12NotifyImageUpdate(struct hal_image *img);
VOID D3D12NotifyImageFree(struct hal_image* img);
VOID D3D12RenderImageNormal(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageAdd(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageSub(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageDim(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageRule(struct hal_image* src_image, struct hal_image* rule_image, int threshold);
VOID D3D12RenderImageMelt(struct hal_image* src_image, struct hal_image* rule_image, int progress);
VOID D3D12RenderImage3DNormal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DAdd(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DSub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DDim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);

#ifdef __cplusplus
};
#endif

#endif
