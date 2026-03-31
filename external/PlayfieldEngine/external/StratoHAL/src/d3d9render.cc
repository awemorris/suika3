/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Graphics HAL for Direct3D 9
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
#include "d3drender.h"
#include <assert.h>
};

// Direct3D 9.0
#include <d3d9.h>

//
// Pipeline types.
//
enum  {
	PIPELINE_NORMAL,
	PIPELINE_ADD,
	PIPELINE_SUB,
	PIPELINE_DIM,
	PIPELINE_RULE,
	PIPELINE_MELT,
	PIPELINE_CROSS,
};

//
// Vertex
//  - We don't use a vertex shader.
//
struct Vertex
{
	float x, y, z;	// (x, y, 0)
	float rhw;		// (1.0)
	DWORD color;	// (alpha, 1.0, 1.0, 1.0)
	float u1, v1;	// (u, v) of samplerColor
	float u2, v2;	// (u, v) of samplerRule
};

//
// Direct3D Objects
//
static LPDIRECT3D9 pD3D;
static LPDIRECT3DDEVICE9 pD3DDevice;
static IDirect3DPixelShader9 *pDimShader;
static IDirect3DPixelShader9 *pRuleShader;
static IDirect3DPixelShader9 *pMeltShader;
static IDirect3DPixelShader9 *pCrossShader;
static D3DPRESENT_PARAMETERS d3dpp;

//
// Rendering Target Window
//
static HWND hMainWnd;

//
// Screen Size
//

static int nScreenWidth;
static int nScreenHeight;

//
// Offsets and Scales
//
static float fDisplayOffsetX;
static float fDisplayOffsetY;
static float fScale;

//
// Shaders
//

// Note:
//  - We don't use a vertex shader, and use a fixed one.
//  - The "normal" and "add" pipelines use the fixed shader.
//  - We compile shaders on offline to avoid dependency of D3D9X runtime.

//
// The "dim" pipeline pixel shader.
//
const char dimShaderSrc[] =
	"ps_1_4                                                    \n"
	"                     // c0: the slot for dim factor       \n"
	"texld r0, t0         // r0 = samplerColor;                \n"
	"mul r0, r0, c0       // r0 *= c0;                         \n"
    "                     // return r0;                        \n";
static  unsigned char dimShaderBin[] {
	0x04, 0x01, 0xff, 0xff, 0x42, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0xb0,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80,
	0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xe4, 0xa0,
	0xff, 0xff, 0x00, 0x00, 
};

//
// The "rule" pipeline pixel shader.
//
const char ruleShaderSrc[] =
	"ps_1_4                                                             \n"
	"def c0, 0, 0, 0, 0  // c0: zeros                                   \n"
	"def c1, 1, 1, 1, 1  // c1: ones                                    \n"
	"                    // c2: the slot for the threshould argument    \n"
	"texld r0, t0        // r0 = samplerColor                           \n"
	"texld r1, t1        // r1 = samplerRule                            \n"
	"sub r1, r1, c2      // tmp = 1.0 - step(threshold, samplerRule);   \n"
	"cmp r2, r1, c0, c1  // ...                                         \n"
	"mov r0.a, r2.b      // samplerColor.a = tmp.b;                     \n"
    "                    // return samplerColor;                        \n";
static const unsigned char ruleShaderBin[] = {
	0x04, 0x01, 0xff, 0xff, 0x51, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
	0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f,
	0x00, 0x00, 0x80, 0x3f, 0x42, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0xb0,
	0x42, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0f, 0x80,
	0x01, 0x00, 0xe4, 0xb0, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x0f, 0x80, 0x01, 0x00, 0xe4, 0x80,
	0x02, 0x00, 0xe4, 0xa0, 0x58, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x0f, 0x80, 0x01, 0x00, 0xe4, 0x80,
	0x00, 0x00, 0xe4, 0xa0, 0x01, 0x00, 0xe4, 0xa0,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80,
	0x02, 0x00, 0xaa, 0x80, 0xff, 0xff, 0x00, 0x00,
};

