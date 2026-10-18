// FSR_Body.glsl: the FidelityFX Super Resolution 1.0 code both passes share.
//
// The same thin wrapper as FSR_Main.hlsl, around the unmodified AMD headers, so
// that the frame is reconstructed by exactly the same arithmetic on the OpenGL
// backend as it is on the Direct3D 11 one.  EASU reconstructs the frame at an
// arbitrary ratio, which is the part that does the upscaling, and RCAS sharpens
// the result.
//
// This file has no #version of its own: the two pass sources below include it,
// which is how one body serves the two entry points a single HLSL file can hold.
//
// The shader constants come from FsrEasuCon() and FsrRcasCon() in ffx_fsr1.h;
// the renderer repeats that arithmetic, so the uniforms hold the bits of the
// four EASU uint4s, the RCAS one, and the size of the pass input.

#define A_GPU 1
#define A_GLSL 1
#include "ffx_a.h"

#define FSR_EASU_F 1
#define FSR_RCAS_F 1
#include "ffx_fsr1.h"

uniform sampler2D fsrInput;

uniform uvec4 con0;
uniform uvec4 con1;
uniform uvec4 con2;
uniform uvec4 con3;
uniform uvec4 conRcas;
uniform uvec4 sizeInfo; // input width, input height, unused, unused

// EASU takes its twelve taps as one 2x2 gather per colour channel, which is
// what its input callbacks are for.
AF4 FsrEasuRF(AF2 p) { return AF4(textureGather(fsrInput, p, 0)); }
AF4 FsrEasuGF(AF2 p) { return AF4(textureGather(fsrInput, p, 1)); }
AF4 FsrEasuBF(AF2 p) { return AF4(textureGather(fsrInput, p, 2)); }

// RCAS takes the 3x3 neighbourhood of the pass before it.  texelFetch outside
// the texture would return zeroes, which the outermost ring would then sharpen
// against, so the taps are clamped to the input size the same way the HLSL pass
// clamps its loads.
AF4 FsrRcasLoadF(ASU2 p)
{
    ivec2 limit = ivec2(sizeInfo.xy) - ivec2(1, 1);

    return texelFetch(fsrInput, clamp(ivec2(p), ivec2(0, 0), limit), 0);
}

void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
