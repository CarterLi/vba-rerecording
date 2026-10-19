// ScaleFX Pass3, pass 3 of the ScaleFX pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "scalefx-pass3.glsl" (libretro/glsl-shaders,
// scalefx/shaders).  The OpenGL 3.3 renderer runs that shader as it is upstream,
// from the resource "SCALEFX_PASS3_GLSL"; this is the port of it, because
// Direct3D compiles HLSL.
//
// ScaleFX interpolates the edges of pixel art and only ever uses colours that
// are already in the picture.  It is a chain of five passes because each one
// hands the next a whole frame of per-pixel data:
//
//   pass 0  the colour distance of the neighbourhood, four at a time
//   pass 1  the strength of the interpolation candidates
//   pass 2  resolves the ambiguous configurations at pixel junctions
//   pass 3  the edge level, and the tags the last pass reads
//   pass 4  the subpixels, at whatever size the target has
//
// The first four run at the size of the frame, so they cost a quarter of a pass
// at the target size, not four of them.
//
// The licence of the ported shader:
//
//   Copyright (c) 2016 Sp00kyFox - ScaleFX@web.de
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
//
cbuffer UpscalerConstants : register(b0)
{
	float4 SourceSize;
	float4 OutputSize;
	float4 Scale;
};

Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 FragColor;

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float2 y)
{
    return x - y * floor(x / y);
}

float3 mod(float3 x, float3 y)
{
    return x - y * floor(x / y);
}

float4 mod(float4 x, float4 y)
{
    return x - y * floor(x / y);
}

bool4 loadCorn(float4 x)
{
    float4 _33 = floor(mod((x * 15.0f) + 0.5f.xxxx, 2.0f.xxxx));
    return bool4(_33.x != 0.0f.xxxx.x, _33.y != 0.0f.xxxx.y, _33.z != 0.0f.xxxx.z, _33.w != 0.0f.xxxx.w);
}

bool4 loadHori(float4 x)
{
    float4 _47 = floor(mod((x * 7.5f) + 0.25f.xxxx, 2.0f.xxxx));
    return bool4(_47.x != 0.0f.xxxx.x, _47.y != 0.0f.xxxx.y, _47.z != 0.0f.xxxx.z, _47.w != 0.0f.xxxx.w);
}

bool4 loadVert(float4 x)
{
    float4 _59 = floor(mod((x * 3.75f) + 0.125f.xxxx, 2.0f.xxxx));
    return bool4(_59.x != 0.0f.xxxx.x, _59.y != 0.0f.xxxx.y, _59.z != 0.0f.xxxx.z, _59.w != 0.0f.xxxx.w);
}

bool4 loadOr(float4 x)
{
    float4 _71 = floor(mod((x * 1.875f) + 0.0625f.xxxx, 2.0f.xxxx));
    return bool4(_71.x != 0.0f.xxxx.x, _71.y != 0.0f.xxxx.y, _71.z != 0.0f.xxxx.z, _71.w != 0.0f.xxxx.w);
}

