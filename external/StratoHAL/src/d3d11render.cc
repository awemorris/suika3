/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Graphics HAL for Direct3D 11
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
};

#include <windows.h>
#include <d3d11_2.h>
#include <d3dcompiler.h>

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
    "SamplerState samLinear : register(s0);                     \n"
    "                                                           \n"
    "float4 PS_Normal(PS_INPUT input) : SV_Target               \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(samLinear, input.Tex1);        \n"
    "    pix.a = pix.a * input.Color.a;                         \n"
    "    return pix;                                            \n"
    "}                                                          \n"
    "                                                           \n"
    "float4 PS_Rule(PS_INPUT input) : SV_Target                 \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(samLinear, input.Tex1);        \n"
    "    float4 rule = tx2.Sample(samLinear, input.Tex2);       \n"
    "    pix.a = 1.0 - step(input.Color.a, rule.b);             \n"
    "    return pix;                                            \n"
    "}                                                          \n"
    "                                                           \n"
    "float4 PS_Melt(PS_INPUT input) : SV_Target                 \n"
    "{                                                          \n"
    "    float4 pix = tx1.Sample(samLinear, input.Tex1);        \n"
    "    float4 rule = tx2.Sample(samLinear, input.Tex2);       \n"
    "    pix.a = clamp((1.0 - rule.b) +                         \n"
    "                  (input.Color.a * 2.0 - 1.0),             \n"
    "                  0.0, 1.0);                               \n"
    "    return pix;                                            \n"
    "}                                                          \n";

//
// Vertex Structure
//

struct SimpleVertex
{
    float Pos[3];
    float Tex1[2];
    float Tex2[2];
    float Color[4];
};

//
// Texture Object for "struct hal_image"
//

struct TextureBundle
{
    ID3D11Texture2D* pTexture;
    ID3D11ShaderResourceView *pView;
};

//
// Direct3D 11 Objects
//

static HWND						g_hWnd;
static D3D_DRIVER_TYPE          g_driverType;
static D3D_FEATURE_LEVEL        g_featureLevel;
static ID3D11Device*            g_pd3dDevice;
static ID3D11Device1*           g_pd3dDevice1;
static ID3D11DeviceContext*     g_pImmediateContext;
static ID3D11DeviceContext1*    g_pImmediateContext1;
static IDXGISwapChain*          g_pSwapChain;
static IDXGISwapChain1*         g_pSwapChain1;
static ID3D11RenderTargetView*  g_pRenderTargetView;
static ID3D11BlendState*        g_pBlendStateNormal;
static ID3D11BlendState*        g_pBlendStateAdd;
static ID3D11BlendState*        g_pBlendStateSub;
static ID3D11VertexShader*      g_pVertexShader;
static ID3D11PixelShader*       g_pPixelShaderNormal;
static ID3D11PixelShader*       g_pPixelShaderRule;
static ID3D11PixelShader*       g_pPixelShaderMelt;
static ID3D11InputLayout*       g_pVertexLayout;
static ID3D11Buffer*            g_pVertexBuffer;
static ID3D11SamplerState*      g_pSamplerLinear;

//
// Screen Size
//

static int g_nWindowWidth;
static int g_nWindowHeight;
static float g_fDisplayWidth;
static float g_fDisplayHeight;

//
// Forward Declarations
//

static BOOL GetScreenSize();
static BOOL CreateDevice();
static BOOL CreateSwapchain();
static BOOL CreateRenderTargetView();
static BOOL CreateBlendState();
static BOOL SetupViewport();
static BOOL CreateVertexShader();
static BOOL CompilePixelShaders();
static HRESULT CompileShaderFromString(const char *szShader,
									   LPCSTR szEntryPoint,
									   LPCSTR szShaderModel,
									   ID3DBlob **ppBlobOut);
static BOOL CreateVertexBuffer();
static BOOL CreateSamplerState();
static VOID DrawPrimitive2D(int dst_left, int dst_top, int dst_width,
							int dst_height, struct hal_image *src_image,
							struct hal_image *rule_image, int src_left,
							int src_top, int src_width, int src_height,
							int alpha, int pipeline);
static VOID DrawPrimitive3D(float x1, float y1, float x2, float y2, float x3,
							float y3, float x4, float y4,
							struct hal_image *src_image, int src_left,
							int src_top, int src_width, int src_height,
							int alpha, int pipeline);
