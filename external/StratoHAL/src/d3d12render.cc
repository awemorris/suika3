/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Graphics HAL for Direct3D 12
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

// Base
extern "C" {
#include <stratohal/platform.h>
#include "d3drender.h"
};

// Windows
#include <windows.h>

#if defined(HAL_TARGET_GDK_XBOX)
#include <d3d12_x.h>
#include <dxgi_x.h>
#else
#include <d3d12.h>
#include <dxgi1_6.h>
#endif

#include <d3dcommon.h>
#include <directxmath.h>
#include <wrl.h>

#if !defined(HAL_TARGET_GDK_XBOX)
#include <d3dcompiler.h>
#endif

// "uuidof" extension
#if !defined(_MSC_VER)
#include "dxguids.h"
#endif

#include <algorithm>
#include <vector>

// ComPtr
using namespace Microsoft::WRL;

//
// Pipeline Types
//

enum pipeline
{
    PIPELINE_NORMAL,
    PIPELINE_ADD,
    PIPELINE_SUB,
    PIPELINE_DIM,
    PIPELINE_RULE,
    PIPELINE_MELT,
};

//
// Maximum Textures
// 

#define TEXTURE_COUNT   128

//
// Temporary Vertex Buffer Size
//  - We record triangle vertices and send them once per a frame before a command list execution.
//

#define TEMP_RECT_COUNT        128

//
// Target Frames
//

#define FRAME_COUNT        2

//
// Vertex Structure
//

struct Vertex
{
    float Pos[3];        // Screen Position
    float Tex1[2];        // Color Texture UV
    float Tex2[2];        // Rule Texture UV
    float Color[4];        // For Alpha and Dimming
};

//
// Texture Object for "struct image"
//

struct TextureBundle
{
    ComPtr<ID3D12Resource> pTexture;
    int nIndex;

    TextureBundle() : nIndex(-1) { }
};

//
// Shaders
//

const char szShader[] =
    "struct VS_INPUT                                            \n"
    "{                                                          \n"
    "    float4 Pos : POSITION;                                 \n"
    "    float2 Tex1 : TEXCOORD0;                               \n"
    "    float2 Tex2 : TEXCOORD1;                               \n"
    "    float4 Color : COLOR0;                                 \n"
    "};                                                         \n"
    "                                                           \n"
    "struct PS_INPUT                                            \n"
    "{                                                          \n"
    "    float4 Pos : SV_POSITION;                              \n"
    "    float2 Tex1 : TEXCOORD0;                               \n"
    "    float2 Tex2 : TEXCOORD1;                               \n"
    "    float4 Color : COLOR0;                                 \n"
    "};                                                         \n"
    "                                                           \n"
    "PS_INPUT VS(VS_INPUT input)                                \n"
    "{                                                          \n"
    "    PS_INPUT output = (PS_INPUT)0;                         \n"
    "    output.Pos = input.Pos;                                \n"
    "    output.Tex1 = input.Tex1;                              \n"
    "    output.Tex2 = input.Tex2;                              \n"
    "    output.Color = input.Color;                            \n"
    "    return output;                                         \n"
    "}                                                          \n"
    "                                                           \n"
    "Texture2D tx1 : register(t0);                              \n"
    "Texture2D tx2 : register(t1);                              \n"
    "SamplerState sm1 : register(s0);                           \n"
    "                                                           \n"
    "float4 PS_Normal(PS_INPUT input) : SV_Target               \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(sm1, input.Tex1);              \n"
    "    pix = pix * input.Color;                               \n"
    "    return pix;                                            \n"
    "}                                                          \n"
    "                                                           \n"
    "float4 PS_Rule(PS_INPUT input) : SV_Target                 \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(sm1, input.Tex1);              \n"
    "    float4 rule = tx2.Sample(sm1, input.Tex2);             \n"
    "    pix.a = 1.0 - step(input.Color.a, rule.b);             \n"
    "    return pix;                                            \n"
    "}                                                          \n"
    "                                                           \n"
    "float4 PS_Melt(PS_INPUT input) : SV_Target                 \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(sm1, input.Tex1);              \n"
    "    float4 rule = tx2.Sample(sm1, input.Tex2);             \n"
    "    pix.a = clamp((1.0 - rule.b) +                         \n"
    "                  (input.Color.a * 2.0 - 1.0),             \n"
    "                  0.0, 1.0);                               \n"
    "    return pix;                                            \n"
    "}                                                          \n";

const uint8_t g_VS_Bytecode[] = {
	0x44, 0x58, 0x42, 0x43, 0x97, 0xc2, 0x38, 0x6f, 0x9a, 0x35, 0xd2,
	0xb3, 0x5b, 0xaf, 0xfe, 0xa6, 0x28, 0xa0, 0x9d, 0xc0, 0x01, 0x00,
	0x00, 0x00, 0x1c, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34,
	0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00,
	0xb4, 0x01, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45,
	0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00,
	0x05, 0xfe, 0xff, 0x00, 0x01, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00,
	0x52, 0x44, 0x31, 0x31, 0x3c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d,
	0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52,
	0x29, 0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64,
	0x65, 0x72, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72,
	0x20, 0x31, 0x30, 0x2e, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4e, 0x80,
	0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f,
	0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x50, 0x4f, 0x53,
	0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4f,
	0x4f, 0x52, 0x44, 0x00, 0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x00, 0x4f,
	0x53, 0x47, 0x4e, 0x84, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x00, 0x00, 0x74, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x03, 0x00, 0x00, 0x7d,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00,
	0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f, 0x53, 0x49, 0x54, 0x49, 0x4f,
	0x4e, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4f, 0x4f, 0x52, 0x44, 0x00,
	0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x00, 0xab, 0x53, 0x48, 0x45, 0x58,
	0xc4, 0x00, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00, 0x31, 0x00, 0x00,
	0x00, 0x6a, 0x08, 0x00, 0x01, 0x5f, 0x00, 0x00, 0x03, 0xf2, 0x10,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x03, 0x32,
	0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x03,
	0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00,
	0x03, 0xf2, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x67, 0x00,
	0x00, 0x04, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0x32, 0x20, 0x10, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xc2, 0x20, 0x10,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xf2, 0x20,
	0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xf2,
	0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x1e, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x32, 0x20, 0x10,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xc2, 0x20, 0x10, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x06, 0x14, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x36, 0x00, 0x00, 0x05, 0xf2, 0x20, 0x10, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x46, 0x1e, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3e, 0x00,
	0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, };

