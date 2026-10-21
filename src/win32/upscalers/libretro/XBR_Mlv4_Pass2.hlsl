// XBR_Mlv4_Pass2, pass 2 of 4 of the xBR-mlv4 pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-mlv4-pass2.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_MLV4_PASS2_GLSL"; this is the port of it, because Direct3D
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

Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);
Texture2D<float4> prev2Tex : register(t1);
SamplerState prev2Sampler : register(s1);

static float4 t2;
static float4 t3;
static float4 t4;
static float4 t1;
static float4 t5;
static float4 t6;
static float4 t7;
static float4 FragColor;
static float4 TEX0;

float4 remapFrom01(float4 v, float4 high)
{
    return floor((high * v) + 0.5f.xxxx);
}

float4 df(float4 A, float4 B)
{
    return abs(A - B);
}

bool4 id(float4 A, float4 B, float4 C, float4 D)
{
    float4 param = C;
    float4 param_1 = D;
    float4 _66 = df(param, param_1);
    float4 param_2 = A;
    float4 param_3 = B;
    float4 _71 = df(param_2, param_3);
    return bool4(_66.x > _71.x, _66.y > _71.y, _66.z > _71.z, _66.w > _71.w);
}

bool4 rd(float4 A, float4 B, float4 C, float4 D)
{
    float4 param = C;
    float4 param_1 = D;
    float4 param_2 = A;
    float4 param_3 = B;
    float4 _56 = df(param, param_1) / (df(param_2, param_3) + 9.999999717180685365747194737196e-10f.xxxx);
    return bool4(_56.x > 2.0f.xxxx.x, _56.y > 2.0f.xxxx.y, _56.z > 2.0f.xxxx.z, _56.w > 2.0f.xxxx.w);
}

float4 remapTo01(float4 v, float4 high)
{
    return v / high;
}