static BOOL UploadTextureIfNeeded(struct hal_image *img);

//
// Initialize Direct3D 11.
//
BOOL
D3D11Initialize(
	HWND hWnd,
	int nWidth,
	int nHeight)
{
    g_hWnd = hWnd;
    g_nWindowWidth = nWidth;
    g_nWindowHeight = nHeight;

    if (!GetScreenSize())
        return FALSE;
    if (!CreateDevice())
        return FALSE;
    if (!CreateSwapchain())
        return FALSE;
    if (!CreateRenderTargetView())
        return FALSE;
    if (!CreateBlendState())
        return FALSE;
    if (!SetupViewport())
        return FALSE;
    if (!CreateVertexShader())
        return FALSE;
    if (!CompilePixelShaders())
        return FALSE;
    if (!CreateVertexBuffer())
        return FALSE;
    if (!CreateSamplerState())
        return FALSE;

    return TRUE;
}

static BOOL
GetScreenSize()
{
    RECT rc;
    GetClientRect(g_hWnd, &rc);

    g_fDisplayWidth = (float)(rc.right - rc.left);
    g_fDisplayHeight = (float)(rc.bottom - rc.top);

    return TRUE;
}

static BOOL
CreateDevice()
{
    UINT createDeviceFlags = 0;

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // Open d3d11.dll dynamically.
    HMODULE hModule = LoadLibraryW(L"d3d11.dll");
    if (hModule == NULL)
        return FALSE;

    // Get a pointer to D3D11CreateDevice().
    HRESULT(__stdcall * pD3D11CreateDevice)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
    pD3D11CreateDevice = (HRESULT(__stdcall*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**))(void*)GetProcAddress(hModule, "D3D11CreateDevice");
    if (pD3D11CreateDevice == NULL)
        return FALSE;

    // For each driver type.
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];

        // Try with Direct3D 11.1+.
        HRESULT hr = pD3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (FAILED(hr))
        {
            // For a special case.
            if (hr == E_INVALIDARG)
            {
                // Try with Direct3D 11.0.
                hr = pD3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1, D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
                if (FAILED(hr))
                    continue;

                // Succeeded.
                return TRUE;
            }

            // Failed.
            continue;
        }

        // Succeedd.
        return TRUE;
    }

    return FALSE;
}

static BOOL
CreateSwapchain()
{
    IDXGIFactory1* dxgiFactory = nullptr;

    // Obtain a DXGI device.
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (FAILED(hr))
        return FALSE;

    // Obtain a DXGI adapter.
    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr))
    {
        dxgiDevice->Release();
        return FALSE;
    }

    // Obtain a DXGI factory.
    hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
    if (FAILED(hr))
    {
        adapter->Release();
        dxgiDevice->Release();
        return FALSE;
    }

    adapter->Release();
    dxgiDevice->Release();

    // Create a swapchain.
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (SUCCEEDED(hr) || dxgiFactory2 != nullptr)
    {
        // Direct3D 11.1+.
        hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
        if (SUCCEEDED(hr))
        {
            (void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));

            DXGI_SWAP_CHAIN_DESC1 sd = {};
            sd.Width = (UINT)g_fDisplayWidth;
            sd.Height = (UINT)g_fDisplayHeight;
            sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.BufferCount = 1;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
            if (SUCCEEDED(hr))
            {
                hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
                dxgiFactory2->Release();
            }
        }
    }

    // If failed.
    if (g_pSwapChain == nullptr)
    {
        // Direct3D 11.0.
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = (UINT)g_fDisplayWidth;
        sd.BufferDesc.Height = (UINT)g_fDisplayHeight;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
        if (FAILED(hr))
            return FALSE;
    }

    // Block the ALT+ENTER shortcut.
    dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    return TRUE;
}

static BOOL
CreateRenderTargetView()
{
    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return FALSE;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return FALSE;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    return TRUE;
}

static BOOL
CreateBlendState()
{
    // Setup the normal blend state.
    D3D11_BLEND_DESC BlendState = {};
    BlendState.RenderTarget[0].BlendEnable = TRUE;
    BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateNormal);

	// Setup the add blend state.
	BlendState.RenderTarget[0].BlendEnable = TRUE;
	BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateAdd);

	// Setup the sub blend state.
	BlendState.RenderTarget[0].BlendEnable = TRUE;
	BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_REV_SUBTRACT;
	BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateSub);

    return TRUE;
}

