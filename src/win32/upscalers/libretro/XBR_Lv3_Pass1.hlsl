// XBR_Lv3_Pass1, pass 2 of 2 of the xBR lv3 Multipass pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-lv3-pass1.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_LV3_PASS1_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// xBR lv3, split in two passes.  The single pass xBR_Lv3.hlsl of this folder
// does the same thing in one go; this is the preset of the libretro shader,
// which hands the edge data over in a frame of its own so that the second
// pass only has to interpolate.
//
//   pass 0  the angle and the direction of every edge, at the frame size
//   pass 1  the interpolation, at whatever size the target has
//
// The licence of the ported shader:
//
//   Copyright (C) 2011-2015 Hyllian - sergiogdb@gmail.com
//
//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files (the "Software"), to
//   deal in the Software without restriction, including without limitation the
//   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//   sell copies of the Software, and to permit persons to whom the Software is
//   furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//   IN THE SOFTWARE.

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

struct ResType
{
    float _m0;
    float _m1;
};

static const float3 _563[12] = { float3(1.0f, 1.0f, 0.75f), float3(1.0f, 1.0f, 0.5f), float3(2.0f, 1.0f, 0.5f), float3(1.0f, 2.0f, 0.5f), float3(3.0f, 1.0f, 0.5f), float3(1.0f, 3.0f, 0.5f), float3(-1.0f, 2.0f, 0.5f), float3(2.0f, -1.0f, 0.5f), float3(-1.0f, 3.0f, 0.5f), float3(3.0f, -1.0f, 0.5f), float3(3.0f, 1.0f, 1.5f), float3(1.0f, 3.0f, 1.5f) };

Texture2D<float4> origTex : register(t1);
SamplerState origSampler : register(s1);
Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 t1;
static float2 delta;
static float4 FragColor;

float remapFrom01(float v, float high)
{
    return (high * v) + 0.5f;
}

float4 unpack_info(inout float i)
{
    ResType _31;
    _31._m0 = modf(i / 2.0f, _31._m1);
    i = _31._m1;
    float4 info;
    info.x = floor(_31._m0 + 0.5f);
    ResType _41;
    _41._m0 = modf(i / 2.0f, _41._m1);
    i = _41._m1;
    info.y = floor(_41._m0 + 0.5f);
    ResType _50;
    _50._m0 = modf(i / 2.0f, _50._m1);
    i = _50._m1;
    info.z = floor(_50._m0 + 0.5f);
    info.w = i;
    return info;
}

