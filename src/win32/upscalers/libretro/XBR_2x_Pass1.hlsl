// XBR_2x_Pass1, pass 2 of 2 of the 2xBR pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "2xbr-lv1-c-pass1.glsl" (libretro/glsl-shaders).
// The OpenGL 3.3 renderer runs that shader as it is upstream, from the
// resource "XBR_2X_PASS1_GLSL"; this is the port of it, because Direct3D
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

struct ResType
{
    float _m0;
    float _m1;
};

Texture2D<float4> origTex : register(t1);
SamplerState origSampler : register(s1);
Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float4 TEX0;
static float4 t1;
static float4 FragColor;

static float4x2 sym_vectors;

float remapFrom01(float v, float high)
{
    return (high * v) + 0.5f;
}

float2 unpack_info(inout float i)
{
    ResType _41;
    _41._m0 = modf(i / 2.0f, _41._m1);
    i = _41._m1;
    float2 info;
    info.x = round(_41._m0);
    info.y = i;
    return info;
}

void frag_main()
{
    sym_vectors = float4x2(1.0f.xx, float2(1.0f, -1.0f), (-1.0f).xx, float2(-1.0f, 1.0f));
    float2 OriginalCoord = floor(float4(SecondSize.xy, 1.0f.xx / SecondSize.xy).xy * TEX0.xy);
    OriginalCoord = (OriginalCoord + 0.5f.xx) * float4(SecondSize.xy, 1.0f.xx / SecondSize.xy).zw;
    float2 pos = frac(TEX0.xy * float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy) - 0.5f.xx;
    float2 dir = sign(pos);
    float2 g1 = dir * t1.xy;
    float2 g2 = dir * t1.zw;
    float3 F = origTex.Sample(origSampler, OriginalCoord + g1).xyz;
    float3 H = origTex.Sample(origSampler, OriginalCoord + g2).xyz;
    float3 E = origTex.Sample(origSampler, OriginalCoord).xyz;
    float4 icomp = round(clamp(mul(sym_vectors, dir), 0.0f.xxxx, 1.0f.xxxx));
    float param = dot(frameTex.Sample(frameSampler, TEX0.xy), icomp);
    float param_1 = 255.0f;
    float info = remapFrom01(param, param_1);
    float param_2 = info;
    float2 _170 = unpack_info(param_2);
    float2 flags = _170;
    float edr = flags.x;
    float px = flags.y;
    float3 color = lerp(E, lerp(H, F, px.xxx), (edr * 0.5f).xxx);
    FragColor = float4(color, 1.0f);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	TEX0.x = input.uv.x;
	TEX0.y = input.uv.y;
	float dx = float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).z;
	float dy = float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).w;
	t1 = float4(dx, 0.0f, 0.0f, dy);

	frag_main();

	return FragColor;
}
