// PT_SkyWalker541, the pixel transparency filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "PT_SkyWalker541.glsl" (libretro/glsl-
// shaders, handheld/shaders).  The OpenGL 3.3 renderer runs that shader as it is
// upstream, from the resource "PT_SKYWALKER541_GLSL"; this is the port of it,
// because Direct3D compiles HLSL.
//
// It restores the look of the reflective backing of a Game Boy screen: the
// unlit pixels of the frame get the grid of the panel behind them instead of
// the flat colour the emulator hands over, and the lit ones keep a little of
// it.  Unlike the pixel filters this one is not factor agnostic in the same
// way: it draws its grid in target pixels, so the cell size follows the window.
//
// Upstream carries no licence statement, only the credit below and the note
// that the LCD dot effect follows Themaister's dot shader, which is in the
// public domain.
//
//   PT SkyWalker541 v2.1.0, by SkyWalker541.
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
//
cbuffer UpscalerConstants : register(b0)
{
	float4 SourceSize;
	float4 OutputSize;
	float4 Scale;
};

Texture2D<float4> origTex : register(t1);
SamplerState origSampler : register(s1);
Texture2D<float4> frameTex : register(t0);
SamplerState frameSampler : register(s0);

static float3 shimmerLight;
static float2 TEX0;
static float2 orig_coord;
static float2 texel;
static float4 FragColor;

float getBrightness(float3 c)
{
    return ((0.2125999927520751953125f * c.x) + (0.715200006961822509765625f * c.y)) + (0.072200000286102294921875f * c.z);
}

float3 applyDarkFilter(float3 c, float level)
{
    float strength = level * 0.00999999977648258209228515625f;
    float3 param = c;
    float luma = getBrightness(param);
    float factor = max(1.0f - (strength * luma), 0.0f);
    return c * factor;
}

float resolveThreshold()
{
    if (false)
    {
        return 0.85000002384185791015625f;
    }
    if (false)
    {
        return 0.89999997615814208984375f;
    }
    if (true)
    {
        return 0.85000002384185791015625f;
    }
    if (true)
    {
        return 0.800000011920928955078125f;
    }
    return 0.75f;
}

float isWhitePixel(float3 pixel, float brightness, float threshold)
{
    float minCh = min(pixel.x, min(pixel.y, pixel.z));
    bool _130 = brightness > threshold;
    bool _137;
    if (_130)
    {
        _137 = minCh > (threshold * 0.89999997615814208984375f);
    }
    else
    {
        _137 = _130;
    }
    if (_137)
    {
        return 1.0f;
    }
    return 0.0f;
}

float noiseHash(float2 p)
{
    float3 p3 = frac(p.xyx * 0.103100001811981201171875f);
    p3 += dot(p3, p3.yzx + 33.3300018310546875f.xxx).xxx;
    return frac((p3.x + p3.y) * p3.z);
}

float3 proceduralBackground(float2 uv)
{
    float2 param = uv * 128.0f;
    float grain = noiseHash(param) * 0.875f;
    float offset = (grain - 0.4375f) * 0.064999997615814208984375f;
    return (0.477999985218048095703125f + offset).xxx;
}

float3 applyGlareAndSheen(inout float3 bg, float2 coord)
{
    float2 g = coord * (SourceSize.xy / SourceSize.xy);
    float2 d = (g - shimmerLight.xy) * float2(SourceSize.xy.x / SourceSize.xy.y, 1.0f);
    float d2 = dot(d, d);
    if (true)
    {
        float ph = (sqrt(d2) * 2.0f) + shimmerLight.z;
        if (false)
        {
            float f = 5.0f;
            float2 u = g * f;
            float3 t3 = abs((frac(float3((u.x * 1.0f) + (u.y * 0.37000000476837158203125f), ((u.y * 1.12999999523162841796875f) - (u.x * 0.61000001430511474609375f)) + 0.310000002384185791015625f, ((u.x * 0.430000007152557373046875f) - (u.y * 1.71000003814697265625f)) + 0.7200000286102294921875f)) * 2.0f) - 1.0f.xxx);
            t3 = (t3 * t3) * (3.0f.xxx - (t3 * 2.0f));
            ph += (((((t3.x + t3.y) + t3.z) - 1.5f) * 0.0f) * 0.449999988079071044921875f);
        }
        float3 h = frac(ph.xxx + float3(0.0f, 0.33329999446868896484375f, 0.66670000553131103515625f));
        float3 w = abs((h * 2.0f) - 1.0f.xxx);
        float3 r = (w * w) * (3.0f.xxx - (w * 2.0f));
        r -= dot(r, 0.33329999446868896484375f.xxx).xxx;
        bg += (r * 0.119999997317790985107421875f);
    }
    if (true)
    {
        float q = d2 / 0.122500002384185791015625f;
        float glare = 0.1500000059604644775390625f / (1.0f + (q * q));
        bg = lerp(bg, 1.0f.xxx, min(glare, 0.85000002384185791015625f).xxx);
    }
    return clamp(bg, 0.0f.xxx, 1.0f.xxx);
}