void frag_main()
{
    float4 PA = frameTex.Sample(frameSampler, t2.xw);
    float4 PB = frameTex.Sample(frameSampler, t2.yw);
    float4 PC = frameTex.Sample(frameSampler, t2.zw);
    float4 PD = frameTex.Sample(frameSampler, t3.xw);
    float4 PE = frameTex.Sample(frameSampler, t3.yw);
    float4 PF = frameTex.Sample(frameSampler, t3.zw);
    float4 PG = frameTex.Sample(frameSampler, t4.xw);
    float4 PH = frameTex.Sample(frameSampler, t4.yw);
    float4 PI = frameTex.Sample(frameSampler, t4.zw);
    float3 A1 = prev2Tex.Sample(prev2Sampler, t1.xw).xyz;
    float3 B1 = prev2Tex.Sample(prev2Sampler, t1.yw).xyz;
    float3 C1 = prev2Tex.Sample(prev2Sampler, t1.zw).xyz;
    float3 A = prev2Tex.Sample(prev2Sampler, t2.xw).xyz;
    float3 B = prev2Tex.Sample(prev2Sampler, t2.yw).xyz;
    float3 C = prev2Tex.Sample(prev2Sampler, t2.zw).xyz;
    float3 D = prev2Tex.Sample(prev2Sampler, t3.xw).xyz;
    float3 E = prev2Tex.Sample(prev2Sampler, t3.yw).xyz;
    float3 F = prev2Tex.Sample(prev2Sampler, t3.zw).xyz;
    float3 G = prev2Tex.Sample(prev2Sampler, t4.xw).xyz;
    float3 H = prev2Tex.Sample(prev2Sampler, t4.yw).xyz;
    float3 I = prev2Tex.Sample(prev2Sampler, t4.zw).xyz;
    float3 G5 = prev2Tex.Sample(prev2Sampler, t5.xw).xyz;
    float3 H5 = prev2Tex.Sample(prev2Sampler, t5.yw).xyz;
    float3 I5 = prev2Tex.Sample(prev2Sampler, t5.zw).xyz;
    float3 A0 = prev2Tex.Sample(prev2Sampler, t6.xy).xyz;
    float3 D0 = prev2Tex.Sample(prev2Sampler, t6.xz).xyz;
    float3 G0 = prev2Tex.Sample(prev2Sampler, t6.xw).xyz;
    float3 C4 = prev2Tex.Sample(prev2Sampler, t7.xy).xyz;
    float3 F4 = prev2Tex.Sample(prev2Sampler, t7.xz).xyz;
    float3 I4 = prev2Tex.Sample(prev2Sampler, t7.xw).xyz;
    float4 b = mul(float4x3(float3(B), float3(D), float3(H), float3(F)), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 c = mul(float4x3(float3(C), float3(A), float3(G), float3(I)), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 e = mul(float4x3(E, E, E, E), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 d = b.yzwx;
    float4 f = b.wxyz;
    float4 g = c.zwxy;
    float4 h = b.zwxy;
    float4 i = c.wxyz;
    float4 i4 = mul(float4x3(float3(I4), float3(C1), float3(A0), float3(G5)), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 i5 = mul(float4x3(float3(I5), float3(C4), float3(A1), float3(G0)), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 h5 = mul(float4x3(float3(H5), float3(F4), float3(B1), float3(D0)), float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f));
    float4 f4 = h5.yzwx;
    float4 param = PE;
    float4 param_1 = 255.0f.xxxx;
    float4 pe = remapFrom01(param, param_1);
    float4 param_2 = PF;
    float4 param_3 = 255.0f.xxxx;
    float4 pf = remapFrom01(param_2, param_3);
    float4 param_4 = PH;
    float4 param_5 = 255.0f.xxxx;
    float4 ph = remapFrom01(param_4, param_5);
    float4 param_6 = PB;
    float4 param_7 = 255.0f.xxxx;
    float4 pb = remapFrom01(param_6, param_7);
    float4 param_8 = PD;
    float4 param_9 = 255.0f.xxxx;
    float4 pd = remapFrom01(param_8, param_9);
    float4 f2 = float4(pf.z, pb.w, pd.x, ph.y);
    float4 h2 = float4(ph.z, pf.w, pb.x, pd.y);
    float4 f1 = float4(pf.y, pb.z, pd.w, ph.x);
    float4 h3 = float4(ph.w, pf.x, pb.y, pd.z);
    bool _497 = pe.y > 1.0f;
    bool _504;
    if (!_497)
    {
        _504 = pe.w > 1.0f;
    }
    else
    {
        _504 = _497;
    }
    bool4 nbrs;
    nbrs.x = _504;
    bool _509 = pe.z > 1.0f;
    bool _516;
    if (!_509)
    {
        _516 = pe.x > 1.0f;
    }
    else
    {
        _516 = _509;
    }
    nbrs.y = _516;
    bool _520 = pe.w > 1.0f;
    bool _527;
    if (!_520)
    {
        _527 = pe.y > 1.0f;
    }
    else
    {
        _527 = _520;
    }
    nbrs.z = _527;
    bool _531 = pe.x > 1.0f;
    bool _538;
    if (!_531)
    {
        _538 = pe.z > 1.0f;
    }
    else
    {
        _538 = _531;
    }
    nbrs.w = _538;
    bool _543 = f2.x > 1.0f;
    bool _550;
    if (!_543)
    {
        _550 = h2.x > 1.0f;
    }
    else
    {
        _550 = _543;
    }
    bool4 jag1;
    jag1.x = _550;
    bool _554 = f2.y > 1.0f;
    bool _561;
    if (!_554)
    {
        _561 = h2.y > 1.0f;
    }
    else
    {
        _561 = _554;
    }
    jag1.y = _561;
    bool _565 = f2.z > 1.0f;
    bool _572;
    if (!_565)
    {
        _572 = h2.z > 1.0f;
    }
    else
    {
        _572 = _565;
    }
    jag1.z = _572;
    bool _576 = f2.w > 1.0f;
    bool _583;
    if (!_576)
    {
        _583 = h2.w > 1.0f;
    }
    else
    {
        _583 = _576;
    }
    jag1.w = _583;
    bool _588 = f2.x > 2.0f;
    bool _595;
    if (!_588)
    {
        _595 = h2.x > 2.0f;
    }
    else
    {
        _595 = _588;
    }
    // Upstream assigns jag2.x four times, which leaves .y, .z and .w
    // uninitialised.  Fixed here and in the GLSL and WGSL copies together.
    bool4 jag2 = bool4(false, false, false, false);
    jag2.x = _595;
    bool _599 = f2.y > 2.0f;
    bool _606;
    if (!_599)
    {
        _606 = h2.y > 2.0f;
    }
    else
    {
        _606 = _599;
    }
    jag2.y = _606;
    bool _610 = f2.z > 2.0f;
    bool _617;
    if (!_610)
    {
        _617 = h2.z > 2.0f;
    }
    else
    {
        _617 = _610;
    }
    jag2.z = _617;
    bool _621 = f2.w > 2.0f;
    bool _628;
    if (!_621)
    {
        _628 = h2.w > 2.0f;
    }
    else
    {
        _628 = _621;
    }
    jag2.w = _628;
    bool _634 = f2.x > 4.0f;
    bool _641;
    if (!_634)
    {
        _641 = h2.x > 4.0f;
    }
    else
    {
        _641 = _634;
    }
    bool4 jag3;
    jag3.x = _641;
    bool _645 = f2.y > 4.0f;
    bool _652;
    if (!_645)
    {
        _652 = h2.y > 4.0f;
    }
    else
    {
        _652 = _645;
    }
    jag3.y = _652;
    bool _656 = f2.z > 4.0f;
    bool _663;
    if (!_656)
    {
        _663 = h2.z > 4.0f;
    }
    else
    {
        _663 = _656;
    }
    jag3.z = _663;
    bool _667 = f2.w > 4.0f;
    bool _674;
    if (!_667)
    {
        _674 = h2.w > 4.0f;
    }
    else
    {
        _674 = _667;
    }
    jag3.w = _674;
    bool _679 = pe.x == 7.0f;
    bool _687;
    if (!_679)
    {
        _687 = pe.x == 8.0f;
    }
    else
    {
        _687 = _679;
    }
    float _688;
    if (_687)
    {
        float _693;
        if (jag3.x)
        {
            _693 = pe.x;
        }
        else
        {
            _693 = pe.x - 2.0f;
        }
        _688 = _693;
    }
    else
    {
        _688 = pe.x;
    }
    pe.x = _688;
    bool _710 = pe.y == 7.0f;
    bool _717;
    if (!_710)
    {
        _717 = pe.y == 8.0f;
    }
    else
    {
        _717 = _710;
    }
    float _718;
    if (_717)
    {
        float _723;
        if (jag3.y)
        {
            _723 = pe.y;
        }
        else
        {
            _723 = pe.y - 2.0f;
        }
        _718 = _723;
    }
    else
    {
        _718 = pe.y;
    }
    pe.y = _718;
    bool _740 = pe.z == 7.0f;
    bool _747;
    if (!_740)
    {
        _747 = pe.z == 8.0f;
    }
    else
    {
        _747 = _740;
    }
    float _748;
    if (_747)
    {
        float _753;
        if (jag3.z)
        {
            _753 = pe.z;
        }
        else
        {
            _753 = pe.z - 2.0f;
        }
        _748 = _753;
    }
    else
    {
        _748 = pe.z;
    }
    pe.z = _748;
    bool _770 = pe.w == 7.0f;
    bool _777;
    if (!_770)
    {
        _777 = pe.w == 8.0f;
    }
    else
    {
        _777 = _770;
    }
    float _778;
    if (_777)
    {
        float _783;
        if (jag3.w)
        {
            _783 = pe.w;
        }
        else
        {
            _783 = pe.w - 2.0f;
        }
        _778 = _783;
    }
    else
    {
        _778 = pe.w;
    }
    pe.w = _778;
    bool _801 = pe.x == 5.0f;
    bool _809;
    if (!_801)
    {
        _809 = pe.x == 6.0f;
    }
    else
    {
        _809 = _801;
    }
    float _810;
    if (_809)
    {
        float _815;
        if (jag2.x)
        {
            _815 = pe.x;
        }
        else
        {
            _815 = pe.x - 2.0f;
        }
        _810 = _815;
    }
    else
    {
        _810 = pe.x;
    }
    pe.x = _810;
    bool _832 = pe.y == 5.0f;
    bool _839;
    if (!_832)
    {
        _839 = pe.y == 6.0f;
    }
    else
    {
        _839 = _832;
    }
    float _840;
    if (_839)
    {
        float _845;
        if (jag2.y)
        {
            _845 = pe.y;
        }
        else
        {
            _845 = pe.y - 2.0f;
        }
        _840 = _845;
    }
    else
    {
        _840 = pe.y;
    }
    pe.y = _840;
    bool _862 = pe.z == 5.0f;
    bool _869;
    if (!_862)
    {
        _869 = pe.z == 6.0f;
    }
    else
    {
        _869 = _862;
    }
    float _870;
    if (_869)
    {
        float _875;
        if (jag2.z)
        {
            _875 = pe.z;
        }
        else
        {
            _875 = pe.z - 2.0f;
        }
        _870 = _875;
    }
    else
    {
        _870 = pe.z;
    }
    pe.z = _870;
    bool _892 = pe.w == 5.0f;
    bool _899;
    if (!_892)
    {
        _899 = pe.w == 6.0f;
    }
    else
    {
        _899 = _892;
    }
    float _900;
    if (_899)
    {
        float _905;
        if (jag2.w)
        {
            _905 = pe.w;
        }
        else
        {
            _905 = pe.w - 2.0f;
        }
        _900 = _905;
    }
    else
    {
        _900 = pe.w;
    }
    pe.w = _900;
    float4 param_10 = h;
    float4 param_11 = i;
    float4 param_12 = e;
    float4 param_13 = h;
    bool4 _929 = id(param_10, param_11, param_12, param_13);
    bool _930 = _929.x;
    bool _944;
    if (!_930)
    {
        float4 param_14 = i4;
        float4 param_15 = i;
        float4 param_16 = f4;
        float4 param_17 = i4;
        _944 = id(param_14, param_15, param_16, param_17).x;
    }
    else
    {
        _944 = _930;
    }
    bool _950;
    if (_944)
    {
        _950 = f2.x > 1.0f;
    }
    else
    {
        _950 = _944;
    }
    bool _956;
    if (_950)
    {
        _956 = f1.x > 1.0f;
    }
    else
    {
        _956 = _950;
    }
    bool4 jag91;
    jag91.x = _956;
    float4 param_18 = h;
    float4 param_19 = i;
    float4 param_20 = e;
    float4 param_21 = h;
    bool4 _966 = id(param_18, param_19, param_20, param_21);
    bool _967 = _966.y;
    bool _981;
    if (!_967)
    {
        float4 param_22 = i4;
        float4 param_23 = i;
        float4 param_24 = f4;
        float4 param_25 = i4;
        _981 = id(param_22, param_23, param_24, param_25).y;
    }
    else
    {
        _981 = _967;
    }
    bool _987;
    if (_981)
    {
        _987 = f2.y > 1.0f;
    }
    else
    {
        _987 = _981;
    }
    bool _993;
    if (_987)
    {
        _993 = f1.y > 1.0f;
    }
    else
    {
        _993 = _987;
    }
    jag91.y = _993;
    float4 param_26 = h;
    float4 param_27 = i;
    float4 param_28 = e;
    float4 param_29 = h;
    bool4 _1003 = id(param_26, param_27, param_28, param_29);
    bool _1004 = _1003.z;
    bool _1018;
    if (!_1004)
    {
        float4 param_30 = i4;
        float4 param_31 = i;
        float4 param_32 = f4;
        float4 param_33 = i4;
        _1018 = id(param_30, param_31, param_32, param_33).z;
    }
    else
    {
        _1018 = _1004;
    }
    bool _1024;
    if (_1018)
    {
        _1024 = f2.z > 1.0f;
    }
    else
    {
        _1024 = _1018;
    }
    bool _1030;
    if (_1024)
    {
        _1030 = f1.z > 1.0f;
    }
    else
    {
        _1030 = _1024;
    }
    jag91.z = _1030;
    float4 param_34 = h;
    float4 param_35 = i;
    float4 param_36 = e;
    float4 param_37 = h;
    bool4 _1040 = id(param_34, param_35, param_36, param_37);
    bool _1041 = _1040.w;
    bool _1055;
    if (!_1041)
    {
        float4 param_38 = i4;
        float4 param_39 = i;
        float4 param_40 = f4;
        float4 param_41 = i4;
        _1055 = id(param_38, param_39, param_40, param_41).w;
    }
    else
    {
        _1055 = _1041;
    }
    bool _1061;
    if (_1055)
    {
        _1061 = f2.w > 1.0f;
    }
    else
    {
        _1061 = _1055;
    }
    bool _1067;
    if (_1061)
    {
        _1067 = f1.w > 1.0f;
    }
    else
    {
        _1067 = _1061;
    }
    jag91.w = _1067;
    float4 param_42 = f;
    float4 param_43 = i;
    float4 param_44 = e;
    float4 param_45 = f;
    bool4 _1078 = id(param_42, param_43, param_44, param_45);
    bool _1079 = _1078.x;
    bool _1093;
    if (!_1079)
    {
        float4 param_46 = i5;
        float4 param_47 = i;
        float4 param_48 = h5;
        float4 param_49 = i5;
        _1093 = id(param_46, param_47, param_48, param_49).x;
    }
    else
    {
        _1093 = _1079;
    }
    bool _1099;
    if (_1093)
    {
        _1099 = h2.x > 1.0f;
    }
    else
    {
        _1099 = _1093;
    }
    bool _1105;
    if (_1099)
    {
        _1105 = h3.x > 1.0f;
    }
    else
    {
        _1105 = _1099;
    }
    bool4 jag92;
    jag92.x = _1105;
    float4 param_50 = f;
    float4 param_51 = i;
    float4 param_52 = e;
    float4 param_53 = f;
    bool4 _1115 = id(param_50, param_51, param_52, param_53);
    bool _1116 = _1115.y;
    bool _1130;
    if (!_1116)
    {
        float4 param_54 = i5;
        float4 param_55 = i;
        float4 param_56 = h5;
        float4 param_57 = i5;
        _1130 = id(param_54, param_55, param_56, param_57).y;
    }
    else
    {
        _1130 = _1116;
    }
    bool _1136;
    if (_1130)
    {
        _1136 = h2.y > 1.0f;
    }
    else
    {
        _1136 = _1130;
    }
    bool _1142;
    if (_1136)
    {
        _1142 = h3.y > 1.0f;
    }
    else
    {
        _1142 = _1136;
    }
    jag92.y = _1142;
    float4 param_58 = f;
    float4 param_59 = i;
    float4 param_60 = e;
    float4 param_61 = f;
    bool4 _1152 = id(param_58, param_59, param_60, param_61);
    bool _1153 = _1152.z;
    bool _1167;
    if (!_1153)
    {
        float4 param_62 = i5;
        float4 param_63 = i;
        float4 param_64 = h5;
        float4 param_65 = i5;
        _1167 = id(param_62, param_63, param_64, param_65).z;
    }
    else
    {
        _1167 = _1153;
    }
    bool _1173;
    if (_1167)
    {
        _1173 = h2.z > 1.0f;
    }
    else
    {
        _1173 = _1167;
    }
    bool _1179;
    if (_1173)
    {
        _1179 = h3.z > 1.0f;
    }
    else
    {
        _1179 = _1173;
    }
    jag92.z = _1179;
    float4 param_66 = f;
    float4 param_67 = i;
    float4 param_68 = e;
    float4 param_69 = f;
    bool4 _1189 = id(param_66, param_67, param_68, param_69);
    bool _1190 = _1189.w;
    bool _1204;
    if (!_1190)
    {
        float4 param_70 = i5;
        float4 param_71 = i;
        float4 param_72 = h5;
        float4 param_73 = i5;
        _1204 = id(param_70, param_71, param_72, param_73).w;
    }
    else
    {
        _1204 = _1190;
    }
    bool _1210;
    if (_1204)
    {
        _1210 = h2.w > 1.0f;
    }
    else
    {
        _1210 = _1204;
    }
    bool _1216;
    if (_1210)
    {
        _1216 = h3.w > 1.0f;
    }
    else
    {
        _1216 = _1210;
    }
    jag92.w = _1216;
    float4 param_74 = h;
    float4 param_75 = g;
    float4 param_76 = e;
    float4 param_77 = g;
    bool4 jag93 = rd(param_74, param_75, param_76, param_77);
    float4 param_78 = f;
    float4 param_79 = c;
    float4 param_80 = e;
    float4 param_81 = c;
    bool4 jag94 = rd(param_78, param_79, param_80, param_81);
    bool _1243 = jag91.x && jag93.x;
    bool _1252;
    if (!_1243)
    {
        _1252 = jag92.x && jag94.x;
    }
    else
    {
        _1252 = _1243;
    }
    bool4 jag9;
    jag9.x = !_1252;
    bool _1259 = jag91.y && jag93.y;
    bool _1268;
    if (!_1259)
    {
        _1268 = jag92.y && jag94.y;
    }
    else
    {
        _1268 = _1259;
    }
    jag9.y = !_1268;
    bool _1275 = jag91.z && jag93.z;
    bool _1284;
    if (!_1275)
    {
        _1284 = jag92.z && jag94.z;
    }
    else
    {
        _1284 = _1275;
    }
    jag9.z = !_1284;
    bool _1291 = jag91.w && jag93.w;
    bool _1300;
    if (!_1291)
    {
        _1300 = jag92.w && jag94.w;
    }
    else
    {
        _1300 = _1291;
    }
    jag9.w = !_1300;
    bool _1305 = pe.x == 0.0f;
    bool _1318;
    if (!_1305)
    {
        _1318 = ((!nbrs.x) || jag1.x) && jag9.x;
    }
    else
    {
        _1318 = _1305;
    }
    float _1319;
    if (_1318)
    {
        _1319 = pe.x;
    }
    else
    {
        _1319 = 1.0f;
    }
    pe.x = _1319;
    bool _1329 = pe.y == 0.0f;
    bool _1342;
    if (!_1329)
    {
        _1342 = ((!nbrs.y) || jag1.y) && jag9.y;
    }
    else
    {
        _1342 = _1329;
    }
    float _1343;
    if (_1342)
    {
        _1343 = pe.y;
    }
    else
    {
        _1343 = 1.0f;
    }
    pe.y = _1343;
    bool _1353 = pe.z == 0.0f;
    bool _1366;
    if (!_1353)
    {
        _1366 = ((!nbrs.z) || jag1.z) && jag9.z;
    }
    else
    {
        _1366 = _1353;
    }
    float _1367;
    if (_1366)
    {
        _1367 = pe.z;
    }
    else
    {
        _1367 = 1.0f;
    }
    pe.z = _1367;
    bool _1377 = pe.w == 0.0f;
    bool _1390;
    if (!_1377)
    {
        _1390 = ((!nbrs.w) || jag1.w) && jag9.w;
    }
    else
    {
        _1390 = _1377;
    }
    float _1391;
    if (_1390)
    {
        _1391 = pe.w;
    }
    else
    {
        _1391 = 1.0f;
    }
    pe.w = _1391;
    float4 param_82 = pe;
    float4 param_83 = 255.0f.xxxx;
    FragColor = remapTo01(param_82, param_83);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	TEX0.x = input.uv.x;
	TEX0.y = input.uv.y;
	float2 ps = float2(1.0f / SourceSize.xy.x, 1.0f / SourceSize.xy.y);
	float dx = ps.x;
	float dy = ps.y;
	t1 = TEX0.xxxy + float4(-dx, 0.0f, dx, (-2.0f) * dy);
	t2 = TEX0.xxxy + float4(-dx, 0.0f, dx, -dy);
	t3 = TEX0.xxxy + float4(-dx, 0.0f, dx, 0.0f);
	t4 = TEX0.xxxy + float4(-dx, 0.0f, dx, dy);
	t5 = TEX0.xxxy + float4(-dx, 0.0f, dx, 2.0f * dy);
	t6 = TEX0.xyyy + float4((-2.0f) * dx, -dy, 0.0f, dy);
	t7 = TEX0.xyyy + float4(2.0f * dx, -dy, 0.0f, dy);

	frag_main();

	return FragColor;
}
