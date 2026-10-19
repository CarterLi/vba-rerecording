// XBR_Mlv4_Pass4, pass 4 of 4 of the xBR-mlv4 pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-mlv4-pass4.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_MLV4_PASS4_GLSL"; this is the port of it, because Direct3D
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

struct ResType
{
    float _m0;
    float _m1;
};

Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);
Texture2D<float4> prev4Tex : register(t1);
SamplerState prev4Sampler : register(s1);

static float4 TEX0;
static float4 t1;
static float scale_factor;
static float4 FragColor;

float4 remapFrom01(float4 v, float4 low, float4 high)
{
    return floor(lerp(low, high, v) + 0.5f.xxxx);
}

float c_df(float3 c1, float3 c2)
{
    float3 df = abs(c1 - c2);
    return (df.x + df.y) + df.z;
}

void frag_main()
{
    float2 fp = frac(TEX0.xy * SecondSize.xy) - 0.5f.xx;
    float2 pxcoord = floor(TEX0.xy * SecondSize.xy) / SecondSize.xy;
    float4 UL = frameTex.Sample(frameSampler, (pxcoord + (t1.xy * 0.25f)) + (t1.zw * 0.25f));
    float4 UR = frameTex.Sample(frameSampler, (pxcoord + (t1.xy * 0.75f)) + (t1.zw * 0.25f));
    float4 DL = frameTex.Sample(frameSampler, (pxcoord + (t1.xy * 0.25f)) + (t1.zw * 0.75f));
    float4 DR = frameTex.Sample(frameSampler, (pxcoord + (t1.xy * 0.75f)) + (t1.zw * 0.75f));
    float4 param = UL;
    float4 param_1 = (-64.0f).xxxx;
    float4 param_2 = 64.0f.xxxx;
    float4 ulparam = remapFrom01(param, param_1, param_2);
    float4 param_3 = UR;
    float4 param_4 = (-64.0f).xxxx;
    float4 param_5 = 64.0f.xxxx;
    float4 urparam = remapFrom01(param_3, param_4, param_5);
    float4 param_6 = DL;
    float4 param_7 = (-64.0f).xxxx;
    float4 param_8 = 64.0f.xxxx;
    float4 dlparam = remapFrom01(param_6, param_7, param_8);
    float4 param_9 = DR;
    float4 param_10 = (-64.0f).xxxx;
    float4 param_11 = 64.0f.xxxx;
    float4 drparam = remapFrom01(param_9, param_10, param_11);
    float3 E = prev4Tex.Sample(prev4Sampler, TEX0.xy).xyz;
    float info = ulparam.w;
    ResType _173;
    _173._m0 = modf(info / 1.99989998340606689453125f, _173._m1);
    info = _173._m1;
    float3 ay;
    ay.z = floor(_173._m0 + 0.5f);
    ResType _181;
    _181._m0 = modf(info / 1.99989998340606689453125f, _181._m1);
    info = _181._m1;
    ay.y = floor(_181._m0 + 0.5f);
    ResType _189;
    _189._m0 = modf(info / 1.99989998340606689453125f, _189._m1);
    info = _189._m1;
    ay.x = floor(_189._m0 + 0.5f);
    ResType _198;
    _198._m0 = modf(info / 1.99989998340606689453125f, _198._m1);
    info = _198._m1;
    float3 ax;
    ax.z = floor(_198._m0 + 0.5f);
    ResType _206;
    _206._m0 = modf(info / 1.99989998340606689453125f, _206._m1);
    info = _206._m1;
    ax.y = floor(_206._m0 + 0.5f);
    ax.x = floor(info + 0.5f);
    float2 iq;
    iq.x = dot(ax, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    iq.y = dot(ay, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    float3 PX = prev4Tex.Sample(prev4Sampler, (TEX0.xy + (t1.xy * iq.x)) + (t1.zw * iq.y)).xyz;
    info = urparam.w;
    ResType _251;
    _251._m0 = modf(info / 1.99989998340606689453125f, _251._m1);
    info = _251._m1;
    ay.z = floor(_251._m0 + 0.5f);
    ResType _259;
    _259._m0 = modf(info / 1.99989998340606689453125f, _259._m1);
    info = _259._m1;
    ay.y = floor(_259._m0 + 0.5f);
    ResType _267;
    _267._m0 = modf(info / 1.99989998340606689453125f, _267._m1);
    info = _267._m1;
    ay.x = floor(_267._m0 + 0.5f);
    ResType _275;
    _275._m0 = modf(info / 1.99989998340606689453125f, _275._m1);
    info = _275._m1;
    ax.z = floor(_275._m0 + 0.5f);
    ResType _283;
    _283._m0 = modf(info / 1.99989998340606689453125f, _283._m1);
    info = _283._m1;
    ax.y = floor(_283._m0 + 0.5f);
    ax.x = floor(info + 0.5f);
    iq.x = dot(ax, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    iq.y = dot(ay, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    float3 PY = prev4Tex.Sample(prev4Sampler, (TEX0.xy + (t1.xy * iq.x)) + (t1.zw * iq.y)).xyz;
    info = dlparam.w;
    ResType _323;
    _323._m0 = modf(info / 1.99989998340606689453125f, _323._m1);
    info = _323._m1;
    ay.z = floor(_323._m0 + 0.5f);
    ResType _331;
    _331._m0 = modf(info / 1.99989998340606689453125f, _331._m1);
    info = _331._m1;
    ay.y = floor(_331._m0 + 0.5f);
    ResType _339;
    _339._m0 = modf(info / 1.99989998340606689453125f, _339._m1);
    info = _339._m1;
    ay.x = floor(_339._m0 + 0.5f);
    ResType _347;
    _347._m0 = modf(info / 1.99989998340606689453125f, _347._m1);
    info = _347._m1;
    ax.z = floor(_347._m0 + 0.5f);
    ResType _355;
    _355._m0 = modf(info / 1.99989998340606689453125f, _355._m1);
    info = _355._m1;
    ax.y = floor(_355._m0 + 0.5f);
    ax.x = floor(info + 0.5f);
    iq.x = dot(ax, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    iq.y = dot(ay, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    float3 PZ = prev4Tex.Sample(prev4Sampler, (TEX0.xy + (t1.xy * iq.x)) + (t1.zw * iq.y)).xyz;
    info = drparam.w;
    ResType _395;
    _395._m0 = modf(info / 1.99989998340606689453125f, _395._m1);
    info = _395._m1;
    ay.z = floor(_395._m0 + 0.5f);
    ResType _403;
    _403._m0 = modf(info / 1.99989998340606689453125f, _403._m1);
    info = _403._m1;
    ay.y = floor(_403._m0 + 0.5f);
    ResType _411;
    _411._m0 = modf(info / 1.99989998340606689453125f, _411._m1);
    info = _411._m1;
    ay.x = floor(_411._m0 + 0.5f);
    ResType _419;
    _419._m0 = modf(info / 1.99989998340606689453125f, _419._m1);
    info = _419._m1;
    ax.z = floor(_419._m0 + 0.5f);
    ResType _427;
    _427._m0 = modf(info / 1.99989998340606689453125f, _427._m1);
    info = _427._m1;
    ax.y = floor(_427._m0 + 0.5f);
    ax.x = floor(info + 0.5f);
    iq.x = dot(ax, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    iq.y = dot(ay, float3(4.0f, 2.0f, 1.0f)) - 1.99989998340606689453125f;
    float3 PW = prev4Tex.Sample(prev4Sampler, (TEX0.xy + (t1.xy * iq.x)) + (t1.zw * iq.y)).xyz;
    float3 fp1 = float3(fp, -1.0f);
    float4 inc = float4(abs(ulparam.x / ulparam.y), abs(urparam.x / urparam.y), abs(dlparam.x / dlparam.y), abs(drparam.x / drparam.y));
    float4 level = max(inc, 1.0f.xxxx / inc);
    float4 fx;
    fx.x = clamp(((dot(fp1, ulparam.xyz) * scale_factor) / (8.0f * level.x)) + 0.5f, 0.0f, 1.0f);
    fx.y = clamp(((dot(fp1, urparam.xyz) * scale_factor) / (8.0f * level.y)) + 0.5f, 0.0f, 1.0f);
    fx.z = clamp(((dot(fp1, dlparam.xyz) * scale_factor) / (8.0f * level.z)) + 0.5f, 0.0f, 1.0f);
    fx.w = clamp(((dot(fp1, drparam.xyz) * scale_factor) / (8.0f * level.w)) + 0.5f, 0.0f, 1.0f);
    float3 c1 = lerp(E, PX, fx.x.xxx);
    float3 c2 = lerp(E, PY, fx.y.xxx);
    float3 c3 = lerp(E, PZ, fx.z.xxx);
    float3 c4 = lerp(E, PW, fx.w.xxx);
    float3 color = c1;
    float3 param_12 = c2;
    float3 param_13 = E;
    float3 param_14 = color;
    float3 param_15 = E;
    bool3 _603 = (c_df(param_12, param_13) > c_df(param_14, param_15)).xxx;
    color = float3(_603.x ? c2.x : color.x, _603.y ? c2.y : color.y, _603.z ? c2.z : color.z);
    float3 param_16 = c3;
    float3 param_17 = E;
    float3 param_18 = color;
    float3 param_19 = E;
    bool3 _618 = (c_df(param_16, param_17) > c_df(param_18, param_19)).xxx;
    color = float3(_618.x ? c3.x : color.x, _618.y ? c3.y : color.y, _618.z ? c3.z : color.z);
    float3 param_20 = c4;
    float3 param_21 = E;
    float3 param_22 = color;
    float3 param_23 = E;
    bool3 _633 = (c_df(param_20, param_21) > c_df(param_22, param_23)).xxx;
    color = float3(_633.x ? c4.x : color.x, _633.y ? c4.y : color.y, _633.z ? c4.z : color.z);
    FragColor = float4(color, 1.0f);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	float2 _34 = input.uv * float2(1.00039994716644287109375f, 0.999499976634979248046875f);
	TEX0.x = _34.x;
	TEX0.y = _34.y;
	float2 ps = float2(1.0f / SecondSize.xy.x, 1.0f / SecondSize.xy.y);
	float dx = ps.x;
	float dy = ps.y;
	t1 = float4(dx, 0.0f, 0.0f, dy);
	scale_factor = OutputSize.xy.x / SecondSize.xy.x;

	frag_main();

	return FragColor;
}
