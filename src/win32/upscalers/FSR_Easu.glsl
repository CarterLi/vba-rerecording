// FSR_Easu.glsl: the reconstruction pass of FidelityFX Super Resolution 1.0.
//
// The full screen quad gives the output pixel in the fractional part of the
// fragment position, so it is truncated to the pixel the shader works on, the
// same way the HLSL entry point truncates SV_Position.

// The AMD headers use the implicit integer conversions, bitfield operations and
// half packing of GLSL 4.20, so this pass needs a 4.2 core context; where the
// driver cannot give one the renderer leaves FSR out, the same way Direct3D 11
// leaves it out below feature level 11.
#version 420 core

#include "FSR_Body.glsl"

out vec4 color;

void main()
{
    AU2 pixel = AU2(gl_FragCoord.xy);

    AF3 rgb;
    FsrEasuF(rgb, pixel, con0, con1, con2, con3);
    color = AF4(rgb, 1.0);
}
