/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Graphics HAL abstraction for Windows
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

#ifndef STRATOHAL_D3DRENDER_H
#define STRATOHAL_D3DRENDER_H

#include "stratohal/platform.h"

#include <windows.h>

/* Public */
BOOL D3DInitialize(HWND hWnd, int nWidth, int nHeight, BOOL bForceD3D9);
VOID D3DCleanup(void);
BOOL D3DResizeWindow(int nScreenWidth, int nScreenHeight, int nOffsetX, int nOffsetY, int nViewportWidth, int nViewportHeight, float scale);
VOID D3DStartFrame(void);
VOID D3DEndFrame(void);
BOOL D3DRedraw(void);
BOOL D3DIsSoftRendering(void);

/* Internal */
BOOL D3D12Initialize(HWND hWnd, int nWidth, int nHeight);
BOOL D3D11Initialize(HWND hWnd, int nWidth, int nHeight);
BOOL D3D9Initialize(HWND hWnd, int nWidth, int nHeight);
BOOL GDIInitialize(HWND hWnd, int nWidth, int nHeight);
VOID D3D12Cleanup(void);
VOID D3D11Cleanup(void);
VOID D3D9Cleanup(void);
VOID GDICleanup(void);
BOOL D3D12ResizeWindow(int nScreenWidth, int nScreenHeight, int nOffsetX, int nOffsetY, int nViewportWidth, int nViewportHeight, float scale);
BOOL D3D11ResizeWindow(int nScreenWidth, int nScreenHeight, int nOffsetX, int nOffsetY, int nViewportWidth, int nViewportHeight, float scale);
BOOL D3D9ResizeWindow(int nScreenWidth, int nScreenHeight, int nOffsetX, int nOffsetY, int nViewportWidth, int nViewportHeight, float scale);
BOOL GDIResizeWindow(int nOffsetX, int nOffsetY, float scale);
VOID D3D12StartFrame(void);
VOID D3D11StartFrame(void);
VOID D3D9StartFrame(void);
VOID GDIStartFrame(void);
VOID D3D12EndFrame(void);
VOID D3D11EndFrame(void);
VOID D3D9EndFrame(void);
VOID GDIEndFrame(void);
VOID D3D12NotifyImageUpdate(struct hal_image *img);
VOID D3D11NotifyImageUpdate(struct hal_image *img);
VOID D3D9NotifyImageUpdate(struct hal_image *img);
VOID GDINotifyImageUpdate(struct hal_image *img);
VOID D3D12NotifyImageFree(struct hal_image* img);
VOID D3D11NotifyImageFree(struct hal_image* img);
VOID D3D9NotifyImageFree(struct hal_image *img);
VOID GDINotifyImageFree(struct hal_image *img);
VOID D3D12RenderImageNormal(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImageNormal(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImageNormal(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImageNormal(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageAdd(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImageAdd(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImageAdd(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImageAdd(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageSub(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImageSub(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImageSub(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImageSub(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageDim(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImageDim(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImageDim(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImageDim(int dst_left, int dst_top, int dst_width, int dst_height, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImageRule(struct hal_image* src_image, struct hal_image* rule_image, int threshold);
VOID D3D11RenderImageRule(struct hal_image *src_image, struct hal_image *rule_image, int threshold);
VOID D3D9RenderImageRule(struct hal_image *src_image, struct hal_image *rule_image, int threshold);
VOID GDIRenderImageRule(struct hal_image *src_image, struct hal_image *rule_image, int threshold);
VOID D3D12RenderImageMelt(struct hal_image* src_image, struct hal_image* rule_image, int progress);
VOID D3D11RenderImageMelt(struct hal_image *src_image, struct hal_image *rule_image, int progress);
VOID D3D9RenderImageMelt(struct hal_image *src_image, struct hal_image *rule_image, int progress);
VOID GDIRenderImageMelt(struct hal_image *src_image, struct hal_image *rule_image, int progress);
VOID D3D12RenderImage3DNormal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImage3DNormal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImage3DNormal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImage3DNormal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DAdd(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImage3DAdd(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImage3DAdd(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImage3DAdd(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DSub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImage3DSub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImage3DSub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImage3DSub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D12RenderImage3DDim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image* src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D11RenderImage3DDim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID D3D9RenderImage3DDim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);
VOID GDIRenderImage3DDim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, struct hal_image *src_image, int src_left, int src_top, int src_width, int src_height, int alpha);

#endif
