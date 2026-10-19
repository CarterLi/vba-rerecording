// xBR-lv3, the shader xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "xbr-lv3.glsl" (libretro/glsl-shaders,
// xbr/shaders).  The OpenGL 3.3 renderer runs the same shader as it is
// upstream, from the resource "XBR_LV3_GLSL"; this is the port of it, because
// Direct3D compiles HLSL.
//
// xBR-lv3 interpolates the edges of three slopes rather than two, so it also
// looks at the pixels two apart.  It is factor agnostic like the rest of them.
//
// The licence of the ported shader:
//
//   Hyllian's xBR-lv3 Shader
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
#define XBR_Y_WEIGHT        48.0
#define XBR_EQ_THRESHOLD    10.0
#define XBR_EQ_THRESHOLD2   2.0
#define XBR_LV2_COEFFICIENT 2.0
#define CORNER_TYPE         3.0

// The luma row of the colour space the shader compares pixels in, scaled by
// the weight above: 48 * (0.299, 0.587, 0.114).
static const float3 yuvLuma = float3(14.352, 28.176, 5.472);
static const float4 delta   = float4(0.4, 0.4, 0.4, 0.4);

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

// Whether two vector components are equal, within a threshold.  Upstream
// compares with lessThan(), so a difference of exactly the threshold is not
// equal; step(df, threshold) would say it is.
float4 eq(float4 A, float4 B)
{
	return 1.0 - step(float4(XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD, XBR_EQ_THRESHOLD), df(A, B));
}

// The same, against the second, much tighter threshold.
float4 eq2(float4 A, float4 B)
{
	return 1.0 - step(float4(XBR_EQ_THRESHOLD2, XBR_EQ_THRESHOLD2, XBR_EQ_THRESHOLD2, XBR_EQ_THRESHOLD2), df(A, B));
}

// The boolean algebra the shader does with bvec4: with 0 and 1 as the two
// values, AND is a product, OR is the same clamped, and NOT is what is left.
float4 bAnd(float4 A, float4 B)
{
	return A * B;
}

float4 bOr(float4 A, float4 B)
{
	return min(A + B, 1.0);
}

float4 bNot(float4 A)
{
	return 1.0 - A;
}

// Comparisons, as GLSL writes them: 1 where the comparison holds.
float4 lt(float4 A, float4 B)  { return 1.0 - step(B, A); } // A <  B
float4 le(float4 A, float4 B)  { return step(A, B); }       // A <= B
float4 ge(float4 A, float4 B)  { return step(B, A); }       // A >= B

// A float the shader reads as a boolean: GLSL converts a float to a bool by
// asking whether it is zero, this asks the same of anything from 0 to 1.
float4 asBool(float4 value)
{
	return ceil(saturate(value));
}

