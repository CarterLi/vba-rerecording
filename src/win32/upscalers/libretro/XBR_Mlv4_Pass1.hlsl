// XBR_Mlv4_Pass1, pass 1 of 4 of the xBR-mlv4 pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-mlv4-pass1.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_MLV4_PASS1_GLSL"; this is the port of it, because Direct3D
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

static float4 TEX0;
static float2 t1;
static float4 FragColor;

float4 df(float4 A, float4 B)
{
    return abs(A - B);
}

bool4 eq(float4 A, float4 B)
{
    float4 param = A;
    float4 param_1 = B;
    float4 _71 = df(param, param_1);
    return bool4(_71.x < 2.0f.xxxx.x, _71.y < 2.0f.xxxx.y, _71.z < 2.0f.xxxx.z, _71.w < 2.0f.xxxx.w);
}

float4 weighted_distance(float4 a, float4 b, float4 c, float4 d, float4 e, float4 f, float4 g, float4 h)
{
    float4 param = a;
    float4 param_1 = b;
    float4 param_2 = a;
    float4 param_3 = c;
    float4 param_4 = d;
    float4 param_5 = e;
    float4 param_6 = d;
    float4 param_7 = f;
    float4 param_8 = g;
    float4 param_9 = h;
    return (((df(param, param_1) + df(param_2, param_3)) + df(param_4, param_5)) + df(param_6, param_7)) + (df(param_8, param_9) * 4.0f);
}

bool4 id(float4 A, float4 B, float4 C, float4 D)
{
    float4 param = C;
    float4 param_1 = D;
    float4 _53 = df(param, param_1);
    float4 param_2 = A;
    float4 param_3 = B;
    float4 _58 = df(param_2, param_3);
    return bool4(_53.x > _58.x, _53.y > _58.y, _53.z > _58.z, _53.w > _58.w);
}

float4 remapTo01(float4 v, float4 high)
{
    return v / high;
}