const uint8_t g_PS_Normal_Bytecode[] = {
	0x44, 0x58, 0x42, 0x43, 0x89, 0x38, 0xae, 0x1b, 0x8f, 0x93, 0x13,
	0x0b, 0xdd, 0x95, 0x67, 0x4a, 0xcd, 0x26, 0x3a, 0xb8, 0x01, 0x00,
	0x00, 0x00, 0xec, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34,
	0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x74, 0x01, 0x00, 0x00,
	0xa8, 0x01, 0x00, 0x00, 0x50, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45,
	0x46, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00,
	0x05, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00,
	0x52, 0x44, 0x31, 0x31, 0x3c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x73, 0x6d, 0x31,
	0x00, 0x74, 0x78, 0x31, 0x00, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73,
	0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4c, 0x53,
	0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6f,
	0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2e, 0x31,
	0x00, 0x49, 0x53, 0x47, 0x4e, 0x84, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x74,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
	0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0f,
	0x0f, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f, 0x53, 0x49, 0x54,
	0x49, 0x4f, 0x4e, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4f, 0x4f, 0x52,
	0x44, 0x00, 0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x00, 0xab, 0x4f, 0x53,
	0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x54, 0x61, 0x72,
	0x67, 0x65, 0x74, 0x00, 0xab, 0xab, 0x53, 0x48, 0x45, 0x58, 0xa0,
	0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
	0x6a, 0x08, 0x00, 0x01, 0x5a, 0x00, 0x00, 0x03, 0x00, 0x60, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x18, 0x00, 0x04, 0x00, 0x70,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x62,
	0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x62, 0x10, 0x00, 0x03, 0xf2, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x65, 0x00, 0x00, 0x03, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x45,
	0x00, 0x00, 0x8b, 0xc2, 0x00, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00,
	0xf2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x7e, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,
	0x00, 0x00, 0x07, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x46, 0x0e, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x1e, 0x10,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x01, 0x53, 0x54,
	0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

const uint8_t g_PS_Rule_Bytecode[] = {
	0x44, 0x58, 0x42, 0x43, 0x38, 0x7f, 0x5b, 0x18, 0x9f, 0xc0, 0x11,
	0xae, 0x66, 0x3a, 0xf5, 0xc6, 0xff, 0xfd, 0xa0, 0x18, 0x01, 0x00,
	0x00, 0x00, 0x90, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34,
	0x00, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00,
	0xcc, 0x01, 0x00, 0x00, 0xf4, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45,
	0x46, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00,
	0x05, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00,
	0x52, 0x44, 0x31, 0x31, 0x3c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x73, 0x6d, 0x31, 0x00,
	0x74, 0x78, 0x31, 0x00, 0x74, 0x78, 0x32, 0x00, 0x4d, 0x69, 0x63,
	0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20,
	0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
	0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31,
	0x30, 0x2e, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4e, 0x84, 0x00, 0x00,
	0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
	0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x0c, 0x0c, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x0f, 0x08, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f,
	0x53, 0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00, 0x54, 0x45, 0x58, 0x43,
	0x4f, 0x4f, 0x52, 0x44, 0x00, 0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x00,
	0xab, 0x4f, 0x53, 0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5f,
	0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xab, 0xab, 0x53, 0x48,
	0x45, 0x58, 0x20, 0x01, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x48,
	0x00, 0x00, 0x00, 0x6a, 0x08, 0x00, 0x01, 0x5a, 0x00, 0x00, 0x03,
	0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x18, 0x00,
	0x04, 0x00, 0x70, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55,
	0x00, 0x00, 0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03,
	0x32, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00,
	0x03, 0xc2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10,
	0x00, 0x03, 0x82, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65,
	0x00, 0x00, 0x03, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00,
	0x8b, 0xc2, 0x00, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00, 0x12, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x1a, 0x10, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x26, 0x7d, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00,
	0x07, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x10, 0x10, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x09, 0x82, 0x20, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x45, 0x00, 0x00, 0x8b, 0xc2,
	0x00, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00, 0x72, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x46, 0x7e, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72,
	0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x02, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41,
	0x54, 0x94, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t g_PS_Melt_Bytecode[] = {
 0x44, 0x58, 0x42, 0x43, 0x81, 0x8b, 0xb7, 0x78, 0xe7, 0xad, 0x49,
	0xc6, 0x49, 0xcb, 0xc0, 0x1e, 0xfc, 0xce, 0x89, 0xc7, 0x01, 0x00,
	0x00, 0x00, 0xb0, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34,
	0x00, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00,
	0xcc, 0x01, 0x00, 0x00, 0x14, 0x03, 0x00, 0x00, 0x52, 0x44, 0x45,
	0x46, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00,
	0x05, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00,
	0x52, 0x44, 0x31, 0x31, 0x3c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x73, 0x6d, 0x31, 0x00,
	0x74, 0x78, 0x31, 0x00, 0x74, 0x78, 0x32, 0x00, 0x4d, 0x69, 0x63,
	0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20,
	0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
	0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31,
	0x30, 0x2e, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4e, 0x84, 0x00, 0x00,
	0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x68, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
	0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x0c, 0x0c, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x0f, 0x08, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f,
	0x53, 0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00, 0x54, 0x45, 0x58, 0x43,
	0x4f, 0x4f, 0x52, 0x44, 0x00, 0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x00,
	0xab, 0x4f, 0x53, 0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5f,
	0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xab, 0xab, 0x53, 0x48,
	0x45, 0x58, 0x40, 0x01, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x50,
	0x00, 0x00, 0x00, 0x6a, 0x08, 0x00, 0x01, 0x5a, 0x00, 0x00, 0x03,
	0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x18, 0x00,
	0x04, 0x00, 0x70, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55,
	0x00, 0x00, 0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03,
	0x32, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00,
	0x03, 0xc2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10,
	0x00, 0x03, 0x82, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65,
	0x00, 0x00, 0x03, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00,
	0x8b, 0xc2, 0x00, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00, 0x12, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x1a, 0x10, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x26, 0x7d, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
	0x10, 0x80, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x32, 0x00, 0x00, 0x09,
	0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x10, 0x10,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x40, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x07, 0x82, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x80, 0xbf, 0x45, 0x00, 0x00, 0x8b, 0xc2, 0x00,
	0x00, 0x80, 0x43, 0x55, 0x15, 0x00, 0x72, 0x00, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x46, 0x7e, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x72, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x02, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54,
	0x94, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//
// Variables
//

static HWND                                 g_hWnd;
static D3D12_VIEWPORT                       g_viewport;
static D3D12_RECT                           g_scissorRect;
static ComPtr<IDXGISwapChain3>              g_swapChain;
static ComPtr<ID3D12Device>                 g_device;
static ComPtr<ID3D12Resource>               g_renderTargets[FRAME_COUNT];
static ComPtr<ID3D12CommandAllocator>       g_commandAllocator;
static ComPtr<ID3D12CommandQueue>           g_commandQueue;
static ComPtr<ID3D12RootSignature>          g_rootSignature;
static ComPtr<ID3D12DescriptorHeap>         g_rtvHeap;
static ComPtr<ID3D12DescriptorHeap>         g_srvHeap;
static ComPtr<ID3D12PipelineState>          g_pipelineStateNormal;
static ComPtr<ID3D12PipelineState>          g_pipelineStateAdd;
static ComPtr<ID3D12PipelineState>          g_pipelineStateSub;
static ComPtr<ID3D12PipelineState>          g_pipelineStateRule;
static ComPtr<ID3D12PipelineState>          g_pipelineStateMelt;
static ComPtr<ID3D12GraphicsCommandList>    g_commandList;
static UINT                                 g_rtvDescriptorSize;
static UINT                                 g_srvDescriptorSize;
static ComPtr<ID3D12Resource>               g_vertexBuffer;
static D3D12_VERTEX_BUFFER_VIEW             g_vertexBufferView;
static UINT                                 g_frameIndex;
static HANDLE                               g_fenceEvent;
static ComPtr<ID3D12Fence>                  g_fence;
static UINT64                               g_fenceValue;
static ComPtr<ID3D12Resource>               g_vertexUploadHeap;
static std::vector<TextureBundle *>         g_allTextureBundleList;
static std::vector<TextureBundle *>         g_freeTextureBundleList;
static std::vector<ID3D12Resource*>         g_freeUploadHeapList;
static Vertex                               g_tempRectVertexBuffer[TEMP_RECT_COUNT * 4];
static UINT                                 g_tempRectCount;
static std::vector<int>                     g_availableTextureIndexList;
static float                                g_fDisplayWidth;
static float                                g_fDisplayHeight;
static int                                  g_nVirtualWidth;
static int                                  g_nVirtualHeight;
static float								g_fScale;
static float								g_fOffsetX;
static float								g_fOffsetY;
static int									g_contextID;

//
// API Pointers
//

HRESULT(__stdcall* pfnCreateDXGIFactory2)(UINT, REFIID, void**);
HRESULT(__stdcall* pfnD3D12CreateDevice)(
	IUnknown *,
	D3D_FEATURE_LEVEL,
	REFIID,
	void**);
HRESULT(__stdcall* pfnD3D12SerializeRootSignature)(
	const D3D12_ROOT_SIGNATURE_DESC *,
	D3D_ROOT_SIGNATURE_VERSION,
	ID3DBlob **,
	ID3DBlob **);
HRESULT(__stdcall *pfnD3DCompile)(
	LPCVOID,
	SIZE_T,
	LPCSTR,
	const D3D_SHADER_MACRO *,
	ID3DInclude *,
	LPCSTR,
	LPCSTR,
	UINT,
	UINT,
	ID3DBlob **,
	ID3DBlob **);
HRESULT(__stdcall* pfnD3DCreateBlob)(
	IUnknown *,
	D3D_FEATURE_LEVEL,
	REFIID,
	void **);

//
// Forward Declarations
//

static BOOL GetAPIPointers();
static BOOL GetScreenSize();
static BOOL RecreateD3DObjects();
static BOOL CreateSwapchain();
static void GetHardwareAdapter(IDXGIFactory1* pFactory,
							   IDXGIAdapter1** ppAdapter,
							   bool requestHighPerformanceAdapter);
static BOOL CreateSwapchain();
static BOOL CreateDescriptorHeap();
static BOOL CreateFrameResources();
static BOOL CreateCommandAllocator();
static BOOL CreateRootSignature();
static BOOL CreatePipelineState();
static HRESULT CompileShaderFromString(const char* szShader,
									   LPCSTR szEntryPoint,
									   LPCSTR szShaderModel,
									   uint8_t** ppShaderData,
									   size_t* pShaderSize);
static BOOL CreateCommandList();
static BOOL CreateVertexBuffer();
static BOOL CreateFence();
static void ReleaseAllD3D12Objects();
static void WaitForPreviousFrame();
static VOID DrawPrimitive2D(int dst_left, int dst_top, int dst_width,
							int dst_height, struct hal_image *src_image,
							struct hal_image *rule_image, int src_left,
							int src_top, int src_width, int src_height,
							int alpha, int pipeline);
static VOID DrawPrimitive3D(float x1, float y1, float x2, float y2, float x3,
							float y3, float x4, float y4,
							struct hal_image *src_image,
							struct hal_image *rule_image,
							int src_left, int src_top, int src_width,
							int src_height, int alpha, int pipeline);
static BOOL UploadTextureIfNeeded(struct hal_image *img);
static UINT64 GetRequiredIntermediateSize_NoD3DX(
	ID3D12Resource *destinationResource,
	UINT firstSubresource,
	UINT numSubresources);
static UINT64 UpdateSubresources_NoD3DX(
	ID3D12GraphicsCommandList *pCmdList,
	ID3D12Resource *pDestinationResource,
	ID3D12Resource *pIntermediate,
	UINT64 IntermediateOffset,
	UINT FirstSubresource,
	UINT NumSubresources,
	const D3D12_SUBRESOURCE_DATA *pSrcData);
static void MemcpySubresource_NoD3DX(
	const D3D12_MEMCPY_DEST *pDest,
	const D3D12_SUBRESOURCE_DATA *pSrc,
	SIZE_T rowSizeInBytes,
	UINT numRows,
	UINT numSlices);

BOOL
D3D12Initialize(
	HWND hWnd,
	int nWidth,
	int nHeight)
{
    g_hWnd = hWnd;
    g_nVirtualWidth = nWidth;
    g_nVirtualHeight = nHeight;
	g_fScale = 1.0f;
	
    // Initialize the available texture index list.
    for (int i = 0; i < TEXTURE_COUNT; i++)
        g_availableTextureIndexList.push_back(i);

    // Initialize Direct3D 12.
    if (!GetAPIPointers())
        return FALSE;
	if (!RecreateD3DObjects())
		return FALSE;

    return TRUE;
}

static BOOL
RecreateD3DObjects()
{
	g_contextID++;

    if (!GetScreenSize())
        return FALSE;
    if (!CreateSwapchain())
        return FALSE;
    if (!CreateDescriptorHeap())
        return FALSE;
    if (!CreateFrameResources())
        return FALSE;
    if (!CreateCommandAllocator())
        return FALSE;
    if (!CreateRootSignature())
        return FALSE;
    if (!CreatePipelineState())
        return FALSE;
    if (!CreateCommandList())
        return FALSE;
    if (!CreateVertexBuffer())
        return FALSE;
    if (!CreateFence())
        return FALSE;

	return TRUE;
}

static BOOL
GetAPIPointers()
{
#if defined(HAL_TARGET_GDK_XBOX_XS)
	//
	// Xbox
	//

    pfnCreateDXGIFactory2 = &CreateDXGIFactory2;
    pfnD3D12CreateDevice = &D3D12CreateDevice;
    pfnD3D12SerializeRootSignature = &D3D12SerializeRootSignature;
    pfnD3DCompile = NULL;

    return TRUE;
#else
	//
	// PC
	//

    HMODULE hModuleDXGI = LoadLibraryW(L"dxgi.dll");
    if (hModuleDXGI == NULL)
        return FALSE;

    HMODULE hModuleD3D12 = LoadLibraryW(L"d3d12.dll");
    if (hModuleD3D12 == NULL)
        return FALSE;

    HMODULE hModuleD3DCompiler = LoadLibraryW(L"d3dcompiler_47.dll");
    if (hModuleD3DCompiler == NULL)
        return E_FAIL;

    pfnCreateDXGIFactory2 = (HRESULT(__stdcall*)(UINT, REFIID, void**))(void*)GetProcAddress(hModuleDXGI, "CreateDXGIFactory2");
    if (pfnCreateDXGIFactory2 == NULL)
        return FALSE;

    pfnD3D12CreateDevice = (HRESULT(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**))(void*)GetProcAddress(hModuleD3D12, "D3D12CreateDevice");
    if (pfnD3D12CreateDevice == NULL)
        return FALSE;

    pfnD3D12SerializeRootSignature = (HRESULT(__stdcall*)(const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION, ID3DBlob**, ID3DBlob**))(void*)GetProcAddress(hModuleD3D12, "D3D12SerializeRootSignature");
    if (pfnD3D12SerializeRootSignature == NULL)
        return FALSE;

    pfnD3DCompile = (HRESULT(__stdcall*)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**))(void*)GetProcAddress(hModuleD3DCompiler, "D3DCompile");
    if (pfnD3DCompile == NULL)
        return E_FAIL;

    return TRUE;
#endif
}

static BOOL
GetScreenSize()
{
    RECT rc;
    GetClientRect(g_hWnd, &rc);

    g_fDisplayWidth = (float)(rc.right - rc.left);
    g_fDisplayHeight = (float)(rc.bottom - rc.top);

    g_viewport.Width = g_fDisplayWidth;
    g_viewport.Height = g_fDisplayHeight;
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_viewport.TopLeftX = 0;
    g_viewport.TopLeftY = 0;

    g_scissorRect.left = 0;
    g_scissorRect.top = 0;
    g_scissorRect.right = (LONG)g_fDisplayWidth;
    g_scissorRect.bottom = (LONG)g_fDisplayHeight;

    return TRUE;
}

static BOOL
CreateSwapchain()
{
    UINT dxgiFactoryFlags = 0;

#if 0
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = pfnCreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return FALSE;

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);
    if (g_device == nullptr)
    {
        hr = pfnD3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device));
        if (FAILED(hr))
            return FALSE;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue));
    if (FAILED(hr))
        return FALSE;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = (UINT)g_fDisplayWidth;
    swapChainDesc.Height = (UINT)g_fDisplayHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    //swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain;
    hr = factory->CreateSwapChainForHwnd(g_commandQueue.Get(),  g_hWnd, &swapChainDesc, nullptr, nullptr,&swapChain);
    if (FAILED(hr))
        return FALSE;

    hr = factory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr))
        return FALSE;

    hr = swapChain.As(&g_swapChain);
    if (FAILED(hr))
        return FALSE;

    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    return TRUE;
}

