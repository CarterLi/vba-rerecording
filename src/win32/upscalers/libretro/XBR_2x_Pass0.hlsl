// XBR_2x_Pass0, pass 1 of 2 of the 2xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "2xbr-lv1-c-pass0.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_2X_PASS0_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// 2xBR, the corner analysing xBR in two passes.  The first one packs the two
// flags of every pixel - whether an edge runs through it, and which of the two
// neighbours it leans to - into the channels of one texel; the second reads
// them back and blends.
//
//   pass 0  the flags of every pixel, at the frame size
//   pass 1  the blend, at whatever size the target has
//
// The licence of the ported shader:
//
// Upstream carries no licence statement and no copyright line, only the name
// of the shader: 2xBR v1.0, by Hyllian.
//

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

static float threshold;
static float y_weight;
static float u_weight;
static float v_weight;
static float3x3 yuv;
static float3x3 yuv_weighted;
static float4 bin;
static float4 maximo;

bool4 _and_(bool4 A, bool4 B)
{
    return bool4(A.x && B.x, A.y && B.y, A.z && B.z, A.w && B.w);
}

float4 df(float4 A, float4 B)
{
    return abs(A - B);
}

bool4 close(float4 A, float4 B)
{
    float4 param = A;
    float4 param_1 = B;
    float4 _157 = df(param, param_1);
    float4 _159 = threshold.xxxx;
    return bool4(_157.x < _159.x, _157.y < _159.y, _157.z < _159.z, _157.w < _159.w);
}

