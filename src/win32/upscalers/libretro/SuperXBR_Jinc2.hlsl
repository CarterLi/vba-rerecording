// SuperXBR_Jinc2, pass 3 of 3 of the super-xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "custom-jinc2-sharper.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "CUSTOM_JINC2_SHARPER_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// super-xBR: the edge data, the picture at twice the size blended with the
// frame it came from, and the windowed jinc filter that finishes it.
//
//   pass 0  the edge data, at the frame size
//   pass 1  the doubled picture blended with the frame, at twice its size
//   pass 2  the jinc filter, at whatever size the target has
//
// The licence of the ported shader:
//
//   Copyright (C) 2011-2014 Hyllian/Jararaca - sergiogdb@gmail.com
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//   more details.

// The quad vertex shader (VSMain) of the renderer feeds this one, so the
// varying has to be declared the way it is there.
struct UpscalerVSOut
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

// The block the renderer hands every one of these passes, the one
// Direct3D11Display::scaleFrameWithUpscaler() fills:
//
//   SourceSize.xy: the size the pass reads, zw: one texel (1 / xy)
//   OutputSize.xy: the size of its target, zw: one pixel of it
//   Scale.xy:      OutputSize.xy / SourceSize.xy
//   SecondSize.xy: the size of the second texture it reads, zw: 1 / xy
//
cbuffer UpscalerConstants : register(b0)
{
	float4 SourceSize;
	float4 OutputSize;
	float4 Scale;
	float4 SecondSize;
};

Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 FragColor;

float d(float2 pt1, float2 pt2)
{
    float2 v = pt2 - pt1;
    return sqrt(dot(v, v));
}

float4 resampler(float4 x)
{
    float _72;
    if (x.x == 0.0f)
    {
        _72 = 3.8136150836944580078125f;
    }
    else
    {
        _72 = (sin(x.x * 1.31946885585784912109375f) * sin(x.x * 2.8902652263641357421875f)) / (x.x * x.x);
    }
    float4 res;
    res.x = _72;
    float _100;
    if (x.y == 0.0f)
    {
        _100 = 3.8136150836944580078125f;
    }
    else
    {
        _100 = (sin(x.y * 1.31946885585784912109375f) * sin(x.y * 2.8902652263641357421875f)) / (x.y * x.y);
    }
    res.y = _100;
    float _125;
    if (x.z == 0.0f)
    {
        _125 = 3.8136150836944580078125f;
    }
    else
    {
        _125 = (sin(x.z * 1.31946885585784912109375f) * sin(x.z * 2.8902652263641357421875f)) / (x.z * x.z);
    }
    res.z = _125;
    float _150;
    if (x.w == 0.0f)
    {
        _150 = 3.8136150836944580078125f;
    }
    else
    {
        _150 = (sin(x.w * 1.31946885585784912109375f) * sin(x.w * 2.8902652263641357421875f)) / (x.w * x.w);
    }
    res.w = _150;
    return res;
}

float3 min4(float3 a, float3 b, float3 c, float3 d_1)
{
    return min(a, min(b, min(c, d_1)));
}

float3 max4(float3 a, float3 b, float3 c, float3 d_1)
{
    return max(a, max(b, max(c, d_1)));
}