// From a DirectX sample.
static void
GetHardwareAdapter(
	IDXGIFactory1 *pFactory,
	IDXGIAdapter1 **ppAdapter,
	bool requestHighPerformanceAdapter)
{
    GUID guidID3D12Device = {0x189819f1, 0x1db6, 0x4b57, {0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7}};

    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
                ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            if (SUCCEEDED(pfnD3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, guidID3D12Device, nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            if (SUCCEEDED(pfnD3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, guidID3D12Device, nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

static BOOL
CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap));
    if (FAILED(hr))
        return FALSE;

    g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = TEXTURE_COUNT;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = g_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_srvHeap));
    if (FAILED(hr))
        return FALSE;

    g_srvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return TRUE;
}

static BOOL
CreateFrameResources()
{
#if defined(_MSC_VER)
    D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
#else
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    g_rtvHeap->GetCPUDescriptorHandleForHeapStart(&handle);
#endif
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = handle;

    for (UINT n = 0; n < FRAME_COUNT; n++)
    {
        HRESULT hr = g_swapChain->GetBuffer(n, IID_PPV_ARGS(&g_renderTargets[n]));
        if (FAILED(hr))
            return FALSE;

        g_device->CreateRenderTargetView(g_renderTargets[n].Get(), nullptr, rtvHandle);

        rtvHandle.ptr += (size_t)g_rtvDescriptorSize;
    }

    return TRUE;
}

