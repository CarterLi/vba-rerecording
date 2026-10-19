// xBR-lv2 no-blend, the shader xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-lv2-noblend.glsl" (libretro/glsl-
// shaders, xbr/shaders).  The OpenGL 3.3 renderer runs the same shader as it is
// upstream, from the resource "XBR_LV2_NOBLEND_GLSL"; this is the port of it,
// because Direct3D compiles HLSL.
//
// It is xBR-lv2 without the blending: where the other one interpolates a colour
// between the two the edge runs between, this one picks one of them, which
// keeps the picture hard edged.  It is factor agnostic like the rest of them.
//
// The licence of the ported shader:
//
//   Hyllian's xBR-lv2-noblend Shader
//
//   Copyright (C) 2011-2016 Hyllian - sergiogdb@gmail.com
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
//   Incorporates some of the ideas from SABR shader. Thanks to Joshua Street.

// The quad vertex shader (VSMain) of the renderer feeds this one, so the
// varying has to be declared the way it is there.
struct XbrVSOut
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

Texture2D    frameTex     : register(t0);
SamplerState frameSampler : register(s0);

// The settings the shader ships with, from the #pragma parameter lines of the
// upstream file.  The renderer has no parameter menu, so they are its defaults.
#define XBR_EQ_THRESHOLD    0.6
#define XBR_LV2_COEFFICIENT 2.0

// Upstream picks the corner detection with one of four defines; "xbr-lv2-
// noblend.glsl" ships with CORNER_A.
#define CORNER_A

static const float3 Y = float3(0.2126, 0.7152, 0.0722);

// Difference between vector components.
float4 df(float4 A, float4 B)
{
	return abs(A - B);
}

// Whether two vector components differ.  GLSL builds a vector out of a
// bvec4, which is a 1 or a 0 a component; this is the same, without the bool
// vector type.
float4 diff(float4 A, float4 B)
{
	return abs(sign(A - B));
}

// Whether two vector components are equal, within a threshold.
float4 eq(float4 A, float4 B)
{
	return step(df(A, B), float4(XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD));
}

// The other way round.
float4 neq(float4 A, float4 B)
{
	return float4(1.0, 1.0, 1.0, 1.0) - eq(A, B);
}

// Weighted distance.
float4 wd(float4 a, float4 b, float4 c, float4 d, float4 e, float4 f, float4 g, float4 h)
{
	return (df(a, b) + df(a, c) + df(d, e) + df(d, f) + 4.0 * df(g, h));
}

float c_df(float3 c1, float3 c2)
{
	float3 diff = abs(c1 - c2);
	return diff.r + diff.g + diff.b;
}

// A float the shader reads as a boolean: GLSL converts a float to a bool by
// asking whether it is zero, this asks the same of anything from 0 to 1.
float4 asBool(float4 value)
{
	return ceil(saturate(value));
}

// Greater than, as GLSL's greaterThan() writes it: 1 where the first is the
// larger of the two.
float4 gt(float4 A, float4 B)
{
	return 1.0 - step(A, B);
}

