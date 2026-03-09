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

const char *gameShader =
        "#include <metal_stdlib>                                                                \n"
        "#include <metal_common>                                                                \n"
        "#include <simd/simd.h>                                                                 \n"
        "                                                                                       \n"
        "using namespace metal;                                                                 \n"
        "                                                                                       \n"
        "enum GameVertexInputIndex {                                                            \n"
        "    GameVertexInputIndexVertices = 0,                                                  \n"
        "};                                                                                     \n"
        "                                                                                       \n"
        "enum {                                                                                 \n"
        "    GameTextureIndexColor = 0,                                                         \n"
        "    GameTextureIndexRule = 1,                                                          \n"
        "};                                                                                     \n"
        "                                                                                       \n"
        "struct GameVertex {                                                                    \n"
        "    vector_float2 xy;                                                                  \n"
        "    vector_float2 uv;                                                                  \n"
        "    float alpha;                                                                       \n"
        "    float padding;  // This is absolutely needed for 64-bit alignments                 \n"
        "};                                                                                     \n"
        "                                                                                       \n"
        "// Vertex shader outputs and fragment shader inputs                                    \n"
        "struct RasterizerData {                                                                \n"
        "    float4 position [[position]];                                                      \n"
        "    float2 textureCoordinate;                                                          \n"
        "    float alpha;                                                                       \n"
        "};                                                                                     \n"
        "                                                                                       \n"
        "vertex RasterizerData                                                                  \n"
        "vertexShader(uint vertexID [[vertex_id]],                                              \n"
        "             constant GameVertex *vertexArray [[buffer(GameVertexInputIndexVertices)]])\n"
        "{                                                                                      \n"
        "    RasterizerData out;                                                                \n"
        "    out.position = vector_float4(vertexArray[vertexID].xy, 0, 1);                      \n"
        "    out.textureCoordinate = vertexArray[vertexID].uv;                                  \n"
        "    out.alpha = vertexArray[vertexID].alpha;                                           \n"
        "    return out;                                                                        \n"
        "}                                                                                      \n"
        "                                                                                       \n"
        "fragment float4                                                                        \n"
        "fragmentNormalShader(RasterizerData in [[stage_in]],                                   \n"
        "                     texture2d<half> colorTexture [[texture(GameTextureIndexColor)]])  \n"
        "{                                                                                      \n"
        "    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);          \n"
        "    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);  \n"
        "    return float4(colorSample.r, colorSample.g, colorSample.b, colorSample.a * in.alpha); \n"
        "}                                                                                      \n"
        "                                                                                       \n"
        "fragment float4                                                                        \n"
        "fragmentCopyShader(RasterizerData in [[stage_in]],                                     \n"
        "                   texture2d<half> colorTexture [[texture(GameTextureIndexColor)]])    \n"
        "{                                                                                      \n"
        "    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);          \n"
        "    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);     \n"
        "    colorSample.a = 1.0;                                                               \n"
        "    return float4(colorSample);                                                        \n"
        "}                                                                                      \n"
        "                                                                                       \n"
        "fragment float4                                                                        \n"
        "fragmentDimShader(RasterizerData in [[stage_in]],                                      \n"
        "                  texture2d<half> colorTexture [[texture(GameTextureIndexColor)]])     \n"
        "{                                                                                      \n"
        "    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);         \n"
        "    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);     \n"
        "    colorSample.rgb *= 0.5;                                                            \n"
        "    return float4(colorSample);                                                        \n"
        "}                                                                                      \n"
        "                                                                                       \n"
        "fragment float4                                                                        \n"
        "fragmentRuleShader(RasterizerData in [[stage_in]],                                     \n"
        "                   texture2d<half> colorTexture [[texture(GameTextureIndexColor)]],    \n"
        "                   texture2d<half> ruleTexture [[texture(GameTextureIndexRule)]])      \n"
        "{                                                                                      \n"
        "    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);         \n"
        "    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);     \n"
        "    const half4 ruleSample = ruleTexture.sample(textureSampler, in.textureCoordinate); \n"
        "    colorSample.a = 1.0 - step(in.alpha, (float)ruleSample.b);                         \n"
        "    return float4(colorSample.r, colorSample.g, colorSample.b, colorSample.a);         \n"
        "}                                                                                      \n"
        "                                                                                       \n"
        "fragment float4                                                                        \n"
        "fragmentMeltShader(RasterizerData in [[stage_in]],                                     \n"
        "                   texture2d<half> colorTexture [[texture(GameTextureIndexColor)]],    \n"
        "                   texture2d<half> ruleTexture [[texture(GameTextureIndexRule)]])      \n"
        "{                                                                                      \n"
        "    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);         \n"
        "    half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);     \n"
        "    const half4 ruleSample = ruleTexture.sample(textureSampler, in.textureCoordinate); \n"
        "    colorSample.a = clamp((1.0 - ruleSample.b) + (in.alpha * 2.0 - 1.0), 0.0, 1.0);    \n"
        "    return float4(colorSample);                                                        \n"
        "}                                                                                      \n";