static BOOL
CreateCommandAllocator()
{
    HRESULT hr = g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator));
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static BOOL
CreateRootSignature()
{
    D3D12_DESCRIPTOR_RANGE range[2];
    for (int i = 0; i < 2; i++)
    {
        range[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[i].NumDescriptors = 1;
        range[i].BaseShaderRegister = (UINT)i;
        range[i].RegisterSpace = 0;
        range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER rootParameters[2];
    for (int i = 0; i < 2; i++)
    {
        rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[i].DescriptorTable.pDescriptorRanges = &range[i];
        rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pParameters = &rootParameters[0];
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &sampler;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = pfnD3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
    if (FAILED(hr))
        return FALSE;

    hr  = g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static BOOL
CreatePipelineState()
{
    uint8_t* vertexShader;
    uint8_t* pixelShaderNormal;
    uint8_t* pixelShaderRule;
    uint8_t* pixelShaderMelt;
    size_t vertexShaderSize;
    size_t pixelShaderNormalSize;
    size_t pixelShaderRuleSize;
    size_t pixelShaderMeltSize;

    HRESULT hr;

    hr = CompileShaderFromString(szShader, "VS", "vs_5_0", &vertexShader, &vertexShaderSize);
    if (FAILED(hr))
        return FALSE;

    hr = CompileShaderFromString(szShader, "PS_Normal", "ps_5_0", &pixelShaderNormal, &pixelShaderNormalSize);
    if (FAILED(hr))
        return FALSE;

    hr = CompileShaderFromString(szShader, "PS_Rule", "ps_5_0", &pixelShaderRule, &pixelShaderRuleSize);
    if (FAILED(hr))
        return FALSE;

    hr = CompileShaderFromString(szShader, "PS_Melt", "ps_5_0", &pixelShaderMelt, &pixelShaderMeltSize);
    if (FAILED(hr))
        return FALSE;

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_BLEND_DESC normalBlendDesc = {};
    for (int i = 0; i < 2; i++)
    {
        normalBlendDesc.RenderTarget[i].BlendEnable = TRUE;
        normalBlendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
        normalBlendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        normalBlendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        normalBlendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        normalBlendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
        normalBlendDesc.RenderTarget[i].LogicOpEnable = FALSE;
        normalBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        normalBlendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        normalBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    }

	D3D12_BLEND_DESC addBlendDesc = {};
	for (int i = 0; i < 2; i++)
	{
		addBlendDesc.RenderTarget[i].BlendEnable = TRUE;
		addBlendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		addBlendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		addBlendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		addBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		addBlendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
		addBlendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
		addBlendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		addBlendDesc.RenderTarget[i].LogicOpEnable = FALSE;
		addBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	D3D12_BLEND_DESC subBlendDesc = {};
	for (int i = 0; i < 2; i++)
	{
		subBlendDesc.RenderTarget[i].BlendEnable = TRUE;
		subBlendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		subBlendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_REV_SUBTRACT;
		subBlendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		subBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		subBlendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		subBlendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		subBlendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		subBlendDesc.RenderTarget[i].LogicOpEnable = FALSE;
		subBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vertexShader;
    psoDesc.VS.BytecodeLength = vertexShaderSize;
    psoDesc.PS.pShaderBytecode = pixelShaderNormal;
    psoDesc.PS.BytecodeLength = pixelShaderNormalSize;
	psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable       = TRUE;
	psoDesc.RasterizerState.MultisampleEnable     = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount     = 0;
	psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState = normalBlendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineStateNormal));
    if (FAILED(hr))
        return FALSE;

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vertexShader;
    psoDesc.VS.BytecodeLength = vertexShaderSize;
    psoDesc.PS.pShaderBytecode = pixelShaderNormal;
    psoDesc.PS.BytecodeLength = pixelShaderNormalSize;
	psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable       = TRUE;
	psoDesc.RasterizerState.MultisampleEnable     = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount     = 0;
	psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState = addBlendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineStateAdd));
    if (FAILED(hr))
        return FALSE;

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vertexShader;
    psoDesc.VS.BytecodeLength = vertexShaderSize;
    psoDesc.PS.pShaderBytecode = pixelShaderNormal;
    psoDesc.PS.BytecodeLength = pixelShaderNormalSize;
	psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable       = TRUE;
	psoDesc.RasterizerState.MultisampleEnable     = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount     = 0;
	psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState = subBlendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineStateSub));
    if (FAILED(hr))
        return FALSE;

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vertexShader;
    psoDesc.VS.BytecodeLength = vertexShaderSize;
    psoDesc.PS.pShaderBytecode = pixelShaderRule;
    psoDesc.PS.BytecodeLength = pixelShaderRuleSize;
	psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable       = TRUE;
	psoDesc.RasterizerState.MultisampleEnable     = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount     = 0;
	psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState = normalBlendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineStateRule));
    if (FAILED(hr))
        return FALSE;

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vertexShader;
    psoDesc.VS.BytecodeLength = vertexShaderSize;
    psoDesc.PS.pShaderBytecode = pixelShaderMelt;
    psoDesc.PS.BytecodeLength = pixelShaderMeltSize;
	psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable       = TRUE;
	psoDesc.RasterizerState.MultisampleEnable     = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount     = 0;
	psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState = normalBlendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineStateMelt));
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static HRESULT
CompileShaderFromString(
	const char *szShader,
	LPCSTR szEntryPoint,
	LPCSTR szShaderModel,
	uint8_t **ppShaderData,
	size_t *pShaderSize)
{
#if 0
    ID3DBlob* pErrorBlob = nullptr;
    ID3DBlob* pBlobOut = nullptr;
    HRESULT hr = pfnD3DCompile(szShader, strlen(szShader), NULL, NULL, NULL, szEntryPoint, szShaderModel, 0, 0, &pBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        auto msg = (char*)pErrorBlob->GetBufferPointer();

        // Check this.
        (void)msg;
        return E_FAIL;
    }

	log_info("shader_%s[] = {", szEntryPoint);
	int size = ppBlobOut->GetBufferSize();
	char *byte = (char *)ppBlobOut->GetBufferPointer();
	for (int i = 0; i < size; i++) {
		log_info("0x%02x, ", (unsigned int)(unsigned char)byte[i]);
	}
	log_info("};");
#else
    if (strcmp(szEntryPoint, "VS") == 0)
	{
		*ppShaderData = (uint8_t*)g_VS_Bytecode;
		*pShaderSize = sizeof(g_VS_Bytecode);
		return S_OK;
	}

    if (strcmp(szEntryPoint, "PS_Normal") == 0)
	{
		*ppShaderData = (uint8_t*)g_PS_Normal_Bytecode;
		*pShaderSize = sizeof(g_PS_Normal_Bytecode);
		return S_OK;
	}

    if (strcmp(szEntryPoint, "PS_Rule") == 0)
	{
		*ppShaderData = (uint8_t*)g_PS_Rule_Bytecode;
		*pShaderSize = sizeof(g_PS_Rule_Bytecode);
		return S_OK;
	}

    if (strcmp(szEntryPoint, "PS_Melt") == 0)
	{
		*ppShaderData = (uint8_t*)g_PS_Melt_Bytecode;
		*pShaderSize = sizeof(g_PS_Melt_Bytecode);
		return S_OK;
	}

    return E_FAIL;
#endif
}

