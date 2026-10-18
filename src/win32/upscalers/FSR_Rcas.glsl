// FSR_Rcas.glsl: the sharpening pass of FidelityFX Super Resolution 1.0.

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

    AF1 r, g, b;
    FsrRcasF(r, g, b, pixel, conRcas);
    color = AF4(r, g, b, 1.0);
}