void frag_main()
{
    float3 A3 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, -3.0f) * t1)).xyz;
    float3 B3 = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, -3.0f) * t1)).xyz;
    float3 C3 = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, -3.0f) * t1)).xyz;
    float3 A1 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, -2.0f) * t1)).xyz;
    float3 B1 = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, -2.0f) * t1)).xyz;
    float3 C1 = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, -2.0f) * t1)).xyz;
    float3 A2 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-3.0f, -1.0f) * t1)).xyz;
    float3 A0 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-2.0f, -1.0f) * t1)).xyz;
    float3 A = frameTex.Sample(frameSampler, TEX0.xy + ((-1.0f).xx * t1)).xyz;
    float3 B = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, -1.0f) * t1)).xyz;
    float3 C = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, -1.0f) * t1)).xyz;
    float3 C4 = frameTex.Sample(frameSampler, TEX0.xy + (float2(2.0f, -1.0f) * t1)).xyz;
    float3 C6 = frameTex.Sample(frameSampler, TEX0.xy + (float2(3.0f, -1.0f) * t1)).xyz;
    float3 D2 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-3.0f, 0.0f) * t1)).xyz;
    float3 D0 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-2.0f, 0.0f) * t1)).xyz;
    float3 D = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, 0.0f) * t1)).xyz;
    float3 E = frameTex.Sample(frameSampler, TEX0.xy + (0.0f.xx * t1)).xyz;
    float3 F = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, 0.0f) * t1)).xyz;
    float3 F4 = frameTex.Sample(frameSampler, TEX0.xy + (float2(2.0f, 0.0f) * t1)).xyz;
    float3 F6 = frameTex.Sample(frameSampler, TEX0.xy + (float2(3.0f, 0.0f) * t1)).xyz;
    float3 G2 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-3.0f, 1.0f) * t1)).xyz;
    float3 G0 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-2.0f, 1.0f) * t1)).xyz;
    float3 G = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, 1.0f) * t1)).xyz;
    float3 H = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, 1.0f) * t1)).xyz;
    float3 I = frameTex.Sample(frameSampler, TEX0.xy + (1.0f.xx * t1)).xyz;
    float3 I4 = frameTex.Sample(frameSampler, TEX0.xy + (float2(2.0f, 1.0f) * t1)).xyz;
    float3 I6 = frameTex.Sample(frameSampler, TEX0.xy + (float2(3.0f, 1.0f) * t1)).xyz;
    float3 G5 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, 2.0f) * t1)).xyz;
    float3 H5 = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, 2.0f) * t1)).xyz;
    float3 I5 = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, 2.0f) * t1)).xyz;
    float3 G7 = frameTex.Sample(frameSampler, TEX0.xy + (float2(-1.0f, 3.0f) * t1)).xyz;
    float3 H7 = frameTex.Sample(frameSampler, TEX0.xy + (float2(0.0f, 3.0f) * t1)).xyz;
    float3 I7 = frameTex.Sample(frameSampler, TEX0.xy + (float2(1.0f, 3.0f) * t1)).xyz;
    float4x3 bdhf = mul(float4x3(float3(B), float3(D), float3(H), float3(F)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 b = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(float3(C), float3(A), float3(G), float3(I)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 c = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(E, E, E, E), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 e = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    float4 d = b.yzwx;
    float4 f = b.wxyz;
    float4 g = c.zwxy;
    float4 h = b.zwxy;
    float4 i = c.wxyz;
    bdhf = mul(float4x3(float3(I4), float3(C1), float3(A0), float3(G5)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 i4 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(float3(I5), float3(C4), float3(A1), float3(G0)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 i5 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(float3(H5), float3(F4), float3(B1), float3(D0)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 h5 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    float4 f4 = h5.yzwx;
    float4 c1 = i4.yzwx;
    float4 g0 = i5.wxyz;
    float4 b1 = h5.zwxy;
    float4 d0 = h5.wxyz;
    bdhf = mul(float4x3(float3(I6), float3(C3), float3(A2), float3(G7)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 i6 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(float3(I7), float3(C6), float3(A3), float3(G2)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 i7 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    bdhf = mul(float4x3(float3(H7), float3(F6), float3(B3), float3(D2)), float3x3(float3(0.2989999949932098388671875f, -0.1689999997615814208984375f, 0.499000012874603271484375f), float3(0.58700001239776611328125f, -0.3310000002384185791015625f, -0.4180000126361846923828125f), float3(0.114000000059604644775390625f, 0.499000012874603271484375f, -0.081299997866153717041015625f)));
    bdhf = float4x3(float3(abs(bdhf[0])), float3(abs(bdhf[1])), float3(abs(bdhf[2])), float3(abs(bdhf[3])));
    float4 h7 = mul(bdhf, float3(4.0f, 1.0f, 2.0f));
    float4 f6 = h7.yzwx;
    float4 c3 = i6.yzwx;
    float4 g2 = i7.wxyz;
    float4 b3 = h7.zwxy;
    float4 d2 = h7.wxyz;
    bool _1000 = e.x != f.x;
    bool _1008;
    if (_1000)
    {
        _1008 = e.x != h.x;
    }
    else
    {
        _1008 = _1000;
    }
    bool4 interp_restriction_lv1;
    interp_restriction_lv1.x = _1008;
    bool _1016 = e.y != f.y;
    bool _1024;
    if (_1016)
    {
        _1024 = e.y != h.y;
    }
    else
    {
        _1024 = _1016;
    }
    interp_restriction_lv1.y = _1024;
    bool _1031 = e.z != f.z;
    bool _1039;
    if (_1031)
    {
        _1039 = e.z != h.z;
    }
    else
    {
        _1039 = _1031;
    }
    interp_restriction_lv1.z = _1039;
    bool _1046 = e.w != f.w;
    bool _1054;
    if (_1046)
    {
        _1054 = e.w != h.w;
    }
    else
    {
        _1054 = _1046;
    }
    interp_restriction_lv1.w = _1054;
    bool _1061 = e.x != g.x;
    bool _1069;
    if (_1061)
    {
        _1069 = d.x != g.x;
    }
    else
    {
        _1069 = _1061;
    }
    bool _1088;
    if (_1069)
    {
        float4 param = e;
        float4 param_1 = d;
        bool4 _1076 = eq(param, param_1);
        bool _1077 = _1076.x;
        bool _1087;
        if (!_1077)
        {
            float4 param_2 = h;
            float4 param_3 = g;
            _1087 = eq(param_2, param_3).x;
        }
        else
        {
            _1087 = _1077;
        }
        _1088 = _1087;
    }
    else
    {
        _1088 = _1069;
    }
    bool4 interp_restriction_lv2_left;
    interp_restriction_lv2_left.x = _1088;
    bool _1094 = e.y != g.y;
    bool _1102;
    if (_1094)
    {
        _1102 = d.y != g.y;
    }
    else
    {
        _1102 = _1094;
    }
    bool _1121;
    if (_1102)
    {
        float4 param_4 = e;
        float4 param_5 = d;
        bool4 _1109 = eq(param_4, param_5);
        bool _1110 = _1109.y;
        bool _1120;
        if (!_1110)
        {
            float4 param_6 = h;
            float4 param_7 = g;
            _1120 = eq(param_6, param_7).y;
        }
        else
        {
            _1120 = _1110;
        }
        _1121 = _1120;
    }
    else
    {
        _1121 = _1102;
    }
    interp_restriction_lv2_left.y = _1121;
    bool _1127 = e.z != g.z;
    bool _1135;
    if (_1127)
    {
        _1135 = d.z != g.z;
    }
    else
    {
        _1135 = _1127;
    }
    bool _1154;
    if (_1135)
    {
        float4 param_8 = e;
        float4 param_9 = d;
        bool4 _1142 = eq(param_8, param_9);
        bool _1143 = _1142.z;
        bool _1153;
        if (!_1143)
        {
            float4 param_10 = h;
            float4 param_11 = g;
            _1153 = eq(param_10, param_11).z;
        }
        else
        {
            _1153 = _1143;
        }
        _1154 = _1153;
    }
    else
    {
        _1154 = _1135;
    }
    interp_restriction_lv2_left.z = _1154;
    bool _1160 = e.w != g.w;
    bool _1168;
    if (_1160)
    {
        _1168 = d.w != g.w;
    }
    else
    {
        _1168 = _1160;
    }
    bool _1187;
    if (_1168)
    {
        float4 param_12 = e;
        float4 param_13 = d;
        bool4 _1175 = eq(param_12, param_13);
        bool _1176 = _1175.w;
        bool _1186;
        if (!_1176)
        {
            float4 param_14 = h;
            float4 param_15 = g;
            _1186 = eq(param_14, param_15).w;
        }
        else
        {
            _1186 = _1176;
        }
        _1187 = _1186;
    }
    else
    {
        _1187 = _1168;
    }
    interp_restriction_lv2_left.w = _1187;
    bool _1194 = e.x != c.x;
    bool _1202;
    if (_1194)
    {
        _1202 = b.x != c.x;
    }
    else
    {
        _1202 = _1194;
    }
    bool _1221;
    if (_1202)
    {
        float4 param_16 = e;
        float4 param_17 = b;
        bool4 _1209 = eq(param_16, param_17);
        bool _1210 = _1209.x;
        bool _1220;
        if (!_1210)
        {
            float4 param_18 = f;
            float4 param_19 = c;
            _1220 = eq(param_18, param_19).x;
        }
        else
        {
            _1220 = _1210;
        }
        _1221 = _1220;
    }
    else
    {
        _1221 = _1202;
    }
    bool4 interp_restriction_lv2_up;
    interp_restriction_lv2_up.x = _1221;
    bool _1227 = e.y != c.y;
    bool _1235;
    if (_1227)
    {
        _1235 = b.y != c.y;
    }
    else
    {
        _1235 = _1227;
    }
    bool _1254;
    if (_1235)
    {
        float4 param_20 = e;
        float4 param_21 = b;
        bool4 _1242 = eq(param_20, param_21);
        bool _1243 = _1242.y;
        bool _1253;
        if (!_1243)
        {
            float4 param_22 = f;
            float4 param_23 = c;
            _1253 = eq(param_22, param_23).y;
        }
        else
        {
            _1253 = _1243;
        }
        _1254 = _1253;
    }
    else
    {
        _1254 = _1235;
    }
    interp_restriction_lv2_up.y = _1254;
    bool _1260 = e.z != c.z;
    bool _1268;
    if (_1260)
    {
        _1268 = b.z != c.z;
    }
    else
    {
        _1268 = _1260;
    }
    bool _1287;
    if (_1268)
    {
        float4 param_24 = e;
        float4 param_25 = b;
        bool4 _1275 = eq(param_24, param_25);
        bool _1276 = _1275.z;
        bool _1286;
        if (!_1276)
        {
            float4 param_26 = f;
            float4 param_27 = c;
            _1286 = eq(param_26, param_27).z;
        }
        else
        {
            _1286 = _1276;
        }
        _1287 = _1286;
    }
    else
    {
        _1287 = _1268;
    }
    interp_restriction_lv2_up.z = _1287;
    bool _1293 = e.w != c.w;
    bool _1301;
    if (_1293)
    {
        _1301 = b.w != c.w;
    }
    else
    {
        _1301 = _1293;
    }
    bool _1320;
    if (_1301)
    {
        float4 param_28 = e;
        float4 param_29 = b;
        bool4 _1308 = eq(param_28, param_29);
        bool _1309 = _1308.w;
        bool _1319;
        if (!_1309)
        {
            float4 param_30 = f;
            float4 param_31 = c;
            _1319 = eq(param_30, param_31).w;
        }
        else
        {
            _1319 = _1309;
        }
        _1320 = _1319;
    }
    else
    {
        _1320 = _1301;
    }
    interp_restriction_lv2_up.w = _1320;
    bool _1327 = e.x != g0.x;
    bool _1335;
    if (_1327)
    {
        _1335 = d0.x != g0.x;
    }
    else
    {
        _1335 = _1327;
    }
    bool _1354;
    if (_1335)
    {
        float4 param_32 = d;
        float4 param_33 = d0;
        bool4 _1342 = eq(param_32, param_33);
        bool _1343 = _1342.x;
        bool _1353;
        if (!_1343)
        {
            float4 param_34 = g;
            float4 param_35 = g0;
            _1353 = eq(param_34, param_35).x;
        }
        else
        {
            _1353 = _1343;
        }
        _1354 = _1353;
    }
    else
    {
        _1354 = _1335;
    }
    bool4 interp_restriction_lv3_left;
    interp_restriction_lv3_left.x = _1354;
    bool _1360 = e.y != g0.y;
    bool _1368;
    if (_1360)
    {
        _1368 = d0.y != g0.y;
    }
    else
    {
        _1368 = _1360;
    }
    bool _1387;
    if (_1368)
    {
        float4 param_36 = d;
        float4 param_37 = d0;
        bool4 _1375 = eq(param_36, param_37);
        bool _1376 = _1375.y;
        bool _1386;
        if (!_1376)
        {
            float4 param_38 = g;
            float4 param_39 = g0;
            _1386 = eq(param_38, param_39).y;
        }
        else
        {
            _1386 = _1376;
        }
        _1387 = _1386;
    }
    else
    {
        _1387 = _1368;
    }
    interp_restriction_lv3_left.y = _1387;
    bool _1393 = e.z != g0.z;
    bool _1401;
    if (_1393)
    {
        _1401 = d0.z != g0.z;
    }
    else
    {
        _1401 = _1393;
    }
    bool _1420;
    if (_1401)
    {
        float4 param_40 = d;
        float4 param_41 = d0;
        bool4 _1408 = eq(param_40, param_41);
        bool _1409 = _1408.z;
        bool _1419;
        if (!_1409)
        {
            float4 param_42 = g;
            float4 param_43 = g0;
            _1419 = eq(param_42, param_43).z;
        }
        else
        {
            _1419 = _1409;
        }
        _1420 = _1419;
    }
    else
    {
        _1420 = _1401;
    }
    interp_restriction_lv3_left.z = _1420;
    bool _1426 = e.w != g0.w;
    bool _1434;
    if (_1426)
    {
        _1434 = d0.w != g0.w;
    }
    else
    {
        _1434 = _1426;
    }
    bool _1453;
    if (_1434)
    {
        float4 param_44 = d;
        float4 param_45 = d0;
        bool4 _1441 = eq(param_44, param_45);
        bool _1442 = _1441.w;
        bool _1452;
        if (!_1442)
        {
            float4 param_46 = g;
            float4 param_47 = g0;
            _1452 = eq(param_46, param_47).w;
        }
        else
        {
            _1452 = _1442;
        }
        _1453 = _1452;
    }
    else
    {
        _1453 = _1434;
    }
    interp_restriction_lv3_left.w = _1453;
    bool _1460 = e.x != c1.x;
    bool _1468;
    if (_1460)
    {
        _1468 = b1.x != c1.x;
    }
    else
    {
        _1468 = _1460;
    }
    bool _1487;
    if (_1468)
    {
        float4 param_48 = b;
        float4 param_49 = b1;
        bool4 _1475 = eq(param_48, param_49);
        bool _1476 = _1475.x;
        bool _1486;
        if (!_1476)
        {
            float4 param_50 = c;
            float4 param_51 = c1;
            _1486 = eq(param_50, param_51).x;
        }
        else
        {
            _1486 = _1476;
        }
        _1487 = _1486;
    }
    else
    {
        _1487 = _1468;
    }
    bool4 interp_restriction_lv3_up;
    interp_restriction_lv3_up.x = _1487;
    bool _1493 = e.y != c1.y;
    bool _1501;
    if (_1493)
    {
        _1501 = b1.y != c1.y;
    }
    else
    {
        _1501 = _1493;
    }
    bool _1520;
    if (_1501)
    {
        float4 param_52 = b;
        float4 param_53 = b1;
        bool4 _1508 = eq(param_52, param_53);
        bool _1509 = _1508.y;
        bool _1519;
        if (!_1509)
        {
            float4 param_54 = c;
            float4 param_55 = c1;
            _1519 = eq(param_54, param_55).y;
        }
        else
        {
            _1519 = _1509;
        }
        _1520 = _1519;
    }
    else
    {
        _1520 = _1501;
    }
    interp_restriction_lv3_up.y = _1520;
    bool _1526 = e.z != c1.z;
    bool _1534;
    if (_1526)
    {
        _1534 = b1.z != c1.z;
    }
    else
    {
        _1534 = _1526;
    }
    bool _1553;
    if (_1534)
    {
        float4 param_56 = b;
        float4 param_57 = b1;
        bool4 _1541 = eq(param_56, param_57);
        bool _1542 = _1541.z;
        bool _1552;
        if (!_1542)
        {
            float4 param_58 = c;
            float4 param_59 = c1;
            _1552 = eq(param_58, param_59).z;
        }
        else
        {
            _1552 = _1542;
        }
        _1553 = _1552;
    }
    else
    {
        _1553 = _1534;
    }
    interp_restriction_lv3_up.z = _1553;
    bool _1559 = e.w != c1.w;
    bool _1567;
    if (_1559)
    {
        _1567 = b1.w != c1.w;
    }
    else
    {
        _1567 = _1559;
    }
    bool _1586;
    if (_1567)
    {
        float4 param_60 = b;
        float4 param_61 = b1;
        bool4 _1574 = eq(param_60, param_61);
        bool _1575 = _1574.w;
        bool _1585;
        if (!_1575)
        {
            float4 param_62 = c;
            float4 param_63 = c1;
            _1585 = eq(param_62, param_63).w;
        }
        else
        {
            _1585 = _1575;
        }
        _1586 = _1585;
    }
    else
    {
        _1586 = _1567;
    }
    interp_restriction_lv3_up.w = _1586;
    bool _1593 = e.x != g2.x;
    bool _1601;
    if (_1593)
    {
        _1601 = d2.x != g2.x;
    }
    else
    {
        _1601 = _1593;
    }
    bool _1620;
    if (_1601)
    {
        float4 param_64 = d0;
        float4 param_65 = d2;
        bool4 _1608 = eq(param_64, param_65);
        bool _1609 = _1608.x;
        bool _1619;
        if (!_1609)
        {
            float4 param_66 = g0;
            float4 param_67 = g2;
            _1619 = eq(param_66, param_67).x;
        }
        else
        {
            _1619 = _1609;
        }
        _1620 = _1619;
    }
    else
    {
        _1620 = _1601;
    }
    bool4 interp_restriction_lv4_left;
    interp_restriction_lv4_left.x = _1620;
    bool _1626 = e.y != g2.y;
    bool _1634;
    if (_1626)
    {
        _1634 = d2.y != g2.y;
    }
    else
    {
        _1634 = _1626;
    }
    bool _1653;
    if (_1634)
    {
        float4 param_68 = d0;
        float4 param_69 = d2;
        bool4 _1641 = eq(param_68, param_69);
        bool _1642 = _1641.y;
        bool _1652;
        if (!_1642)
        {
            float4 param_70 = g0;
            float4 param_71 = g2;
            _1652 = eq(param_70, param_71).y;
        }
        else
        {
            _1652 = _1642;
        }
        _1653 = _1652;
    }
    else
    {
        _1653 = _1634;
    }
    interp_restriction_lv4_left.y = _1653;
    bool _1659 = e.z != g2.z;
    bool _1667;
    if (_1659)
    {
        _1667 = d2.z != g2.z;
    }
    else
    {
        _1667 = _1659;
    }
    bool _1686;
    if (_1667)
    {
        float4 param_72 = d0;
        float4 param_73 = d2;
        bool4 _1674 = eq(param_72, param_73);
        bool _1675 = _1674.z;
        bool _1685;
        if (!_1675)
        {
            float4 param_74 = g0;
            float4 param_75 = g2;
            _1685 = eq(param_74, param_75).z;
        }
        else
        {
            _1685 = _1675;
        }
        _1686 = _1685;
    }
    else
    {
        _1686 = _1667;
    }
    interp_restriction_lv4_left.z = _1686;
    bool _1692 = e.w != g2.w;
    bool _1700;
    if (_1692)
    {
        _1700 = d2.w != g2.w;
    }
    else
    {
        _1700 = _1692;
    }
    bool _1719;
    if (_1700)
    {
        float4 param_76 = d0;
        float4 param_77 = d2;
        bool4 _1707 = eq(param_76, param_77);
        bool _1708 = _1707.w;
        bool _1718;
        if (!_1708)
        {
            float4 param_78 = g0;
            float4 param_79 = g2;
            _1718 = eq(param_78, param_79).w;
        }
        else
        {
            _1718 = _1708;
        }
        _1719 = _1718;
    }
    else
    {
        _1719 = _1700;
    }
    interp_restriction_lv4_left.w = _1719;
    bool _1726 = e.x != c3.x;
    bool _1734;
    if (_1726)
    {
        _1734 = b3.x != c3.x;
    }
    else
    {
        _1734 = _1726;
    }
    bool _1753;
    if (_1734)
    {
        float4 param_80 = b1;
        float4 param_81 = b3;
        bool4 _1741 = eq(param_80, param_81);
        bool _1742 = _1741.x;
        bool _1752;
        if (!_1742)
        {
            float4 param_82 = c1;
            float4 param_83 = c3;
            _1752 = eq(param_82, param_83).x;
        }
        else
        {
            _1752 = _1742;
        }
        _1753 = _1752;
    }
    else
    {
        _1753 = _1734;
    }
    bool4 interp_restriction_lv4_up;
    interp_restriction_lv4_up.x = _1753;
    bool _1759 = e.y != c3.y;
    bool _1767;
    if (_1759)
    {
        _1767 = b3.y != c3.y;
    }
    else
    {
        _1767 = _1759;
    }
    bool _1786;
    if (_1767)
    {
        float4 param_84 = b1;
        float4 param_85 = b3;
        bool4 _1774 = eq(param_84, param_85);
        bool _1775 = _1774.y;
        bool _1785;
        if (!_1775)
        {
            float4 param_86 = c1;
            float4 param_87 = c3;
            _1785 = eq(param_86, param_87).y;
        }
        else
        {
            _1785 = _1775;
        }
        _1786 = _1785;
    }
    else
    {
        _1786 = _1767;
    }
    interp_restriction_lv4_up.y = _1786;
    bool _1792 = e.z != c3.z;
    bool _1800;
    if (_1792)
    {
        _1800 = b3.z != c3.z;
    }
    else
    {
        _1800 = _1792;
    }
    bool _1819;
    if (_1800)
    {
        float4 param_88 = b1;
        float4 param_89 = b3;
        bool4 _1807 = eq(param_88, param_89);
        bool _1808 = _1807.z;
        bool _1818;
        if (!_1808)
        {
            float4 param_90 = c1;
            float4 param_91 = c3;
            _1818 = eq(param_90, param_91).z;
        }
        else
        {
            _1818 = _1808;
        }
        _1819 = _1818;
    }
    else
    {
        _1819 = _1800;
    }
    interp_restriction_lv4_up.z = _1819;
    bool _1825 = e.w != c3.w;
    bool _1833;
    if (_1825)
    {
        _1833 = b3.w != c3.w;
    }
    else
    {
        _1833 = _1825;
    }
    bool _1852;
    if (_1833)
    {
        float4 param_92 = b1;
        float4 param_93 = b3;
        bool4 _1840 = eq(param_92, param_93);
        bool _1841 = _1840.w;
        bool _1851;
        if (!_1841)
        {
            float4 param_94 = c1;
            float4 param_95 = c3;
            _1851 = eq(param_94, param_95).w;
        }
        else
        {
            _1851 = _1841;
        }
        _1852 = _1851;
    }
    else
    {
        _1852 = _1833;
    }
    interp_restriction_lv4_up.w = _1852;
    float4 param_96 = e;
    float4 param_97 = c;
    float4 param_98 = g;
    float4 param_99 = i;
    float4 param_100 = h5;
    float4 param_101 = f4;
    float4 param_102 = h;
    float4 param_103 = f;
    float4 wd1 = weighted_distance(param_96, param_97, param_98, param_99, param_100, param_101, param_102, param_103);
    float4 param_104 = h;
    float4 param_105 = d;
    float4 param_106 = i5;
    float4 param_107 = f;
    float4 param_108 = i4;
    float4 param_109 = b;
    float4 param_110 = e;
    float4 param_111 = i;
    float4 wd2 = weighted_distance(param_104, param_105, param_106, param_107, param_108, param_109, param_110, param_111);
    bool4 edr0;
    edr0.x = (wd1.x <= wd2.x) && interp_restriction_lv1.x;
    edr0.y = (wd1.y <= wd2.y) && interp_restriction_lv1.y;
    edr0.z = (wd1.z <= wd2.z) && interp_restriction_lv1.z;
    edr0.w = (wd1.w <= wd2.w) && interp_restriction_lv1.w;
    bool _1935 = (wd1.x < wd2.x) && interp_restriction_lv1.x;
    bool _2004;
    if (_1935)
    {
        float4 param_112 = f;
        float4 param_113 = b;
        bool _1944 = !eq(param_112, param_113).x;
        bool _1958;
        if (_1944)
        {
            float4 param_114 = f;
            float4 param_115 = c;
            float4 param_116 = f;
            float4 param_117 = b;
            _1958 = !id(param_114, param_115, param_116, param_117).x;
        }
        else
        {
            _1958 = _1944;
        }
        bool _1983;
        if (!_1958)
        {
            float4 param_118 = h;
            float4 param_119 = d;
            bool _1968 = !eq(param_118, param_119).x;
            bool _1982;
            if (_1968)
            {
                float4 param_120 = h;
                float4 param_121 = g;
                float4 param_122 = h;
                float4 param_123 = d;
                _1982 = !id(param_120, param_121, param_122, param_123).x;
            }
            else
            {
                _1982 = _1968;
            }
            _1983 = _1982;
        }
        else
        {
            _1983 = _1958;
        }
        bool _1993;
        if (!_1983)
        {
            float4 param_124 = e;
            float4 param_125 = g;
            _1993 = eq(param_124, param_125).x;
        }
        else
        {
            _1993 = _1983;
        }
        bool _2003;
        if (!_1993)
        {
            float4 param_126 = e;
            float4 param_127 = c;
            _2003 = eq(param_126, param_127).x;
        }
        else
        {
            _2003 = _1993;
        }
        _2004 = _2003;
    }
    else
    {
        _2004 = _1935;
    }
    bool4 edr;
    edr.x = _2004;
    bool _2013 = (wd1.y < wd2.y) && interp_restriction_lv1.y;
    bool _2082;
    if (_2013)
    {
        float4 param_128 = f;
        float4 param_129 = b;
        bool _2022 = !eq(param_128, param_129).y;
        bool _2036;
        if (_2022)
        {
            float4 param_130 = f;
            float4 param_131 = c;
            float4 param_132 = f;
            float4 param_133 = b;
            _2036 = !id(param_130, param_131, param_132, param_133).y;
        }
        else
        {
            _2036 = _2022;
        }
        bool _2061;
        if (!_2036)
        {
            float4 param_134 = h;
            float4 param_135 = d;
            bool _2046 = !eq(param_134, param_135).y;
            bool _2060;
            if (_2046)
            {
                float4 param_136 = h;
                float4 param_137 = g;
                float4 param_138 = h;
                float4 param_139 = d;
                _2060 = !id(param_136, param_137, param_138, param_139).y;
            }
            else
            {
                _2060 = _2046;
            }
            _2061 = _2060;
        }
        else
        {
            _2061 = _2036;
        }
        bool _2071;
        if (!_2061)
        {
            float4 param_140 = e;
            float4 param_141 = g;
            _2071 = eq(param_140, param_141).y;
        }
        else
        {
            _2071 = _2061;
        }
        bool _2081;
        if (!_2071)
        {
            float4 param_142 = e;
            float4 param_143 = c;
            _2081 = eq(param_142, param_143).y;
        }
        else
        {
            _2081 = _2071;
        }
        _2082 = _2081;
    }
    else
    {
        _2082 = _2013;
    }
    edr.y = _2082;
    bool _2091 = (wd1.z < wd2.z) && interp_restriction_lv1.z;
    bool _2160;
    if (_2091)
    {
        float4 param_144 = f;
        float4 param_145 = b;
        bool _2100 = !eq(param_144, param_145).z;
        bool _2114;
        if (_2100)
        {
            float4 param_146 = f;
            float4 param_147 = c;
            float4 param_148 = f;
            float4 param_149 = b;
            _2114 = !id(param_146, param_147, param_148, param_149).z;
        }
        else
        {
            _2114 = _2100;
        }
        bool _2139;
        if (!_2114)
        {
            float4 param_150 = h;
            float4 param_151 = d;
            bool _2124 = !eq(param_150, param_151).z;
            bool _2138;
            if (_2124)
            {
                float4 param_152 = h;
                float4 param_153 = g;
                float4 param_154 = h;
                float4 param_155 = d;
                _2138 = !id(param_152, param_153, param_154, param_155).z;
            }
            else
            {
                _2138 = _2124;
            }
            _2139 = _2138;
        }
        else
        {
            _2139 = _2114;
        }
        bool _2149;
        if (!_2139)
        {
            float4 param_156 = e;
            float4 param_157 = g;
            _2149 = eq(param_156, param_157).z;
        }
        else
        {
            _2149 = _2139;
        }
        bool _2159;
        if (!_2149)
        {
            float4 param_158 = e;
            float4 param_159 = c;
            _2159 = eq(param_158, param_159).z;
        }
        else
        {
            _2159 = _2149;
        }
        _2160 = _2159;
    }
    else
    {
        _2160 = _2091;
    }
    edr.z = _2160;
    bool _2169 = (wd1.w < wd2.w) && interp_restriction_lv1.w;
    bool _2238;
    if (_2169)
    {
        float4 param_160 = f;
        float4 param_161 = b;
        bool _2178 = !eq(param_160, param_161).w;
        bool _2192;
        if (_2178)
        {
            float4 param_162 = f;
            float4 param_163 = c;
            float4 param_164 = f;
            float4 param_165 = b;
            _2192 = !id(param_162, param_163, param_164, param_165).w;
        }
        else
        {
            _2192 = _2178;
        }
        bool _2217;
        if (!_2192)
        {
            float4 param_166 = h;
            float4 param_167 = d;
            bool _2202 = !eq(param_166, param_167).w;
            bool _2216;
            if (_2202)
            {
                float4 param_168 = h;
                float4 param_169 = g;
                float4 param_170 = h;
                float4 param_171 = d;
                _2216 = !id(param_168, param_169, param_170, param_171).w;
            }
            else
            {
                _2216 = _2202;
            }
            _2217 = _2216;
        }
        else
        {
            _2217 = _2192;
        }
        bool _2227;
        if (!_2217)
        {
            float4 param_172 = e;
            float4 param_173 = g;
            _2227 = eq(param_172, param_173).w;
        }
        else
        {
            _2227 = _2217;
        }
        bool _2237;
        if (!_2227)
        {
            float4 param_174 = e;
            float4 param_175 = c;
            _2237 = eq(param_174, param_175).w;
        }
        else
        {
            _2237 = _2227;
        }
        _2238 = _2237;
    }
    else
    {
        _2238 = _2169;
    }
    edr.w = _2238;
    float4 param_176 = f;
    float4 param_177 = g;
    float4 param_178 = h;
    float4 param_179 = c;
    bool4 edr_left;
    edr_left.x = (((2.0f * df(param_176, param_177).x) <= df(param_178, param_179).x) && interp_restriction_lv2_left.x) && edr.x;
    float4 param_180 = f;
    float4 param_181 = g;
    float4 param_182 = h;
    float4 param_183 = c;
    edr_left.y = (((2.0f * df(param_180, param_181).y) <= df(param_182, param_183).y) && interp_restriction_lv2_left.y) && edr.y;
    float4 param_184 = f;
    float4 param_185 = g;
    float4 param_186 = h;
    float4 param_187 = c;
    edr_left.z = (((2.0f * df(param_184, param_185).z) <= df(param_186, param_187).z) && interp_restriction_lv2_left.z) && edr.z;
    float4 param_188 = f;
    float4 param_189 = g;
    float4 param_190 = h;
    float4 param_191 = c;
    edr_left.w = (((2.0f * df(param_188, param_189).w) <= df(param_190, param_191).w) && interp_restriction_lv2_left.w) && edr.w;
    float4 param_192 = f;
    float4 param_193 = g;
    float4 param_194 = h;
    float4 param_195 = c;
    bool4 edr_up;
    edr_up.x = ((df(param_192, param_193).x >= (2.0f * df(param_194, param_195).x)) && interp_restriction_lv2_up.x) && edr.x;
    float4 param_196 = f;
    float4 param_197 = g;
    float4 param_198 = h;
    float4 param_199 = c;
    edr_up.y = ((df(param_196, param_197).y >= (2.0f * df(param_198, param_199).y)) && interp_restriction_lv2_up.y) && edr.y;
    float4 param_200 = f;
    float4 param_201 = g;
    float4 param_202 = h;
    float4 param_203 = c;
    edr_up.z = ((df(param_200, param_201).z >= (2.0f * df(param_202, param_203).z)) && interp_restriction_lv2_up.z) && edr.z;
    float4 param_204 = f;
    float4 param_205 = g;
    float4 param_206 = h;
    float4 param_207 = c;
    edr_up.w = ((df(param_204, param_205).w >= (2.0f * df(param_206, param_207).w)) && interp_restriction_lv2_up.w) && edr.w;
    float4 param_208 = f;
    float4 param_209 = g0;
    float4 param_210 = h;
    float4 param_211 = c1;
    bool4 edr3_left;
    edr3_left.x = (((4.0f * df(param_208, param_209).x) <= df(param_210, param_211).x) && interp_restriction_lv3_left.x) && edr_left.x;
    float4 param_212 = f;
    float4 param_213 = g0;
    float4 param_214 = h;
    float4 param_215 = c1;
    edr3_left.y = (((4.0f * df(param_212, param_213).y) <= df(param_214, param_215).y) && interp_restriction_lv3_left.y) && edr_left.y;
    float4 param_216 = f;
    float4 param_217 = g0;
    float4 param_218 = h;
    float4 param_219 = c1;
    edr3_left.w = (((4.0f * df(param_216, param_217).w) <= df(param_218, param_219).w) && interp_restriction_lv3_left.w) && edr_left.w;
    float4 param_220 = f;
    float4 param_221 = g0;
    float4 param_222 = h;
    float4 param_223 = c1;
    edr3_left.z = (((4.0f * df(param_220, param_221).z) <= df(param_222, param_223).z) && interp_restriction_lv3_left.z) && edr_left.z;
    float4 param_224 = f;
    float4 param_225 = g0;
    float4 param_226 = h;
    float4 param_227 = c1;
    bool4 edr3_up;
    edr3_up.x = ((df(param_224, param_225).x >= (4.0f * df(param_226, param_227).x)) && interp_restriction_lv3_up.x) && edr_up.x;
    float4 param_228 = f;
    float4 param_229 = g0;
    float4 param_230 = h;
    float4 param_231 = c1;
    edr3_up.y = ((df(param_228, param_229).y >= (4.0f * df(param_230, param_231).y)) && interp_restriction_lv3_up.y) && edr_up.y;
    float4 param_232 = f;
    float4 param_233 = g0;
    float4 param_234 = h;
    float4 param_235 = c1;
    edr3_up.w = ((df(param_232, param_233).w >= (4.0f * df(param_234, param_235).w)) && interp_restriction_lv3_up.w) && edr_up.w;
    float4 param_236 = f;
    float4 param_237 = g0;
    float4 param_238 = h;
    float4 param_239 = c1;
    edr3_up.z = ((df(param_236, param_237).z >= (4.0f * df(param_238, param_239).z)) && interp_restriction_lv3_up.z) && edr_up.z;
    float4 param_240 = f;
    float4 param_241 = g2;
    float4 param_242 = h;
    float4 param_243 = c3;
    bool4 edr4_left;
    edr4_left.x = (((4.0f * df(param_240, param_241).x) <= df(param_242, param_243).x) && interp_restriction_lv4_left.x) && edr3_left.x;
    float4 param_244 = f;
    float4 param_245 = g2;
    float4 param_246 = h;
    float4 param_247 = c3;
    edr4_left.y = (((4.0f * df(param_244, param_245).y) <= df(param_246, param_247).y) && interp_restriction_lv4_left.y) && edr3_left.y;
    float4 param_248 = f;
    float4 param_249 = g2;
    float4 param_250 = h;
    float4 param_251 = c3;
    edr4_left.z = (((4.0f * df(param_248, param_249).z) <= df(param_250, param_251).z) && interp_restriction_lv4_left.z) && edr3_left.z;
    float4 param_252 = f;
    float4 param_253 = g2;
    float4 param_254 = h;
    float4 param_255 = c3;
    edr4_left.w = (((4.0f * df(param_252, param_253).w) <= df(param_254, param_255).w) && interp_restriction_lv4_left.w) && edr3_left.w;
    float4 param_256 = f;
    float4 param_257 = g2;
    float4 param_258 = h;
    float4 param_259 = c3;
    bool4 edr4_up;
    edr4_up.x = ((df(param_256, param_257).x >= (4.0f * df(param_258, param_259).x)) && interp_restriction_lv4_up.x) && edr3_up.x;
    float4 param_260 = f;
    float4 param_261 = g2;
    float4 param_262 = h;
    float4 param_263 = c3;
    edr4_up.y = ((df(param_260, param_261).y >= (4.0f * df(param_262, param_263).y)) && interp_restriction_lv4_up.y) && edr3_up.y;
    float4 param_264 = f;
    float4 param_265 = g2;
    float4 param_266 = h;
    float4 param_267 = c3;
    edr4_up.z = ((df(param_264, param_265).z >= (4.0f * df(param_266, param_267).z)) && interp_restriction_lv4_up.z) && edr3_up.z;
    float4 param_268 = f;
    float4 param_269 = g2;
    float4 param_270 = h;
    float4 param_271 = c3;
    edr4_up.w = ((df(param_268, param_269).w >= (4.0f * df(param_270, param_271).w)) && interp_restriction_lv4_up.w) && edr3_up.w;
    bool _2758;
    if (edr4_left.x)
    {
        _2758 = !edr4_up.x;
    }
    else
    {
        _2758 = edr4_left.x;
    }
    float _2759;
    if (_2758)
    {
        _2759 = 8.0f;
    }
    else
    {
        bool _2771;
        if (edr4_up.x)
        {
            _2771 = !edr4_left.x;
        }
        else
        {
            _2771 = edr4_up.x;
        }
        float _2772;
        if (_2771)
        {
            _2772 = 7.0f;
        }
        else
        {
            bool _2784;
            if (edr3_left.x)
            {
                _2784 = !edr3_up.x;
            }
            else
            {
                _2784 = edr3_left.x;
            }
            float _2785;
            if (_2784)
            {
                _2785 = 6.0f;
            }
            else
            {
                bool _2797;
                if (edr3_up.x)
                {
                    _2797 = !edr3_left.x;
                }
                else
                {
                    _2797 = edr3_up.x;
                }
                float _2798;
                if (_2797)
                {
                    _2798 = 5.0f;
                }
                else
                {
                    bool _2810;
                    if (edr_left.x)
                    {
                        _2810 = !edr_up.x;
                    }
                    else
                    {
                        _2810 = edr_left.x;
                    }
                    float _2811;
                    if (_2810)
                    {
                        _2811 = 4.0f;
                    }
                    else
                    {
                        bool _2822;
                        if (edr_up.x)
                        {
                            _2822 = !edr_left.x;
                        }
                        else
                        {
                            _2822 = edr_up.x;
                        }
                        float _2823;
                        if (_2822)
                        {
                            _2823 = 3.0f;
                        }
                        else
                        {
                            float _2829;
                            if (edr.x)
                            {
                                _2829 = 2.0f;
                            }
                            else
                            {
                                _2829 = float(edr0.x);
                            }
                            _2823 = _2829;
                        }
                        _2811 = _2823;
                    }
                    _2798 = _2811;
                }
                _2785 = _2798;
            }
            _2772 = _2785;
        }
        _2759 = _2772;
    }
    float4 info;
    info.x = _2759;
    bool _2851;
    if (edr4_left.y)
    {
        _2851 = !edr4_up.y;
    }
    else
    {
        _2851 = edr4_left.y;
    }
    float _2852;
    if (_2851)
    {
        _2852 = 8.0f;
    }
    else
    {
        bool _2863;
        if (edr4_up.y)
        {
            _2863 = !edr4_left.y;
        }
        else
        {
            _2863 = edr4_up.y;
        }
        float _2864;
        if (_2863)
        {
            _2864 = 7.0f;
        }
        else
        {
            bool _2875;
            if (edr3_left.y)
            {
                _2875 = !edr3_up.y;
            }
            else
            {
                _2875 = edr3_left.y;
            }
            float _2876;
            if (_2875)
            {
                _2876 = 6.0f;
            }
            else
            {
                bool _2887;
                if (edr3_up.y)
                {
                    _2887 = !edr3_left.y;
                }
                else
                {
                    _2887 = edr3_up.y;
                }
                float _2888;
                if (_2887)
                {
                    _2888 = 5.0f;
                }
                else
                {
                    bool _2899;
                    if (edr_left.y)
                    {
                        _2899 = !edr_up.y;
                    }
                    else
                    {
                        _2899 = edr_left.y;
                    }
                    float _2900;
                    if (_2899)
                    {
                        _2900 = 4.0f;
                    }
                    else
                    {
                        bool _2911;
                        if (edr_up.y)
                        {
                            _2911 = !edr_left.y;
                        }
                        else
                        {
                            _2911 = edr_up.y;
                        }
                        float _2912;
                        if (_2911)
                        {
                            _2912 = 3.0f;
                        }
                        else
                        {
                            float _2918;
                            if (edr.y)
                            {
                                _2918 = 2.0f;
                            }
                            else
                            {
                                _2918 = float(edr0.y);
                            }
                            _2912 = _2918;
                        }
                        _2900 = _2912;
                    }
                    _2888 = _2900;
                }
                _2876 = _2888;
            }
            _2864 = _2876;
        }
        _2852 = _2864;
    }
    info.y = _2852;
    bool _2940;
    if (edr4_left.z)
    {
        _2940 = !edr4_up.z;
    }
    else
    {
        _2940 = edr4_left.z;
    }
    float _2941;
    if (_2940)
    {
        _2941 = 8.0f;
    }
    else
    {
        bool _2952;
        if (edr4_up.z)
        {
            _2952 = !edr4_left.z;
        }
        else
        {
            _2952 = edr4_up.z;
        }
        float _2953;
        if (_2952)
        {
            _2953 = 7.0f;
        }
        else
        {
            bool _2964;
            if (edr3_left.z)
            {
                _2964 = !edr3_up.z;
            }
            else
            {
                _2964 = edr3_left.z;
            }
            float _2965;
            if (_2964)
            {
                _2965 = 6.0f;
            }
            else
            {
                bool _2976;
                if (edr3_up.z)
                {
                    _2976 = !edr3_left.z;
                }
                else
                {
                    _2976 = edr3_up.z;
                }
                float _2977;
                if (_2976)
                {
                    _2977 = 5.0f;
                }
                else
                {
                    bool _2988;
                    if (edr_left.z)
                    {
                        _2988 = !edr_up.z;
                    }
                    else
                    {
                        _2988 = edr_left.z;
                    }
                    float _2989;
                    if (_2988)
                    {
                        _2989 = 4.0f;
                    }
                    else
                    {
                        bool _3000;
                        if (edr_up.z)
                        {
                            _3000 = !edr_left.z;
                        }
                        else
                        {
                            _3000 = edr_up.z;
                        }
                        float _3001;
                        if (_3000)
                        {
                            _3001 = 3.0f;
                        }
                        else
                        {
                            float _3007;
                            if (edr.z)
                            {
                                _3007 = 2.0f;
                            }
                            else
                            {
                                _3007 = float(edr0.z);
                            }
                            _3001 = _3007;
                        }
                        _2989 = _3001;
                    }
                    _2977 = _2989;
                }
                _2965 = _2977;
            }
            _2953 = _2965;
        }
        _2941 = _2953;
    }
    info.z = _2941;
    bool _3029;
    if (edr4_left.w)
    {
        _3029 = !edr4_up.w;
    }
    else
    {
        _3029 = edr4_left.w;
    }
    float _3030;
    if (_3029)
    {
        _3030 = 8.0f;
    }
    else
    {
        bool _3041;
        if (edr4_up.w)
        {
            _3041 = !edr4_left.w;
        }
        else
        {
            _3041 = edr4_up.w;
        }
        float _3042;
        if (_3041)
        {
            _3042 = 7.0f;
        }
        else
        {
            bool _3053;
            if (edr3_left.w)
            {
                _3053 = !edr3_up.w;
            }
            else
            {
                _3053 = edr3_left.w;
            }
            float _3054;
            if (_3053)
            {
                _3054 = 6.0f;
            }
            else
            {
                bool _3065;
                if (edr3_up.w)
                {
                    _3065 = !edr3_left.w;
                }
                else
                {
                    _3065 = edr3_up.w;
                }
                float _3066;
                if (_3065)
                {
                    _3066 = 5.0f;
                }
                else
                {
                    bool _3077;
                    if (edr_left.w)
                    {
                        _3077 = !edr_up.w;
                    }
                    else
                    {
                        _3077 = edr_left.w;
                    }
                    float _3078;
                    if (_3077)
                    {
                        _3078 = 4.0f;
                    }
                    else
                    {
                        bool _3089;
                        if (edr_up.w)
                        {
                            _3089 = !edr_left.w;
                        }
                        else
                        {
                            _3089 = edr_up.w;
                        }
                        float _3090;
                        if (_3089)
                        {
                            _3090 = 3.0f;
                        }
                        else
                        {
                            float _3096;
                            if (edr.w)
                            {
                                _3096 = 2.0f;
                            }
                            else
                            {
                                _3096 = float(edr0.w);
                            }
                            _3090 = _3096;
                        }
                        _3078 = _3090;
                    }
                    _3066 = _3078;
                }
                _3054 = _3066;
            }
            _3042 = _3054;
        }
        _3030 = _3042;
    }
    info.w = _3030;
    float4 param_272 = info;
    float4 param_273 = 255.0f.xxxx;
    FragColor = remapTo01(param_272, param_273);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	TEX0.x = input.uv.x;
	TEX0.y = input.uv.y;
	float2 ps = float2(1.0f / SourceSize.xy.x, 1.0f / SourceSize.xy.y);
	float dx = ps.x;
	float dy = ps.y;
	t1 = float2(dx, dy);

	frag_main();

	return FragColor;
}
