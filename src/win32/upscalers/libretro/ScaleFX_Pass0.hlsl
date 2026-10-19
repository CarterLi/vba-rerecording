// ScaleFX Pass0, pass 0 of the ScaleFX pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "scalefx-pass0.glsl" (libretro/glsl-shaders,
// scalefx/shaders).  The OpenGL 3.3 renderer runs that shader as it is upstream,
// from the resource "SCALEFX_PASS0_GLSL"; this is the port of it, because
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

float dist(float3 A, float3 B)
{
    float r = 0.5f * (A.x + B.x);
    float3 d = A - B;
    float3 c = float3(2.0f + r, 4.0f, 3.0f - r);
    return sqrt(dot(c * d, d)) / 3.0f;
}

void frag_main()
{
    float3 A = frameTex.Sample(frameSampler, TEX0.xy, int2(-1, -1)).xyz;
    float3 B = frameTex.Sample(frameSampler, TEX0.xy, int2(0, -1)).xyz;
    float3 C = frameTex.Sample(frameSampler, TEX0.xy, int2(1, -1)).xyz;
    float3 E = frameTex.Sample(frameSampler, TEX0.xy, int2(0, 0)).xyz;
    float3 F = frameTex.Sample(frameSampler, TEX0.xy, int2(1, 0)).xyz;
    float3 param = E;
    float3 param_1 = A;
    float3 param_2 = E;
    float3 param_3 = B;
    float3 param_4 = E;
    float3 param_5 = C;
    float3 param_6 = E;
    float3 param_7 = F;
    FragColor = float4(dist(param, param_1), dist(param_2, param_3), dist(param_4, param_5), dist(param_6, param_7));
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	frag_main();

	return FragColor;
}
