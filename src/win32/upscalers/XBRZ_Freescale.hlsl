// xBRZ-Freescale, the shader xBRZ pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbrz-freescale.glsl" (libretro/glsl-
// shaders, xbrz/shaders).  It applies the xBRZ corner analysis and blending in
// a single pass and without a fixed scaling factor: the CPU xBRZ filters in
// src/filters are built for a fixed factor and for a fixed 4 MB destination
// buffer, this one samples the frame texture and blends it while the quad is
// stretched over the window, so the emulation thread never runs a filter, which
// is what keeps the audio device fed.
//
// The shader is factor agnostic and takes the frame as the renderer has it: the
// CPU pixel filter may already have scaled it, and the factor to stretch it by
// is handed over with the frame size.
//
// The original licences of the ported shader:
//
//   Hyllian's xBR-vertex code and texel mapping
//
//   Copyright (C) 2011/2016 Hyllian - sergiogdb@gmail.com
//
//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files (the "Software"), to deal
//   in the Software without restriction, including without limitation the rights
//   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//   copies of the Software, and to permit persons to whom the Software is
//   furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//   THE SOFTWARE.
//
//   This shader also uses code and/or concepts from xBRZ as it appears in the
//   Desmume source code. The license for which is as follows:
//
//   This file is part of the HqMAME project. It is distributed under GNU
//   General Public License: http://www.gnu.org/licenses/gpl-3.0
//   Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved
//
//   Additionally and as a special exception, the author gives permission to
//   link the code of this program with the MAME library (or with modified
//   versions of MAME that use the same license as MAME), and distribute linked
//   combinations including the two. You must obey the GNU General Public
//   License in all respects for all of the code used other than MAME. If you
//   modify this file, you may extend this exception to your version of the
//   file, but you are not obligated to do so. If you do not wish to do so,
//   delete this exception statement from your version.

// The quad vertex shader (VSMain) of the renderer feeds this one, so the
// varying has to be declared the way it is there.
struct XbrzVSOut
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

// The block the renderer hands every single pass upscaler, the one
// Direct3D11Display::scaleFrameWithSinglePass() fills:
//
//   SourceSize.xy: the frame size in pixels, zw: one texel (1 / xy)
//   OutputSize.xy: the size of the target, zw: one pixel of it
//   Scale.xy:      the factor the frame is stretched by (OutputSize / SourceSize)
//
cbuffer UpscalerConstants : register(b0)
{
	float4 SourceSize;
	float4 OutputSize;
	float4 Scale;
};

Texture2D    frameTex     : register(t0);
SamplerState frameSampler : register(s0);

#define BLEND_NONE     0
#define BLEND_NORMAL   1
#define BLEND_DOMINANT 2

#define LUMINANCE_WEIGHT             1.0
#define EQUAL_COLOR_TOLERANCE        (30.0 / 255.0)
#define STEEP_DIRECTION_THRESHOLD    2.2
#define DOMINANT_DIRECTION_THRESHOLD 3.6

// GLSL compares whole vectors with == and !=, HLSL does it component wise.
#define pixEq(a, b) all((a) == (b))
#define pixNeq(a, b) any((a) != (b))

float DistYCbCr(float3 pixA, float3 pixB)
{
	const float3 w      = float3(0.2627, 0.6780, 0.0593);
	const float  scaleB = 0.5 / (1.0 - w.b);
	const float  scaleR = 0.5 / (1.0 - w.r);

	float3 diff = pixA - pixB;
	float  Y    = dot(diff.rgb, w);
	float  Cb   = scaleB * (diff.b - Y);
	float  Cr   = scaleR * (diff.r - Y);

	return sqrt(((LUMINANCE_WEIGHT * Y) * (LUMINANCE_WEIGHT * Y)) + (Cb * Cb) + (Cr * Cr));
}

bool IsPixEqual(float3 pixA, float3 pixB)
{
	return DistYCbCr(pixA, pixB) < EQUAL_COLOR_TOLERANCE;
}

float get_left_ratio(float2 center, float2 origin, float2 direction, float2 scale)
{
	float2 P0    = center - origin;
	float2 proj  = direction * (dot(P0, direction) / dot(direction, direction));
	float2 distv = P0 - proj;
	float2 orth  = float2(-direction.y, direction.x);
	float  side  = sign(dot(P0, orth));
	float  v     = side * length(distv * scale);

	return smoothstep(-sqrt(2.0) / 2.0, sqrt(2.0) / 2.0, v);
}

// The taps are taken at texel centres, so the sampler has to be the point one.
float3 FetchTap(float2 coord, float2 texel, float2 offset)
{
	return frameTex.Sample(frameSampler, coord + texel * offset).rgb;
}

#define P(x, y) FetchTap(coord, texel, float2(x, y))

