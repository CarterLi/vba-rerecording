// XBR_Mlv4_Pass3, pass 3 of 4 of the xBR-mlv4 pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-mlv4-pass3.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_MLV4_PASS3_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// xBR MultiLevel4, the fourth level of the xBR family: four passes of edge
// data before the picture is doubled and interpolated.
//
//   pass 1  the level 1 metrics, at the frame size
//   pass 2  the level 2 metrics, on top of them
//   pass 3  the picture doubled, from both
//   pass 4  the interpolation, at whatever size the target has
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

static const float3 _598[13] = { 4.0f.xxx, float3(4.0f, 4.0f, 3.0f), float3(4.0f, 4.0f, 2.0f), float3(8.0f, 4.0f, 2.0f), float3(4.0f, 8.0f, 2.0f), float3(12.0f, 4.0f, 2.0f), float3(4.0f, 12.0f, 2.0f), float3(16.0f, 4.0f, 2.0f), float3(4.0f, 16.0f, 2.0f), float3(12.0f, 4.0f, 6.0f), float3(4.0f, 12.0f, 6.0f), float3(16.0f, 4.0f, 6.0f), float3(4.0f, 16.0f, 6.0f) };

Texture2D<float4> prev3Tex : register(t1);
SamplerState prev3Sampler : register(s1);
Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 t1;
static float4 FragColor;

float remapFrom01(float v, float high)
{
    return floor((high * v) + 0.5f);
}

float df(float A, float B)
{
    return abs(A - B);
}

float4 remapTo01(float4 v, float4 low, float4 high)
{
    return clamp((v - low) / (high - low), 0.0f.xxxx, 1.0f.xxxx);
}