void frag_main()
{
    float2 pos = frac(TEX0.xy * float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy) - 0.5f.xx;
    float2 dir = sign(pos);
    float2 g1 = dir * ((t1.zw * clamp((-dir.y) * dir.x, 0.0f, 1.0f)) + (t1.xy * clamp(dir.y * dir.x, 0.0f, 1.0f)));
    float2 g2 = dir * ((t1.zw * clamp(dir.y * dir.x, 0.0f, 1.0f)) + (t1.xy * clamp((-dir.y) * dir.x, 0.0f, 1.0f)));
    float3 F = origTex.Sample(origSampler, TEX0.xy + g1).xyz;
    float3 B = origTex.Sample(origSampler, TEX0.xy - g2).xyz;
    float3 D = origTex.Sample(origSampler, TEX0.xy - g1).xyz;
    float3 H = origTex.Sample(origSampler, TEX0.xy + g2).xyz;
    float3 E = origTex.Sample(origSampler, TEX0.xy).xyz;
    float3 F4 = origTex.Sample(origSampler, TEX0.xy + (g1 * 2.0f)).xyz;
    float3 I = origTex.Sample(origSampler, (TEX0.xy + g1) + g2).xyz;
    float3 H5 = origTex.Sample(origSampler, TEX0.xy + (g2 * 2.0f)).xyz;
    float4 icomp = floor(clamp(mul(dir, float2x4(float4(1.0f, 1.0f, -1.0f, -1.0f), float4(1.0f, -1.0f, -1.0f, 1.0f))), 0.0f.xxxx, 1.0f.xxxx) + 0.5f.xxxx);
    float param = dot(frameTex.Sample(frameSampler, TEX0.xy), icomp);
    float param_1 = 255.0f;
    float info = remapFrom01(param, param_1);
    float param_2 = dot(frameTex.Sample(frameSampler, TEX0.xy + g1), icomp);
    float param_3 = 255.0f;
    float info_nr = remapFrom01(param_2, param_3);
    float param_4 = dot(frameTex.Sample(frameSampler, TEX0.xy + g2), icomp);
    float param_5 = 255.0f;
    float info_nd = remapFrom01(param_4, param_5);
    ResType _262;
    _262._m0 = modf(info / 2.0f, _262._m1);
    info = _262._m1;
    ResType _267;
    _267._m0 = modf(info / 2.0f, _267._m1);
    info = _267._m1;
    ResType _273;
    _273._m0 = modf(info / 2.0f, _273._m1);
    info = _273._m1;
    float2 px;
    px.x = floor(_273._m0 + 0.5f);
    ResType _281;
    _281._m0 = modf(info / 2.0f, _281._m1);
    info = _281._m1;
    px.y = floor(_281._m0 + 0.5f);
    float param_6 = info;
    float4 _290 = unpack_info(param_6);
    float4 flags = _290;
    ResType _294;
    _294._m0 = modf(info_nr / 2.0f, _294._m1);
    info_nr = _294._m1;
    float edr3_nrl = floor(_294._m0 + 0.5f);
    ResType _301;
    _301._m0 = modf(info_nr / 2.0f, _301._m1);
    info_nr = _301._m1;
    ResType _306;
    _306._m0 = modf(info_nr / 2.0f, _306._m1);
    info_nr = _306._m1;
    ResType _312;
    _312._m0 = modf(info_nr / 2.0f, _312._m1);
    info_nr = _312._m1;
    float pxr = floor(_312._m0 + 0.5f);
    ResType _319;
    _319._m0 = modf(info_nd / 2.0f, _319._m1);
    info_nd = _319._m1;
    ResType _325;
    _325._m0 = modf(info_nd / 2.0f, _325._m1);
    info_nd = _325._m1;
    float edr3_ndu = floor(_325._m0 + 0.5f);
    ResType _332;
    _332._m0 = modf(info_nd / 2.0f, _332._m1);
    info_nd = _332._m1;
    ResType _338;
    _338._m0 = modf(info_nd / 2.0f, _338._m1);
    info_nd = _338._m1;
    float pxd = floor(_338._m0 + 0.5f);
    float aux = floor(dot(float4(8.0f, 4.0f, 2.0f, 1.0f), flags) + 0.5f);
    float3 slep;
    if (aux >= 6.0f)
    {
        float3 _360;
        if (aux == 6.0f)
        {
            _360 = float3(-1.0f, 2.0f, 0.5f);
        }
        else
        {
            float3 _368;
            if (aux == 7.0f)
            {
                _368 = float3(2.0f, -1.0f, 0.5f);
            }
            else
            {
                float3 _375;
                if (aux == 8.0f)
                {
                    _375 = float3(-1.0f, 3.0f, 0.5f);
                }
                else
                {
                    float3 _384;
                    if (aux == 9.0f)
                    {
                        _384 = float3(3.0f, -1.0f, 0.5f);
                    }
                    else
                    {
                        bool3 _396 = (aux == 10.0f).xxx;
                        _384 = float3(_396.x ? float3(3.0f, 1.0f, 1.5f).x : float3(1.0f, 3.0f, 1.5f).x, _396.y ? float3(3.0f, 1.0f, 1.5f).y : float3(1.0f, 3.0f, 1.5f).y, _396.z ? float3(3.0f, 1.0f, 1.5f).z : float3(1.0f, 3.0f, 1.5f).z);
                    }
                    _375 = _384;
                }
                _368 = _375;
            }
            _360 = _368;
        }
        slep = _360;
    }
    else
    {
        float3 _405;
        if (aux == 0.0f)
        {
            _405 = float3(1.0f, 1.0f, 0.75f);
        }
        else
        {
            float3 _413;
            if (aux == 1.0f)
            {
                _413 = float3(1.0f, 1.0f, 0.5f);
            }
            else
            {
                float3 _420;
                if (aux == 2.0f)
                {
                    _420 = float3(2.0f, 1.0f, 0.5f);
                }
                else
                {
                    float3 _427;
                    if (aux == 3.0f)
                    {
                        _427 = float3(1.0f, 2.0f, 0.5f);
                    }
                    else
                    {
                        bool3 _436 = (aux == 4.0f).xxx;
                        _427 = float3(_436.x ? float3(3.0f, 1.0f, 0.5f).x : float3(1.0f, 3.0f, 0.5f).x, _436.y ? float3(3.0f, 1.0f, 0.5f).y : float3(1.0f, 3.0f, 0.5f).y, _436.z ? float3(3.0f, 1.0f, 0.5f).z : float3(1.0f, 3.0f, 0.5f).z);
                    }
                    _420 = _427;
                }
                _413 = _420;
            }
            _405 = _413;
        }
        slep = _405;
    }
    float2 _449;
    if ((dir.x * dir.y) > 0.0f)
    {
        _449 = abs(pos);
    }
    else
    {
        _449 = abs(pos.yx);
    }
    float2 fp = _449;
    float3 fp1 = float3(fp.yx, -1.0f);
    float3 color = E;
    float fx;
    if (aux < 10.0f)
    {
        fx = clamp((dot(fp1, slep) / (2.0f * delta.x)) + 0.5f, 0.0f, 1.0f);
        color = lerp(E, lerp(lerp(H, F, px.y.xxx), lerp(D, B, px.y.xxx), px.x.xxx), fx.xxx);
    }
    else
    {
        if (edr3_nrl == 1.0f)
        {
            fx = clamp((dot(fp1, float3(3.0f, 1.0f, 1.5f)) / (2.0f * delta.x)) + 0.5f, 0.0f, 1.0f);
            color = lerp(E, lerp(I, F4, pxr.xxx), fx.xxx);
        }
        else
        {
            if (edr3_ndu == 1.0f)
            {
                fx = clamp((dot(fp1, float3(1.0f, 3.0f, 1.5f)) / (2.0f * delta.x)) + 0.5f, 0.0f, 1.0f);
                color = lerp(E, lerp(H5, I, pxd.xxx), fx.xxx);
            }
        }
    }
    FragColor = float4(color, 1.0f);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	float2 _32 = input.uv * 1.00039994716644287109375f;
	TEX0.x = _32.x;
	TEX0.y = _32.y;
	float2 ps = float2(1.0f / SecondSize.xy.x, 1.0f / SecondSize.xy.y);
	float dx = ps.x;
	float dy = ps.y;
	t1 = float4(dx, 0.0f, 0.0f, dy);
	delta = float2(SourceSize.xy.x / OutputSize.xy.x, (0.5f * SourceSize.xy.x) / OutputSize.xy.x);

	frag_main();

	return FragColor;
}
