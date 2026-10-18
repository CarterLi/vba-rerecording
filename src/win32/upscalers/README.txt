Frame upscaling shaders

The Direct3D 11 renderer can rebuild the frame at the window size with either
of two SDKs, or filter it with xBRZ while stretching it.  All three shaders are
compiled at run time with D3DCompile.  Their sources are not compiled into the
project, but stored in the executable as RCDATA resources (see the Upscaler
shaders section of src/win32/vba.rc); the FidelityFX headers are far too large
for a C++ string literal, and resources keep these files readable and diffable
against upstream.  Nothing has to be shipped next to the executable.

The two SDKs are pipelines of their own (a compute shader with a coefficient
texture, and a reconstruction pass plus a sharpening pass).  xBRZ belongs to the
simpler kind, the single pass pixel shader upscalers, which the table
g_singlePassUpscalers in Direct3D11.cpp describes: such a shader is a row in that
table, a resource here, a UPSCALER_* value and a menu item, and the renderer
hands it the frame texture with the whole frame mapped onto the whole window,
sampled with the point sampler.  Adding one needs no code anywhere else.

NVIDIA Image Scaling (NIS)
--------------------------
NIS_Config.h   vendor file, verbatim from the NVIDIA Image Scaling SDK v1.0.3
               https://github.com/NVIDIAGameWorks/NVIDIAImageScaling
               (NIS/NIS_Config.h).  Provides NISConfig, NVScalerUpdateConfig()
               and the coefficient tables the compute shader reads.
NIS_Main.hlsl  vendor file, verbatim (NIS/NIS_Main.hlsl) -> NIS_SHADER_MAIN.
NIS_Scaler.h   vendor file, verbatim (NIS/NIS_Scaler.h) -> NIS_SHADER_SCALER,
               served to the include of NIS_Main.hlsl.
licence.txt    vendor file, verbatim (licence.txt upstream spelling).

AMD FidelityFX Super Resolution 1.0 (FSR)
-----------------------------------------
ffx_a.h        vendor file, verbatim from
               https://github.com/GPUOpen-Effects/FidelityFX-FSR
               (ffx_a.h, "[A] shader portability 1.20210629") ->
               FSR_SHADER_A.
ffx_fsr1.h     vendor file, verbatim (ffx_fsr1.h) -> FSR_SHADER_FSR1, included
               by FSR_Main.hlsl.
FSR_Main.hlsl  ours.  The two entry points the renderer compiles, "FSR_Easu"
               (the reconstruction pass) and "FSR_Rcas" (the sharpening pass),
               plus the input callbacks FSR asks for.  It defines A_GPU and
               A_HLSL, so it is the 32-bit path of the SDK.
licence-ffx.txt  the copyright notice at the head of both ffx files.

xBRZ
----
XBRZ_Freescale.hlsl  ours, a port of the GLSL shader "xbrz-freescale.glsl" from
                https://github.com/libretro/glsl-shaders (xbrz/shaders) to
                HLSL.  xBRZ is a pixel filter rather than a reconstruction, so
                the renderer leaves the frame texture at the size the emulation
                thread produced it and lets this shader blend it while the frame
                is stretched over the window; unlike the CPU filters in
                src/filters there is no fixed scaling factor and no 32-bit
                colour requirement.  The port carries the licences of the
                original: Hyllian's xBR shader code under the MIT licence, and
                the xBRZ part under the GPL-3 with the MAME linking exception.
                -> XBRZ_SHADER_FREESCALE, and the first row of the single pass
                table in Direct3D11.cpp.

The constants a single pass upscaler is handed (SinglePassConstants in
Direct3D11.cpp), the way its source declares them:

    float4 SourceSize;  // xy: the frame size, zw: 1 / xy (one texel)
    float4 OutputSize;  // xy: the target size, zw: 1 / xy
    float4 Scale;       // xy: OutputSize.xy / SourceSize.xy

Rebuilding: vba.rc pulls the files above in as RCDATA resources, and the build
tracks them as inputs, so editing one is enough - MSBuild then recompiles the
resource script and relinks.
