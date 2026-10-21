// Lanczos3, the Lanczos upscaler of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "lanczos3.glsl" (libretro/glsl-shaders,
// windowed/shaders).  The OpenGL 3.3 renderer runs that file as it is upstream,
// from the resource "LANCZOS3_GLSL"; this is the port of it, because Direct3D
// compiles HLSL.
//
// It is a separable windowed Lanczos filter of three taps per axis, taken at
// texel centres and stretched over the whole window, so it is factor agnostic
// like the rest of the single pass upscalers: at 1x it hands the frame back, at
// 4x it gives a smooth enlargement with the ringing of the kernel at the edges.
// Upstream calls the window Lanczos2 in a comment, because the radius of the
// kernel is 2 even though it reads three taps; the file and the preset it
// belongs to are called lanczos3.
//
// The upstream shader carries no licence statement, only its place in the
// windowed folder of the libretro shaders.

// The quad vertex shader (VSMain) of the renderer feeds this one, so the
// varying has to be declared the way it is there.
struct LanczosVSOut
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
// The taps are taken at texel centres, which is what the point sampler the
// renderer binds at s0 gives.
cbuffer UpscalerConstants : register(b0)
{
	float4 SourceSize;
	float4 OutputSize;
	float4 Scale;
};

Texture2D    frameTex     : register(t0);
SamplerState frameSampler : register(s0);

#define LANCZOS_PI     3.1415926535897932384626433832795
#define LANCZOS_RADIUS 2.0

// The three taps of one axis, for the distance x between the sample and the
// middle tap: the kernel at 1 + x, x and 1 - x, normalised to sum to one.
// Upstream's FIX() keeps the middle term from dividing zero by zero at a texel
// centre, where sin() of a multiple of pi is not quite zero.
float3 lanczosTaps(float x)
{
	float3 smpl = max(abs(LANCZOS_PI * float3(1.0 + x, x, 1.0 - x)), 0.000001);

	// No point in multiplying by the radius, the taps are normalised below.
	float3 taps = sin(smpl) * sin(smpl / LANCZOS_RADIUS) / (smpl * smpl);

	return taps / dot(taps, float3(1.0, 1.0, 1.0));
}

float3 lanczosPixel(float2 texcoord)
{
	return frameTex.SampleLevel(frameSampler, texcoord, 0).rgb;
}

// One row of three pixels, weighted by the taps of the other axis.
float3 lanczosLine(float y, float3 x, float3 taps)
{
	return lanczosPixel(float2(x.x, y)) * taps.x
	     + lanczosPixel(float2(x.y, y)) * taps.y
	     + lanczosPixel(float2(x.z, y)) * taps.z;
}

float4 main(LanczosVSOut input) : SV_TARGET
{
	float2 onePixel = SourceSize.zw;

	// Where the sample falls in the frame, in texels, and how far past the
	// texel centre it starts at it lies.
	float2 pos = input.uv + onePixel * 0.5;
	float2 f   = frac(pos * SourceSize.xy);

	float3 linetaps   = lanczosTaps(f.x);
	float3 columntaps = lanczosTaps(f.y);

	// The first of the three taps of each axis, one texel left of the texel
	// the sample starts in.
	float2 start = pos - onePixel * (f + 1.5);

	float3 xpos = float3(start.x,
	                     start.x + onePixel.x,
	                     start.x + onePixel.x * 2.0);

	float3 rgb = lanczosLine(start.y,                     xpos, linetaps) * columntaps.x
	           + lanczosLine(start.y + onePixel.y,        xpos, linetaps) * columntaps.y
	           + lanczosLine(start.y + onePixel.y * 2.0,  xpos, linetaps) * columntaps.z;

	return float4(saturate(rgb), 1.0);
}