void frag_main()
{
    float4 E = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 0));
    float4 D = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, 0));
    float4 D0 = frameTex.Sample(frameSampler, TEX0.xy, int2(-2, 0));
    float4 D1 = frameTex.Sample(frameSampler, TEX0.xy, int2(-3, 0));
    float4 F = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 0));
    float4 F0 = frameTex.Sample(frameSampler, TEX0.xy, int2(2, 0));
    float4 F1 = frameTex.Sample(frameSampler, TEX0.xy, int2(3, 0));
    float4 B = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -1));
    float4 B0 = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -2));
    float4 B1 = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -3));
    float4 H = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 1));
    float4 H0 = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 2));
    float4 H1 = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 3));
    float4 param = E;
    bool4 Ec = loadCorn(param);
    float4 param_1 = E;
    bool4 Eh = loadHori(param_1);
    float4 param_2 = E;
    bool4 Ev = loadVert(param_2);
    float4 param_3 = E;
    bool4 Eo = loadOr(param_3);
    float4 param_4 = D;
    bool4 Dc = loadCorn(param_4);
    float4 param_5 = D;
    bool4 Dh = loadHori(param_5);
    float4 param_6 = D;
    bool4 Do = loadOr(param_6);
    float4 param_7 = D0;
    bool4 D0c = loadCorn(param_7);
    float4 param_8 = D0;
    bool4 D0h = loadHori(param_8);
    float4 param_9 = D1;
    bool4 D1h = loadHori(param_9);
    float4 param_10 = F;
    bool4 Fc = loadCorn(param_10);
    float4 param_11 = F;
    bool4 Fh = loadHori(param_11);
    float4 param_12 = F;
    bool4 Fo = loadOr(param_12);
    float4 param_13 = F0;
    bool4 F0c = loadCorn(param_13);
    float4 param_14 = F0;
    bool4 F0h = loadHori(param_14);
    float4 param_15 = F1;
    bool4 F1h = loadHori(param_15);
    float4 param_16 = B;
    bool4 Bc = loadCorn(param_16);
    float4 param_17 = B;
    bool4 Bv = loadVert(param_17);
    float4 param_18 = B;
    bool4 Bo = loadOr(param_18);
    float4 param_19 = B0;
    bool4 B0c = loadCorn(param_19);
    float4 param_20 = B0;
    bool4 B0v = loadVert(param_20);
    float4 param_21 = B1;
    bool4 B1v = loadVert(param_21);
    float4 param_22 = H;
    bool4 Hc = loadCorn(param_22);
    float4 param_23 = H;
    bool4 Hv = loadVert(param_23);
    float4 param_24 = H;
    bool4 Ho = loadOr(param_24);
    float4 param_25 = H0;
    bool4 H0c = loadCorn(param_25);
    float4 param_26 = H0;
    bool4 H0v = loadVert(param_26);
    float4 param_27 = H1;
    bool4 H1v = loadVert(param_27);
    bool _298;
    if (Ec.x)
    {
        _298 = (Dc.z || Bc.z) || true;
    }
    else
    {
        _298 = Ec.x;
    }
    bool lvl1x = _298;
    bool _312;
    if (Ec.y)
    {
        _312 = (Fc.w || Bc.w) || true;
    }
    else
    {
        _312 = Ec.y;
    }
    bool lvl1y = _312;
    bool _324;
    if (Ec.z)
    {
        _324 = (Fc.x || Hc.x) || true;
    }
    else
    {
        _324 = Ec.z;
    }
    bool lvl1z = _324;
    bool _336;
    if (Ec.w)
    {
        _336 = (Dc.y || Hc.y) || true;
    }
    else
    {
        _336 = Ec.w;
    }
    bool lvl1w = _336;
    bool2 lvl2x = bool2((Ec.x && Eh.y) && Dc.z, (Ec.y && Eh.x) && Fc.w);
    bool2 lvl2y = bool2((Ec.y && Ev.z) && Bc.w, (Ec.z && Ev.y) && Hc.x);
    bool2 lvl2z = bool2((Ec.w && Eh.z) && Dc.y, (Ec.z && Eh.w) && Fc.x);
    bool2 lvl2w = bool2((Ec.x && Ev.w) && Bc.z, (Ec.w && Ev.x) && Hc.y);
    bool _421;
    if (lvl2x.y)
    {
        _421 = Dh.y && Dh.x;
    }
    else
    {
        _421 = lvl2x.y;
    }
    bool _434;
    if (lvl2w.y)
    {
        _434 = Bv.w && Bv.x;
    }
    else
    {
        _434 = lvl2w.y;
    }
    bool2 lvl3x = bool2(_421 && Fh.z, _434 && Hv.z);
    bool _449;
    if (lvl2x.x)
    {
        _449 = Fh.x && Fh.y;
    }
    else
    {
        _449 = lvl2x.x;
    }
    bool _462;
    if (lvl2y.y)
    {
        _462 = Bv.z && Bv.y;
    }
    else
    {
        _462 = lvl2y.y;
    }
    bool2 lvl3y = bool2(_449 && Dh.w, _462 && Hv.w);
    bool _477;
    if (lvl2z.x)
    {
        _477 = Fh.w && Fh.z;
    }
    else
    {
        _477 = lvl2z.x;
    }
    bool _490;
    if (lvl2y.x)
    {
        _490 = Hv.y && Hv.z;
    }
    else
    {
        _490 = lvl2y.x;
    }
    bool2 lvl3z = bool2(_477 && Dh.x, _490 && Bv.x);
    bool _505;
    if (lvl2z.y)
    {
        _505 = Dh.z && Dh.w;
    }
    else
    {
        _505 = lvl2z.y;
    }
    bool _518;
    if (lvl2w.x)
    {
        _518 = Hv.x && Hv.w;
    }
    else
    {
        _518 = lvl2w.x;
    }
    bool2 lvl3w = bool2(_505 && Fh.y, _518 && Bv.y);
    bool _540 = ((((Dc.x && Dh.y) && Eh.x) && Eh.y) && Fh.x) && Fh.y;
    bool _548;
    if (_540)
    {
        _548 = D0c.z && D0h.w;
    }
    else
    {
        _548 = _540;
    }
    bool _565 = ((((Bc.x && Bv.w) && Ev.x) && Ev.w) && Hv.x) && Hv.w;
    bool _573;
    if (_565)
    {
        _573 = B0c.z && B0v.y;
    }
    else
    {
        _573 = _565;
    }
    bool2 lvl4x = bool2(_548, _573);
    bool _592 = ((((Fc.y && Fh.x) && Eh.y) && Eh.x) && Dh.y) && Dh.x;
    bool _600;
    if (_592)
    {
        _600 = F0c.w && F0h.z;
    }
    else
    {
        _600 = _592;
    }
    bool _617 = ((((Bc.y && Bv.z) && Ev.y) && Ev.z) && Hv.y) && Hv.z;
    bool _625;
    if (_617)
    {
        _625 = B0c.w && B0v.x;
    }
    else
    {
        _625 = _617;
    }
    bool2 lvl4y = bool2(_600, _625);
    bool _644 = ((((Fc.z && Fh.w) && Eh.z) && Eh.w) && Dh.z) && Dh.w;
    bool _652;
    if (_644)
    {
        _652 = F0c.x && F0h.y;
    }
    else
    {
        _652 = _644;
    }
    bool _669 = ((((Hc.z && Hv.y) && Ev.z) && Ev.y) && Bv.z) && Bv.y;
    bool _677;
    if (_669)
    {
        _677 = H0c.x && H0v.w;
    }
    else
    {
        _677 = _669;
    }
    bool2 lvl4z = bool2(_652, _677);
    bool _696 = ((((Dc.w && Dh.z) && Eh.w) && Eh.z) && Fh.w) && Fh.z;
    bool _704;
    if (_696)
    {
        _704 = D0c.y && D0h.x;
    }
    else
    {
        _704 = _696;
    }
    bool _721 = ((((Hc.w && Hv.x) && Ev.w) && Ev.x) && Bv.w) && Bv.x;
    bool _729;
    if (_721)
    {
        _729 = H0c.y && H0v.z;
    }
    else
    {
        _729 = _721;
    }
    bool2 lvl4w = bool2(_704, _729);
    bool _741;
    if (lvl4x.x)
    {
        _741 = F0h.x && F0h.y;
    }
    else
    {
        _741 = lvl4x.x;
    }
    bool _749;
    if (_741)
    {
        _749 = D1h.z && D1h.w;
    }
    else
    {
        _749 = _741;
    }
    bool _759;
    if (lvl4y.x)
    {
        _759 = D0h.y && D0h.x;
    }
    else
    {
        _759 = lvl4y.x;
    }
    bool _767;
    if (_759)
    {
        _767 = F1h.w && F1h.z;
    }
    else
    {
        _767 = _759;
    }
    bool2 lvl5x = bool2(_749, _767);
    bool _779;
    if (lvl4y.y)
    {
        _779 = H0v.y && H0v.z;
    }
    else
    {
        _779 = lvl4y.y;
    }
    bool _787;
    if (_779)
    {
        _787 = B1v.w && B1v.x;
    }
    else
    {
        _787 = _779;
    }
    bool _797;
    if (lvl4z.y)
    {
        _797 = B0v.z && B0v.y;
    }
    else
    {
        _797 = lvl4z.y;
    }
    bool _805;
    if (_797)
    {
        _805 = H1v.x && H1v.w;
    }
    else
    {
        _805 = _797;
    }
    bool2 lvl5y = bool2(_787, _805);
    bool _817;
    if (lvl4w.x)
    {
        _817 = F0h.w && F0h.z;
    }
    else
    {
        _817 = lvl4w.x;
    }
    bool _825;
    if (_817)
    {
        _825 = D1h.y && D1h.x;
    }
    else
    {
        _825 = _817;
    }
    bool _835;
    if (lvl4z.x)
    {
        _835 = D0h.z && D0h.w;
    }
    else
    {
        _835 = lvl4z.x;
    }
    bool _843;
    if (_835)
    {
        _843 = F1h.x && F1h.y;
    }
    else
    {
        _843 = _835;
    }
    bool2 lvl5z = bool2(_825, _843);
    bool _855;
    if (lvl4x.y)
    {
        _855 = H0v.x && H0v.w;
    }
    else
    {
        _855 = lvl4x.y;
    }
    bool _863;
    if (_855)
    {
        _863 = B1v.z && B1v.y;
    }
    else
    {
        _863 = _855;
    }
    bool _873;
    if (lvl4w.y)
    {
        _873 = B0v.w && B0v.x;
    }
    else
    {
        _873 = lvl4w.y;
    }
    bool _881;
    if (_873)
    {
        _881 = H1v.y && H1v.z;
    }
    else
    {
        _881 = _873;
    }
    bool2 lvl5w = bool2(_863, _881);
    bool _893;
    if (lvl5x.y)
    {
        _893 = D1h.y && D1h.x;
    }
    else
    {
        _893 = lvl5x.y;
    }
    bool _903;
    if (lvl5w.y)
    {
        _903 = B1v.w && B1v.x;
    }
    else
    {
        _903 = lvl5w.y;
    }
    bool2 lvl6x = bool2(_893, _903);
    bool _915;
    if (lvl5x.x)
    {
        _915 = F1h.x && F1h.y;
    }
    else
    {
        _915 = lvl5x.x;
    }
    bool _925;
    if (lvl5y.y)
    {
        _925 = B1v.z && B1v.y;
    }
    else
    {
        _925 = lvl5y.y;
    }
    bool2 lvl6y = bool2(_915, _925);
    bool _937;
    if (lvl5z.x)
    {
        _937 = F1h.w && F1h.z;
    }
    else
    {
        _937 = lvl5z.x;
    }
    bool _947;
    if (lvl5y.x)
    {
        _947 = H1v.y && H1v.z;
    }
    else
    {
        _947 = lvl5y.x;
    }
    bool2 lvl6z = bool2(_937, _947);
    bool _959;
    if (lvl5z.y)
    {
        _959 = D1h.z && D1h.w;
    }
    else
    {
        _959 = lvl5z.y;
    }
    bool _969;
    if (lvl5w.x)
    {
        _969 = H1v.x && H1v.w;
    }
    else
    {
        _969 = lvl5w.x;
    }
    bool2 lvl6w = bool2(_959, _969);
    bool _975 = lvl1x && Eo.x;
    bool _984;
    if (!_975)
    {
        _984 = lvl3x.x && Eo.y;
    }
    else
    {
        _984 = _975;
    }
    bool _993;
    if (!_984)
    {
        _993 = lvl4x.x && Do.x;
    }
    else
    {
        _993 = _984;
    }
    bool _1002;
    if (!_993)
    {
        _1002 = lvl6x.x && Fo.y;
    }
    else
    {
        _1002 = _993;
    }
    float _1004;
    if (_1002)
    {
        _1004 = 5.0f;
    }
    else
    {
        bool _1021;
        if (!lvl1x)
        {
            bool _1020;
            if (lvl3x.y)
            {
                _1020 = !Eo.w;
            }
            else
            {
                _1020 = lvl3x.y;
            }
            _1021 = _1020;
        }
        else
        {
            _1021 = lvl1x;
        }
        bool _1033;
        if (!_1021)
        {
            bool _1032;
            if (lvl4x.y)
            {
                _1032 = !Bo.x;
            }
            else
            {
                _1032 = lvl4x.y;
            }
            _1033 = _1032;
        }
        else
        {
            _1033 = _1021;
        }
        bool _1045;
        if (!_1033)
        {
            bool _1044;
            if (lvl6x.y)
            {
                _1044 = !Ho.w;
            }
            else
            {
                _1044 = lvl6x.y;
            }
            _1045 = _1044;
        }
        else
        {
            _1045 = _1033;
        }
        float _1046;
        if (_1045)
        {
            _1046 = 1.0f;
        }
        else
        {
            float _1053;
            if (lvl3x.x)
            {
                _1053 = 3.0f;
            }
            else
            {
                float _1060;
                if (lvl3x.y)
                {
                    _1060 = 7.0f;
                }
                else
                {
                    float _1067;
                    if (lvl4x.x)
                    {
                        _1067 = 2.0f;
                    }
                    else
                    {
                        float _1073;
                        if (lvl4x.y)
                        {
                            _1073 = 6.0f;
                        }
                        else
                        {
                            float _1080;
                            if (lvl6x.x)
                            {
                                _1080 = 4.0f;
                            }
                            else
                            {
                                _1080 = lvl6x.y ? 8.0f : 0.0f;
                            }
                            _1073 = _1080;
                        }
                        _1067 = _1073;
                    }
                    _1060 = _1067;
                }
                _1053 = _1060;
            }
            _1046 = _1053;
        }
        _1004 = _1046;
    }
    float4 crn;
    crn.x = _1004;
    bool _1100 = lvl1y && Eo.y;
    bool _1109;
    if (!_1100)
    {
        _1109 = lvl3y.x && Eo.x;
    }
    else
    {
        _1109 = _1100;
    }
    bool _1118;
    if (!_1109)
    {
        _1118 = lvl4y.x && Fo.y;
    }
    else
    {
        _1118 = _1109;
    }
    bool _1127;
    if (!_1118)
    {
        _1127 = lvl6y.x && Do.x;
    }
    else
    {
        _1127 = _1118;
    }
    float _1128;
    if (_1127)
    {
        _1128 = 5.0f;
    }
    else
    {
        bool _1144;
        if (!lvl1y)
        {
            bool _1143;
            if (lvl3y.y)
            {
                _1143 = !Eo.z;
            }
            else
            {
                _1143 = lvl3y.y;
            }
            _1144 = _1143;
        }
        else
        {
            _1144 = lvl1y;
        }
        bool _1156;
        if (!_1144)
        {
            bool _1155;
            if (lvl4y.y)
            {
                _1155 = !Bo.y;
            }
            else
            {
                _1155 = lvl4y.y;
            }
            _1156 = _1155;
        }
        else
        {
            _1156 = _1144;
        }
        bool _1168;
        if (!_1156)
        {
            bool _1167;
            if (lvl6y.y)
            {
                _1167 = !Ho.z;
            }
            else
            {
                _1167 = lvl6y.y;
            }
            _1168 = _1167;
        }
        else
        {
            _1168 = _1156;
        }
        float _1169;
        if (_1168)
        {
            _1169 = 3.0f;
        }
        else
        {
            float _1175;
            if (lvl3y.x)
            {
                _1175 = 1.0f;
            }
            else
            {
                float _1181;
                if (lvl3y.y)
                {
                    _1181 = 7.0f;
                }
                else
                {
                    float _1187;
                    if (lvl4y.x)
                    {
                        _1187 = 4.0f;
                    }
                    else
                    {
                        float _1193;
                        if (lvl4y.y)
                        {
                            _1193 = 6.0f;
                        }
                        else
                        {
                            float _1199;
                            if (lvl6y.x)
                            {
                                _1199 = 2.0f;
                            }
                            else
                            {
                                _1199 = lvl6y.y ? 8.0f : 0.0f;
                            }
                            _1193 = _1199;
                        }
                        _1187 = _1193;
                    }
                    _1181 = _1187;
                }
                _1175 = _1181;
            }
            _1169 = _1175;
        }
        _1128 = _1169;
    }
    crn.y = _1128;
    bool _1217 = lvl1z && Eo.z;
    bool _1226;
    if (!_1217)
    {
        _1226 = lvl3z.x && Eo.w;
    }
    else
    {
        _1226 = _1217;
    }
    bool _1235;
    if (!_1226)
    {
        _1235 = lvl4z.x && Fo.z;
    }
    else
    {
        _1235 = _1226;
    }
    bool _1244;
    if (!_1235)
    {
        _1244 = lvl6z.x && Do.w;
    }
    else
    {
        _1244 = _1235;
    }
    float _1245;
    if (_1244)
    {
        _1245 = 7.0f;
    }
    else
    {
        bool _1261;
        if (!lvl1z)
        {
            bool _1260;
            if (lvl3z.y)
            {
                _1260 = !Eo.y;
            }
            else
            {
                _1260 = lvl3z.y;
            }
            _1261 = _1260;
        }
        else
        {
            _1261 = lvl1z;
        }
        bool _1273;
        if (!_1261)
        {
            bool _1272;
            if (lvl4z.y)
            {
                _1272 = !Ho.z;
            }
            else
            {
                _1272 = lvl4z.y;
            }
            _1273 = _1272;
        }
        else
        {
            _1273 = _1261;
        }
        bool _1285;
        if (!_1273)
        {
            bool _1284;
            if (lvl6z.y)
            {
                _1284 = !Bo.y;
            }
            else
            {
                _1284 = lvl6z.y;
            }
            _1285 = _1284;
        }
        else
        {
            _1285 = _1273;
        }
        float _1286;
        if (_1285)
        {
            _1286 = 3.0f;
        }
        else
        {
            float _1292;
            if (lvl3z.x)
            {
                _1292 = 1.0f;
            }
            else
            {
                float _1298;
                if (lvl3z.y)
                {
                    _1298 = 5.0f;
                }
                else
                {
                    float _1304;
                    if (lvl4z.x)
                    {
                        _1304 = 4.0f;
                    }
                    else
                    {
                        float _1310;
                        if (lvl4z.y)
                        {
                            _1310 = 8.0f;
                        }
                        else
                        {
                            float _1316;
                            if (lvl6z.x)
                            {
                                _1316 = 2.0f;
                            }
                            else
                            {
                                _1316 = lvl6z.y ? 6.0f : 0.0f;
                            }
                            _1310 = _1316;
                        }
                        _1304 = _1310;
                    }
                    _1298 = _1304;
                }
                _1292 = _1298;
            }
            _1286 = _1292;
        }
        _1245 = _1286;
    }
    crn.z = _1245;
    bool _1334 = lvl1w && Eo.w;
    bool _1343;
    if (!_1334)
    {
        _1343 = lvl3w.x && Eo.z;
    }
    else
    {
        _1343 = _1334;
    }
    bool _1352;
    if (!_1343)
    {
        _1352 = lvl4w.x && Do.w;
    }
    else
    {
        _1352 = _1343;
    }
    bool _1361;
    if (!_1352)
    {
        _1361 = lvl6w.x && Fo.z;
    }
    else
    {
        _1361 = _1352;
    }
    float _1362;
    if (_1361)
    {
        _1362 = 7.0f;
    }
    else
    {
        bool _1378;
        if (!lvl1w)
        {
            bool _1377;
            if (lvl3w.y)
            {
                _1377 = !Eo.x;
            }
            else
            {
                _1377 = lvl3w.y;
            }
            _1378 = _1377;
        }
        else
        {
            _1378 = lvl1w;
        }
        bool _1390;
        if (!_1378)
        {
            bool _1389;
            if (lvl4w.y)
            {
                _1389 = !Ho.w;
            }
            else
            {
                _1389 = lvl4w.y;
            }
            _1390 = _1389;
        }
        else
        {
            _1390 = _1378;
        }
        bool _1402;
        if (!_1390)
        {
            bool _1401;
            if (lvl6w.y)
            {
                _1401 = !Bo.x;
            }
            else
            {
                _1401 = lvl6w.y;
            }
            _1402 = _1401;
        }
        else
        {
            _1402 = _1390;
        }
        float _1403;
        if (_1402)
        {
            _1403 = 1.0f;
        }
        else
        {
            float _1409;
            if (lvl3w.x)
            {
                _1409 = 3.0f;
            }
            else
            {
                float _1415;
                if (lvl3w.y)
                {
                    _1415 = 5.0f;
                }
                else
                {
                    float _1421;
                    if (lvl4w.x)
                    {
                        _1421 = 2.0f;
                    }
                    else
                    {
                        float _1427;
                        if (lvl4w.y)
                        {
                            _1427 = 8.0f;
                        }
                        else
                        {
                            float _1433;
                            if (lvl6w.x)
                            {
                                _1433 = 4.0f;
                            }
                            else
                            {
                                _1433 = lvl6w.y ? 6.0f : 0.0f;
                            }
                            _1427 = _1433;
                        }
                        _1421 = _1427;
                    }
                    _1415 = _1421;
                }
                _1409 = _1415;
            }
            _1403 = _1409;
        }
        _1362 = _1403;
    }
    crn.w = _1362;
    bool _1453 = lvl2x.x && Eo.x;
    bool _1462;
    if (!_1453)
    {
        _1462 = lvl2x.y && Eo.y;
    }
    else
    {
        _1462 = _1453;
    }
    bool _1471;
    if (!_1462)
    {
        _1471 = lvl5x.x && Do.x;
    }
    else
    {
        _1471 = _1462;
    }
    bool _1480;
    if (!_1471)
    {
        _1480 = lvl5x.y && Fo.y;
    }
    else
    {
        _1480 = _1471;
    }
    float _1481;
    if (_1480)
    {
        _1481 = 5.0f;
    }
    else
    {
        float _1487;
        if (lvl2x.x)
        {
            _1487 = 1.0f;
        }
        else
        {
            float _1493;
            if (lvl2x.y)
            {
                _1493 = 3.0f;
            }
            else
            {
                float _1499;
                if (lvl5x.x)
                {
                    _1499 = 2.0f;
                }
                else
                {
                    float _1505;
                    if (lvl5x.y)
                    {
                        _1505 = 4.0f;
                    }
                    else
                    {
                        float _1520;
                        if (((Ec.x && Dc.z) && Ec.y) && Fc.w)
                        {
                            float _1525;
                            if (Eo.x)
                            {
                                _1525 = Eo.y ? 5.0f : 3.0f;
                            }
                            else
                            {
                                _1525 = 1.0f;
                            }
                            _1520 = _1525;
                        }
                        else
                        {
                            _1520 = 0.0f;
                        }
                        _1505 = _1520;
                    }
                    _1499 = _1505;
                }
                _1493 = _1499;
            }
            _1487 = _1493;
        }
        _1481 = _1487;
    }
    float4 mid;
    mid.x = _1481;
    bool _1548;
    if (lvl2y.x)
    {
        _1548 = !Eo.y;
    }
    else
    {
        _1548 = lvl2y.x;
    }
    bool _1560;
    if (!_1548)
    {
        bool _1559;
        if (lvl2y.y)
        {
            _1559 = !Eo.z;
        }
        else
        {
            _1559 = lvl2y.y;
        }
        _1560 = _1559;
    }
    else
    {
        _1560 = _1548;
    }
    bool _1572;
    if (!_1560)
    {
        bool _1571;
        if (lvl5y.x)
        {
            _1571 = !Bo.y;
        }
        else
        {
            _1571 = lvl5y.x;
        }
        _1572 = _1571;
    }
    else
    {
        _1572 = _1560;
    }
    bool _1584;
    if (!_1572)
    {
        bool _1583;
        if (lvl5y.y)
        {
            _1583 = !Ho.z;
        }
        else
        {
            _1583 = lvl5y.y;
        }
        _1584 = _1583;
    }
    else
    {
        _1584 = _1572;
    }
    float _1585;
    if (_1584)
    {
        _1585 = 3.0f;
    }
    else
    {
        float _1591;
        if (lvl2y.x)
        {
            _1591 = 5.0f;
        }
        else
        {
            float _1597;
            if (lvl2y.y)
            {
                _1597 = 7.0f;
            }
            else
            {
                float _1603;
                if (lvl5y.x)
                {
                    _1603 = 6.0f;
                }
                else
                {
                    float _1609;
                    if (lvl5y.y)
                    {
                        _1609 = 8.0f;
                    }
                    else
                    {
                        float _1624;
                        if (((Ec.y && Bc.w) && Ec.z) && Hc.x)
                        {
                            float _1630;
                            if (!Eo.y)
                            {
                                _1630 = (!Eo.z) ? 3.0f : 7.0f;
                            }
                            else
                            {
                                _1630 = 5.0f;
                            }
                            _1624 = _1630;
                        }
                        else
                        {
                            _1624 = 0.0f;
                        }
                        _1609 = _1624;
                    }
                    _1603 = _1609;
                }
                _1597 = _1603;
            }
            _1591 = _1597;
        }
        _1585 = _1591;
    }
    mid.y = _1585;
    bool _1651 = lvl2z.x && Eo.w;
    bool _1660;
    if (!_1651)
    {
        _1660 = lvl2z.y && Eo.z;
    }
    else
    {
        _1660 = _1651;
    }
    bool _1669;
    if (!_1660)
    {
        _1669 = lvl5z.x && Do.w;
    }
    else
    {
        _1669 = _1660;
    }
    bool _1678;
    if (!_1669)
    {
        _1678 = lvl5z.y && Fo.z;
    }
    else
    {
        _1678 = _1669;
    }
    float _1679;
    if (_1678)
    {
        _1679 = 7.0f;
    }
    else
    {
        float _1685;
        if (lvl2z.x)
        {
            _1685 = 1.0f;
        }
        else
        {
            float _1691;
            if (lvl2z.y)
            {
                _1691 = 3.0f;
            }
            else
            {
                float _1697;
                if (lvl5z.x)
                {
                    _1697 = 2.0f;
                }
                else
                {
                    float _1703;
                    if (lvl5z.y)
                    {
                        _1703 = 4.0f;
                    }
                    else
                    {
                        float _1718;
                        if (((Ec.z && Fc.x) && Ec.w) && Dc.y)
                        {
                            float _1723;
                            if (Eo.z)
                            {
                                _1723 = Eo.w ? 7.0f : 1.0f;
                            }
                            else
                            {
                                _1723 = 3.0f;
                            }
                            _1718 = _1723;
                        }
                        else
                        {
                            _1718 = 0.0f;
                        }
                        _1703 = _1718;
                    }
                    _1697 = _1703;
                }
                _1691 = _1697;
            }
            _1685 = _1691;
        }
        _1679 = _1685;
    }
    mid.z = _1679;
    bool _1746;
    if (lvl2w.x)
    {
        _1746 = !Eo.x;
    }
    else
    {
        _1746 = lvl2w.x;
    }
    bool _1758;
    if (!_1746)
    {
        bool _1757;
        if (lvl2w.y)
        {
            _1757 = !Eo.w;
        }
        else
        {
            _1757 = lvl2w.y;
        }
        _1758 = _1757;
    }
    else
    {
        _1758 = _1746;
    }
    bool _1770;
    if (!_1758)
    {
        bool _1769;
        if (lvl5w.x)
        {
            _1769 = !Bo.x;
        }
        else
        {
            _1769 = lvl5w.x;
        }
        _1770 = _1769;
    }
    else
    {
        _1770 = _1758;
    }
    bool _1782;
    if (!_1770)
    {
        bool _1781;
        if (lvl5w.y)
        {
            _1781 = !Ho.w;
        }
        else
        {
            _1781 = lvl5w.y;
        }
        _1782 = _1781;
    }
    else
    {
        _1782 = _1770;
    }
    float _1783;
    if (_1782)
    {
        _1783 = 1.0f;
    }
    else
    {
        float _1789;
        if (lvl2w.x)
        {
            _1789 = 5.0f;
        }
        else
        {
            float _1795;
            if (lvl2w.y)
            {
                _1795 = 7.0f;
            }
            else
            {
                float _1801;
                if (lvl5w.x)
                {
                    _1801 = 6.0f;
                }
                else
                {
                    float _1807;
                    if (lvl5w.y)
                    {
                        _1807 = 8.0f;
                    }
                    else
                    {
                        float _1822;
                        if (((Ec.w && Hc.y) && Ec.x) && Bc.z)
                        {
                            float _1828;
                            if (!Eo.w)
                            {
                                _1828 = (!Eo.x) ? 1.0f : 5.0f;
                            }
                            else
                            {
                                _1828 = 7.0f;
                            }
                            _1822 = _1828;
                        }
                        else
                        {
                            _1822 = 0.0f;
                        }
                        _1807 = _1822;
                    }
                    _1801 = _1807;
                }
                _1795 = _1801;
            }
            _1789 = _1795;
        }
        _1783 = _1789;
    }
    mid.w = _1783;
    FragColor = (crn + (mid * 9.0f)) / 80.0f.xxxx;
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	frag_main();

	return FragColor;
}
