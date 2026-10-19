// SuperXBR_Pass1, pass 2 of 3 of the super-xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "super-xbr-pass1.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "SUPER_XBR_PASS1_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// super-xBR: the edge data, the picture at twice the size blended with the
// frame it came from, and the windowed jinc filter that finishes it.
//
//   pass 0  the edge data, at the frame size
//   pass 1  the doubled picture blended with the frame, at twice its size
//   pass 2  the jinc filter, at whatever size the target has
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
Texture2D<float4> prev2Tex : register(t1);
SamplerState prev2Sampler : register(s1);

static float4 TEX0;
static float4 FragColor;

float RGBtoYUV(float3 color)
{
    return dot(color, float3(0.2125999927520751953125f, 0.715200006961822509765625f, 0.072200000286102294921875f));
}

float df(float A, float B)
{
    return abs(A - B);
}

float d_wd(float wp1, float wp2, float wp3, float wp4, float wp5, float wp6, float b0, float b1, float c0, float c1, float c2, float d0, float d1, float d2, float d3, float e1, float e2, float e3, float f2, float f3)
{
    float param = c1;
    float param_1 = c2;
    float param_2 = c1;
    float param_3 = c0;
    float param_4 = e2;
    float param_5 = e1;
    float param_6 = e2;
    float param_7 = e3;
    float param_8 = d2;
    float param_9 = d3;
    float param_10 = d0;
    float param_11 = d1;
    float param_12 = d1;
    float param_13 = d3;
    float param_14 = d0;
    float param_15 = d2;
    float param_16 = d1;
    float param_17 = d2;
    float param_18 = c0;
    float param_19 = c2;
    float param_20 = e1;
    float param_21 = e3;
    float param_22 = b0;
    float param_23 = b1;
    float param_24 = f2;
    float param_25 = f3;
    return (((((wp1 * (((df(param, param_1) + df(param_2, param_3)) + df(param_4, param_5)) + df(param_6, param_7))) + (wp2 * (df(param_8, param_9) + df(param_10, param_11)))) + (wp3 * (df(param_12, param_13) + df(param_14, param_15)))) + (wp4 * df(param_16, param_17))) + (wp5 * (df(param_18, param_19) + df(param_20, param_21)))) + (wp6 * (df(param_22, param_23) + df(param_24, param_25)));
}

float hv_wd(float wp1, float wp2, float wp3, float wp4, float wp5, float wp6, float i1, float i2, float i3, float i4, float e1, float e2, float e3, float e4)
{
    float param = i1;
    float param_1 = i2;
    float param_2 = i3;
    float param_3 = i4;
    float param_4 = i1;
    float param_5 = e1;
    float param_6 = i2;
    float param_7 = e2;
    float param_8 = i3;
    float param_9 = e3;
    float param_10 = i4;
    float param_11 = e4;
    float param_12 = i1;
    float param_13 = e2;
    float param_14 = i3;
    float param_15 = e4;
    float param_16 = e1;
    float param_17 = i2;
    float param_18 = e3;
    float param_19 = i4;
    return ((wp4 * (df(param, param_1) + df(param_2, param_3))) + (wp1 * (((df(param_4, param_5) + df(param_6, param_7)) + df(param_8, param_9)) + df(param_10, param_11)))) + (wp3 * (((df(param_12, param_13) + df(param_14, param_15)) + df(param_16, param_17)) + df(param_18, param_19)));
}

float max4float(float a, float b, float c, float d)
{
    return max(a, max(b, max(c, d)));
}

float3 min4(float3 a, float3 b, float3 c, float3 d)
{
    return min(a, min(b, min(c, d)));
}

float3 max4(float3 a, float3 b, float3 c, float3 d)
{
    return max(a, max(b, max(c, d)));
}