static BOOL
CreateCommandList()
{
    HRESULT hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), g_pipelineStateNormal.Get(), IID_PPV_ARGS(&g_commandList));
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static BOOL
CreateVertexBuffer()
{
    const UINT vertexBufferSize = sizeof(g_tempRectVertexBuffer);

	D3D12_HEAP_PROPERTIES type = {};
	type.Type                 = D3D12_HEAP_TYPE_UPLOAD;
	type.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	type.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	type.CreationNodeMask     = 1;
	type.VisibleNodeMask      = 1;

	D3D12_RESOURCE_DESC buf = {};
	buf.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
	buf.Alignment          = 0;
	buf.Width              = vertexBufferSize;
	buf.Height             = 1;
	buf.DepthOrArraySize   = 1;
	buf.MipLevels          = 1;
	buf.Format             = DXGI_FORMAT_UNKNOWN;
	buf.SampleDesc.Count   = 1;
	buf.SampleDesc.Quality = 0;
	buf.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	buf.Flags              = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = g_device->CreateCommittedResource(
		&type,
		D3D12_HEAP_FLAG_NONE,
		&buf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g_vertexBuffer)
		);
	if (FAILED(hr))
		return FALSE;

    g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
    g_vertexBufferView.StrideInBytes = sizeof(Vertex);
    g_vertexBufferView.SizeInBytes = vertexBufferSize;

    hr = g_commandList->Close();
    if (FAILED(hr))
        return FALSE;

    ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
    g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    return TRUE;
}

static BOOL
CreateFence()
{
    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    HRESULT hr = g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    if (FAILED(hr))
        return FALSE;

    g_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return FALSE;

    WaitForPreviousFrame();

    return TRUE;
}

VOID
D3D12Cleanup(VOID)
{
    WaitForPreviousFrame();
	ReleaseAllD3D12Objects();
}

BOOL
D3D12ResizeWindow(
	int nWindowWidth,
	int nWindowHeight,
	int nOffsetX,
	int nOffsetY,
	int nViewportWidth,
	int nViewportHeight,
	float scale)
{
	// g_nVirtualWidth ... virtual screen size
    // g_nVirtualHeight ... virtual screen size
    // nWindowWidth ... window physical size
	// nWindowHeight ... window physcal size
	// nViewportWidth ... nWindowWidth - nOffsetX * 2
	// nViewportHeight ... nWindowHeight - nOffsetY * 2
	// nOffsetX ... window physical px
	// nOffsetY ... window physical px

    g_fDisplayWidth = (float)nWindowWidth;
    g_fDisplayHeight = (float)nWindowHeight;
	g_fScale = scale;
	g_fOffsetX = (float)nOffsetX;
	g_fOffsetY = (float)nOffsetY;

    g_viewport.TopLeftX = 0;
    g_viewport.TopLeftY = 0;
    g_viewport.Width = (float)nWindowWidth;
    g_viewport.Height = (float)nWindowHeight;

    g_scissorRect.left = nOffsetX;
    g_scissorRect.top = nOffsetY;
    g_scissorRect.right = nOffsetX + nViewportWidth;
    g_scissorRect.bottom = nOffsetY + nViewportHeight;

	ReleaseAllD3D12Objects();

	if (!RecreateD3DObjects())
			return FALSE;

    return TRUE;
}

static void
ReleaseAllD3D12Objects()
{
	WaitForPreviousFrame();

	// Release all textures.
    for (TextureBundle *pTextureBundle : g_allTextureBundleList)
	{
        g_availableTextureIndexList.push_back(pTextureBundle->nIndex);
        delete pTextureBundle;
	}
	g_allTextureBundleList.clear();

	// Release other objects.
    g_commandList.Reset();
    g_commandAllocator.Reset();
    for (UINT i = 0; i < FRAME_COUNT; ++i)
        g_renderTargets[i].Reset();
    g_pipelineStateNormal.Reset();
    g_pipelineStateAdd.Reset();
    g_pipelineStateSub.Reset();
    g_pipelineStateRule.Reset();
    g_pipelineStateMelt.Reset();
    g_rootSignature.Reset();
    g_rtvHeap.Reset();
    g_srvHeap.Reset();
    g_vertexBuffer.Reset();
    g_vertexUploadHeap.Reset();
    g_swapChain.Reset();
    g_fence.Reset();
    CloseHandle(g_fenceEvent);
    g_commandQueue.Reset();
    g_device.Reset();
}