//
// The "melt" pipeline pixel shader.
//
const char meltShaderSrc[] =
	"ps_1_4                                                                    \n"
	"def c0, 0, 0, 0, 0	  // c0: zeros                                         \n"
	"def c1, 1, 1, 1, 1   // c1: ones                                          \n"
	                      // c2: the slot for the threshould argument          \n"
	"texld r0, t0         // r0 = samplerColor                                 \n"
	"texld r1, t1         // r1 = samplerRule                                  \n"
	                      // tmp = (1.0 - rule) + (threshold * 2.0 - 1.0);     \n"
	"add r2, c2, c2       //   ... <<r2 = progress * 2.0>>                     \n"
	"sub r2, r2, r1       //   ... <<r2 = r2 - rule>>                          \n"
	                      // tmp = clamp(tmp);                                 \n"
	"cmp r2, r2, r2, c0   //   ... <<r2 = r2 > 0 ? r2 : 0>>                    \n"
	"sub r3, c1, r2       //   ... <<r3 = 1.0 - r3>>                           \n"
	"cmp r2, r3, r2, c1   //   ... <<r2 = r3 > 0 ? r2 : c1>>                   \n"
	"mov r0.a, r2.b       // samplerRule.a = tmp.b;                            \n"
	"                     // return samplerRule.a;                             \n";
static const unsigned char meltShaderBin[] = {
	0x04, 0x01, 0xff, 0xff, 0x51, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f, 
	0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 
	0x00, 0x00, 0x80, 0x3f, 0x42, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0xb0, 
	0x42, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0f, 0x80, 
	0x01, 0x00, 0xe4, 0xb0, 0x02, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x0f, 0x80, 0x02, 0x00, 0xe4, 0xa0, 
	0x02, 0x00, 0xe4, 0xa0, 0x03, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x0f, 0x80, 0x02, 0x00, 0xe4, 0x80, 
	0x01, 0x00, 0xe4, 0x80, 0x58, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x0f, 0x80, 0x02, 0x00, 0xe4, 0x80, 
	0x02, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xe4, 0xa0, 
	0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x0f, 0x80, 
	0x01, 0x00, 0xe4, 0xa0, 0x02, 0x00, 0xe4, 0x80, 
	0x58, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0f, 0x80, 
	0x03, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xe4, 0x80, 
	0x01, 0x00, 0xe4, 0xa0, 0x01, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x80, 0x02, 0x00, 0xaa, 0x80, 
	0xff, 0xff, 0x00, 0x00, 
};

//
// The "cross" pipeline pixel shader.
//
const char crossShaderSrc[] =
	"ps_1_4                                                                    \n"
	"def c0, 0, 0, 0, 0	  // c0: zeros                                         \n"
	"def c1, 1, 1, 1, 1   // c1: ones                                          \n"
	                      // c2: the slot for the alpha argument               \n"
	"texld r0, t0         // r0 = samplerSrc1                                  \n"
	"texld r1, t1         // r1 = samplerSrc2                                  \n"
	"sub r2, r0, r1       // r0 = c2 * (src1 - src2) + src2                    \n"
	"mad r0, c2, r2, r1   //    = c2 * src1 + (1 - c2) * src2                  \n";
static const unsigned char crossShaderBin[] = {
	0x04, 0x01, 0xff, 0xff, 0x51, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f, 
	0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 
	0x00, 0x00, 0x80, 0x3f, 0x42, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0xb0, 
	0x42, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0f, 0x80, 
	0x01, 0x00, 0xe4, 0xb0, 0x03, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0x80, 
	0x01, 0x00, 0xe4, 0x80, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0x80, 0x02, 0x00, 0xe4, 0xa0, 
	0x02, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xe4, 0x80, 
	0xff, 0xff, 0x00, 0x00, 
};

//
// Forward declaration
//
static VOID DrawPrimitives2D(int dst_left,
							 int dst_top,
							 int dst_width,
							 int dst_height,
							 struct hal_image *src1_image,
							 struct hal_image *src2_image,
							 int src1_left,
							 int src1_top,
							 int src1_width,
							 int src1_height,
							 int src2_left,
							 int src2_top,
							 int src2_width,
							 int src2_height,
							 int alpha,
							 int pipeline);
