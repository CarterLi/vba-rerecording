// FSR_Main.hlsl: FidelityFX Super Resolution 1.0 entry points.
//
// A thin wrapper around the unmodified AMD headers.  EASU reconstructs the
// frame at an arbitrary ratio, which is the part that does the upscaling, and
// RCAS sharpens the result - together they cover what NVIDIA Image Scaling
// does in a single compute shader.  Both entry points are pixel shaders, so
// the renderer compiles this file once per entry point.
//
// The shader constants come from FsrEasuCon() and FsrRcasCon() in ffx_fsr1.h;
// the renderer repeats that arithmetic, so the buffer holds the bits of the
// four EASU float4s, the RCAS one, and the size of the pass input.

#define A_GPU 1
#define A_HLSL 1
#include "ffx_a.h"

#define FSR_EASU_F 1
#define FSR_RCAS_F 1
#include "ffx_fsr1.h"

Texture2D fsrInput : register(t0);
SamplerState fsrSampler : register(s0);

cbuffer fsrConstants : register(b0)
{
    uint4 con0;
    uint4 con1;
    uint4 con2;
    uint4 con3;
    uint4 conRcas;
    uint4 sizeInfo; // input width, input height, unused, unused
};

// EASU takes its twelve taps as one 2x2 gather per colour channel, which is
// what its input callbacks are for.
AF4 FsrEasuRF(AF2 p) { return fsrInput.GatherRed(fsrSampler, p); }
AF4 FsrEasuGF(AF2 p) { return fsrInput.GatherGreen(fsrSampler, p); }
AF4 FsrEasuBF(AF2 p) { return fsrInput.GatherBlue(fsrSampler, p); }

// RCAS takes the 3x3 neighbourhood of the pass before it.  Load() returns
// zeroes outside the texture, which the outermost ring would then sharpen
// against, so the taps are clamped to the input size.
AF4 FsrRcasLoadF(ASU2 p)
{
    int2 limit = int2(sizeInfo.xy) - int2(1, 1);

    return fsrInput.Load(int3(clamp(p, int2(0, 0), limit), 0));
}

void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}

// The full screen quad gives the output pixel in the fractional part of the
// position, so it is truncated to the pixel the shader works on.
void FSR_Easu(float4 position : SV_Position, out float4 color : SV_Target)
{
    AU2 pixel = AU2(position.xy);

    AF3 rgb;
    FsrEasuF(rgb, pixel, con0, con1, con2, con3);
    color = AF4(rgb, 1.0);
}

void FSR_Rcas(float4 position : SV_Position, out float4 color : SV_Target)
{
    AU2 pixel = AU2(position.xy);

    AF1 r, g, b;
    FsrRcasF(r, g, b, pixel, conRcas);
    color = AF4(r, g, b, 1.0);
}