VOID
D3D12StartFrame(VOID)
{
    g_tempRectCount = 0;

    HRESULT hr;
    hr = g_commandAllocator->Reset();
    if (FAILED(hr))
        return;
    hr = g_commandList->Reset(g_commandAllocator.Get(), g_pipelineStateNormal.Get());
    if (FAILED(hr))
        return;

    g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { g_srvHeap.Get() };
    g_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

#if defined(_MSC_VER)
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
#else
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    g_srvHeap->GetGPUDescriptorHandleForHeapStart(&srvHandleGPU);
#endif
    g_commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU);
    g_commandList->RSSetViewports(1, &g_viewport);
    g_commandList->RSSetScissorRects(1, &g_scissorRect);

	D3D12_RESOURCE_BARRIER trans = {};
	trans.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	trans.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	trans.Transition.pResource   = g_renderTargets[g_frameIndex].Get();
	trans.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	trans.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	trans.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	g_commandList->ResourceBarrier(1, &trans);

#if defined(_MSC_VER)
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
#else
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU;
    g_rtvHeap->GetCPUDescriptorHandleForHeapStart(&rtvHandleCPU);
#endif
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandleCPU;
	rtvHandle.ptr += static_cast<SIZE_T>(g_frameIndex) * g_rtvDescriptorSize;
	g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

VOID
D3D12EndFrame(VOID)
{
    // Upload the temporary vertex buffer to GPU.
    UINT8* pVertexDataBegin;

	D3D12_RANGE readRange = {};
	readRange.Begin = 0;
	readRange.End   = 0; // We do not intend to read from this resource on the CPU.
	HRESULT hr = g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if(FAILED(hr))
        return;
    memcpy(pVertexDataBegin, g_tempRectVertexBuffer, sizeof(g_tempRectVertexBuffer));
    g_vertexBuffer->Unmap(0, nullptr);

    // Indicate that the back buffer will now be used to present.
	D3D12_RESOURCE_BARRIER trans = {};
	trans.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	trans.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	trans.Transition.pResource   = g_renderTargets[g_frameIndex].Get();
	trans.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	trans.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	trans.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	g_commandList->ResourceBarrier(1, &trans);
    hr = g_commandList->Close();
    if (FAILED(hr))
        return;

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
    g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    hr = g_swapChain->Present(1, 0);
    if (FAILED(hr))
        return;

    WaitForPreviousFrame();

    // Free texture upload heaps.
    for (ID3D12Resource* pTexUploadHeap : g_freeUploadHeapList)
        pTexUploadHeap->Release();
    g_freeUploadHeapList.clear();

    // Free textures which are requested to be destroyed in the frame.
    for (TextureBundle* pTextureBundle : g_freeTextureBundleList)
    {
        g_availableTextureIndexList.push_back(pTextureBundle->nIndex);
		g_allTextureBundleList.erase(
			std::find(g_allTextureBundleList.begin(),
					  g_allTextureBundleList.end(),
					  pTextureBundle));
        delete pTextureBundle;
    }
    g_freeTextureBundleList.clear();
}

static void
WaitForPreviousFrame()
{
	if (g_commandQueue == nullptr ||
		g_fence == nullptr ||
		g_fenceEvent == nullptr ||
		g_swapChain == nullptr)
		return;

    const UINT64 fence = g_fenceValue;
    HRESULT hr = g_commandQueue->Signal(g_fence.Get(), fence);
    if (FAILED(hr))
        return;
    g_fenceValue++;
 
    if (g_fence->GetCompletedValue() < fence)
    {
        hr = g_fence->SetEventOnCompletion(fence, g_fenceEvent);
        if (FAILED(hr))
            return;

        WaitForSingleObject(g_fenceEvent, INFINITE);
    }

    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
}

VOID
D3D12NotifyImageUpdate(
	struct hal_image *img)
{
    img->need_upload = true;
}

VOID
D3D12NotifyImageFree(
	struct hal_image *img)
{
    TextureBundle* pTextureBundle = (TextureBundle*)img->texture;
    if (pTextureBundle != NULL)
    {
        // Free it after a execution of the command list.
        g_freeTextureBundleList.push_back(pTextureBundle);
        img->texture = NULL;
    }
}

VOID
D3D12RenderImageNormal(
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
    DrawPrimitive2D(dst_left,
					dst_top,
					dst_width,
					dst_height,
					src_image,
					nullptr,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_NORMAL);
}

VOID
D3D12RenderImageAdd(
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
    DrawPrimitive2D(dst_left,
					dst_top,
					dst_width,
					dst_height,
					src_image,
					nullptr,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_ADD);
}

VOID
D3D12RenderImageSub(
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
    DrawPrimitive2D(dst_left,
					dst_top,
					dst_width,
					dst_height,
					src_image,
					nullptr,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_SUB);
}

VOID
D3D12RenderImageDim(
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
    DrawPrimitive2D(dst_left,
					dst_top,
					dst_width,
					dst_height,
					src_image,
					nullptr,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_DIM);
}

VOID
D3D12RenderImageRule(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
    DrawPrimitive2D(0,
					0,
					g_nVirtualWidth,
					g_nVirtualHeight,
					src_image,
					rule_image,
					0,
					0,
					g_nVirtualWidth,
					g_nVirtualHeight,
					threshold,
					PIPELINE_RULE);
}

VOID
D3D12RenderImageMelt(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int progress)
{
    DrawPrimitive2D(0,
					0,
					g_nVirtualWidth,
					g_nVirtualHeight,
					src_image,
					rule_image,
					0,
					0,
					g_nVirtualWidth,
					g_nVirtualHeight,
					progress,
					PIPELINE_MELT);
}

VOID
D3D12RenderImage3DNormal(
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
    DrawPrimitive3D(x1,
					y1,
					x2,
					y2,
					x3,
					y3,
					x4,
					y4,
					src_image,
					NULL,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_NORMAL);
}

VOID
D3D12RenderImage3DAdd(
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
    DrawPrimitive3D(x1,
					y1,
					x2,
					y2,
					x3,
					y3,
					x4,
					y4,
					src_image,
					NULL,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_ADD);
}

VOID
D3D12RenderImage3DSub(
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
    DrawPrimitive3D(x1,
					y1,
					x2,
					y2,
					x3,
					y3,
					x4,
					y4,
					src_image,
					NULL,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_SUB);
}

VOID
D3D12RenderImage3DDim(
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
    DrawPrimitive3D(x1,
					y1,
					x2,
					y2,
					x3,
					y3,
					x4,
					y4,
					src_image,
					NULL,
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_DIM);
}

static VOID
DrawPrimitive2D(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha,
	int pipeline)
{
    if (dst_width == -1)
        dst_width = src_image->width;
    if (dst_height == -1)
        dst_height = src_image->height;
    if (src_width == -1)
        src_width = src_image->width;
    if (src_height == -1)
        src_height = src_image->height;

    DrawPrimitive3D((float)dst_left,
                    (float)dst_top,
                    (float)(dst_left + dst_width),
                    (float)dst_top,
                    (float)dst_left,
                    (float)(dst_top + dst_height),
                    (float)(dst_left + dst_width),
                    (float)(dst_top + dst_height),
                    src_image,
                    rule_image,
                    src_left,
                    src_top,
                    src_width,
                    src_height,
                    alpha,
                    pipeline);
}