float3 computeBacking(float threshold)
{
    float2 param = TEX0;
    float3 bg = proceduralBackground(param);
    if (true)
    {
        float2 shadowDir;
        if (true)
        {
            shadowDir = float2(-1.0f, 1.0f);
        }
        else
        {
            if (true)
            {
                shadowDir = 1.0f.xx;
            }
            else
            {
                if (true)
                {
                    shadowDir = (-1.0f).xx;
                }
                else
                {
                    shadowDir = float2(1.0f, -1.0f);
                }
            }
        }
        float2 shadowPos = orig_coord + ((shadowDir * 1.0f) * texel);
        float3 shadowSrc = origTex.Sample(origSampler, shadowPos).xyz;
        float3 param_1 = shadowSrc;
        float shadowBright = getBrightness(param_1);
        float3 param_2 = shadowSrc;
        float param_3 = shadowBright;
        float param_4 = threshold;
        float shadowSrcWhite = isWhitePixel(param_2, param_3, param_4);
        if (shadowSrcWhite < 0.5f)
        {
            float shadowStrength = (1.0f - shadowBright) * 0.300000011920928955078125f;
            bg = lerp(bg, bg * 0.20000000298023223876953125f, shadowStrength.xxx);
        }
    }
    if (true)
    {
        float3 tint;
        if (true)
        {
            tint = float3(0.65100002288818359375f, 0.675000011920928955078125f, 0.51800000667572021484375f);
        }
        else
        {
            if (true)
            {
                tint = 0.736999988555908203125f.xxx;
            }
            else
            {
                tint = 1.0f.xxx;
            }
        }
        float3 tinted = clamp(tint + ((bg * 2.0f) - 1.0f.xxx), 0.0f.xxx, 1.0f.xxx);
        bg = lerp(bg, tinted, 2.0f.xxx);
    }
    if (false)
    {
        float3 param_5 = bg;
        float2 param_6 = TEX0;
        float3 _679 = applyGlareAndSheen(param_5, param_6);
        bg = _679;
    }
    return bg;
}

float3 resolveGapColor(float3 bg)
{
    if (true)
    {
        return bg;
    }
    if (true)
    {
        return 0.0f.xxx;
    }
    return 1.0f.xxx;
}

float3 applyGrid(float3 color, float2 coord, float3 gapColor)
{
    float2 cellUV = frac(coord * SourceSize.xy);
    float edgeDist = min(min(cellUV.x, 1.0f - cellUV.x), min(cellUV.y, 1.0f - cellUV.y));
    float halfWidth = 0.125f;
    float lineMask = 1.0f - smoothstep(halfWidth - 0.039999999105930328369140625f, halfWidth, edgeDist);
    return lerp(color, gapColor, (lineMask * 0.5f).xxx);
}

float blackLevelFade(float rawLuma)
{
    return smoothstep(0.0f, 0.1500000059604644775390625f, rawLuma);
}

float3 applyLCDDot(float3 color, float2 coord, float rawLuma, float3 gapColor)
{
    if (rawLuma < 0.00999999977648258209228515625f)
    {
        return color;
    }
    float param = rawLuma;
    float lumaFade = blackLevelFade(param);
    float3 param_1 = color;
    float luma = getBrightness(param_1);
    float2 cellUV = frac(coord * SourceSize.xy);
    float2 delta = cellUV - 0.5f.xx;
    float dist = sqrt(dot(delta, delta));
    float bloomBias = lerp(0.0f, 0.07999999821186065673828125f, luma);
    float radius = 0.449999988079071044921875f + bloomBias;
    float softEdge = max(1.0f * radius, 0.0199999995529651641845703125f);
    float dotMask = clamp((radius - dist) / softEdge, 0.0f, 1.0f);
    float3 dotResult = lerp(color, gapColor, ((1.0f - dotMask) * 0.5f).xxx);
    float3 result = lerp(color, dotResult, lumaFade.xxx);
    return result;
}

