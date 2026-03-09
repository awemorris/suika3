/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Graphics HAL for Windows GDI (software rendering)
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

#include "d3drender.h"

static HWND hMainWnd;
static HDC hWndDC;
static struct hal_image *pBackImage;
static HDC hBitmapDC;
static HBITMAP hBitmap;
static int nWindowWidth;
static int nWindowHeight;

BOOL GDIInitialize(HWND hWnd, int nWidth, int nHeight)
{
	hMainWnd = hWnd;

	nWindowWidth = nWidth;
	nWindowHeight = nHeight;

	// Get a device context for the window.
	hWndDC = GetDC(hMainWnd);

	// Create a device conetxt for RGBA32 bitmap.
	BITMAPINFO bi;
	memset(&bi, 0, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = (LONG)nWidth;
	bi.bmiHeader.biHeight = (LONG)-nHeight; /* Top-down */
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	hBitmapDC = CreateCompatibleDC(NULL);
	if(hBitmapDC == NULL)
		return FALSE;

	// Create a backing bitmap.
	hal_pixel_t *pixels = NULL;
	hBitmap = CreateDIBSection(hBitmapDC, &bi, DIB_RGB_COLORS, (VOID **)&pixels, NULL, 0);
	if(hBitmap == NULL || pixels == NULL)
		return FALSE;
	SelectObject(hBitmapDC, hBitmap);

	// Create a image.
	if (!hal_create_image_with_pixels((int)nWidth, (int)nHeight, pixels, &pBackImage))
		return FALSE;

	return TRUE;
}

VOID GDICleanup(void)
{
}

BOOL GDIResizeWindow(int nOffsetX, int nOffsetY, float scale)
{
	UNUSED_PARAMETER(nOffsetX);
	UNUSED_PARAMETER(nOffsetY);
	UNUSED_PARAMETER(scale);
	return TRUE;
}

VOID GDIStartFrame(void)
{
}

VOID GDIEndFrame(void)
{
	BitBlt(hWndDC, 0, 0, nWindowWidth, nWindowHeight, hBitmapDC, 0, 0, SRCCOPY);
}

VOID GDINotifyImageUpdate(struct hal_image *img)
{
	UNUSED_PARAMETER(img);
}

VOID GDINotifyImageFree(struct hal_image *img)
{
	UNUSED_PARAMETER(img);
}

void GDIRenderImageNormal(
	int dst_left,					/* The X coordinate of the screen */
	int dst_top,					/* The Y coordinate of the screen */
	int dst_width,					/* The width of the destination rectangle */
	int dst_height,					/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] an image to be rendered */
	int src_left,					/* The X coordinate of a source image */
	int src_top,					/* The Y coordinate of a source image */
	int src_width,					/* The width of the source rectangle */
	int src_height,					/* The height of the source rectangle */
	int alpha)						/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(dst_width);
	UNUSED_PARAMETER(dst_height);

	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_alpha(pBackImage,
						 dst_left,
						 dst_top,
						 src_image,
						 src_width,
						 src_height,
						 src_left,
						 src_top,
						 alpha);
}

void GDIRenderImageAdd(
	int dst_left,					/* The X coordinate of the screen */
	int dst_top,					/* The Y coordinate of the screen */
	int dst_width,					/* The width of the destination rectangle */
	int dst_height,					/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] an image to be rendered */
	int src_left,					/* The X coordinate of a source image */
	int src_top,					/* The Y coordinate of a source image */
	int src_width,					/* The width of the source rectangle */
	int src_height,					/* The height of the source rectangle */
	int alpha)						/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(dst_width);
	UNUSED_PARAMETER(dst_height);

	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_add(pBackImage,
					   dst_left,
					   dst_top,
					   src_image,
					   src_width,
					   src_height,
					   src_left,
					   src_top,
					   alpha);
}

void GDIRenderImageSub(
	int dst_left,					/* The X coordinate of the screen */
	int dst_top,					/* The Y coordinate of the screen */
	int dst_width,					/* The width of the destination rectangle */
	int dst_height,					/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] an image to be rendered */
	int src_left,					/* The X coordinate of a source image */
	int src_top,					/* The Y coordinate of a source image */
	int src_width,					/* The width of the source rectangle */
	int src_height,					/* The height of the source rectangle */
	int alpha)						/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(dst_width);
	UNUSED_PARAMETER(dst_height);

	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_sub(pBackImage,
					   dst_left,
					   dst_top,
					   src_image,
					   src_width,
					   src_height,
					   src_left,
					   src_top,
					   alpha);
}

void GDIRenderImageDim(
	int dst_left,					/* The X coordinate of the screen */
	int dst_top,					/* The Y coordinate of the screen */
	int dst_width,					/* The width of the destination rectangle */
	int dst_height,					/* The height of the destination rectangle */
	struct hal_image *src_image,	/* [IN] an image to be rendered */
	int src_left,					/* The X coordinate of a source image */
	int src_top,					/* The Y coordinate of a source image */
	int src_width,					/* The width of the source rectangle */
	int src_height,					/* The height of the source rectangle */
	int alpha)						/* The alpha value (0 to 255) */
{
	UNUSED_PARAMETER(dst_width);
	UNUSED_PARAMETER(dst_height);

	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	hal_draw_image_dim(pBackImage,
					   dst_left,
					   dst_top,
					   src_image,
					   src_width,
					   src_height,
					   src_left,
					   src_top,
					   alpha);
}

void GDIRenderImageRule(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
	hal_draw_image_rule(pBackImage,
						src_image,
						rule_image,
						threshold);
}

void GDIRenderImageMelt(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int progress)
{
	hal_draw_image_melt(pBackImage,
						src_image,
						rule_image,
						progress);
}

VOID GDIRenderImage3DNormal(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	/* TODO */
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
}

VOID GDIRenderImage3DAdd(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	/* TODO */
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
}

VOID GDIRenderImage3DSub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	/* TODO */
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
}

VOID GDIRenderImage3DDim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	/* TODO */
	UNUSED_PARAMETER(x1);
	UNUSED_PARAMETER(y1);
	UNUSED_PARAMETER(x2);
	UNUSED_PARAMETER(y2);
	UNUSED_PARAMETER(x3);
	UNUSED_PARAMETER(y3);
	UNUSED_PARAMETER(x4);
	UNUSED_PARAMETER(y4);
	UNUSED_PARAMETER(src_image);
	UNUSED_PARAMETER(src_left);
	UNUSED_PARAMETER(src_top);
	UNUSED_PARAMETER(src_width);
	UNUSED_PARAMETER(src_height);
	UNUSED_PARAMETER(alpha);
}
