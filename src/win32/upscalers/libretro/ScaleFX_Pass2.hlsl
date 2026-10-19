// ScaleFX Pass2, pass 2 of the ScaleFX pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "scalefx-pass2.glsl" (libretro/glsl-shaders,
// scalefx/shaders).  The OpenGL 3.3 renderer runs that shader as it is upstream,
// from the resource "SCALEFX_PASS2_GLSL"; this is the port of it, because
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

Texture2D<float4> prev2Tex : register(t1);
SamplerState prev2Sampler : register(s1);
Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 FragColor;

float4 dom(float3 x, float3 y, float3 z, float3 w)
{
    return (float4(x.y, y.y, z.y, w.y) * 2.0f) - (float4(x.x, y.x, z.x, w.x) + float4(x.z, y.z, z.z, w.z));
}

float clear(float2 crn, float2 a, float2 b)
{
    bool _77 = crn.x >= max(min(a.x, a.y), min(b.x, b.y));
    bool _94;
    if (_77)
    {
        _94 = crn.y >= max(min(a.x, b.y), min(b.x, a.y));
    }
    else
    {
        _94 = _77;
    }
    return float(_94);
}

void frag_main()
{
    float4 A = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(-1, -1));
    float4 B = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(0, -1));
    float4 D = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(-1, 0));
    float4 E = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(0, 0));
    float4 F = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(1, 0));
    float4 G = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(-1, 1));
    float4 H = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(0, 1));
    float4 I = prev2Tex.Sample(prev2Sampler, TEX0.xy, int2(1, 1));
    float4 As = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, -1));
    float4 Bs = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -1));
    float4 Cs = frameTex.Sample(frameSampler, TEX0.xy, int2(1, -1));
    float4 Ds = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, 0));
    float4 Es = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 0));
    float4 Fs = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 0));
    float4 Gs = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, 1));
    float4 Hs = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 1));
    float4 Is = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 1));
    float4 jSx = float4(As.z, Bs.w, Es.x, Ds.y);
    float3 param = As.yzw;
    float3 param_1 = Bs.zwx;
    float3 param_2 = Es.wxy;
    float3 param_3 = Ds.xyz;
    float4 jDx = dom(param, param_1, param_2, param_3);
    float4 jSy = float4(Bs.z, Cs.w, Fs.x, Es.y);
    float3 param_4 = Bs.yzw;
    float3 param_5 = Cs.zwx;
    float3 param_6 = Fs.wxy;
    float3 param_7 = Es.xyz;
    float4 jDy = dom(param_4, param_5, param_6, param_7);
    float4 jSz = float4(Es.z, Fs.w, Is.x, Hs.y);
    float3 param_8 = Es.yzw;
    float3 param_9 = Fs.zwx;
    float3 param_10 = Is.wxy;
    float3 param_11 = Hs.xyz;
    float4 jDz = dom(param_8, param_9, param_10, param_11);
    float4 jSw = float4(Ds.z, Es.w, Hs.x, Gs.y);
    float3 param_12 = Ds.yzw;
    float3 param_13 = Es.zwx;
    float3 param_14 = Hs.wxy;
    float3 param_15 = Gs.xyz;
    float4 jDw = dom(param_12, param_13, param_14, param_15);
    float4 zero4 = 0.0f.xxxx;
    float4 jx = min((1.0f.xxxx - step(jDx, zero4)) * ((step(jDx.yzwx, zero4) * step(jDx.wxyz, zero4)) + (1.0f.xxxx - step(jDx + jDx.zwxy, jDx.yzwx + jDx.wxyz))), 1.0f.xxxx);
    float4 jy = min((1.0f.xxxx - step(jDy, zero4)) * ((step(jDy.yzwx, zero4) * step(jDy.wxyz, zero4)) + (1.0f.xxxx - step(jDy + jDy.zwxy, jDy.yzwx + jDy.wxyz))), 1.0f.xxxx);
    float4 jz = min((1.0f.xxxx - step(jDz, zero4)) * ((step(jDz.yzwx, zero4) * step(jDz.wxyz, zero4)) + (1.0f.xxxx - step(jDz + jDz.zwxy, jDz.yzwx + jDz.wxyz))), 1.0f.xxxx);
    float4 jw = min((1.0f.xxxx - step(jDw, zero4)) * ((step(jDw.yzwx, zero4) * step(jDw.wxyz, zero4)) + (1.0f.xxxx - step(jDw + jDw.zwxy, jDw.yzwx + jDw.wxyz))), 1.0f.xxxx);
    float4 res;
    res.x = min(jx.z + ((((1.0f - jx.y) * (1.0f - jx.w)) * (1.0f - step(jSx.z, 0.0f))) * (jx.x + (1.0f - step(jSx.x + jSx.z, jSx.y + jSx.w)))), 1.0f);
    res.y = min(jy.w + ((((1.0f - jy.z) * (1.0f - jy.x)) * (1.0f - step(jSy.w, 0.0f))) * (jy.y + (1.0f - step(jSy.y + jSy.w, jSy.x + jSy.z)))), 1.0f);
    res.z = min(jz.x + ((((1.0f - jz.w) * (1.0f - jz.y)) * (1.0f - step(jSz.x, 0.0f))) * (jz.z + (1.0f - step(jSz.x + jSz.z, jSz.y + jSz.w)))), 1.0f);
    res.w = min(jw.y + ((((1.0f - jw.x) * (1.0f - jw.z)) * (1.0f - step(jSw.y, 0.0f))) * (jw.w + (1.0f - step(jSw.y + jSw.w, jSw.x + jSw.z)))), 1.0f);
    res = min(res * (float4(jx.z, jy.w, jz.x, jw.y) + (1.0f.xxxx - (res.wxyz * res.yzwx))), 1.0f.xxxx);
    float2 param_16 = float2(D.z, E.x);
    float2 param_17 = float2(D.w, E.y);
    float2 param_18 = float2(A.w, D.y);
    float4 clr;
    clr.x = clear(param_16, param_17, param_18);
    float2 param_19 = float2(F.x, E.z);
    float2 param_20 = float2(E.w, E.y);
    float2 param_21 = float2(B.w, F.y);
    clr.y = clear(param_19, param_20, param_21);
    float2 param_22 = float2(H.z, I.x);
    float2 param_23 = float2(E.w, H.y);
    float2 param_24 = float2(H.w, I.y);
    clr.z = clear(param_22, param_23, param_24);
    float2 param_25 = float2(H.x, G.z);
    float2 param_26 = float2(D.w, H.y);
    float2 param_27 = float2(G.w, G.y);
    clr.w = clear(param_25, param_26, param_27);
    float4 h = float4(min(D.w, A.w), min(E.w, B.w), min(E.w, H.w), min(D.w, G.w));
    float4 v = float4(min(E.y, D.y), min(E.y, F.y), min(H.y, I.y), min(H.y, G.y));
    float4 or = 1.0f.xxxx - step(h + float4(D.w, E.w, E.w, D.w), v + float4(E.y, E.y, H.y, H.y));
    float4 hori = (1.0f.xxxx - step(v, h)) * clr;
    float4 vert = (1.0f.xxxx - step(h, v)) * clr;
    FragColor = (((res + (hori * 2.0f)) + (vert * 4.0f)) + (or * 8.0f)) / 15.0f.xxxx;
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	frag_main();

	return FragColor;
}