void frag_main()
{
    float2 dx = float2(1.0f, 0.0f);
    float2 dy = float2(0.0f, 1.0f);
    float2 pc = TEX0.xy * float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy;
    float2 tc = floor(pc - 0.5f.xx) + 0.5f.xx;
    float2 param = pc;
    float2 param_1 = (tc - dx) - dy;
    float2 param_2 = pc;
    float2 param_3 = tc - dy;
    float2 param_4 = pc;
    float2 param_5 = (tc + dx) - dy;
    float2 param_6 = pc;
    float2 param_7 = (tc + (dx * 2.0f)) - dy;
    float4 param_8 = float4(d(param, param_1), d(param_2, param_3), d(param_4, param_5), d(param_6, param_7));
    float4x4 weights;
    weights[0] = resampler(param_8);
    float2 param_9 = pc;
    float2 param_10 = tc - dx;
    float2 param_11 = pc;
    float2 param_12 = tc;
    float2 param_13 = pc;
    float2 param_14 = tc + dx;
    float2 param_15 = pc;
    float2 param_16 = tc + (dx * 2.0f);
    float4 param_17 = float4(d(param_9, param_10), d(param_11, param_12), d(param_13, param_14), d(param_15, param_16));
    weights[1] = resampler(param_17);
    float2 param_18 = pc;
    float2 param_19 = (tc - dx) + dy;
    float2 param_20 = pc;
    float2 param_21 = tc + dy;
    float2 param_22 = pc;
    float2 param_23 = (tc + dx) + dy;
    float2 param_24 = pc;
    float2 param_25 = (tc + (dx * 2.0f)) + dy;
    float4 param_26 = float4(d(param_18, param_19), d(param_20, param_21), d(param_22, param_23), d(param_24, param_25));
    weights[2] = resampler(param_26);
    float2 param_27 = pc;
    float2 param_28 = (tc - dx) + (dy * 2.0f);
    float2 param_29 = pc;
    float2 param_30 = tc + (dy * 2.0f);
    float2 param_31 = pc;
    float2 param_32 = (tc + dx) + (dy * 2.0f);
    float2 param_33 = pc;
    float2 param_34 = (tc + (dx * 2.0f)) + (dy * 2.0f);
    float4 param_35 = float4(d(param_27, param_28), d(param_29, param_30), d(param_31, param_32), d(param_33, param_34));
    weights[3] = resampler(param_35);
    dx /= float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy;
    dy /= float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy;
    tc /= float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy;
    float3 c00 = frameTex.Sample(frameSampler, (tc - dx) - dy).xyz;
    float3 c10 = frameTex.Sample(frameSampler, tc - dy).xyz;
    float3 c20 = frameTex.Sample(frameSampler, (tc + dx) - dy).xyz;
    float3 c30 = frameTex.Sample(frameSampler, (tc + (dx * 2.0f)) - dy).xyz;
    float3 c01 = frameTex.Sample(frameSampler, tc - dx).xyz;
    float3 c11 = frameTex.Sample(frameSampler, tc).xyz;
    float3 c21 = frameTex.Sample(frameSampler, tc + dx).xyz;
    float3 c31 = frameTex.Sample(frameSampler, tc + (dx * 2.0f)).xyz;
    float3 c02 = frameTex.Sample(frameSampler, (tc - dx) + dy).xyz;
    float3 c12 = frameTex.Sample(frameSampler, tc + dy).xyz;
    float3 c22 = frameTex.Sample(frameSampler, (tc + dx) + dy).xyz;
    float3 c32 = frameTex.Sample(frameSampler, (tc + (dx * 2.0f)) + dy).xyz;
    float3 c03 = frameTex.Sample(frameSampler, (tc - dx) + (dy * 2.0f)).xyz;
    float3 c13 = frameTex.Sample(frameSampler, tc + (dy * 2.0f)).xyz;
    float3 c23 = frameTex.Sample(frameSampler, (tc + dx) + (dy * 2.0f)).xyz;
    float3 c33 = frameTex.Sample(frameSampler, (tc + (dx * 2.0f)) + (dy * 2.0f)).xyz;
    float3 color = mul(weights[0], float4x3(float3(c00), float3(c10), float3(c20), float3(c30)));
    color += mul(weights[1], float4x3(float3(c01), float3(c11), float3(c21), float3(c31)));
    color += mul(weights[2], float4x3(float3(c02), float3(c12), float3(c22), float3(c32)));
    color += mul(weights[3], float4x3(float3(c03), float3(c13), float3(c23), float3(c33)));
    color /= dot(mul(weights, 1.0f.xxxx), 1.0f.xxxx).xxx;
    pc = TEX0.xy;
    c00 = frameTex.Sample(frameSampler, pc).xyz;
    c11 = frameTex.Sample(frameSampler, pc + dx).xyz;
    c21 = frameTex.Sample(frameSampler, pc - dx).xyz;
    c12 = frameTex.Sample(frameSampler, pc + dy).xyz;
    c22 = frameTex.Sample(frameSampler, pc - dy).xyz;
    float3 param_36 = c11;
    float3 param_37 = c21;
    float3 param_38 = c12;
    float3 param_39 = c22;
    float3 min_sample = min4(param_36, param_37, param_38, param_39);
    float3 param_40 = c11;
    float3 param_41 = c21;
    float3 param_42 = c12;
    float3 param_43 = c22;
    float3 max_sample = max4(param_40, param_41, param_42, param_43);
    min_sample = min(min_sample, c00);
    max_sample = max(max_sample, c00);
    float3 aux = color;
    color = clamp(color, min_sample, max_sample);
    color = lerp(aux, color, 1.0f.xxx);
    FragColor = float4(color, 1.0f);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	float2 _35 = input.uv * 1.00010001659393310546875f;
	TEX0.x = _35.x;
	TEX0.y = _35.y;

	frag_main();

	return FragColor;
}