float4 main(XbrzVSOut input) : SV_TARGET
{
	//---------------------------------------
	// Input Pixel Mapping:  -|x|x|x|-
	//                       x|A|B|C|x
	//                       x|D|E|F|x
	//                       x|G|H|I|x
	//                       -|x|x|x|-

	float2 texel = SourceSize.zw;
	float2 scale = Scale.xy;
	float2 pos   = frac(input.uv / texel) - float2(0.5, 0.5);
	float2 coord = input.uv - pos * texel;

	float3 A = P(-1., -1.);
	float3 B = P( 0., -1.);
	float3 C = P( 1., -1.);
	float3 D = P(-1.,  0.);
	float3 E = P( 0.,  0.);
	float3 F = P( 1.,  0.);
	float3 G = P(-1.,  1.);
	float3 H = P( 0.,  1.);
	float3 I = P( 1.,  1.);

	// blendResult Mapping: x|y|
	//                      w|z|
	int4 blendResult = int4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE);

	// Preprocess corners
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (!((pixEq(E, F) && pixEq(H, I)) || (pixEq(E, H) && pixEq(F, I))))
	{
		float dist_H_F = DistYCbCr(G, E) + DistYCbCr(E, C) + DistYCbCr(P(0., 2.), I) + DistYCbCr(I, P(2., 0.)) + (4.0 * DistYCbCr(H, F));
		float dist_E_I = DistYCbCr(D, H) + DistYCbCr(H, P(1., 2.)) + DistYCbCr(B, F) + DistYCbCr(F, P(2., 1.)) + (4.0 * DistYCbCr(E, I));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_H_F) < dist_E_I;
		blendResult.z = ((dist_H_F < dist_E_I) && pixNeq(E, F) && pixNeq(E, H)) ? (dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (!((pixEq(D, E) && pixEq(G, H)) || (pixEq(D, G) && pixEq(E, H))))
	{
		float dist_G_E = DistYCbCr(P(-2., 1.), D) + DistYCbCr(D, B) + DistYCbCr(P(-1., 2.), H) + DistYCbCr(H, F) + (4.0 * DistYCbCr(G, E));
		float dist_D_H = DistYCbCr(P(-2., 0.), G) + DistYCbCr(G, P(0., 2.)) + DistYCbCr(A, E) + DistYCbCr(E, I) + (4.0 * DistYCbCr(D, H));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_H) < dist_G_E;
		blendResult.w = ((dist_G_E > dist_D_H) && pixNeq(E, D) && pixNeq(E, H)) ? (dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (!((pixEq(B, C) && pixEq(E, F)) || (pixEq(B, E) && pixEq(C, F))))
	{
		float dist_E_C = DistYCbCr(D, B) + DistYCbCr(B, P(1., -2.)) + DistYCbCr(H, F) + DistYCbCr(F, P(2., -1.)) + (4.0 * DistYCbCr(E, C));
		float dist_B_F = DistYCbCr(A, E) + DistYCbCr(E, I) + DistYCbCr(P(0., -2.), C) + DistYCbCr(C, P(2., 0.)) + (4.0 * DistYCbCr(B, F));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_B_F) < dist_E_C;
		blendResult.y = ((dist_E_C > dist_B_F) && pixNeq(E, B) && pixNeq(E, F)) ? (dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (!((pixEq(A, B) && pixEq(D, E)) || (pixEq(A, D) && pixEq(B, E))))
	{
		float dist_D_B = DistYCbCr(P(-2., 0.), A) + DistYCbCr(A, P(0., -2.)) + DistYCbCr(G, E) + DistYCbCr(E, C) + (4.0 * DistYCbCr(D, B));
		float dist_A_E = DistYCbCr(P(-2., -1.), D) + DistYCbCr(D, H) + DistYCbCr(P(-1., -2.), B) + DistYCbCr(B, F) + (4.0 * DistYCbCr(A, E));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_B) < dist_A_E;
		blendResult.x = ((dist_D_B < dist_A_E) && pixNeq(E, D) && pixNeq(E, B)) ? (dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	float3 res = E;

	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (blendResult.z != BLEND_NONE)
	{
		float dist_F_G = DistYCbCr(F, G);
		float dist_H_C = DistYCbCr(H, C);
		bool  doLineBlend = (blendResult.z == BLEND_DOMINANT ||
		                     !((blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) || (blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) ||
		                       (IsPixEqual(G, H) && IsPixEqual(H, I) && IsPixEqual(I, F) && IsPixEqual(F, C) && !IsPixEqual(E, I))));

		float2 origin    = float2(0.0, 1.0 / sqrt(2.0));
		float2 direction = float2(1.0, -1.0);

		if (doLineBlend)
		{
			bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_F_G <= dist_H_C) && pixNeq(E, G) && pixNeq(D, G);
			bool haveSteepLine   = (STEEP_DIRECTION_THRESHOLD * dist_H_C <= dist_F_G) && pixNeq(E, C) && pixNeq(B, C);

			origin        = haveShallowLine ? float2(0.0, 0.25) : float2(0.0, 0.5);
			direction.x  += haveShallowLine ? 1.0 : 0.0;
			direction.y  -= haveSteepLine ? 1.0 : 0.0;
		}

		float3 blendPix = lerp(H, F, step(DistYCbCr(E, F), DistYCbCr(E, H)));
		res = lerp(res, blendPix, get_left_ratio(pos, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (blendResult.w != BLEND_NONE)
	{
		float dist_H_A = DistYCbCr(H, A);
		float dist_D_I = DistYCbCr(D, I);
		bool  doLineBlend = (blendResult.w == BLEND_DOMINANT ||
		                     !((blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) || (blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) ||
		                       (IsPixEqual(A, D) && IsPixEqual(D, G) && IsPixEqual(G, H) && IsPixEqual(H, I) && !IsPixEqual(E, G))));

		float2 origin    = float2(-1.0 / sqrt(2.0), 0.0);
		float2 direction = float2(1.0, 1.0);

		if (doLineBlend)
		{
			bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_H_A <= dist_D_I) && pixNeq(E, A) && pixNeq(B, A);
			bool haveSteepLine   = (STEEP_DIRECTION_THRESHOLD * dist_D_I <= dist_H_A) && pixNeq(E, I) && pixNeq(F, I);

			origin        = haveShallowLine ? float2(-0.25, 0.0) : float2(-0.5, 0.0);
			direction.y  += haveShallowLine ? 1.0 : 0.0;
			direction.x  += haveSteepLine ? 1.0 : 0.0;
		}

		float3 blendPix = lerp(H, D, step(DistYCbCr(E, D), DistYCbCr(E, H)));
		res = lerp(res, blendPix, get_left_ratio(pos, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (blendResult.y != BLEND_NONE)
	{
		float dist_B_I = DistYCbCr(B, I);
		float dist_F_A = DistYCbCr(F, A);
		bool  doLineBlend = (blendResult.y == BLEND_DOMINANT ||
		                     !((blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) || (blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) ||
		                       (IsPixEqual(I, F) && IsPixEqual(F, C) && IsPixEqual(C, B) && IsPixEqual(B, A) && !IsPixEqual(E, C))));

		float2 origin    = float2(1.0 / sqrt(2.0), 0.0);
		float2 direction = float2(-1.0, -1.0);

		if (doLineBlend)
		{
			bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_B_I <= dist_F_A) && pixNeq(E, I) && pixNeq(H, I);
			bool haveSteepLine   = (STEEP_DIRECTION_THRESHOLD * dist_F_A <= dist_B_I) && pixNeq(E, A) && pixNeq(D, A);

			origin        = haveShallowLine ? float2(0.25, 0.0) : float2(0.5, 0.0);
			direction.y  -= haveShallowLine ? 1.0 : 0.0;
			direction.x  -= haveSteepLine ? 1.0 : 0.0;
		}

		float3 blendPix = lerp(F, B, step(DistYCbCr(E, B), DistYCbCr(E, F)));
		res = lerp(res, blendPix, get_left_ratio(pos, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (blendResult.x != BLEND_NONE)
	{
		float dist_D_C = DistYCbCr(D, C);
		float dist_B_G = DistYCbCr(B, G);
		bool  doLineBlend = (blendResult.x == BLEND_DOMINANT ||
		                     !((blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) || (blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) ||
		                       (IsPixEqual(C, B) && IsPixEqual(B, A) && IsPixEqual(A, D) && IsPixEqual(D, G) && !IsPixEqual(E, A))));

		float2 origin    = float2(0.0, -1.0 / sqrt(2.0));
		float2 direction = float2(-1.0, 1.0);

		if (doLineBlend)
		{
			bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_D_C <= dist_B_G) && pixNeq(E, C) && pixNeq(F, C);
			bool haveSteepLine   = (STEEP_DIRECTION_THRESHOLD * dist_B_G <= dist_D_C) && pixNeq(E, G) && pixNeq(H, G);

			origin        = haveShallowLine ? float2(0.0, -0.25) : float2(0.0, -0.5);
			direction.x  -= haveShallowLine ? 1.0 : 0.0;
			direction.y  += haveSteepLine ? 1.0 : 0.0;
		}

		float3 blendPix = lerp(D, B, step(DistYCbCr(E, B), DistYCbCr(E, D)));
		res = lerp(res, blendPix, get_left_ratio(pos, origin, direction, scale));
	}

	return float4(res, 1.0);
}