static BOOL
SetupViewport()
{
    D3D11_VIEWPORT vp;
    vp.Width = g_fDisplayWidth;
    vp.Height = g_fDisplayHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;

    g_pImmediateContext->RSSetViewports(1, &vp);

    return TRUE;
}

static BOOL
CreateVertexShader()
{
    // Compile the vertex shader.
    ID3DBlob* pVSBlob = nullptr;
    HRESULT hr = CompileShaderFromString(szShader, "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
        return FALSE;

    // Create the vertex shader.
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return FALSE;
    }

    // Define the input layout.
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout.
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return FALSE;

    // Set the input layout.
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    return TRUE;
}

static BOOL
CompilePixelShaders()
{
    // Normal Shader
    ID3DBlob* pPSBlob = nullptr;
    HRESULT hr = CompileShaderFromString(szShader, "PS_Normal", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
        return FALSE;

    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShaderNormal);
    pPSBlob->Release();
    if (FAILED(hr))
        return FALSE;

    // Rule Shader
    pPSBlob = nullptr;
    hr = CompileShaderFromString(szShader, "PS_Rule", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
        return FALSE;

    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShaderRule);
    pPSBlob->Release();
    if (FAILED(hr))
        return FALSE;

    // Melt Shader
    pPSBlob = nullptr;
    hr = CompileShaderFromString(szShader, "PS_Melt", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
        return FALSE;

    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShaderMelt);
    pPSBlob->Release();
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static BOOL
CreateSamplerState()
{
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HRESULT hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static HRESULT
CompileShaderFromString(
	const char* szShader,
	LPCSTR szEntryPoint,
	LPCSTR szShaderModel,
	ID3DBlob** ppBlobOut)
{
    HMODULE hModule = LoadLibraryW(L"d3dcompiler_47.dll");
    if (hModule == NULL)
        return E_FAIL;

    HRESULT(__stdcall * pD3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    pD3DCompile = (HRESULT(__stdcall*)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**))(void*)GetProcAddress(hModule, "D3DCompile");
    if (pD3DCompile == NULL)
        return E_FAIL;

    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = pD3DCompile(szShader, strlen(szShader), NULL, NULL, NULL, szEntryPoint, szShaderModel, 0, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        auto error = (char*)pErrorBlob->GetBufferPointer();
        (void)error;
        return E_FAIL;
    }
    return S_OK;
}

static BOOL
CreateVertexBuffer()
{
    // Create vertex buffer
    SimpleVertex vertices[4] = {};

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;
    HRESULT hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return FALSE;

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    return TRUE;
}

VOID
D3D11Cleanup(VOID)
{
    if (g_pSamplerLinear)
    {
        g_pSamplerLinear->Release();
        g_pSamplerLinear = nullptr;
    }
    if (g_pVertexBuffer)
    {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
    if (g_pVertexLayout)
    {
        g_pVertexLayout->Release();
        g_pVertexLayout = nullptr;
    }
    if (g_pPixelShaderMelt)
    {
        g_pPixelShaderMelt->Release();
        g_pPixelShaderMelt = nullptr;
    }
    if (g_pPixelShaderRule)
    {
        g_pPixelShaderRule->Release();
        g_pPixelShaderRule = nullptr;
    }
    if (g_pPixelShaderNormal)
    {
        g_pPixelShaderNormal->Release();
        g_pPixelShaderNormal = nullptr;
    }
    if (g_pVertexShader)
    {
        g_pVertexShader->Release();
        g_pVertexShader = nullptr;
    }
    if (g_pBlendStateNormal)
    {
        g_pBlendStateNormal->Release();
        g_pBlendStateNormal = nullptr;
    }
    if (g_pBlendStateAdd)
    {
        g_pBlendStateAdd->Release();
        g_pBlendStateAdd = nullptr;
    }
    if (g_pBlendStateSub)
    {
        g_pBlendStateSub->Release();
        g_pBlendStateSub = nullptr;
    }
    if (g_pRenderTargetView)
    {
        g_pRenderTargetView->Release();
        g_pRenderTargetView = nullptr;
    }
    if (g_pSwapChain1)
    {
        g_pSwapChain1->Release();
        g_pSwapChain1 = nullptr;
    }
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pImmediateContext1)
    {
        g_pImmediateContext1->Release();
        g_pImmediateContext1 = nullptr;
    }
    if (g_pImmediateContext)
    {
        g_pImmediateContext->Release();
        g_pImmediateContext = nullptr;
    }
    if (g_pd3dDevice1)
    {
        g_pd3dDevice1->Release();
        g_pd3dDevice1 = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

BOOL
D3D11ResizeWindow(
	int nScreenWidth,
	int nScreenHeight,
	int nOffsetX,
	int nOffsetY,
	int nViewportWidth,
	int nViewportHeight,
	float scale)
{
    UNUSED_PARAMETER(nOffsetX);
    UNUSED_PARAMETER(nOffsetY);
    UNUSED_PARAMETER(scale);

    g_fDisplayWidth = (float)nScreenWidth;
    g_fDisplayHeight = (float)nScreenHeight;

    float ratioX = (float)g_nWindowWidth / g_fDisplayWidth;
    float ratioY = (float)g_nWindowHeight / g_fDisplayHeight;

    D3D11_VIEWPORT vp;
    vp.TopLeftX = (float)nOffsetX * ratioX;
    vp.TopLeftY = (float)nOffsetY * ratioY;
    vp.Width = (float)nViewportWidth * ratioX;
    vp.Height = (float)nViewportHeight * ratioY;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    g_pImmediateContext->RSSetViewports(1, &vp);

    RECT sci;
    sci.left = 0;
    sci.top = 0;
    sci.right = g_nWindowWidth - (int)((float)nOffsetX / ratioX);
    sci.bottom = g_nWindowHeight - (int)((float)nOffsetY / ratioY);

    g_pImmediateContext->RSSetScissorRects(1, &sci);

#if 0
    g_fDisplayWidth = (float)nScreenWidth;
    g_fDisplayHeight = (float)nScreenHeight;

    float ratioX = (float)g_nWindowWidth / (g_fDisplayWidth + (float)nOffsetX * 2.0f);
    float ratioY = (float)g_nWindowHeight / (g_fDisplayHeight + (float)nOffsetY * 2.0f);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = (float)nOffsetX * ratioX;
    vp.TopLeftY = (float)nOffsetY * ratioY;
    vp.Width = (float)g_nWindowWidth - (float)nOffsetX * ratioX * 2.0f;
    vp.Height = (float)g_nWindowHeight - (float)nOffsetY * ratioY * 2.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    g_pImmediateContext->RSSetViewports(1, &vp);

    RECT sci;
    sci.left = 0;
    sci.top = 0;
    sci.right = g_nWindowWidth - (int)((float)nOffsetX / ratioX);
    sci.bottom = g_nWindowHeight - (int)((float)nOffsetY / ratioY);

    g_pImmediateContext->RSSetScissorRects(1, &sci);
#endif

    return TRUE;
}

VOID
D3D11StartFrame(VOID)
{
    float color[4] = { 0, 0, 0, 0 };

    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, color);
}

VOID
D3D11EndFrame(VOID)
{
    HRESULT hr = g_pSwapChain->Present(0, 0);
    (void)hr;
}

VOID
D3D11NotifyImageUpdate(
	struct hal_image *img)
{
    img->need_upload = true;
}

VOID
D3D11NotifyImageFree(
	struct hal_image *img)
{
    TextureBundle* pTextureBundle = (TextureBundle *)img->texture;
    if (pTextureBundle != NULL)
    {
        if (pTextureBundle->pView != NULL)
            pTextureBundle->pView->Release();
        if (pTextureBundle->pTexture != NULL)
            pTextureBundle->pTexture->Release();
        free(pTextureBundle);
        img->texture = NULL;
    }
}

VOID
D3D11RenderImageNormal(
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
D3D11RenderImageAdd(
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
D3D11RenderImageSub(
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
D3D11RenderImageDim(
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
D3D11RenderImageRule(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
    DrawPrimitive2D(0,
					0,
					g_nWindowWidth,
					g_nWindowHeight,
					src_image,
					rule_image,
					0,
					0,
					g_nWindowWidth,
					g_nWindowHeight,
					threshold,
					PIPELINE_RULE);
}

VOID
D3D11RenderImageMelt(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int progress)
{
    DrawPrimitive2D(0,
					0,
					g_nWindowWidth,
					g_nWindowHeight,
					src_image,
					rule_image,
					0,
					0,
					g_nWindowWidth,
					g_nWindowHeight,
					progress,
					PIPELINE_MELT);
}

VOID
D3D11RenderImage3DNormal(
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
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_NORMAL);
}

VOID
D3D11RenderImage3DAdd(
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
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_ADD);
}

VOID
D3D11RenderImage3DSub(
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
					src_left,
					src_top,
					src_width,
					src_height,
					alpha,
					PIPELINE_SUB);
}

VOID
D3D11RenderImage3DDim(
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
    UNUSED_PARAMETER(alpha);

    if (!UploadTextureIfNeeded(src_image))
        return;
    if (rule_image != nullptr)
        if (!UploadTextureIfNeeded(rule_image))
            return;

    TextureBundle* pTextureBundle1 = (TextureBundle*)src_image->texture;
    TextureBundle* pTextureBundle2 = rule_image != nullptr ? (TextureBundle*)rule_image->texture : nullptr;

    if (dst_width == -1)
        dst_width = src_image->width;
    if (dst_height == -1)
        dst_height = src_image->height;
    if (src_width == -1)
        src_width = src_image->width;
    if (src_height == -1)
        src_height = src_image->height;

    float x1 = (float)dst_left / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y1 = 1.0f - (float)dst_top / ((float)g_nWindowHeight / 2.0f);
    float x2 = (float)(dst_left + dst_width) / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y2 = 1.0f - (float)dst_top / ((float)g_nWindowHeight / 2.0f);
    float x3 = (float)dst_left / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y3 = 1.0f - (float)(dst_top + dst_height) / ((float)g_nWindowHeight / 2.0f);
    float x4 = (float)(dst_left + dst_width) / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y4 = 1.0f - (float)(dst_top + dst_height) / ((float)g_nWindowHeight / 2.0f);

    float u1 = (float)src_left / (float)src_image->width;
    float v1 = (float)src_top / (float)src_image->height;
    float u2 = (float)(src_left + src_width) / (float)src_image->width;
    float v2 = (float)src_top / (float)src_image->height;
    float u3 = (float)src_left / (float)src_image->width;
    float v3 = (float)(src_top + src_height) / (float)src_image->height;
    float u4 = (float)(src_left + src_width) / (float)src_image->width;
    float v4 = (float)(src_top + src_height) / (float)src_image->height;

    float color = (pipeline == PIPELINE_DIM) ? 0.5f : 1.0f;

    SimpleVertex v[4] = {
        {{x1, y1, 0.0f}, {u1, v1}, {  0,   0}, {color, color, color, (float)alpha / 255.0f}},
        {{x2, y2, 0.0f}, {u2, v2}, {1.0,   0}, {color, color, color, (float)alpha / 255.0f}},
        {{x3, y3, 0.0f}, {u3, v3}, {  0, 1.0}, {color, color, color, (float)alpha / 255.0f}},
        {{x4, y4, 0.0f}, {u4, v4}, {1.0, 1.0}, {color, color, color, (float)alpha / 255.0f}},
    };

    D3D11_MAPPED_SUBRESOURCE resource;
    g_pImmediateContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    memcpy(resource.pData, (void*)&v, sizeof(v));
    g_pImmediateContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);

    float factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    switch (pipeline)
    {
    case PIPELINE_NORMAL:
    case PIPELINE_DIM:
		g_pImmediateContext->OMSetBlendState(g_pBlendStateNormal, factor, 0xffffffff);
        g_pImmediateContext->PSSetShader(g_pPixelShaderNormal, nullptr, 0);
        break;
    case PIPELINE_ADD:
		g_pImmediateContext->OMSetBlendState(g_pBlendStateAdd, factor, 0xffffffff);
        g_pImmediateContext->PSSetShader(g_pPixelShaderNormal, nullptr, 0);
        break;
    case PIPELINE_SUB:
		g_pImmediateContext->OMSetBlendState(g_pBlendStateSub, factor, 0xffffffff);
        g_pImmediateContext->PSSetShader(g_pPixelShaderNormal, nullptr, 0);
        break;
    case PIPELINE_RULE:
		g_pImmediateContext->OMSetBlendState(g_pBlendStateNormal, factor, 0xffffffff);
        g_pImmediateContext->PSSetShader(g_pPixelShaderRule, nullptr, 0);
        break;
    case PIPELINE_MELT:
		g_pImmediateContext->OMSetBlendState(g_pBlendStateNormal, factor, 0xffffffff);
        g_pImmediateContext->PSSetShader(g_pPixelShaderMelt, nullptr, 0);
        break;
    }

    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

    ID3D11ShaderResourceView* pView[2];
    pView[0] = pTextureBundle1->pView;
    pView[1] = pTextureBundle2 != nullptr ? pTextureBundle2->pView : nullptr;
    g_pImmediateContext->PSSetShaderResources(0, 2, pView);

    g_pImmediateContext->Draw(4, 0);
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
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha,
	int pipeline)
{
	UNUSED_PARAMETER(pipeline);

    if (!UploadTextureIfNeeded(src_image))
        return;

    TextureBundle* pTextureBundle = (TextureBundle*)src_image->texture;

    float x1_ = x1 / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y1_ = 1.0f - y1 / ((float)g_nWindowHeight / 2.0f);
    float x2_ = x2 / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y2_ = 1.0f - y2 / ((float)g_nWindowHeight / 2.0f);
    float x3_ = x3 / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y3_ = 1.0f - y3 / ((float)g_nWindowHeight / 2.0f);
    float x4_ = x4 / ((float)g_nWindowWidth / 2.0f) - 1.0f;
    float y4_ = 1.0f - y4 / ((float)g_nWindowHeight / 2.0f);

    float u1 = (float)src_left / (float)src_image->width;
    float v1 = (float)src_top / (float)src_image->height;
    float u2 = (float)(src_left + src_width) / (float)src_image->width;
    float v2 = (float)src_top / (float)src_image->height;
    float u3 = (float)src_left / (float)src_image->width;
    float v3 = (float)(src_top + src_height) / (float)src_image->height;
    float u4 = (float)(src_left + src_width) / (float)src_image->width;
    float v4 = (float)(src_top + src_height) / (float)src_image->height;

    SimpleVertex v[4] = {
        {{x1_, y1_, 0.0f}, {u1, v1}, {  0,   0}, {1.0f, 1.0f, 1.0f, (float)alpha / 255.0f}},
        {{x2_, y2_, 0.0f}, {u2, v2}, {1.0,   0}, {1.0f, 1.0f, 1.0f, (float)alpha / 255.0f}},
        {{x3_, y3_, 0.0f}, {u3, v3}, {  0, 1.0}, {1.0f, 1.0f, 1.0f, (float)alpha / 255.0f}},
        {{x4_, y4_, 0.0f}, {u4, v4}, {1.0, 1.0}, {1.0f, 1.0f, 1.0f, (float)alpha / 255.0f}},
    };

    D3D11_MAPPED_SUBRESOURCE resource;
    g_pImmediateContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    memcpy(resource.pData, (void*)&v, sizeof(v));
    g_pImmediateContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShaderNormal, nullptr, 0);
    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

    ID3D11ShaderResourceView* pView[2];
    pView[0] = pTextureBundle->pView;
    pView[1] = nullptr;
    g_pImmediateContext->PSSetShaderResources(0, 2, pView);

    // TODO: PIPELINE_ADD

    g_pImmediateContext->Draw(4, 0);
}

static BOOL
UploadTextureIfNeeded(
	struct hal_image *img)
{
    if (!img->need_upload)
        return TRUE;

    if (img->texture == NULL)
    {
        TextureBundle *pTextureBundle = (TextureBundle *)malloc(sizeof(TextureBundle));
        if (pTextureBundle == NULL)
            return FALSE;

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = (UINT)img->width;
        desc.Height = (UINT)img->height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pTextureBundle->pTexture);
        if (FAILED(hr))
            return FALSE;

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        hr = g_pd3dDevice->CreateShaderResourceView(pTextureBundle->pTexture, &SRVDesc, &pTextureBundle->pView);
        if (FAILED(hr))
        {
            pTextureBundle->pTexture->Release();
            free(pTextureBundle);
            return FALSE;
        }

        img->texture = pTextureBundle;
    }

    TextureBundle *pTextureBundle = (TextureBundle *)img->texture;

    D3D11_MAPPED_SUBRESOURCE resource;
    g_pImmediateContext->Map(pTextureBundle->pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    for (int row = 0; row < img->height; row++)
    {
        memcpy((char*)resource.pData + resource.RowPitch * (UINT)row, (char *)img->pixels + 4 * img->width * row, 4U * (UINT)img->width);
    }
    g_pImmediateContext->Unmap(pTextureBundle->pTexture, 0);

    img->need_upload = FALSE;

    return TRUE;
}