void frag_main()
{
    float2 pos = frac(TEX0.xy * SecondSize.xy) - 0.5f.xx;
    float2 dir = sign(pos);
    float2 g1 = dir * ((t1.zw * clamp((-dir.y) * dir.x, 0.0f, 1.0f)) + (t1.xy * clamp(dir.y * dir.x, 0.0f, 1.0f)));
    float2 g2 = dir * ((t1.zw * clamp(dir.y * dir.x, 0.0f, 1.0f)) + (t1.xy * clamp((-dir.y) * dir.x, 0.0f, 1.0f)));
    float3 E = prev3Tex.Sample(prev3Sampler, TEX0.xy).xyz;
    float3 F = prev3Tex.Sample(prev3Sampler, TEX0.xy + g1).xyz;
    float3 H = prev3Tex.Sample(prev3Sampler, TEX0.xy + g2).xyz;
    float3 I = prev3Tex.Sample(prev3Sampler, (TEX0.xy + g1) + g2).xyz;
    float3 F4 = prev3Tex.Sample(prev3Sampler, TEX0.xy + (g1 * 2.0f)).xyz;
    float3 H5 = prev3Tex.Sample(prev3Sampler, TEX0.xy + (g2 * 2.0f)).xyz;
    float e = dot(E, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float f = dot(F, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float h = dot(H, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float i = dot(I, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float f4 = dot(F4, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float h5 = dot(H5, float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 icomp = floor(clamp(mul(dir, float2x4(float4(1.0f, 1.0f, -1.0f, -1.0f), float4(1.0f, -1.0f, -1.0f, 1.0f))), 0.0f.xxxx, 1.0f.xxxx) + 0.5f.xxxx);
    float param = dot(frameTex.Sample(frameSampler, TEX0.xy), icomp);
    float param_1 = 255.0f;
    float infoE = remapFrom01(param, param_1);
    float param_2 = dot(frameTex.Sample(frameSampler, TEX0.xy + g1), icomp);
    float param_3 = 255.0f;
    float infoF = remapFrom01(param_2, param_3);
    float param_4 = dot(frameTex.Sample(frameSampler, TEX0.xy + g2), icomp);
    float param_5 = 255.0f;
    float infoH = remapFrom01(param_4, param_5);
    float4 lparam;
    float px;
    float2 addr;
    if (infoF == 8.0f)
    {
        lparam.x = 4.0f;
        lparam.y = 16.0f;
        lparam.z = 6.0f;
        float param_6 = f;
        float param_7 = f4;
        float param_8 = f;
        float param_9 = i;
        px = float(df(param_6, param_7) <= df(param_8, param_9));
        addr.x = (2.0f * px) + clamp(1.0f - px, 0.0f, 1.0f);
        addr.y = clamp(1.0f - px, 0.0f, 1.0f);
    }
    else
    {
        if (infoH == 7.0f)
        {
            lparam.x = 16.0f;
            lparam.y = 4.0f;
            lparam.z = 6.0f;
            float param_10 = h;
            float param_11 = h5;
            float param_12 = h;
            float param_13 = i;
            px = float(df(param_10, param_11) <= df(param_12, param_13));
            addr.x = clamp(1.0f - px, 0.0f, 1.0f);
            addr.y = (2.0f * px) + clamp(1.0f - px, 0.0f, 1.0f);
        }
        else
        {
            if (infoF == 6.0f)
            {
                lparam.x = 4.0f;
                lparam.y = 12.0f;
                lparam.z = 6.0f;
                float param_14 = f;
                float param_15 = f4;
                float param_16 = f;
                float param_17 = i;
                px = float(df(param_14, param_15) <= df(param_16, param_17));
                addr.x = (2.0f * px) + clamp(1.0f - px, 0.0f, 1.0f);
                addr.y = clamp(1.0f - px, 0.0f, 1.0f);
            }
            else
            {
                if (infoH == 5.0f)
                {
                    lparam.x = 12.0f;
                    lparam.y = 4.0f;
                    lparam.z = 6.0f;
                    float param_18 = h;
                    float param_19 = h5;
                    float param_20 = h;
                    float param_21 = i;
                    px = float(df(param_18, param_19) <= df(param_20, param_21));
                    addr.x = clamp(1.0f - px, 0.0f, 1.0f);
                    addr.y = (2.0f * px) + clamp(1.0f - px, 0.0f, 1.0f);
                }
                else
                {
                    float param_22 = e;
                    float param_23 = f;
                    float param_24 = e;
                    float param_25 = h;
                    px = float(df(param_22, param_23) <= df(param_24, param_25));
                    addr.x = px;
                    addr.y = clamp(1.0f - px, 0.0f, 1.0f);
                    float3 _421;
                    if (infoE == 8.0f)
                    {
                        _421 = float3(4.0f, 16.0f, 2.0f);
                    }
                    else
                    {
                        float3 _428;
                        if (infoE == 7.0f)
                        {
                            _428 = float3(16.0f, 4.0f, 2.0f);
                        }
                        else
                        {
                            float3 _435;
                            if (infoE == 6.0f)
                            {
                                _435 = float3(4.0f, 12.0f, 2.0f);
                            }
                            else
                            {
                                float3 _442;
                                if (infoE == 5.0f)
                                {
                                    _442 = float3(12.0f, 4.0f, 2.0f);
                                }
                                else
                                {
                                    float3 _449;
                                    if (infoE == 4.0f)
                                    {
                                        _449 = float3(4.0f, 8.0f, 2.0f);
                                    }
                                    else
                                    {
                                        float3 _457;
                                        if (infoE == 3.0f)
                                        {
                                            _457 = float3(8.0f, 4.0f, 2.0f);
                                        }
                                        else
                                        {
                                            float3 _464;
                                            if (infoE == 2.0f)
                                            {
                                                _464 = float3(4.0f, 4.0f, 2.0f);
                                            }
                                            else
                                            {
                                                bool3 _474 = (infoE == 1.0f).xxx;
                                                _464 = float3(_474.x ? float3(4.0f, 4.0f, 3.0f).x : 4.0f.xxx.x, _474.y ? float3(4.0f, 4.0f, 3.0f).y : 4.0f.xxx.y, _474.z ? float3(4.0f, 4.0f, 3.0f).z : 4.0f.xxx.z);
                                            }
                                            _457 = _464;
                                        }
                                        _449 = _457;
                                    }
                                    _442 = _449;
                                }
                                _435 = _442;
                            }
                            _428 = _435;
                        }
                        _421 = _428;
                    }
                    lparam.x = _421.x;
                    lparam.y = _421.y;
                    lparam.z = _421.z;
                }
            }
        }
    }
    bool inv = ((dir.x * dir.y) < 0.0f) ? true : false;
    addr *= dir.yx;
    float2 _505;
    if (inv)
    {
        _505 = addr.yx;
    }
    else
    {
        _505 = addr;
    }
    addr = _505;
    float4 _513 = lparam;
    float2 _517 = _513.xy * dir.yx;
    lparam.x = _517.x;
    lparam.y = _517.y;
    float2 _523;
    if (inv)
    {
        _523 = lparam.yx;
    }
    else
    {
        _523 = lparam.xy;
    }
    lparam.x = _523.x;
    lparam.y = _523.y;
    addr.x += 2.0f;
    addr.y += 2.0f;
    lparam.w = (addr.x * 8.0f) + addr.y;
    float4 param_26 = lparam;
    float4 param_27 = (-64.0f).xxxx;
    float4 param_28 = 64.0f.xxxx;
    FragColor = remapTo01(param_26, param_27, param_28);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	TEX0.x = input.uv.x;
	TEX0.y = input.uv.y;
	float2 ps = float2(1.0f / SecondSize.xy.x, 1.0f / SecondSize.xy.y);
	float dx = ps.x;
	float dy = ps.y;
	t1 = float4(dx, 0.0f, 0.0f, dy);

	frag_main();

	return FragColor;
}
