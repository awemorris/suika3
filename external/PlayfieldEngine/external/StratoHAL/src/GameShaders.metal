// -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*-

/*
 * StratoHAL
 * Apple Metal Shader
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

#include <metal_stdlib>
#include <metal_common>
#include <simd/simd.h>

using namespace metal;

enum GameVertexInputIndex {
    GameVertexInputIndexVertices = 0,
};

enum {
    GameTextureIndexSrc1 = 0,
    GameTextureIndexSrc2 = 1,
};

// Vertex shader inputs
struct GameVertex {
    vector_float2 xy;
    vector_float2 uv1;
    vector_float2 uv2;
    float alpha;
    float padding;  // This is absolutely needed for 64-bit alignments
};

// Vertex shader outputs and fragment shader inputs
struct RasterizerData {
    float4 position [[position]];
    float2 textureCoordinate1;
    float2 textureCoordinate2;
    float alpha;
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant GameVertex *vertexArray [[buffer(GameVertexInputIndexVertices)]])
{
    RasterizerData out;
    out.position = vector_float4(vertexArray[vertexID].xy, 0, 1);
    out.textureCoordinate1 = vertexArray[vertexID].uv1;
    out.textureCoordinate2 = vertexArray[vertexID].uv2;
    out.alpha = vertexArray[vertexID].alpha;
    return out;
}

fragment float4
fragmentNormalShader(RasterizerData in [[stage_in]],
                     texture2d<half> colorTexture [[texture(GameTextureIndexSrc1)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate1);
    return float4(colorSample.r, colorSample.g, colorSample.b, colorSample.a * in.alpha);
}

fragment float4
fragmentCopyShader(RasterizerData in [[stage_in]],
                   texture2d<half> colorTexture [[texture(GameTextureIndexSrc1)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate1);
    colorSample.a = 1.0;
    return float4(colorSample);
}

fragment float4
fragmentDimShader(RasterizerData in [[stage_in]],
                  texture2d<half> colorTexture [[texture(GameTextureIndexSrc1)]])
{
    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate1);
    colorSample.rgb *= 0.5;
    return float4(colorSample);
}

fragment float4
fragmentRuleShader(RasterizerData in [[stage_in]],
                   texture2d<half> colorTexture [[texture(GameTextureIndexSrc1)]],
                   texture2d<half> ruleTexture [[texture(GameTextureIndexSrc2)]])
{
    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate1);
    const half4 ruleSample = ruleTexture.sample(textureSampler, in.textureCoordinate2);
    colorSample.a = 1.0 - step(in.alpha, (float)ruleSample.b);
    return float4(colorSample.r, colorSample.g, colorSample.b, colorSample.a);
}

fragment float4
fragmentMeltShader(RasterizerData in [[stage_in]],
                   texture2d<half> colorTexture [[texture(GameTextureIndexSrc1)]],
                   texture2d<half> ruleTexture [[texture(GameTextureIndexSrc2)]])
{
    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate1);
    const half4 ruleSample = ruleTexture.sample(textureSampler, in.textureCoordinate2);
    colorSample.a = clamp((1.0 - ruleSample.b) + (in.alpha * 2.0 - 1.0), 0.0, 1.0);
    return float4(colorSample);
}

fragment float4
fragmentCrossShader(RasterizerData in [[stage_in]],
                    texture2d<half> src1Texture [[texture(GameTextureIndexSrc1)]],
                    texture2d<half> src2Texture [[texture(GameTextureIndexSrc2)]])
{
    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
    const half4 src1Sample = src1Texture.sample(textureSampler, in.textureCoordinate1);
    const half4 src2Sample = src2Texture.sample(textureSampler, in.textureCoordinate2);
    const half a = (half)in.alpha;
    return float4(src2Sample * (1.0h - a) + src1Sample * a);
}
