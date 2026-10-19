// xBR-lv2, the shader xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-lv2.glsl" (libretro/glsl-shaders,
// xbr/shaders).  The OpenGL 3.3 renderer runs the same shader as it is
// upstream, from the resource "XBR_LV2_GLSL"; this is the port of it, because
// Direct3D compiles HLSL.
//
// Like the xBRZ port it filters the frame rather than reconstructing it, at
// whatever size the emulation thread produced it, and it is factor agnostic:
// the pass hands it the frame size and the target size and it blends the edge
// over the whole window, so no CPU filter runs.
//
// The licence of the ported shader:
//
//   Hyllian's xBR-lv2 Shader
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
#define XBR_Y_WEIGHT         48.0
#define XBR_EQ_THRESHOLD     15.0
#define XBR_LV1_COEFFICIENT  0.5
#define XBR_LV2_COEFFICIENT  2.0

// Upstream picks the corner detection with one of four defines; "xbr-lv2.glsl"
// ships with CORNER_C, which also turns the smooth tips on.
#define CORNER_C
#define SMOOTH_TIPS

// The width over which the straight lines of the interpolation are smoothed.
#define XBR_SCALE 3.0

static const float coef        = 2.0;
static const float3 rgbw       = float3(14.352, 28.176, 5.472);

static const float4 delta   = float4(1.0 / XBR_SCALE, 1.0 / XBR_SCALE, 1.0 / XBR_SCALE, 1.0 / XBR_SCALE);
static const float4 delta_l = float4(0.5 / XBR_SCALE, 1.0 / XBR_SCALE, 0.5 / XBR_SCALE, 1.0 / XBR_SCALE);
static const float4 delta_u = float4(1.0 / XBR_SCALE, 0.5 / XBR_SCALE, 1.0 / XBR_SCALE, 0.5 / XBR_SCALE);

static const float4 Ao = float4( 1.0, -1.0, -1.0, 1.0 );
static const float4 Bo = float4( 1.0,  1.0, -1.0,-1.0 );
static const float4 Co = float4( 1.5,  0.5, -0.5, 0.5 );
static const float4 Ax = float4( 1.0, -1.0, -1.0, 1.0 );
static const float4 Bx = float4( 0.5,  2.0, -0.5,-2.0 );
static const float4 Cx = float4( 1.0,  1.0, -0.5, 0.0 );
static const float4 Ay = float4( 1.0, -1.0, -1.0, 1.0 );
static const float4 By = float4( 2.0,  0.5, -2.0,-0.5 );
static const float4 Cy = float4( 2.0,  0.0, -1.0, 0.5 );
static const float4 Ci = float4(0.25, 0.25, 0.25, 0.25);

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

float4 main(XbrVSOut input) : SV_TARGET
{
	float2 texel = SourceSize.zw;
	float2 coord = input.uv;
	// Upstream multiplies by the size rather than dividing by the texel it
	// derived, which is the same number and one rounding less.
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

	float4 b = float4(dot(B, rgbw), dot(D, rgbw), dot(H, rgbw), dot(F, rgbw));
	float4 c = float4(dot(C, rgbw), dot(A, rgbw), dot(G, rgbw), dot(I, rgbw));
	float4 d = b.yzwx;
	float4 e = float4(dot(E, rgbw), dot(E, rgbw), dot(E, rgbw), dot(E, rgbw));
	float4 f = b.wxyz;
	float4 g = c.zwxy;
	float4 h = b.zwxy;
	float4 i = c.wxyz;

	float4 i4 = float4(dot(I4, rgbw), dot(C1, rgbw), dot(A0, rgbw), dot(G5, rgbw));
	float4 i5 = float4(dot(I5, rgbw), dot(C4, rgbw), dot(A1, rgbw), dot(G0, rgbw));
	float4 h5 = float4(dot(H5, rgbw), dot(F4, rgbw), dot(B1, rgbw), dot(D0, rgbw));
	float4 f4 = h5.yzwx;

	// These inequations define the line below which interpolation occurs.
	float4 fx   = (Ao * fp.y + Bo * fp.x);
	float4 fx_l = (Ax * fp.y + Bx * fp.x);
	float4 fx_u = (Ay * fp.y + By * fp.x);

	float4 irlv0 = diff(e, f) * diff(e, h);
	float4 irlv1 = irlv0;

#ifdef CORNER_B
	irlv1 = (irlv0 * (neq(f, b) * neq(h, d) + eq(e, i) * neq(f, i4) * neq(h, i5) + eq(e, g) + eq(e, c)));
#endif
#ifdef CORNER_C
	irlv1 = (irlv0 * (neq(f, b) * neq(f, c) + neq(h, d) * neq(h, g) +
	                  eq(e, i) * (neq(f, f4) * neq(f, i4) + neq(h, h5) * neq(h, i5)) + eq(e, g) + eq(e, c)));
#endif

	float4 irlv2l = diff(e, g) * diff(d, g);
	float4 irlv2u = diff(e, c) * diff(b, c);

	float4 fx45i = clamp((fx   + delta   - Co - Ci) / (2.0 * delta  ), 0.0, 1.0);
	float4 fx45  = clamp((fx   + delta   - Co     ) / (2.0 * delta  ), 0.0, 1.0);
	float4 fx30  = clamp((fx_l + delta_l - Cx     ) / (2.0 * delta_l), 0.0, 1.0);
	float4 fx60  = clamp((fx_u + delta_u - Cy     ) / (2.0 * delta_u), 0.0, 1.0);

	float4 wd1 = wd(e, c, g, i, h5, f4, h, f);
	float4 wd2 = wd(h, d, i5, f, i4, b, e, i);

	float4 edri  = step(wd1, wd2) * irlv0;
	float4 edr   = step(wd1 + float4(0.1, 0.1, 0.1, 0.1), wd2) * step(float4(0.5, 0.5, 0.5, 0.5), irlv1);
	float4 edr_l = step(XBR_LV2_COEFFICIENT * df(f, g), df(h, c)) * irlv2l * edr;
	float4 edr_u = step(XBR_LV2_COEFFICIENT * df(h, c), df(f, g)) * irlv2u * edr;

	fx45  = edr   * fx45;
	fx30  = edr_l * fx30;
	fx60  = edr_u * fx60;
	fx45i = edri  * fx45i;

	float4 px = step(df(e, f), df(e, h));

#ifdef SMOOTH_TIPS
	float4 maximos = max(max(fx30, fx60), max(fx45, fx45i));
#else
	float4 maximos = max(max(fx30, fx60), fx45);
#endif

	float3 res1 = E;
	res1 = lerp(res1, lerp(H, F, px.x), maximos.x);
	res1 = lerp(res1, lerp(B, D, px.z), maximos.z);

	float3 res2 = E;
	res2 = lerp(res2, lerp(F, B, px.y), maximos.y);
	res2 = lerp(res2, lerp(D, H, px.w), maximos.w);

	float3 res = lerp(res1, res2, step(c_df(E, res1), c_df(E, res2)));

	return float4(res, 1.0);
}
