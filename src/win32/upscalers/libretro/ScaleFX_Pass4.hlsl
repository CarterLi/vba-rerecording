// ScaleFX Pass4, pass 4 of the ScaleFX pixel filter of the Direct3D 11 renderer.
//
// Ported from the libretro GLSL shader "scalefx-pass4.glsl" (libretro/glsl-shaders,
// scalefx/shaders).  The OpenGL 3.3 renderer runs that shader as it is upstream,
// from the resource "SCALEFX_PASS4_GLSL"; this is the port of it, because
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

Texture2D<float4> prev5Tex : register(t1);
SamplerState prev5Sampler : register(s1);

static float4 TEX0;
static float4 FragColor;

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float2 y)
{
    return x - y * floor(x / y);
}

float3 mod(float3 x, float3 y)
{
    return x - y * floor(x / y);
}

float4 mod(float4 x, float4 y)
{
    return x - y * floor(x / y);
}

float4 loadCrn(float4 x)
{
    return floor(mod((x * 80.0f) + 0.5f.xxxx, 9.0f.xxxx));
}

float4 loadMid(float4 x)
{
    return floor(mod((x * 8.88888835906982421875f) + 0.05555500090122222900390625f.xxxx, 9.0f.xxxx));
}

void frag_main()
{
    float4 E = frameTex.Sample(frameSampler, TEX0.xy);
    float4 param = E;
    float4 crn = loadCrn(param);
    float4 param_1 = E;
    float4 mid = loadMid(param_1);
    float2 fp = floor(frac(TEX0.xy * float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy) * 3.0f);
    float _90;
    if (fp.y == 0.0f)
    {
        float _97;
        if (fp.x == 0.0f)
        {
            _97 = crn.x;
        }
        else
        {
            float _106;
            if (fp.x == 1.0f)
            {
                _106 = mid.x;
            }
            else
            {
                _106 = crn.y;
            }
            _97 = _106;
        }
        _90 = _97;
    }
    else
    {
        float _120;
        if (fp.y == 1.0f)
        {
            float _126;
            if (fp.x == 0.0f)
            {
                _126 = mid.w;
            }
            else
            {
                float _136;
                if (fp.x == 1.0f)
                {
                    _136 = 0.0f;
                }
                else
                {
                    _136 = mid.y;
                }
                _126 = _136;
            }
            _120 = _126;
        }
        else
        {
            float _148;
            if (fp.x == 0.0f)
            {
                _148 = crn.w;
            }
            else
            {
                float _157;
                if (fp.x == 1.0f)
                {
                    _157 = mid.z;
                }
                else
                {
                    _157 = crn.z;
                }
                _148 = _157;
            }
            _120 = _148;
        }
        _90 = _120;
    }
    float sp = _90;
    float2 _173;
    if (sp == 0.0f)
    {
        _173 = 0.0f.xx;
    }
    else
    {
        float2 _180;
        if (sp == 1.0f)
        {
            _180 = float2(-1.0f, 0.0f);
        }
        else
        {
            float2 _189;
            if (sp == 2.0f)
            {
                _189 = float2(-2.0f, 0.0f);
            }
            else
            {
                float2 _197;
                if (sp == 3.0f)
                {
                    _197 = float2(1.0f, 0.0f);
                }
                else
                {
                    float2 _205;
                    if (sp == 4.0f)
                    {
                        _205 = float2(2.0f, 0.0f);
                    }
                    else
                    {
                        float2 _213;
                        if (sp == 5.0f)
                        {
                            _213 = float2(0.0f, -1.0f);
                        }
                        else
                        {
                            float2 _221;
                            if (sp == 6.0f)
                            {
                                _221 = float2(0.0f, -2.0f);
                            }
                            else
                            {
                                bool2 _232 = (sp == 7.0f).xx;
                                _221 = float2(_232.x ? float2(0.0f, 1.0f).x : float2(0.0f, 2.0f).x, _232.y ? float2(0.0f, 1.0f).y : float2(0.0f, 2.0f).y);
                            }
                            _213 = _221;
                        }
                        _205 = _213;
                    }
                    _197 = _205;
                }
                _189 = _197;
            }
            _180 = _189;
        }
        _173 = _180;
    }
    float2 res = _173;
    FragColor = prev5Tex.Sample(prev5Sampler, TEX0.xy + (res / float4(SourceSize.xy, 1.0f.xx / SourceSize.xy).xy));
}

float4 main(UpscalerVSOut input) : SV_TARGET
{
	TEX0 = float4(input.uv, 0.0f, 0.0f);

	frag_main();

	return FragColor;
}