static VOID
DrawPrimitive3D(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha,
	int pipeline)
{
    // Check src_image.
    if (!UploadTextureIfNeeded(src_image))
        return;
    TextureBundle* pTextureBundle1 = (TextureBundle*)src_image->texture;

    // Check rule_image.
    if (rule_image != nullptr)
        if (!UploadTextureIfNeeded(rule_image))
            return;
    TextureBundle* pTextureBundle2 = rule_image != nullptr ? (TextureBundle*)rule_image->texture : nullptr;

    // Normalize vertices.
    float x1_ = (x1 * g_fScale + g_fOffsetX) / (g_fDisplayWidth / 2.0f) - 1.0f;
    float y1_ = 1.0f - (y1 * g_fScale + g_fOffsetY) / (g_fDisplayHeight / 2.0f);
    float x2_ = (x2 * g_fScale + g_fOffsetX) / (g_fDisplayWidth / 2.0f) - 1.0f;
    float y2_ = 1.0f - (y2 * g_fScale + g_fOffsetY) / (g_fDisplayHeight / 2.0f);
    float x3_ = (x3 * g_fScale + g_fOffsetX) / (g_fDisplayWidth / 2.0f) - 1.0f;
    float y3_ = 1.0f - (y3 * g_fScale + g_fOffsetY) / (g_fDisplayHeight / 2.0f);
    float x4_ = (x4 * g_fScale + g_fOffsetX) / (g_fDisplayWidth / 2.0f) - 1.0f;
    float y4_ = 1.0f - (y4 * g_fScale + g_fOffsetY) / (g_fDisplayHeight / 2.0f);

    // Normalize texture UV.
    float u1 = (float)src_left / (float)src_image->width;
    float v1 = (float)src_top / (float)src_image->height;
    float u2 = (float)(src_left + src_width) / (float)src_image->width;
    float v2 = (float)src_top / (float)src_image->height;
    float u3 = (float)src_left / (float)src_image->width;
    float v3 = (float)(src_top + src_height) / (float)src_image->height;
    float u4 = (float)(src_left + src_width) / (float)src_image->width;
    float v4 = (float)(src_top + src_height) / (float)src_image->height;

    // Create a vertex array.
    float color = (pipeline == PIPELINE_DIM) ? 0.5f : 1.0f;
    Vertex v[4] = {
        {{x1_, y1_, 0.0f}, {u1, v1}, {  0,   0}, {color, color, color, (float)alpha / 255.0f}},
        {{x2_, y2_, 0.0f}, {u2, v2}, {1.0,   0}, {color, color, color, (float)alpha / 255.0f}},
        {{x3_, y3_, 0.0f}, {u3, v3}, {  0, 1.0}, {color, color, color, (float)alpha / 255.0f}},
        {{x4_, y4_, 0.0f}, {u4, v4}, {1.0, 1.0}, {color, color, color, (float)alpha / 255.0f}},
    };

    // Save the vertices to the temporary buffer.
    if (g_tempRectCount >= TEMP_RECT_COUNT)
        return;
    memcpy(&g_tempRectVertexBuffer[g_tempRectCount * 4], &v, sizeof(v));

    // Record a pipeline.
    g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
    g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    switch (pipeline)
    {
    case PIPELINE_NORMAL:
    case PIPELINE_DIM:
        g_commandList->SetPipelineState(g_pipelineStateNormal.Get());
        break;
    case PIPELINE_ADD:
        g_commandList->SetPipelineState(g_pipelineStateAdd.Get());
        break;
    case PIPELINE_SUB:
        g_commandList->SetPipelineState(g_pipelineStateSub.Get());
        break;
    case PIPELINE_RULE:
        g_commandList->SetPipelineState(g_pipelineStateRule.Get());
        break;
    case PIPELINE_MELT:
        g_commandList->SetPipelineState(g_pipelineStateMelt.Get());
        break;
    }

#if defined(_MSC_VER)
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
#else
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU;
    g_srvHeap->GetGPUDescriptorHandleForHeapStart(&handleGPU);
#endif

    // Record texture1.
	D3D12_GPU_DESCRIPTOR_HANDLE handle1 = handleGPU;
	handle1.ptr += static_cast<UINT64>(pTextureBundle1->nIndex) * g_srvDescriptorSize;
	g_commandList->SetGraphicsRootDescriptorTable(0, handle1);

    // Record texture2.
    if (pTextureBundle2)
    {
		D3D12_GPU_DESCRIPTOR_HANDLE handle2 = handleGPU;
		handle2.ptr += static_cast<UINT64>(pTextureBundle2->nIndex) * g_srvDescriptorSize;
		g_commandList->SetGraphicsRootDescriptorTable(1, handle2);
    }

    // Record a draw.
    g_commandList->DrawInstanced(4, 1, g_tempRectCount * 4U, 0);

    // Increment the temporary buffer count.
    g_tempRectCount++;
}

static BOOL
UploadTextureIfNeeded(
	struct hal_image *img)
{
	bool is_needed;

	// Check if an upload is needed.
	is_needed = false;
	if (img->context != g_contextID)
		is_needed = true;	// Device recreated.
	if (img->need_upload)
		is_needed = true;	// Image updated.
	if (!is_needed)
        return TRUE;	// Not needed.

	// Schedule deleting the previous texture.
    if (img->texture != NULL)
    {
		if (img->context == g_contextID)
		{
			// Device is not recreated: Need to delete the texture.
			g_freeTextureBundleList.push_back((TextureBundle*)img->texture);
		}
		else
		{
			// Device was recreated: the texture was already deleted,
			// and the pTextureBundle is an invalid pointer that is already freed.
		}

		img->texture = NULL;
    }

    // Check if a texture index is available.
    if (g_availableTextureIndexList.size() == 0)
        return FALSE;

    // Get a texture index.
    int textureIndex = g_availableTextureIndexList.back();
    g_availableTextureIndexList.pop_back();

    // Create a texture bundle.
    TextureBundle* pTextureBundle = new TextureBundle();
    pTextureBundle->nIndex = textureIndex;
    img->texture = pTextureBundle;
    img->need_upload = FALSE;
	img->context = g_contextID;
	g_allTextureBundleList.push_back(pTextureBundle);

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.Width = (UINT)img->width;
    textureDesc.Height = (UINT)img->height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type                 = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask     = 1;
	prop.VisibleNodeMask      = 1;

	HRESULT hr = g_device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&pTextureBundle->pTexture));
    if (FAILED(hr))
        return FALSE;

    // Describe and create a SRV for the texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