void frag_main()
{
    float wp1;
    float wp2;
    float wp3;
    float wp4;
    float wp5;
    float wp6;
    float weight1;
    float weight2;
    if (false)
    {
        wp1 = 1.0f;
        wp2 = 0.0f;
        wp3 = 0.0f;
        wp4 = 1.0f;
        wp5 = -1.0f;
        wp6 = 0.0f;
        weight1 = 0.17506800591945648193359375f;
        weight2 = 0.064816497266292572021484375f;
    }
    else
    {
        if (false)
        {
            wp1 = 8.0f;
            wp2 = 0.0f;
            wp3 = 0.0f;
            wp4 = 0.0f;
            wp5 = 0.0f;
            wp6 = 0.0f;
            weight1 = 0.17506800591945648193359375f;
            weight2 = 0.064816497266292572021484375f;
        }
        else
        {
            wp1 = 8.0f;
            wp2 = 0.0f;
            wp3 = 0.0f;
            wp4 = 0.0f;
            wp5 = 0.0f;
            wp6 = 0.0f;
            weight1 = 0.17506800591945648193359375f;
            weight2 = 0.064816497266292572021484375f;
        }
    }
    float2 fp = frac(TEX0.xy * float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy);
    float2 dir = fp - 0.5f.xx;
    if ((dir.x * dir.y) > 0.0f)
    {
        float4 _344;
        if (fp.x > 0.5f)
        {
            _344 = frameTex.Sample(frameSampler, TEX0.xy);
        }
        else
        {
            _344 = prev2Tex.Sample(prev2Sampler, TEX0.xy);
        }
        FragColor = _344;
    }
    else
    {
        float2 _367;
        if (fp.x > 0.5f)
        {
            _367 = float2(0.5f / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).x, 0.0f);
        }
        else
        {
            _367 = float2(0.0f, 0.5f / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).y);
        }
        float2 g1 = _367;
        float2 _400;
        if (fp.x > 0.5f)
        {
            _400 = float2(0.0f, 0.5f / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).y);
        }
        else
        {
            _400 = float2(0.5f / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).x, 0.0f);
        }
        float2 g2 = _400;
        float3 P0 = prev2Tex.Sample(prev2Sampler, TEX0.xy - (g1 * 3.0f)).xyz;
        float3 P1 = frameTex.Sample(frameSampler, TEX0.xy - (g2 * 3.0f)).xyz;
        float3 P2 = frameTex.Sample(frameSampler, TEX0.xy + (g2 * 3.0f)).xyz;
        float3 P3 = prev2Tex.Sample(prev2Sampler, TEX0.xy + (g1 * 3.0f)).xyz;
        float3 B = frameTex.Sample(frameSampler, (TEX0.xy - (g1 * 2.0f)) - g2).xyz;
        float3 C = prev2Tex.Sample(prev2Sampler, (TEX0.xy - g1) - (g2 * 2.0f)).xyz;
        float3 D = frameTex.Sample(frameSampler, (TEX0.xy - (g1 * 2.0f)) + g2).xyz;
        float3 E = prev2Tex.Sample(prev2Sampler, TEX0.xy - g1).xyz;
        float3 F = frameTex.Sample(frameSampler, TEX0.xy - g2).xyz;
        float3 G = prev2Tex.Sample(prev2Sampler, (TEX0.xy - g1) + (g2 * 2.0f)).xyz;
        float3 H = frameTex.Sample(frameSampler, TEX0.xy + g2).xyz;
        float3 I = prev2Tex.Sample(prev2Sampler, TEX0.xy + g1).xyz;
        float3 F4 = prev2Tex.Sample(prev2Sampler, (TEX0.xy + g1) - (g2 * 2.0f)).xyz;
        float3 I4 = frameTex.Sample(frameSampler, (TEX0.xy + (g1 * 2.0f)) - g2).xyz;
        float3 H5 = prev2Tex.Sample(prev2Sampler, (TEX0.xy + g1) + (g2 * 2.0f)).xyz;
        float3 I5 = frameTex.Sample(frameSampler, (TEX0.xy + (g1 * 2.0f)) + g2).xyz;
        float3 param = B;
        float b = RGBtoYUV(param);
        float3 param_1 = C;
        float c = RGBtoYUV(param_1);
        float3 param_2 = D;
        float d = RGBtoYUV(param_2);
        float3 param_3 = E;
        float e = RGBtoYUV(param_3);
        float3 param_4 = F;
        float f = RGBtoYUV(param_4);
        float3 param_5 = G;
        float g = RGBtoYUV(param_5);
        float3 param_6 = H;
        float h = RGBtoYUV(param_6);
        float3 param_7 = I;
        float i = RGBtoYUV(param_7);
        float3 param_8 = I4;
        float i4 = RGBtoYUV(param_8);
        float3 param_9 = P0;
        float p0 = RGBtoYUV(param_9);
        float3 param_10 = I5;
        float i5 = RGBtoYUV(param_10);
        float3 param_11 = P1;
        float p1 = RGBtoYUV(param_11);
        float3 param_12 = H5;
        float h5 = RGBtoYUV(param_12);
        float3 param_13 = P2;
        float p2 = RGBtoYUV(param_13);
        float3 param_14 = F4;
        float f4 = RGBtoYUV(param_14);
        float3 param_15 = P3;
        float p3 = RGBtoYUV(param_15);
        float param_16 = wp1;
        float param_17 = wp2;
        float param_18 = wp3;
        float param_19 = wp4;
        float param_20 = wp5;
        float param_21 = wp6;
        float param_22 = d;
        float param_23 = b;
        float param_24 = g;
        float param_25 = e;
        float param_26 = c;
        float param_27 = p2;
        float param_28 = h;
        float param_29 = f;
        float param_30 = p1;
        float param_31 = h5;
        float param_32 = i;
        float param_33 = f4;
        float param_34 = i5;
        float param_35 = i4;
        float param_36 = wp1;
        float param_37 = wp2;
        float param_38 = wp3;
        float param_39 = wp4;
        float param_40 = wp5;
        float param_41 = wp6;
        float param_42 = c;
        float param_43 = f4;
        float param_44 = b;
        float param_45 = f;
        float param_46 = i4;
        float param_47 = p0;
        float param_48 = e;
        float param_49 = i;
        float param_50 = p3;
        float param_51 = d;
        float param_52 = h;
        float param_53 = i5;
        float param_54 = g;
        float param_55 = h5;
        float d_edge = d_wd(param_16, param_17, param_18, param_19, param_20, param_21, param_22, param_23, param_24, param_25, param_26, param_27, param_28, param_29, param_30, param_31, param_32, param_33, param_34, param_35) - d_wd(param_36, param_37, param_38, param_39, param_40, param_41, param_42, param_43, param_44, param_45, param_46, param_47, param_48, param_49, param_50, param_51, param_52, param_53, param_54, param_55);
        float param_56 = wp1;
        float param_57 = wp2;
        float param_58 = wp3;
        float param_59 = wp4;
        float param_60 = wp5;
        float param_61 = wp6;
        float param_62 = f;
        float param_63 = i;
        float param_64 = e;
        float param_65 = h;
        float param_66 = c;
        float param_67 = i5;
        float param_68 = b;
        float param_69 = h5;
        float param_70 = wp1;
        float param_71 = wp2;
        float param_72 = wp3;
        float param_73 = wp4;
        float param_74 = wp5;
        float param_75 = wp6;
        float param_76 = e;
        float param_77 = f;
        float param_78 = h;
        float param_79 = i;
        float param_80 = d;
        float param_81 = f4;
        float param_82 = g;
        float param_83 = i4;
        float hv_edge = hv_wd(param_56, param_57, param_58, param_59, param_60, param_61, param_62, param_63, param_64, param_65, param_66, param_67, param_68, param_69) - hv_wd(param_70, param_71, param_72, param_73, param_74, param_75, param_76, param_77, param_78, param_79, param_80, param_81, param_82, param_83);
        float limits = 0.60000097751617431640625f;
        float edge_strength = smoothstep(0.0f, limits, abs(d_edge));
        float4 w1;
        float4 w2;
        float3 c3;
        float3 c4;
        if (false)
        {
            float param_84 = e;
            float param_85 = f;
            float param_86 = e;
            float param_87 = i;
            float param_88 = e;
            float param_89 = h;
            float param_90 = f;
            float param_91 = h;
            float param_92 = df(param_84, param_85);
            float param_93 = df(param_86, param_87);
            float param_94 = df(param_88, param_89);
            float param_95 = df(param_90, param_91);
            float param_96 = f;
            float param_97 = i;
            float param_98 = h;
            float param_99 = i;
            float contrast = max(max4float(param_92, param_93, param_94, param_95), max(df(param_96, param_97), df(param_98, param_99))) / (e + 0.001000000047497451305389404296875f);
            float wgt1 = weight1 * ((smoothstep(0.0f, 0.60000002384185791015625f, contrast) * 0.4000000059604644775390625f) + 1.0f);
            float wgt2 = weight2 * ((smoothstep(0.0f, 0.60000002384185791015625f, contrast) * 0.4000000059604644775390625f) + 1.0f);
            w1 = float4(-wgt1, wgt1 + 0.5f, wgt1 + 0.5f, -wgt1);
            w2 = float4(-wgt2, wgt2 + 0.25f, wgt2 + 0.25f, -wgt2);
            c3 = mul(w2, float4x3(float3((P0 + ((D + G) * 2.0f)) + P2), float3((B + ((E + H) * 2.0f)) + H5), float3((C + ((F + I) * 2.0f)) + I5), float3((P1 + ((F4 + I4) * 2.0f)) + P3))) / 3.0f.xxx;
            c4 = mul(w2, float4x3(float3((P0 + ((C + B) * 2.0f)) + P1), float3((D + ((F + E) * 2.0f)) + F4), float3((G + ((I + H) * 2.0f)) + I4), float3((P2 + ((I5 + H5) * 2.0f)) + P3))) / 3.0f.xxx;
        }
        else
        {
            w1 = float4(-weight1, weight1 + 0.5f, weight1 + 0.5f, -weight1);
            w2 = float4(-weight2, weight2 + 0.25f, weight2 + 0.25f, -weight2);
            c3 = mul(w2, float4x3(float3(D + G), float3(E + H), float3(F + I), float3(F4 + I4)));
            c4 = mul(w2, float4x3(float3(C + B), float3(F + E), float3(I + H), float3(I5 + H5)));
        }
        float3 c1 = mul(w1, float4x3(float3(P2), float3(H), float3(F), float3(P1)));
        float3 c2 = mul(w1, float4x3(float3(P0), float3(E), float3(I), float3(P3)));
        float3 color = lerp(lerp(c1, c2, step(0.0f, d_edge).xxx), lerp(c3, c4, step(0.0f, hv_edge).xxx), (1.0f - edge_strength).xxx);
        float3 param_100 = E;
        float3 param_101 = F;
        float3 param_102 = H;
        float3 param_103 = I;
        float3 min_sample = min4(param_100, param_101, param_102, param_103) + (lerp((P2 - H) * (F - P1), (P0 - E) * (I - P3), step(0.0f, d_edge).xxx) * 0.0f);
        float3 param_104 = E;
        float3 param_105 = F;
        float3 param_106 = H;
        float3 param_107 = I;
        float3 max_sample = max4(param_104, param_105, param_106, param_107) - (lerp((P2 - H) * (F - P1), (P0 - E) * (I - P3), step(0.0f, d_edge).xxx) * 0.0f);
        color = clamp(color, min_sample, max_sample);
        FragColor = float4(color, 1.0f);
    }
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	float2 _35 = input.uv * 1.00010001659393310546875f;
	TEX0.x = _35.x;
	TEX0.y = _35.y;

	frag_main();

	return FragColor;
}