float3 applyPixelEffect(float3 color, float2 coord, float rawLuma, float3 bg)
{
    if (false)
    {
        return color;
    }
    float3 param = bg;
    float3 gapColor = resolveGapColor(param);
    if (true)
    {
        float3 param_1 = color;
        float2 param_2 = coord;
        float3 param_3 = gapColor;
        return applyGrid(param_1, param_2, param_3);
    }
    float3 param_4 = color;
    float2 param_5 = coord;
    float param_6 = rawLuma;
    float3 param_7 = gapColor;
    return applyLCDDot(param_4, param_5, param_6, param_7);
}

float3 applyBezelShadow(float3 color, float2 coord)
{
    if (false)
    {
        return color;
    }
    float scale = 35.0f;
    if (false)
    {
        scale = 22.0f;
    }
    else
    {
        if (true)
        {
            scale = 35.0f;
        }
        else
        {
            if (true)
            {
                scale = 70.0f;
            }
            else
            {
                scale = 55.0f;
            }
        }
    }
    float2 gameUV = coord / (SourceSize.xy / SourceSize.xy);
    float2 uv = (gameUV * 2.0f) - 1.0f.xx;
    float shadow = clamp((1.0f - abs(uv.x)) * scale, 0.0f, 1.0f) * clamp((1.0f - abs(uv.y)) * scale, 0.0f, 1.0f);
    return color * lerp(0.60000002384185791015625f, 1.0f, shadow);
}

void frag_main()
{
    float4 lcd = frameTex.Sample(frameSampler, TEX0);
    float3 pixel = lcd.xyz;
    float3 rawPixel = origTex.Sample(origSampler, orig_coord).xyz;
    float3 param = pixel;
    float pixBrightness = getBrightness(param);
    float3 param_1 = rawPixel;
    float rawBrightness = getBrightness(param_1);
    if (false)
    {
        float3 param_2 = pixel;
        float param_3 = 0.0f;
        pixel = applyDarkFilter(param_2, param_3);
        float3 param_4 = pixel;
        pixBrightness = getBrightness(param_4);
    }
    float threshold = resolveThreshold();
    float3 param_5 = rawPixel;
    float param_6 = rawBrightness;
    float param_7 = threshold;
    float isWhite = isWhitePixel(param_5, param_6, param_7);
    float alpha = 0.0f;
    if (false && (isWhite < 0.5f))
    {
        float3 gapBacking = 0.0f.xxx;
        bool _737;
        if (true)
        {
            _737 = !(false && (rawBrightness < 0.00999999977648258209228515625f));
        }
        else
        {
            _737 = true;
        }
        if (_737)
        {
            float param_8 = threshold;
            gapBacking = computeBacking(param_8);
        }
        float3 param_9 = pixel;
        float2 param_10 = TEX0;
        float param_11 = rawBrightness;
        float3 param_12 = gapBacking;
        float3 result = applyPixelEffect(param_9, param_10, param_11, param_12);
        float3 param_13 = result;
        float2 param_14 = TEX0;
        result = applyBezelShadow(param_13, param_14);
        FragColor = float4(result, lcd.w);
        return;
    }
    float param_15 = threshold;
    float3 bg = computeBacking(param_15);
    if (false)
    {
        alpha = clamp((pixBrightness / 3.0f) + 0.100000001490116119384765625f, 0.0f, 1.0f);
    }
    else
    {
        if (false)
        {
            alpha = clamp((0.100000001490116119384765625f * pixBrightness) * 2.66499996185302734375f, 0.0f, 1.0f);
        }
        else
        {
            alpha = clamp((pixBrightness / 3.0f) + 0.100000001490116119384765625f, 0.0f, 1.0f);
        }
    }
    if (isWhite > 0.5f)
    {
        alpha = max(alpha, 0.20000000298023223876953125f);
    }
    float3 result_1 = lerp(pixel, bg, alpha.xxx);
    float3 param_16 = result_1;
    float2 param_17 = TEX0;
    float param_18 = rawBrightness;
    float3 param_19 = bg;
    result_1 = applyPixelEffect(param_16, param_17, param_18, param_19);
    float3 param_20 = result_1;
    float2 param_21 = TEX0;
    result_1 = applyBezelShadow(param_20, param_21);
    FragColor = float4(result_1, lcd.w);
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	// The frame has no padding here, so the input size and the texture size
	// are the same and the coordinate needs no correction.
	TEX0         = input.uv;
	texel        = SourceSize.zw;
	orig_coord   = input.uv;
	shimmerLight = float3(0.5f, 0.45f, 0.0f);

	frag_main();

	return FragColor;
}