#if defined(_MSC_VER)
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
#else
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    g_srvHeap->GetCPUDescriptorHandleForHeapStart(&srvHandleCPU);
#endif

	D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHandleCPU;
	handle.ptr += static_cast<SIZE_T>(pTextureBundle->nIndex) * g_srvDescriptorSize;
	g_device->CreateShaderResourceView(
		pTextureBundle->pTexture.Get(),
		&srvDesc,
		handle);

    // Create a GPU upload buffer.
    ID3D12Resource *pTextureUploadHeap;
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize_NoD3DX(pTextureBundle->pTexture.Get(), 0, 1);
	D3D12_HEAP_PROPERTIES prop2 = {};
	prop2.Type                 = D3D12_HEAP_TYPE_UPLOAD;
	prop2.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop2.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop2.CreationNodeMask     = 1;
	prop2.VisibleNodeMask      = 1;
	D3D12_RESOURCE_DESC buf = {};
	buf.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
	buf.Alignment          = 0;
	buf.Width              = uploadBufferSize;
	buf.Height             = 1;
	buf.DepthOrArraySize   = 1;
	buf.MipLevels          = 1;
	buf.Format             = DXGI_FORMAT_UNKNOWN;
	buf.SampleDesc.Count   = 1;
	buf.SampleDesc.Quality = 0;
	buf.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	buf.Flags              = D3D12_RESOURCE_FLAG_NONE;
	hr = g_device->CreateCommittedResource(
		&prop2,
		D3D12_HEAP_FLAG_NONE,
		&buf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pTextureUploadHeap)
		);
    if (FAILED(hr))
        return FALSE;

    // Copy data to the intermediate upload heap and then schedule a copy
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &img->pixels[0];
    textureData.RowPitch = img->width * 4;
    textureData.SlicePitch = img->width * img->height * 4;
    UpdateSubresources_NoD3DX(g_commandList.Get(), pTextureBundle->pTexture.Get(), pTextureUploadHeap, 0, 0, 1, &textureData);

	D3D12_RESOURCE_BARRIER trans = {};
	trans.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	trans.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	trans.Transition.pResource   = pTextureBundle->pTexture.Get();
	trans.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	trans.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	trans.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	g_commandList->ResourceBarrier(1, &trans);

    // Release the upload buffer later.
    g_freeUploadHeapList.push_back(pTextureUploadHeap);

    return TRUE;
}

static UINT64
GetRequiredIntermediateSize_NoD3DX(
    ID3D12Resource *destinationResource,
    UINT firstSubresource,
    UINT numSubresources)
{
    if (!destinationResource) return 0;

    D3D12_RESOURCE_DESC desc;
#if defined(_MSC_VER)
    desc = destinationResource->GetDesc();
#else
    destinationResource->GetDesc(&desc);
#endif

    ComPtr<ID3D12Device> device;
    HRESULT hr = destinationResource->GetDevice(IID_PPV_ARGS(&device));
    if (FAILED(hr) || !device) return 0;

    UINT64 requiredSize = 0;
    device->GetCopyableFootprints(
        &desc,
        firstSubresource,
        numSubresources,
        0,          // BaseOffset
        nullptr,    // pLayouts
        nullptr,    // pNumRows
        nullptr,    // pRowSizeInBytes
        &requiredSize
    );

    return requiredSize;
}

static inline UINT64
AlignUp(
	UINT64 value,
	UINT64 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static inline UINT64
MinU64(
	UINT64 a,
	UINT64 b)
{
	return a < b ? a : b;
}

struct MEMCPY_DEST
{
    void*  pData;
    SIZE_T RowPitch;
    SIZE_T SlicePitch;
};

static UINT64
UpdateSubresources_NoD3DX(
    ID3D12GraphicsCommandList *pCmdList,
    ID3D12Resource *pDestinationResource,
    ID3D12Resource *pIntermediate,
    UINT64 IntermediateOffset,
    UINT FirstSubresource,
    UINT NumSubresources,
    const D3D12_SUBRESOURCE_DATA *pSrcData)
{
    if (!pCmdList || !pDestinationResource || !pIntermediate || !pSrcData || NumSubresources == 0)
        return 0;

    D3D12_RESOURCE_DESC desc;
#if defined(_MSC_VER)
    desc = pDestinationResource->GetDesc();
#else
    pDestinationResource->GetDesc(&desc);
#endif

    ID3D12Device* device = nullptr;
    HRESULT hr = pDestinationResource->GetDevice(IID_PPV_ARGS(&device));
    if (FAILED(hr) || !device)
        return 0;

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        UINT64 numBytes = (UINT64)desc.Width;
        UINT64 srcBytes = (UINT64)((pSrcData[0].SlicePitch > 0) ? pSrcData[0].SlicePitch : pSrcData[0].RowPitch);
        UINT64 copyBytes = (UINT64)MinU64(numBytes, srcBytes);

        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        hr = pIntermediate->Map(0, &readRange, &mapped);
        if (FAILED(hr))
        {
            device->Release();
            return 0;
        }

        memcpy(reinterpret_cast<uint8_t*>(mapped) + IntermediateOffset, pSrcData[0].pData, (size_t)copyBytes);

        D3D12_RANGE written = {
            (SIZE_T)IntermediateOffset,
            (SIZE_T)(IntermediateOffset + copyBytes)
        };
        pIntermediate->Unmap(0, &written);

        pCmdList->CopyBufferRegion(
            pDestinationResource, 0,
            pIntermediate, IntermediateOffset,
            copyBytes
        );

        device->Release();
        return copyBytes;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts =
        (D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * NumSubresources);
    UINT* numRows =
        (UINT*)_alloca(sizeof(UINT) * NumSubresources);
    UINT64* rowSizeInBytes =
        (UINT64*)_alloca(sizeof(UINT64) * NumSubresources);

    UINT64 requiredSize = 0;
    device->GetCopyableFootprints(
        &desc,
        FirstSubresource,
        NumSubresources,
        IntermediateOffset,
        layouts,
        numRows,
        rowSizeInBytes,
        &requiredSize
    );

    uint8_t* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hr = pIntermediate->Map(0, &readRange, reinterpret_cast<void**>(&mapped));
    if (FAILED(hr))
    {
        device->Release();
        return 0;
    }

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        D3D12_MEMCPY_DEST destData = {};
        destData.pData      = mapped + layouts[i].Offset;
        destData.RowPitch   = layouts[i].Footprint.RowPitch;
        destData.SlicePitch = (SIZE_T)(layouts[i].Footprint.RowPitch * numRows[i]);
        MemcpySubresource_NoD3DX(
            &destData,
            &pSrcData[i],
            (SIZE_T)rowSizeInBytes[i],
            numRows[i],
            layouts[i].Footprint.Depth
        );
    }

    D3D12_RANGE writtenRange = {
        (SIZE_T)IntermediateOffset,
        (SIZE_T)(IntermediateOffset + requiredSize)
    };
    pIntermediate->Unmap(0, &writtenRange);

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource        = pDestinationResource;
        dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = FirstSubresource + i;

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource       = pIntermediate;
        srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = layouts[i];

        pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }

    device->Release();
    return requiredSize;
}

static void
MemcpySubresource_NoD3DX(
    const D3D12_MEMCPY_DEST *pDest,
    const D3D12_SUBRESOURCE_DATA *pSrc,
    SIZE_T rowSizeInBytes,
    UINT numRows,
    UINT numSlices)
{
    for (UINT z = 0; z < numSlices; ++z)
    {
        uint8_t* dstSlice = reinterpret_cast<uint8_t*>(pDest->pData) + pDest->SlicePitch * z;
        const uint8_t* srcSlice = reinterpret_cast<const uint8_t*>(pSrc->pData) + pSrc->SlicePitch * z;

        for (UINT y = 0; y < numRows; ++y)
        {
            memcpy(dstSlice + pDest->RowPitch * y,
                   srcSlice + pSrc->RowPitch * y,
                   rowSizeInBytes);
        }
    }
}