float4 main(XbrVSOut input) : SV_TARGET
{
	float2 texel = SourceSize.zw;
	float2 coord = input.uv;
	float2 fp    = frac(coord * SourceSize.xy);

	// The taps are taken at texel centres, so the sampler has to be the point
	// one.  Upstream computes them in its vertex shader; this one computes
	// them here, which is the same thing to a point sampler.
#define P(x, y) frameTex.Sample(frameSampler, coord + texel * float2(x, y)).rgb

	float3 A1 = P(-1., -2.);
	float3 B1 = P( 0., -2.);
	float3 C1 = P( 1., -2.);
	float3 A  = P(-1., -1.);
	float3 B  = P( 0., -1.);
	float3 C  = P( 1., -1.);
	float3 D  = P(-1.,  0.);
	float3 E  = P( 0.,  0.);
	float3 F  = P( 1.,  0.);
	float3 G  = P(-1.,  1.);
	float3 H  = P( 0.,  1.);
	float3 I  = P( 1.,  1.);
	float3 G5 = P(-1.,  2.);
	float3 H5 = P( 0.,  2.);
	float3 I5 = P( 1.,  2.);
	float3 A0 = P(-2., -1.);
	float3 D0 = P(-2.,  0.);
	float3 G0 = P(-2.,  1.);
	float3 C4 = P( 2., -1.);
	float3 F4 = P( 2.,  0.);
	float3 I4 = P( 2.,  1.);

	// The luminance of the neighbourhood, four components at a time: b is the
	// four pixels the edge can run into, c the four corners.
	float4 b = float4(dot(B, Y), dot(D, Y), dot(H, Y), dot(F, Y));
	float4 c = float4(dot(C, Y), dot(A, Y), dot(G, Y), dot(I, Y));
	float4 e = float4(dot(E, Y), dot(E, Y), dot(E, Y), dot(E, Y));
	float4 d = b.yzwx;
	float4 f = b.wxyz;
	float4 g = c.zwxy;
	float4 h = b.zwxy;
	float4 i = c.wxyz;

	float4 i4 = float4(dot(I4, Y), dot(C1, Y), dot(A0, Y), dot(G5, Y));
	float4 i5 = float4(dot(I5, Y), dot(C4, Y), dot(A1, Y), dot(G0, Y));
	float4 h5 = float4(dot(H5, Y), dot(F4, Y), dot(B1, Y), dot(D0, Y));
	float4 f4 = h5.yzwx;

	const float4 Ao = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 Bo = float4( 1.0,  1.0, -1.0,-1.0 );
	const float4 Co = float4( 1.5,  0.5, -0.5, 0.5 );
	const float4 Ax = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 Bx = float4( 0.5,  2.0, -0.5,-2.0 );
	const float4 Cx = float4( 1.0,  1.0, -0.5, 0.0 );
	const float4 Ay = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 By = float4( 2.0,  0.5, -2.0,-0.5 );
	const float4 Cy = float4( 2.0,  0.0, -1.0, 0.5 );

	// These inequations define the line below which interpolation occurs.
	float4 fx   = gt(Ao * fp.y + Bo * fp.x, Co);
	float4 fx_l = gt(Ax * fp.y + Bx * fp.x, Cx);
	float4 fx_u = gt(Ay * fp.y + By * fp.x, Cy);

	float4 irlv1 = diff(e, f) * diff(e, h);

#ifdef CORNER_B
	irlv1 = neq(f, b) * neq(h, d) + eq(e, i) * neq(f, i4) * neq(h, i5) + eq(e, g) + eq(e, c);
#endif
#ifdef CORNER_D
	float4 c1 = i4.yzwx;
	float4 g0 = i5.wxyz;
	irlv1 = (neq(f, b) * neq(h, d) + eq(e, i) * neq(f, i4) * neq(h, i5) + eq(e, g) + eq(e, c)) *
	        (diff(f, f4) * diff(f, i) + diff(h, h5) * diff(h, i) + diff(h, g) + diff(f, c) + eq(b, c1) * eq(d, g0));
#endif
#ifdef CORNER_C
	irlv1 = neq(f, b) * neq(f, c) + neq(h, d) * neq(h, g) +
	        eq(e, i) * (neq(f, f4) * neq(f, i4) + neq(h, h5) * neq(h, i5)) + eq(e, g) + eq(e, c);
#endif

	float4 irlv2l = diff(e, g) * diff(d, g);
	float4 irlv2u = diff(e, c) * diff(b, c);

	float4 wd1 = wd(e, c, g, i, h5, f4, h, f);
	float4 wd2 = wd(h, d, i5, f, i4, b, e, i);

	float4 edri  = step(wd1, wd2) * irlv1;
	float4 edr   = step(wd1 + float4(0.1, 0.1, 0.1, 0.1), wd2) * step(float4(0.5, 0.5, 0.5, 0.5), irlv1);
	float4 edr_l = step(XBR_LV2_COEFFICIENT * df(f, g), df(h, c)) * irlv2l * edr;
	float4 edr_u = step(XBR_LV2_COEFFICIENT * df(h, c), df(f, g)) * irlv2u * edr;

	float4 nc = asBool(edr * (fx + edr_l * fx_l) + edr_u * fx_u);
	float4 px = step(df(e, f), df(e, h));

	float3 res1 = E;
	float3 res2 = E;

	if (nc.x > 0.5)      res1 = px.x > 0.5 ? F : H;
	else if (nc.y > 0.5) res1 = px.y > 0.5 ? B : F;
	else if (nc.z > 0.5) res1 = px.z > 0.5 ? D : B;

	if (nc.w > 0.5)      res2 = px.w > 0.5 ? H : D;
	else if (nc.z > 0.5) res2 = px.z > 0.5 ? D : B;
	else if (nc.y > 0.5) res2 = px.y > 0.5 ? B : F;

	float2 df12 = abs(float2(dot(Y, res1), dot(Y, res2)) - e.xy);

	float3 res = lerp(res1, res2, step(df12.x, df12.y));

	return float4(res, 1.0);
}
