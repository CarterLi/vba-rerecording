// ScaleFX Pass1, pass 1 of the ScaleFX pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "scalefx-pass1.glsl" (libretro/glsl-shaders,
// scalefx/shaders).  The OpenGL 3.3 renderer runs that shader as it is upstream,
// from the resource "SCALEFX_PASS1_GLSL"; this is the port of it, because
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

float str(float d, float2 a, float2 b)
{
    float diff = a.x - a.y;
    float wght1 = max(0.5f - d, 0.0f) / 0.5f;
    float _54;
    if ((min(a.x, b.x) + a.x) > (min(a.y, b.y) + a.y))
    {
        _54 = diff;
    }
    else
    {
        _54 = -diff;
    }
    float wght2 = clamp((1.0f - d) + _54, 0.0f, 1.0f);
    bool _77;
    if (!true)
    {
        _77 = (2.0f * d) < (a.x + a.y);
    }
    else
    {
        _77 = true;
    }
    float _78;
    if (_77)
    {
        _78 = (wght1 * wght2) * (a.x * a.y);
    }
    else
    {
        _78 = 0.0f;
    }
    return _78;
}

void frag_main()
{
    float4 A = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, -1));
    float4 B = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -1));
    float4 D = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, 0));
    float4 E = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 0));
    float4 F = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 0));
    float4 G = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, 1));
    float4 H = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 1));
    float4 I = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 1));
    float param = D.z;
    float2 param_1 = float2(D.w, E.y);
    float2 param_2 = float2(A.w, D.y);
    float4 res;
    res.x = str(param, param_1, param_2);
    float param_3 = F.x;
    float2 param_4 = float2(E.w, E.y);
    float2 param_5 = float2(B.w, F.y);
    res.y = str(param_3, param_4, param_5);
    float param_6 = H.z;
    float2 param_7 = float2(E.w, H.y);
    float2 param_8 = float2(H.w, I.y);
    res.z = str(param_6, param_7, param_8);
    float param_9 = H.x;
    float2 param_10 = float2(D.w, H.y);
    float2 param_11 = float2(G.w, G.y);
    res.w = str(param_9, param_10, param_11);
    FragColor = res;
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	frag_main();

	return FragColor;
}