static VOID DrawPrimitives3D(float x1,
							 float y1,
							 float x2,
							 float y2,
							 float x3,
							 float y3,
							 float x4,
							 float y4,
							 struct hal_image *src1_image,
							 struct hal_image *src2_image,
							 float src1_tx1,
							 float src1_ty1,
							 float src1_tx2,
							 float src1_ty2,
							 float src1_tx3,
							 float src1_ty3,
							 float src1_tx4,
							 float src1_ty4,
							 float src2_tx1,
							 float src2_ty1,
							 float src2_tx2,
							 float src2_ty2,
							 float src2_tx3,
							 float src2_ty3,
							 float src2_tx4,
							 float src2_ty4,
							 int alpha,
							 int pipeline);
static BOOL UploadTextureIfNeeded(struct hal_image *img);
#if 1
static void CompileShader(const char *pSrc, unsigned char *pDst, BOOL bHLSL);
#endif

//
// Initialize Direct3D.
//
BOOL
D3D9Initialize(
	HWND hWnd,
	int nWidth,
	int nHeight)
{
	HRESULT hResult;

	nScreenWidth = nWidth;
	nScreenHeight = nHeight;

	hMainWnd = hWnd;
	fDisplayOffsetX = 0.0f;
	fDisplayOffsetY = 0.0f;
	fScale = 1.0f;

	// Open d3d9.dll.
	LPDIRECT3D9 (__stdcall *pDirect3DCreate9)(DWORD) = NULL;
	HMODULE hModule = LoadLibraryA("d3d9.dll");
	if (hModule == NULL)
		return FALSE;
	pDirect3DCreate9 = (LPDIRECT3D9 (__stdcall *)(DWORD))(void *)GetProcAddress(hModule, "Direct3DCreate9");
	if (pDirect3DCreate9 == NULL)
		return FALSE;

	// Create a Direct3D instance.
	pD3D = pDirect3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL)
    {
		hal_log_error("Direct3DCreate9() failed.");
        return FALSE;
    }

	// Create a Direct3D device.
	// (Windowed on boot)
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed = TRUE;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	hResult = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
								 D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp,
								 &pD3DDevice);
	if (FAILED(hResult))
	{
		pD3D->Release();
		pD3D = NULL;
		return FALSE;
	}

	// Create a shader.
	do {
		hResult = pD3DDevice->CreatePixelShader((DWORD *)dimShaderBin, &pDimShader);
		if (FAILED(hResult))
			break;

		hResult = pD3DDevice->CreatePixelShader((DWORD *)ruleShaderBin, &pRuleShader);
		if (FAILED(hResult))
			break;

		hResult = pD3DDevice->CreatePixelShader((DWORD *)meltShaderBin, &pMeltShader);
		if (FAILED(hResult))
			break;

		hResult = pD3DDevice->CreatePixelShader((DWORD *)crossShaderBin, &pCrossShader);
		if (FAILED(hResult))
			break;

	} while (0);
	if (FAILED(hResult))
	{
		hal_log_error("Direct3DDevice9::CreatePixelShader() failed.");
		pD3DDevice->Release();
		pD3DDevice = NULL;
		pD3D->Release();
		pD3D = NULL;
		return FALSE;
	}

	return TRUE;
}

//
// Cleanup Direct3D.
//
VOID
D3D9Cleanup(VOID)
{
	if (pCrossShader != NULL)
	{
		pD3DDevice->SetPixelShader(NULL);
		pCrossShader->Release();
		pCrossShader = NULL;
	}
	if (pMeltShader != NULL)
	{
		pD3DDevice->SetPixelShader(NULL);
		pMeltShader->Release();
		pMeltShader = NULL;
	}
	if (pRuleShader != NULL)
	{
		pD3DDevice->SetPixelShader(NULL);
		pRuleShader->Release();
		pRuleShader = NULL;
	}
	if (pD3DDevice != NULL)
	{
		pD3DDevice->Release();
		pD3DDevice = NULL;
	}
	if (pD3D != NULL)
	{
		pD3D->Release();
		pD3D = NULL;
	}
}

//
// Resize the window.
//
BOOL
D3D9ResizeWindow(
	int nScreenWidth,
	int nScreenHeight,
	int nOffsetX,
	int nOffsetY,
	int nViewportWidth,
	int nViewportHeight,
	float scale)
{
	fDisplayOffsetX = (float)nOffsetX;
	fDisplayOffsetY = (float)nOffsetY;
	fScale = scale;

	if (pD3DDevice != NULL)
	{
		// Reset a Direct3D device.
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dpp.BackBufferCount = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.Windowed = TRUE;
		d3dpp.hDeviceWindow = hMainWnd;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		pD3DDevice->Reset(&d3dpp);
	}

	return TRUE;
}

