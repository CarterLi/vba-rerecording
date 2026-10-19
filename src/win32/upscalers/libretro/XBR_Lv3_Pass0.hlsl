// XBR_Lv3_Pass0, pass 1 of 2 of the xBR lv3 Multipass pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-lv3-pass0.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_LV3_PASS0_GLSL"; this is the port of it, because Direct3D
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

Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 t1;
static float4 t2;
static float4 t3;
static float4 t4;
static float4 t5;
static float4 t6;
static float4 t7;
static float4 FragColor;
static float4 TEX0;

bool4 and(bool4 A, bool4 B)
{
    return bool4(A.x && B.x, A.y && B.y, A.z && B.z, A.w && B.w);
}

float4 df(float4 A, float4 B)
{
    return abs(A - B);
}

bool4 eq(float4 A, float4 B)
{
    float4 param = A;
    float4 param_1 = B;
    float4 _61 = df(param, param_1);
    return bool4(_61.x < 15.0f.xxxx.x, _61.y < 15.0f.xxxx.y, _61.z < 15.0f.xxxx.z, _61.w < 15.0f.xxxx.w);
}

bool4 or(bool4 A, bool4 B)
{
    return bool4(A.x || B.x, A.y || B.y, A.z || B.z, A.w || B.w);
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

float4 remapTo01(float4 v, float4 high)
{
    return v / high;
}

void frag_main()
{
    float3 A1 = frameTex.Sample(frameSampler, t1.xw).xyz;
    float3 B1 = frameTex.Sample(frameSampler, t1.yw).xyz;
    float3 C1 = frameTex.Sample(frameSampler, t1.zw).xyz;
    float3 A = frameTex.Sample(frameSampler, t2.xw).xyz;
    float3 B = frameTex.Sample(frameSampler, t2.yw).xyz;
    float3 C = frameTex.Sample(frameSampler, t2.zw).xyz;
    float3 D = frameTex.Sample(frameSampler, t3.xw).xyz;
    float3 E = frameTex.Sample(frameSampler, t3.yw).xyz;
    float3 F = frameTex.Sample(frameSampler, t3.zw).xyz;
    float3 G = frameTex.Sample(frameSampler, t4.xw).xyz;
    float3 H = frameTex.Sample(frameSampler, t4.yw).xyz;
    float3 I = frameTex.Sample(frameSampler, t4.zw).xyz;
    float3 G5 = frameTex.Sample(frameSampler, t5.xw).xyz;
    float3 H5 = frameTex.Sample(frameSampler, t5.yw).xyz;
    float3 I5 = frameTex.Sample(frameSampler, t5.zw).xyz;
    float3 A0 = frameTex.Sample(frameSampler, t6.xy).xyz;
    float3 D0 = frameTex.Sample(frameSampler, t6.xz).xyz;
    float3 G0 = frameTex.Sample(frameSampler, t6.xw).xyz;
    float3 C4 = frameTex.Sample(frameSampler, t7.xy).xyz;
    float3 F4 = frameTex.Sample(frameSampler, t7.xz).xyz;
    float3 I4 = frameTex.Sample(frameSampler, t7.xw).xyz;
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
    float4 c1 = i4.yzwx;
    float4 g0 = i5.wxyz;
    float4 b1 = h5.zwxy;
    float4 d0 = h5.wxyz;
    bool4 param = bool4(e.x != f.x, e.y != f.y, e.z != f.z, e.w != f.w);
    bool4 param_1 = bool4(e.x != h.x, e.y != h.y, e.z != h.z, e.w != h.w);
    bool4 interp_restriction_lv0 = and(param, param_1);
    float4 param_2 = h;
    float4 param_3 = h5;
    bool4 _464 = eq(param_2, param_3);
    float4 param_4 = h;
    float4 param_5 = i5;
    bool4 _470 = eq(param_4, param_5);
    bool4 param_6 = bool4(!_464.x, !_464.y, !_464.z, !_464.w);
    bool4 param_7 = bool4(!_470.x, !_470.y, !_470.z, !_470.w);
    bool4 comp1 = and(param_6, param_7);
    float4 param_8 = h;
    float4 param_9 = d;
    bool4 _480 = eq(param_8, param_9);
    float4 param_10 = h;
    float4 param_11 = g;
    bool4 _486 = eq(param_10, param_11);
    bool4 param_12 = bool4(!_480.x, !_480.y, !_480.z, !_480.w);
    bool4 param_13 = bool4(!_486.x, !_486.y, !_486.z, !_486.w);
    bool4 comp2 = and(param_12, param_13);
    float4 param_14 = f;
    float4 param_15 = f4;
    bool4 _496 = eq(param_14, param_15);
    float4 param_16 = f;
    float4 param_17 = i4;
    bool4 _502 = eq(param_16, param_17);
    bool4 param_18 = bool4(!_496.x, !_496.y, !_496.z, !_496.w);
    bool4 param_19 = bool4(!_502.x, !_502.y, !_502.z, !_502.w);
    bool4 comp3 = and(param_18, param_19);
    float4 param_20 = f;
    float4 param_21 = b;
    bool4 _512 = eq(param_20, param_21);
    float4 param_22 = f;
    float4 param_23 = c;
    bool4 _518 = eq(param_22, param_23);
    bool4 param_24 = bool4(!_512.x, !_512.y, !_512.z, !_512.w);
    bool4 param_25 = bool4(!_518.x, !_518.y, !_518.z, !_518.w);
    bool4 comp4 = and(param_24, param_25);
    float4 param_26 = e;
    float4 param_27 = i;
    bool4 param_28 = comp3;
    bool4 param_29 = comp1;
    bool4 param_30 = eq(param_26, param_27);
    bool4 param_31 = or(param_28, param_29);
    bool4 comp5 = and(param_30, param_31);
    float4 param_32 = e;
    float4 param_33 = g;
    float4 param_34 = e;
    float4 param_35 = c;
    bool4 param_36 = eq(param_32, param_33);
    bool4 param_37 = eq(param_34, param_35);
    bool4 param_38 = comp5;
    bool4 param_39 = or(param_36, param_37);
    bool4 param_40 = comp2;
    bool4 param_41 = or(param_38, param_39);
    bool4 param_42 = comp4;
    bool4 param_43 = or(param_40, param_41);
    bool4 interp_restriction_lv1 = or(param_42, param_43);
    bool4 param_44 = bool4(e.x != g.x, e.y != g.y, e.z != g.z, e.w != g.w);
    bool4 param_45 = bool4(d.x != g.x, d.y != g.y, d.z != g.z, d.w != g.w);
    bool4 interp_restriction_lv2_left = and(param_44, param_45);
    bool4 param_46 = bool4(e.x != c.x, e.y != c.y, e.z != c.z, e.w != c.w);
    bool4 param_47 = bool4(b.x != c.x, b.y != c.y, b.z != c.z, b.w != c.w);
    bool4 interp_restriction_lv2_up = and(param_46, param_47);
    bool4 param_48 = bool4(e.x != g0.x, e.y != g0.y, e.z != g0.z, e.w != g0.w);
    bool4 param_49 = bool4(d0.x != g0.x, d0.y != g0.y, d0.z != g0.z, d0.w != g0.w);
    bool4 interp_restriction_lv3_left = and(param_48, param_49);
    bool4 param_50 = bool4(e.x != c1.x, e.y != c1.y, e.z != c1.z, e.w != c1.w);
    bool4 param_51 = bool4(b1.x != c1.x, b1.y != c1.y, b1.z != c1.z, b1.w != c1.w);
    bool4 interp_restriction_lv3_up = and(param_50, param_51);
    float4 param_52 = e;
    float4 param_53 = c;
    float4 param_54 = g;
    float4 param_55 = i;
    float4 param_56 = h5;
    float4 param_57 = f4;
    float4 param_58 = h;
    float4 param_59 = f;
    float4 _620 = weighted_distance(param_52, param_53, param_54, param_55, param_56, param_57, param_58, param_59);
    float4 param_60 = h;
    float4 param_61 = d;
    float4 param_62 = i5;
    float4 param_63 = f;
    float4 param_64 = i4;
    float4 param_65 = b;
    float4 param_66 = e;
    float4 param_67 = i;
    float4 _637 = weighted_distance(param_60, param_61, param_62, param_63, param_64, param_65, param_66, param_67);
    bool4 param_68 = bool4(_620.x < _637.x, _620.y < _637.y, _620.z < _637.z, _620.w < _637.w);
    bool4 param_69 = interp_restriction_lv0;
    bool4 edr0 = and(param_68, param_69);
    bool4 param_70 = edr0;
    bool4 param_71 = interp_restriction_lv1;
    bool4 edr = and(param_70, param_71);
    float4 param_72 = f;
    float4 param_73 = g;
    float4 _656 = df(param_72, param_73) * 2.0f;
    float4 param_74 = h;
    float4 param_75 = c;
    float4 _661 = df(param_74, param_75);
    bool4 param_76 = interp_restriction_lv2_left;
    bool4 param_77 = edr;
    bool4 param_78 = bool4(_656.x <= _661.x, _656.y <= _661.y, _656.z <= _661.z, _656.w <= _661.w);
    bool4 param_79 = and(param_76, param_77);
    bool4 edr_left = and(param_78, param_79);
    float4 param_80 = f;
    float4 param_81 = g;
    float4 _676 = df(param_80, param_81);
    float4 param_82 = h;
    float4 param_83 = c;
    float4 _682 = df(param_82, param_83) * 2.0f;
    bool4 param_84 = interp_restriction_lv2_up;
    bool4 param_85 = edr;
    bool4 param_86 = bool4(_676.x >= _682.x, _676.y >= _682.y, _676.z >= _682.z, _676.w >= _682.w);
    bool4 param_87 = and(param_84, param_85);
    bool4 edr_up = and(param_86, param_87);
    float4 param_88 = f;
    float4 param_89 = g0;
    float4 _698 = df(param_88, param_89) * 4.0f;
    float4 param_90 = h;
    float4 param_91 = c1;
    float4 _703 = df(param_90, param_91);
    bool4 param_92 = interp_restriction_lv3_left;
    bool4 param_93 = edr_left;
    bool4 param_94 = bool4(_698.x <= _703.x, _698.y <= _703.y, _698.z <= _703.z, _698.w <= _703.w);
    bool4 param_95 = and(param_92, param_93);
    bool4 edr3_left = and(param_94, param_95);
    float4 param_96 = f;
    float4 param_97 = g0;
    float4 _718 = df(param_96, param_97);
    float4 param_98 = h;
    float4 param_99 = c1;
    float4 _724 = df(param_98, param_99) * 4.0f;
    bool4 param_100 = interp_restriction_lv3_up;
    bool4 param_101 = edr_up;
    bool4 param_102 = bool4(_718.x >= _724.x, _718.y >= _724.y, _718.z >= _724.z, _718.w >= _724.w);
    bool4 param_103 = and(param_100, param_101);
    bool4 edr3_up = and(param_102, param_103);
    float4 param_104 = e;
    float4 param_105 = f;
    float4 _739 = df(param_104, param_105);
    float4 param_106 = e;
    float4 param_107 = h;
    float4 _744 = df(param_106, param_107);
    bool4 px = bool4(_739.x <= _744.x, _739.y <= _744.y, _739.z <= _744.z, _739.w <= _744.w);
    bool4 lin3 = bool4(true, true, true, true);
    bool4 lin2 = bool4(true, true, true, true);
    bool4 lin1 = bool4(true, true, true, true);
    bool4 lin0 = bool4(true, true, true, true);
    bool _759;
    if (edr_left.x)
    {
        _759 = !edr_up.x;
    }
    else
    {
        _759 = edr_left.x;
    }
    bool2 px0;
    bool2 px3;
    bool2 px1;
    if (_759)
    {
        px0 = bool2(false, px.x);
        px3 = bool2(px.x, true);
        if (edr3_left.x)
        {
            lin0 = bool4(false, true, false, false);
            lin3 = bool4(true, false, false, false);
        }
        else
        {
            lin0 = bool4(false, false, true, false);
            lin3 = bool4(false, true, true, false);
        }
    }
    else
    {
        bool _790;
        if (edr_up.x)
        {
            _790 = !edr_left.x;
        }
        else
        {
            _790 = edr_up.x;
        }
        if (_790)
        {
            px0 = bool2(false, px.x);
            px1 = bool2(!px.x, false);
            if (edr3_up.x)
            {
                lin0 = bool4(false, true, false, true);
                lin1 = bool4(true, false, false, true);
            }
            else
            {
                lin0 = bool4(false, false, true, true);
                lin1 = bool4(false, true, true, true);
            }
        }
        else
        {
            if (edr.x)
            {
                edr3_up.x = false;
                edr3_left.x = false;
                edr_up.x = false;
                edr_left.x = false;
                px0 = bool2(false, px.x);
                lin0 = bool4(false, false, false, true);
            }
            else
            {
                if (edr0.x)
                {
                    edr3_up.x = false;
                    edr3_left.x = false;
                    edr_up.x = false;
                    edr_left.x = false;
                    px0 = bool2(false, px.x);
                    lin0 = bool4(false, false, false, false);
                }
            }
        }
    }
    bool _843;
    if (edr_left.y)
    {
        _843 = !edr_up.y;
    }
    else
    {
        _843 = edr_left.y;
    }
    bool2 px2;
    if (_843)
    {
        px1 = bool2(false, px.y);
        px0 = bool2(px.y, true);
        if (edr3_left.y)
        {
            lin1 = bool4(false, true, false, false);
            lin0 = bool4(true, false, false, false);
        }
        else
        {
            lin1 = bool4(false, false, true, false);
            lin0 = bool4(false, true, true, false);
        }
    }
    else
    {
        bool _865;
        if (edr_up.y)
        {
            _865 = !edr_left.y;
        }
        else
        {
            _865 = edr_up.y;
        }
        if (_865)
        {
            px1 = bool2(false, px.y);
            px2 = bool2(!px.y, false);
            if (edr3_up.y)
            {
                lin1 = bool4(false, true, false, true);
                lin2 = bool4(true, false, false, true);
            }
            else
            {
                lin1 = bool4(false, false, true, true);
                lin2 = bool4(false, true, true, true);
            }
        }
        else
        {
            if (edr.y)
            {
                edr3_up.y = false;
                edr3_left.y = false;
                edr_up.y = false;
                edr_left.y = false;
                px1 = bool2(false, px.y);
                lin1 = bool4(false, false, false, true);
            }
            else
            {
                if (edr0.y)
                {
                    edr3_up.y = false;
                    edr3_left.y = false;
                    edr_up.y = false;
                    edr_left.y = false;
                    px1 = bool2(false, px.y);
                    lin1 = bool4(false, false, false, false);
                }
            }
        }
    }
    bool _912;
    if (edr_left.z)
    {
        _912 = !edr_up.z;
    }
    else
    {
        _912 = edr_left.z;
    }
    if (_912)
    {
        px2 = bool2(false, px.z);
        px1 = bool2(px.z, true);
        if (edr3_left.z)
        {
            lin2 = bool4(false, true, false, false);
            lin1 = bool4(true, false, false, false);
        }
        else
        {
            lin2 = bool4(false, false, true, false);
            lin1 = bool4(false, true, true, false);
        }
    }
    else
    {
        bool _934;
        if (edr_up.z)
        {
            _934 = !edr_left.z;
        }
        else
        {
            _934 = edr_up.z;
        }
        if (_934)
        {
            px2 = bool2(false, px.z);
            px3 = bool2(!px.z, false);
            if (edr3_up.z)
            {
                lin2 = bool4(false, true, false, true);
                lin3 = bool4(true, false, false, true);
            }
            else
            {
                lin2 = bool4(false, false, true, true);
                lin3 = bool4(false, true, true, true);
            }
        }
        else
        {
            if (edr.z)
            {
                edr3_up.z = false;
                edr3_left.z = false;
                edr_up.z = false;
                edr_left.z = false;
                px2 = bool2(false, px.z);
                lin2 = bool4(false, false, false, true);
            }
            else
            {
                if (edr0.z)
                {
                    edr3_up.z = false;
                    edr3_left.z = false;
                    edr_up.z = false;
                    edr_left.z = false;
                    px2 = bool2(false, px.z);
                    lin2 = bool4(false, false, false, false);
                }
            }
        }
    }
    bool _980;
    if (edr_left.w)
    {
        _980 = !edr_up.w;
    }
    else
    {
        _980 = edr_left.w;
    }
    if (_980)
    {
        px3 = bool2(false, px.w);
        px2 = bool2(px.w, true);
        if (edr3_left.w)
        {
            lin3 = bool4(false, true, false, false);
            lin2 = bool4(true, false, false, false);
        }
        else
        {
            lin3 = bool4(false, false, true, false);
            lin2 = bool4(false, true, true, false);
        }
    }
    else
    {
        bool _1002;
        if (edr_up.w)
        {
            _1002 = !edr_left.w;
        }
        else
        {
            _1002 = edr_up.w;
        }
        if (_1002)
        {
            px3 = bool2(false, px.w);
            px0 = bool2(!px.w, false);
            if (edr3_up.w)
            {
                lin3 = bool4(false, true, false, true);
                lin0 = bool4(true, false, false, true);
            }
            else
            {
                lin3 = bool4(false, false, true, true);
                lin0 = bool4(false, true, true, true);
            }
        }
        else
        {
            if (edr.w)
            {
                edr3_up.w = false;
                edr3_left.w = false;
                edr_up.w = false;
                edr_left.w = false;
                px3 = bool2(false, px.w);
                lin3 = bool4(false, false, false, true);
            }
            else
            {
                if (edr0.w)
                {
                    edr3_up.w = false;
                    edr3_left.w = false;
                    edr_up.w = false;
                    edr_left.w = false;
                    px3 = bool2(false, px.w);
                    lin3 = bool4(false, false, false, false);
                }
            }
        }
    }
    float4 info = mul(float4(1.0f, 2.0f, 4.0f, 8.0f), float4x4(float4(float4(edr3_left)), float4(float4(edr3_up)), float4(float(px0.x), float(px1.x), float(px2.x), float(px3.x)), float4(float(px0.y), float(px1.y), float(px2.y), float(px3.y))));
    info += mul(float4(16.0f, 32.0f, 64.0f, 128.0f), float4x4(float4(float(lin0.x), float(lin1.x), float(lin2.x), float(lin3.x)), float4(float(lin0.y), float(lin1.y), float(lin2.y), float(lin3.y)), float4(float(lin0.z), float(lin1.z), float(lin2.z), float(lin3.z)), float4(float(lin0.w), float(lin1.w), float(lin2.w), float(lin3.w))));
    float4 param_108 = info;
    float4 param_109 = 255.0f.xxxx;
    FragColor = remapTo01(param_108, param_109);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	float2 _32 = input.uv * 1.00039994716644287109375f;
	TEX0.x = _32.x;
	TEX0.y = _32.y;
	float2 ps = 1.0f.xx / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy;
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