bool4 _or_(bool4 A, bool4 B)
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
    threshold = 15.0f;
    y_weight = 48.0f;
    u_weight = 7.0f;
    v_weight = 6.0f;
    yuv = float3x3(float3(0.2989999949932098388671875f, 0.58700001239776611328125f, 0.114000000059604644775390625f), float3(-0.1689999997615814208984375f, -0.3310000002384185791015625f, 0.499000012874603271484375f), float3(0.499000012874603271484375f, -0.4180000126361846923828125f, -0.081299997866153717041015625f));
    yuv_weighted = float3x3(float3(14.35200023651123046875f, 28.1760005950927734375f, 5.4720001220703125f), float3(-1.18299996852874755859375f, -2.316999912261962890625f, 3.493000030517578125f), float3(2.993999958038330078125f, 2.507999897003173828125f, -0.48780000209808349609375f));
    bin = float4(1.0f, 2.0f, 4.0f, 8.0f);
    maximo = 255.0f.xxxx;
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
    float4 bdhf = mul(float4x3(float3(B), float3(D), float3(H), float3(F)), yuv_weighted[0]);
    float4 cagi = mul(float4x3(float3(C), float3(A), float3(G), float3(I)), yuv_weighted[0]);
    float4 e = mul(float4x3(E, E, E, E), yuv_weighted[0]);
    float4 i4 = mul(float4x3(float3(I4), float3(C4), float3(A0), float3(G0)), yuv_weighted[0]);
    float4 i5 = mul(float4x3(float3(I5), float3(C1), float3(A1), float3(G5)), yuv_weighted[0]);
    float4 h5x = mul(float4x3(float3(H5), float3(F4), float3(B1), float3(D0)), yuv_weighted[0]);
    float4 b = bdhf.xzzx;
    float4 d = bdhf.yyww;
    float4 h = bdhf.zxxz;
    float4 f = bdhf.wwyy;
    float4 c = cagi.xwzy;
    float4 a = cagi.yzwx;
    float4 g = cagi.zyxw;
    float4 i = cagi.wxyz;
    float4 f4 = h5x.yyww;
    float4 h5 = h5x.xzzx;
    bool4 param = bool4(e.x != f.x, e.y != f.y, e.z != f.z, e.w != f.w);
    bool4 param_1 = bool4(e.x != h.x, e.y != h.y, e.z != h.z, e.w != h.w);
    bool4 r1 = _and_(param, param_1);
    float4 param_2 = f;
    float4 param_3 = b;
    bool4 _521 = close(param_2, param_3);
    float4 param_4 = f;
    float4 param_5 = c;
    bool4 _527 = close(param_4, param_5);
    bool4 param_6 = bool4(!_521.x, !_521.y, !_521.z, !_521.w);
    bool4 param_7 = bool4(!_527.x, !_527.y, !_527.z, !_527.w);
    bool4 r2 = _and_(param_6, param_7);
    float4 param_8 = h;
    float4 param_9 = d;
    bool4 _537 = close(param_8, param_9);
    float4 param_10 = h;
    float4 param_11 = g;
    bool4 _543 = close(param_10, param_11);
    bool4 param_12 = bool4(!_537.x, !_537.y, !_537.z, !_537.w);
    bool4 param_13 = bool4(!_543.x, !_543.y, !_543.z, !_543.w);
    bool4 r3 = _and_(param_12, param_13);
    float4 param_14 = f;
    float4 param_15 = f4;
    bool4 _553 = close(param_14, param_15);
    float4 param_16 = f;
    float4 param_17 = i4;
    bool4 _559 = close(param_16, param_17);
    bool4 param_18 = bool4(!_553.x, !_553.y, !_553.z, !_553.w);
    bool4 param_19 = bool4(!_559.x, !_559.y, !_559.z, !_559.w);
    bool4 r4 = _and_(param_18, param_19);
    float4 param_20 = h;
    float4 param_21 = h5;
    bool4 _569 = close(param_20, param_21);
    float4 param_22 = h;
    float4 param_23 = i5;
    bool4 _575 = close(param_22, param_23);
    bool4 param_24 = bool4(!_569.x, !_569.y, !_569.z, !_569.w);
    bool4 param_25 = bool4(!_575.x, !_575.y, !_575.z, !_575.w);
    bool4 r5 = _and_(param_24, param_25);
    float4 param_26 = e;
    float4 param_27 = i;
    bool4 param_28 = r4;
    bool4 param_29 = r5;
    bool4 param_30 = close(param_26, param_27);
    bool4 param_31 = _or_(param_28, param_29);
    bool4 r6 = _and_(param_30, param_31);
    float4 param_32 = e;
    float4 param_33 = g;
    float4 param_34 = e;
    float4 param_35 = c;
    bool4 param_36 = close(param_32, param_33);
    bool4 param_37 = close(param_34, param_35);
    bool4 r7 = _or_(param_36, param_37);
    bool4 param_38 = r2;
    bool4 param_39 = r3;
    bool4 param_40 = _or_(param_38, param_39);
    bool4 param_41 = r6;
    bool4 param_42 = _or_(param_40, param_41);
    bool4 param_43 = r7;
    bool4 param_44 = r1;
    bool4 param_45 = _or_(param_42, param_43);
    bool4 interp_restriction_lv1 = _and_(param_44, param_45);
    float4 param_46 = e;
    float4 param_47 = c;
    float4 param_48 = g;
    float4 param_49 = i;
    float4 param_50 = h5;
    float4 param_51 = f4;
    float4 param_52 = h;
    float4 param_53 = f;
    float4 _643 = weighted_distance(param_46, param_47, param_48, param_49, param_50, param_51, param_52, param_53);
    float4 param_54 = h;
    float4 param_55 = d;
    float4 param_56 = i5;
    float4 param_57 = f;
    float4 param_58 = i4;
    float4 param_59 = b;
    float4 param_60 = e;
    float4 param_61 = i;
    float4 _660 = weighted_distance(param_54, param_55, param_56, param_57, param_58, param_59, param_60, param_61);
    bool4 param_62 = bool4(_643.x < _660.x, _643.y < _660.y, _643.z < _660.z, _643.w < _660.w);
    bool4 param_63 = interp_restriction_lv1;
    bool4 edr = _and_(param_62, param_63);
    float4 param_64 = e;
    float4 param_65 = f;
    float4 _671 = df(param_64, param_65);
    float4 param_66 = e;
    float4 param_67 = h;
    float4 _676 = df(param_66, param_67);
    bool4 px = bool4(_671.x <= _676.x, _671.y <= _676.y, _671.z <= _676.z, _671.w <= _676.w);
    float4 info = mul(float4x4(float4(float(edr.x), float(px.x), 0.0f, 0.0f), float4(float(edr.y), float(px.y), 0.0f, 0.0f), float4(float(edr.z), float(px.z), 0.0f, 0.0f), float4(float(edr.w), float(px.w), 0.0f, 0.0f)), bin);
    float4 param_68 = info;
    float4 param_69 = maximo;
    FragColor = remapTo01(param_68, param_69);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	TEX0.x = input.uv.x;
	TEX0.y = input.uv.y;
	float dx = float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).z;
	float dy = float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).w;
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