//
// Start a frame rendering.
//
VOID
D3D9StartFrame(VOID)
{
	// Celar.
	pD3DDevice->Clear(0,
					  NULL,
					  D3DCLEAR_TARGET,
					  D3DCOLOR_RGBA(0, 0, 0, 255),
					  0,
					  0);

	// Begin a scene.
	pD3DDevice->BeginScene();
}

//
// End a frame rendering.
//
VOID
D3D9EndFrame(VOID)
{
	// End a scene.
	pD3DDevice->EndScene();

	// Clear the top and bottom strips.
	RECT rcClient;
	GetClientRect(hMainWnd, &rcClient);
	if (fDisplayOffsetY > 0)
	{
		D3DRECT rc[2] = {
			{0, 0, rcClient.right, (LONG)fDisplayOffsetY},
			{0, (LONG)(rcClient.bottom - (int)fDisplayOffsetY), rcClient.right, rcClient.bottom}
		};			
		pD3DDevice->Clear(2, &rc[0], D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0, 0);
	}
	if (fDisplayOffsetX > 0)
	{
		D3DRECT rc[2] = {
			{0, 0, (LONG)fDisplayOffsetX, rcClient.bottom},
			{(LONG)(rcClient.right - (int)fDisplayOffsetX), 0, rcClient.right, rcClient.bottom}
		};			
		pD3DDevice->Clear(2, &rc[0], D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0, 0);
	}

	// Show.
	if(pD3DDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST)
	{
		// If a Direct3D device is lost.
		if(pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		{
			// Reset a Direct3D device.
			ZeroMemory(&d3dpp, sizeof(d3dpp));
			d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
			d3dpp.BackBufferCount = 1;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3dpp.Windowed = TRUE;
			d3dpp.hDeviceWindow = hMainWnd;
			d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
			d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			pD3DDevice->Reset(&d3dpp);
		}
	}
}

VOID
D3D9NotifyImageUpdate(
	struct hal_image *img)
{
	img->need_upload = true;
}

VOID
D3D9NotifyImageFree(
	struct hal_image *img)
{
	IDirect3DTexture9 *pTex = (IDirect3DTexture9 *)img->texture;
	if(pTex == NULL)
		return;
	pTex->Release();
	img->texture = NULL;
}

VOID
D3D9RenderImageNormal(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	DrawPrimitives2D(dst_left,
					 dst_top,
					 dst_width,
					 dst_height,
					 src_image,
					 NULL,
					 src_left,
					 src_top,
					 src_width,
					 src_height,
					 0, 0, 0, 0,
					 alpha,
					 PIPELINE_NORMAL);
}

VOID
D3D9RenderImageAdd(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	DrawPrimitives2D(dst_left,
					 dst_top,
					 dst_width,
					 dst_height,
					 src_image,
					 NULL,
					 src_left,
					 src_top,
					 src_width,
					 src_height,
					 0, 0, 0, 0,
					 alpha,
					 PIPELINE_ADD);
}

VOID
D3D9RenderImageSub(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	DrawPrimitives2D(dst_left,
					 dst_top,
					 dst_width,
					 dst_height,
					 src_image,
					 NULL,
					 src_left,
					 src_top,
					 src_width,
					 src_height,
					 0, 0, 0, 0,
					 alpha,
					 PIPELINE_SUB);
}

VOID
D3D9RenderImageDim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	DrawPrimitives2D(dst_left,
					 dst_top,
					 dst_width,
					 dst_height,
					 src_image,
					 NULL,
					 src_left,
					 src_top,
					 src_width,
					 src_height,
					 0, 0, 0, 0,
					 alpha,
					 PIPELINE_DIM);
}

VOID
D3D9RenderImageRule(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
    DrawPrimitives2D(0,
					 0,
					 nScreenWidth,
					 nScreenHeight,
					 src_image,
					 rule_image,
					 0,
					 0,
					 nScreenWidth,
					 nScreenHeight,
					 0,
					 0,
					 rule_image->width,
					 rule_image->height,
					 threshold,
					 PIPELINE_RULE);
}