float4 weighted_distance(float4 a, float4 b, float4 c, float4 d, float4 e, float4 f, float4 g, float4 h)
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

	float4 b = float4(dot(B, yuvLuma), dot(D, yuvLuma), dot(H, yuvLuma), dot(F, yuvLuma));
	float4 c = float4(dot(C, yuvLuma), dot(A, yuvLuma), dot(G, yuvLuma), dot(I, yuvLuma));
	float4 e = float4(dot(E, yuvLuma), dot(E, yuvLuma), dot(E, yuvLuma), dot(E, yuvLuma));
	float4 d = b.yzwx;
	float4 f = b.wxyz;
	float4 g = c.zwxy;
	float4 h = b.zwxy;
	float4 i = c.wxyz;

	float4 i4 = float4(dot(I4, yuvLuma), dot(C1, yuvLuma), dot(A0, yuvLuma), dot(G5, yuvLuma));
	float4 i5 = float4(dot(I5, yuvLuma), dot(C4, yuvLuma), dot(A1, yuvLuma), dot(G0, yuvLuma));
	float4 h5 = float4(dot(H5, yuvLuma), dot(F4, yuvLuma), dot(B1, yuvLuma), dot(D0, yuvLuma));
	float4 f4 = h5.yzwx;

	float4 c1 = i4.yzwx;
	float4 g0 = i5.wxyz;
	float4 b1 = h5.zwxy;
	float4 d0 = h5.wxyz;

	const float4 Ao = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 Bo = float4( 1.0,  1.0, -1.0,-1.0 );
	const float4 Co = float4( 1.5,  0.5, -0.5, 0.5 );
	const float4 Ax = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 Bx = float4( 0.5,  2.0, -0.5,-2.0 );
	const float4 Cx = float4( 1.0,  1.0, -0.5, 0.0 );
	const float4 Ay = float4( 1.0, -1.0, -1.0, 1.0 );
	const float4 By = float4( 2.0,  0.5, -2.0,-0.5 );
	const float4 Cy = float4( 2.0,  0.0, -1.0, 0.5 );

	const float4 Az = float4( 6.0, -2.0, -6.0, 2.0 );
	const float4 Bz = float4( 2.0,  6.0, -2.0,-6.0 );
	const float4 Cz = float4( 5.0,  3.0, -3.0,-1.0 );
	const float4 Aw = float4( 2.0, -6.0, -2.0, 6.0 );
	const float4 Bw = float4( 6.0,  2.0, -6.0,-2.0 );
	const float4 Cw = float4( 5.0, -1.0, -3.0, 3.0 );

	float4 fx       = (Ao * fp.y + Bo * fp.x);
	float4 fx_left  = (Ax * fp.y + Bx * fp.x);
	float4 fx_up    = (Ay * fp.y + By * fp.x);
	float4 fx3_left = (Az * fp.y + Bz * fp.x);
	float4 fx3_up   = (Aw * fp.y + Bw * fp.x);

	// Which corners may be interpolated at all.  Upstream picks one of four
	// rules; "xbr-lv3.glsl" ships with the third, which is the branch below.
	float4 interp_restriction_lv1;

	if (CORNER_TYPE == 1.0)
	{
		interp_restriction_lv1 = bAnd(diff(e, f), diff(e, h));
	}
	else if (CORNER_TYPE == 2.0)
	{
		interp_restriction_lv1 = bAnd(bAnd(diff(e, f), diff(e, h)),
		                              bOr(bOr(bAnd(bAnd(bOr(bAnd(bNot(eq(f, b)), bNot(eq(h, d))),
		                                                      eq(e, i)),
		                                                bNot(eq(f, i4))),
		                                           bNot(eq(h, i5))),
		                                      eq(e, g)),
		                                  eq(e, c)));
	}
	else
	{
		interp_restriction_lv1 = bAnd(bAnd(diff(e, f), diff(e, h)),
		                              bOr(bOr(bAnd(bNot(eq(f, b)), bNot(eq(f, c))),
		                                      bAnd(bNot(eq(h, d)), bNot(eq(h, g)))),
		                                  bOr(bAnd(eq(e, i),
		                                           bOr(bAnd(bNot(eq(f, f4)), bNot(eq(f, i4))),
		                                               bAnd(bNot(eq(h, h5)), bNot(eq(h, i5))))),
		                                      bOr(eq(e, g), eq(e, c)))));
	}

	float4 interp_restriction_lv2_left = bAnd(diff(e, g), diff(d, g));
	float4 interp_restriction_lv2_up   = bAnd(diff(e, c), diff(b, c));
	float4 interp_restriction_lv3_left = bAnd(eq2(g, g0), bNot(eq2(d0, g0)));
	float4 interp_restriction_lv3_up   = bAnd(eq2(c, c1), bNot(eq2(b1, c1)));

	float4 fx45 = smoothstep(Co - delta, Co + delta, fx);
	float4 fx30 = smoothstep(Cx - delta, Cx + delta, fx_left);
	float4 fx60 = smoothstep(Cy - delta, Cy + delta, fx_up);
	float4 fx15 = smoothstep(Cz - delta, Cz + delta, fx3_left);
	float4 fx75 = smoothstep(Cw - delta, Cw + delta, fx3_up);

	float4 edr       = bAnd(lt(weighted_distance(e, c, g, i, h5, f4, h, f),
	                           weighted_distance(h, d, i5, f, i4, b, e, i)), interp_restriction_lv1);
	float4 edr_left  = bAnd(le(XBR_LV2_COEFFICIENT * df(f, g), df(h, c)), interp_restriction_lv2_left);
	float4 edr_up    = bAnd(ge(df(f, g), XBR_LV2_COEFFICIENT * df(h, c)), interp_restriction_lv2_up);
	float4 edr3_left = interp_restriction_lv3_left;
	float4 edr3_up   = interp_restriction_lv3_up;

	float4 nc45 = bAnd(edr, asBool(fx45));
	float4 nc30 = bAnd(edr, bAnd(edr_left, asBool(fx30)));
	float4 nc60 = bAnd(edr, bAnd(edr_up, asBool(fx60)));
	float4 nc15 = bAnd(bAnd(edr, edr_left), bAnd(edr3_left, asBool(fx15)));
	float4 nc75 = bAnd(bAnd(edr, edr_up), bAnd(edr3_up, asBool(fx75)));

	float4 px = le(df(e, f), df(e, h));

	float4 nc = bOr(bOr(bOr(bOr(nc75, nc15), nc30), nc60), nc45);

	float4 final45 = nc45 * fx45;
	float4 final30 = nc30 * fx30;
	float4 final60 = nc60 * fx60;
	float4 final15 = nc15 * fx15;
	float4 final75 = nc75 * fx75;

	float4 maximo = max(max(max(final15, final75), max(final30, final60)), final45);

	float3 pix1 = E, pix2 = E;
	float  blend1 = 0.0, blend2 = 0.0;

	if (nc.x > 0.5)      { pix1 = px.x > 0.5 ? F : H; blend1 = maximo.x; }
	else if (nc.y > 0.5) { pix1 = px.y > 0.5 ? B : F; blend1 = maximo.y; }
	else if (nc.z > 0.5) { pix1 = px.z > 0.5 ? D : B; blend1 = maximo.z; }
	else if (nc.w > 0.5) { pix1 = px.w > 0.5 ? H : D; blend1 = maximo.w; }

	if (nc.w > 0.5)      { pix2 = px.w > 0.5 ? H : D; blend2 = maximo.w; }
	else if (nc.z > 0.5) { pix2 = px.z > 0.5 ? D : B; blend2 = maximo.z; }
	else if (nc.y > 0.5) { pix2 = px.y > 0.5 ? B : F; blend2 = maximo.y; }
	else if (nc.x > 0.5) { pix2 = px.x > 0.5 ? F : H; blend2 = maximo.x; }

	float3 res1 = lerp(E, pix1, blend1);
	float3 res2 = lerp(E, pix2, blend2);
	float3 res  = lerp(res1, res2, step(c_df(E, res1), c_df(E, res2)));

	return float4(res, 1.0);
}