VOID
D3D9RenderImageMelt(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int progress)
{
    DrawPrimitives2D(0,
					 0,
					 nScreenWidth,
					 nScreenHeight,
					 src_image,
					 rule_image,
					 0,
					 0,
					 nScreenWidth,
					 nScreenHeight,
					 0,
					 0,
					 rule_image->width,
					 rule_image->height,
					 progress,
					 PIPELINE_MELT);
}

VOID
D3D9RenderImageCross(
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	float src1_left,
	float src1_top,
	float src2_left,
	float src2_top,
	int alpha)
{
	D3D9RenderImage3DCross(
		src1_image,
		src2_image,
		src1_left,
		src1_top,
		src1_left + src1_image->width,
		src1_top,
		src1_left,
		src1_top + src1_image->height,
		src1_left + src1_image->width,
		src1_top + src1_image->height,
		src2_left,
		src2_top,
		src2_left + src1_image->width,
		src2_top,
		src2_left,
		src2_top + src1_image->height,
		src2_left + src1_image->width,
		src2_top + src1_image->height,
		alpha);
}

VOID
D3D9RenderImage3DNormal(
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
	DrawPrimitives3D(x1,
					 y1,
					 x2,
					 y2,
					 x3,
					 y3,
					 x4,
					 y4,
					 src_image,
					 NULL,
					 (float)src_left,
					 (float)src_top,
					 (float)(src_left + src_width),
					 (float)(src_top),
					 (float)(src_left),
					 (float)(src_top + src_height),
					 (float)(src_left + src_width),
					 (float)(src_top + src_height),
					 0, 0, 0, 0, 0, 0, 0, 0,
					 alpha,
					 PIPELINE_NORMAL);
}

VOID
D3D9RenderImage3DAdd(
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
	DrawPrimitives3D(x1,
					 y1,
					 x2,
					 y2,
					 x3,
					 y3,
					 x4,
					 y4,
					 src_image,
					 NULL,
					 (float)src_left,
					 (float)src_top,
					 (float)(src_left + src_width),
					 (float)(src_top),
					 (float)(src_left),
					 (float)(src_top + src_height),
					 (float)(src_left + src_width),
					 (float)(src_top + src_height),
					 0, 0, 0, 0, 0, 0, 0, 0,
					 alpha,
					 PIPELINE_ADD);
}

VOID
D3D9RenderImage3DSub(
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
	DrawPrimitives3D(x1,
					 y1,
					 x2,
					 y2,
					 x3,
					 y3,
					 x4,
					 y4,
					 src_image,
					 NULL,
					 (float)src_left,
					 (float)src_top,
					 (float)(src_left + src_width),
					 (float)(src_top),
					 (float)(src_left),
					 (float)(src_top + src_height),
					 (float)(src_left + src_width),
					 (float)(src_top + src_height),
					 0, 0, 0, 0, 0, 0, 0, 0,
					 alpha,
					 PIPELINE_SUB);
}

VOID
D3D9RenderImage3DDim(
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
	DrawPrimitives3D(x1,
					 y1,
					 x2,
					 y2,
					 x3,
					 y3,
					 x4,
					 y4,
					 src_image,
					 NULL,
					 (float)src_left,
					 (float)src_top,
					 (float)(src_left + src_width),
					 (float)(src_top),
					 (float)(src_left),
					 (float)(src_top + src_height),
					 (float)(src_left + src_width),
					 (float)(src_top + src_height),
					 0, 0, 0, 0, 0, 0, 0, 0,
					 alpha,
					 PIPELINE_DIM);
}

VOID
D3D9RenderImage3DCross(
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	float src1_x1,
	float src1_y1,
	float src1_x2,
	float src1_y2,
	float src1_x3,
	float src1_y3,
	float src1_x4,
	float src1_y4,
	float src2_x1,
	float src2_y1,
	float src2_x2,
	float src2_y2,
	float src2_x3,
	float src2_y3,
	float src2_x4,
	float src2_y4,
	int alpha)
{
    float s1_tx[4], s1_ty[4];
    float s2_tx[4], s2_ty[4];
    float screen_x[] = { 0.0f, (float)nScreenWidth, 0.0f, (float)nScreenWidth };
    float screen_y[] = { 0.0f, 0.0f, (float)nScreenHeight, (float)nScreenHeight };

	{
		float dx1 = src1_x2 - src1_x1;
		float dy1 = src1_y2 - src1_y1;
		float dx2 = src1_x3 - src1_x1;
		float dy2 = src1_y3 - src1_y1;
		float det = dx1 * dy2 - dy1 * dx2;
		if (det != 0.0f)
		{
			for (int i = 0; i < 4; i++)
			{
				float rx = screen_x[i] - src1_x1;
				float ry = screen_y[i] - src1_y1;
				float a = ( dy2 * rx - dx2 * ry) / det;
				float b = (-dy1 * rx + dx1 * ry) / det;
				s1_tx[i] = a * (float)src1_image->width;
				s1_ty[i] = b * (float)src1_image->height;
			}
		}
		else
		{
			for (int i = 0; i < 4; i++) s1_tx[i] = s1_ty[i] = 0.0f;
		}
	}

	{
		float dx1 = src2_x2 - src2_x1;
		float dy1 = src2_y2 - src2_y1;
		float dx2 = src2_x3 - src2_x1;
		float dy2 = src2_y3 - src2_y1;
		float det = dx1 * dy2 - dy1 * dx2;

		if (det != 0.0f)
		{
			for (int i = 0; i < 4; i++)
			{
				float rx = screen_x[i] - src2_x1;
				float ry = screen_y[i] - src2_y1;
				float a = ( dy2 * rx - dx2 * ry) / det;
				float b = (-dy1 * rx + dx1 * ry) / det;
				s2_tx[i] = a * (float)src2_image->width;
				s2_ty[i] = b * (float)src2_image->height;
			}
		}
		else
		{
			for (int i = 0; i < 4; i++) s2_tx[i] = s2_ty[i] = 0.0f;
		}
	}

    DrawPrimitives3D(0.0f,
					 0.0f,
					 (float)nScreenWidth,
					 0.0f,
					 0.0f,
					 (float)nScreenHeight,
					 (float)nScreenWidth,
					 (float)nScreenHeight,
					 src1_image,
					 src2_image,
					 s1_tx[0], s1_ty[0], s1_tx[1], s1_ty[1],
					 s1_tx[2], s1_ty[2], s1_tx[3], s1_ty[3],
					 s2_tx[0], s2_ty[0], s2_tx[1], s2_ty[1],
					 s2_tx[2], s2_ty[2], s2_tx[3], s2_ty[3],
					 alpha,
					 PIPELINE_CROSS);
}

static VOID
DrawPrimitives2D(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	int src1_left,
	int src1_top,
	int src1_width,
	int src1_height,
	int src2_left,
	int src2_top,
	int src2_width,
	int src2_height,
	int alpha,
	int pipeline)
{
	if (dst_width == -1)
		dst_width = src1_image->width;
	if (dst_height == -1)
		dst_height = src1_image->height;
	if (src1_width == -1)
		src1_width = src1_image->width;
	if (src1_height == -1)
		src1_height = src1_image->height;

	DrawPrimitives3D((float)dst_left,
					 (float)dst_top,
					 (float)(dst_left + dst_width - 1),
					 (float)dst_top,
					 (float)dst_left,
					 (float)(dst_top + dst_height - 1),
					 (float)(dst_left + dst_width - 1),
					 (float)(dst_top + dst_height - 1),
					 src1_image,
					 src2_image,
					 src1_left,
					 src1_top,
					 src1_left + src1_width,
					 src1_top,
					 src1_left,
					 src1_top + src1_height,
					 src1_left + src1_width,
					 src1_top + src1_height,
					 src2_left,
					 src2_top,
					 src2_left + src1_width,
					 src2_top,
					 src2_left,
					 src2_top + src1_height,
					 src2_left + src1_width,
					 src2_top + src1_height,
					 alpha,
					 pipeline);
}

static VOID
DrawPrimitives3D(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	float src1_tx1,
	float src1_ty1,
	float src1_tx2,
	float src1_ty2,
	float src1_tx3,
	float src1_ty3,
	float src1_tx4,
	float src1_ty4,
	float src2_tx1,
	float src2_ty1,
	float src2_tx2,
	float src2_ty2,
	float src2_tx3,
	float src2_ty3,
	float src2_tx4,
	float src2_ty4,
	int alpha,
	int pipeline)
{
	IDirect3DTexture9 *pTex1 = NULL;
	IDirect3DTexture9 *pTex2 = NULL;

	// Upload textures.
	if (!UploadTextureIfNeeded(src1_image))
		return;
	pTex1 = (IDirect3DTexture9 *)src1_image->texture;
	if (src2_image != NULL) {
		if (!UploadTextureIfNeeded(src2_image))
			return;
		pTex2 = (IDirect3DTexture9 *)src2_image->texture;
	}

	float img1_w = (float)src1_image->width;
	float img1_h = (float)src1_image->height;
	float img2_w = 1.0f;
	float img2_h = 1.0f;
	if (src2_image != NULL) {
		img2_w = (float)src2_image->width;
		img2_h = (float)src2_image->height;
	}

	Vertex v[4];

	// Left Top
	v[0].x = x1 * fScale + fDisplayOffsetX - 0.5f;
	v[0].y = y1 * fScale + fDisplayOffsetY - 0.5f;
	v[0].z = 0.0f;
	v[0].rhw = 1.0f;
	v[0].u1 = src1_tx1 / img1_w;
	v[0].v1 = src1_ty1 / img1_h;
	v[0].u2 = src2_tx1 / img2_w;
	v[0].v2 = src2_ty1 / img2_h;
	v[0].color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);

	// Right Top
	v[1].x = x2 * fScale + fDisplayOffsetX + 0.5f;
	v[1].y = y2 * fScale + fDisplayOffsetY - 0.5f;
	v[1].z = 0.0f;
	v[1].rhw = 1.0f;
	v[1].u1 = src1_tx2 / img1_w;
	v[1].v1 = src1_ty2 / img1_h;
	v[1].u2 = src2_tx2 / img2_w;
	v[1].v2 = src2_ty2 / img2_h;
	v[1].color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);

	// Left Bottom
	v[2].x = x3 * fScale + fDisplayOffsetX - 0.5f;
	v[2].y = y3 * fScale + fDisplayOffsetY + 0.5f;
	v[2].z = 0.0f;
	v[2].rhw = 1.0f;
	v[2].u1 = src1_tx3 / img1_w;
	v[2].v1 = src1_ty3 / img1_h;
	v[2].u2 = src2_tx3 / img2_w;
	v[2].v2 = src2_ty3 / img2_h;
	v[2].color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);

	// Right Bottom
	v[3].x = x4 * fScale + fDisplayOffsetX + 0.5f;
	v[3].y = y4 * fScale + fDisplayOffsetY + 0.5f;
	v[3].z = 0.0f;
	v[3].rhw = 1.0f;
	v[3].u1 = src1_tx4 / img1_w;
	v[3].v1 = src1_ty4 / img1_h;
	v[3].u2 = src2_tx4 / img2_w;
	v[3].v2 = src2_ty4 / img2_h;
	v[3].color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);

	FLOAT th = (float)alpha / 255.0f;
	FLOAT th4[4] = {th, th, th, th};

	switch (pipeline)
	{
	case PIPELINE_NORMAL:
		pD3DDevice->SetPixelShader(NULL);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLOROP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		break;
	case PIPELINE_ADD:
		pD3DDevice->SetPixelShader(NULL);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLOROP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		break;
	case PIPELINE_SUB:
		pD3DDevice->SetPixelShader(NULL);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_REVSUBTRACT);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLOROP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		break;
	case PIPELINE_DIM:
		pD3DDevice->SetPixelShader(pDimShader);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		th4[0] = th4[1] = th4[2] = 0.5f;
		pD3DDevice->SetPixelShaderConstantF(0, th4, 1);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLOROP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		pD3DDevice->SetTextureStageState(0,	D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		break;
	case PIPELINE_RULE:
		pD3DDevice->SetPixelShader(pRuleShader);
		pD3DDevice->SetPixelShaderConstantF(2, th4, 1);
		pD3DDevice->SetTexture(1, pTex2);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case PIPELINE_MELT:
		pD3DDevice->SetPixelShader(pMeltShader);
		pD3DDevice->SetPixelShaderConstantF(2, th4, 1);
		pD3DDevice->SetTexture(1, pTex2);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case PIPELINE_CROSS:
		pD3DDevice->SetPixelShader(pCrossShader);
		pD3DDevice->SetPixelShaderConstantF(2, th4, 1);
		pD3DDevice->SetTexture(1, pTex2);
		pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pD3DDevice->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		pD3DDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	default:
		assert(0);
		break;
	}

	// Set texture0.
	pD3DDevice->SetTexture(0, pTex1);
	pD3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX2 | D3DFVF_DIFFUSE);

	// Set a linear filter.
	pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pD3DDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pD3DDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	// Set a UV wrapping.
	pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	pD3DDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pD3DDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	// Render.
	if(x1 == x2 && x2 == x3 && x3 == x4 &&
	   y1 == y2 && y2 == y3 && y3 == y4)
	{
		pD3DDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, v, sizeof(Vertex));
	}
	else if((x1 == x2 && x2 == x3 && x3 == x4) ||
			(y1 == y2 && y2 == y3 && y3 == y4))
	{
		pD3DDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, v + 1, sizeof(Vertex));
	}
	else
	{
		pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vertex));
	}
}

// Upload a texture.
static BOOL
UploadTextureIfNeeded(
	struct hal_image *img)
{
	HRESULT hResult;

	if (!img->need_upload)
		return TRUE;

	IDirect3DTexture9 *pTex = (IDirect3DTexture9 *)img->texture;
	if (pTex == NULL)
	{
		// Create a Direct3D texture object.
		hResult = pD3DDevice->CreateTexture((UINT)img->width,
											(UINT)img->height,
											1, // mip map levels
											0, // usage
											D3DFMT_A8R8G8B8,
											D3DPOOL_MANAGED,
											&pTex,
											NULL);
		if (FAILED(hResult))
			return FALSE;

		img->texture = pTex;
	}

	// Lock a Direct3D texture rectangle.
	D3DLOCKED_RECT lockedRect;
	hResult = pTex->LockRect(0, &lockedRect, NULL, 0);
	if (FAILED(hResult))
	{
		pTex->Release();
		img->texture = NULL;
		return FALSE;
	}

	// Copy pixel data.
	memcpy(lockedRect.pBits,
		   img->pixels,
		   (UINT)img->width * (UINT)img->height * sizeof(hal_pixel_t));

	// Unlock the rectangle.
	hResult = pTex->UnlockRect(0);
	if (FAILED(hResult))
	{
		pTex->Release();
		img->texture = NULL;
		return FALSE;
	}

	// Finished an upload.
	img->need_upload = false;
	return TRUE;
}

//
// [FYI]
//  - The following is a code to compile shader (includin assembly and HLSL).
//  - You need d3dx9_43.dll to compile shaders. (use -ld3dx9)
//
#if 1
#include <d3dx9.h>
#include <stdio.h>

static void
CompileShader(
	const char *pSrc,
	unsigned char *pDst,
	BOOL bHLSL)
{
	ID3DXBuffer *pShader;
	ID3DXBuffer *pError;

	if (!bHLSL)
	{
		// For pixel shader assembly
		if (FAILED(D3DXAssembleShader(pSrc, strlen(pSrc), 0, NULL, 0,
									  &pShader, &pError)))
		{
			hal_log_error("D3DXAssembleShader");

			LPSTR pszError = (LPSTR)pError->GetBufferPointer();
			if (pszError != NULL)
				hal_log_error("%s", pszError);

			exit(1);
		}
	}
	else
	{
		// For pixel shader HLSL
		if (FAILED(D3DXCompileShader(pSrc, strlen(pSrc) - 1,
									 NULL, NULL, "blur", "ps_2_0", 0,
									 &pShader, &pError, NULL)))
		{
			hal_log_error("D3DXCompileShader");

			LPSTR pszError = (LPSTR)pError->GetBufferPointer();
			if (pszError != NULL)
				hal_log_error("%s", pszError);

			exit(1);
		}
	}

	FILE *fp;
	fp = fopen("shader.txt", "w");
	if (fp == NULL)
		exit(1);

	int size = pShader->GetBufferSize();
	unsigned char *p = (unsigned char *)pShader->GetBufferPointer();
	for (int i=0; i<size; i++) {
		pDst[i] = p[i];
		fprintf(fp, "0x%02x, ", p[i]);
		if (i % 8 == 7)
			fprintf(fp, "\n");
	}

	fclose(fp);
}
#endif
