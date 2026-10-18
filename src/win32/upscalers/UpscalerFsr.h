// UpscalerFsr.h: the FidelityFX Super Resolution 1.0 shader constants.
//
// FsrEasuCon() and FsrRcasCon() in ffx_fsr1.h produce them, and the renderers
// repeat that arithmetic here because those headers are compiled as shader code
// only.  Both backends hand the same bits to the same AMD code, so a frame is
// reconstructed identically whichever one is running.

#ifndef VBA_WIN32_UPSCALER_FSR_H
#define VBA_WIN32_UPSCALER_FSR_H

#include <math.h>

// Two passes: EASU reconstructs the frame, RCAS sharpens it.
#define FSR_PASS_COUNT 2

// The constants block: the four float4s EASU wants, the RCAS one, and the size
// of the pass before RCAS as four integers, see fsrWriteConstants().
#define FSR_CONSTANT_FLOATS (5 * 4 + 4)

// Maps the output pixels onto the whole input frame, the same mapping the
// textured quad would use for a 1:1 upload.
inline void fsrEasuCon(float *con, float inputViewportWidth, float inputViewportHeight,
                       float inputWidth, float inputHeight,
                       float outputWidth, float outputHeight)
{
	float rcpOutputWidth  = 1.0f / outputWidth;
	float rcpOutputHeight = 1.0f / outputHeight;

	float rcpInputWidth  = 1.0f / inputWidth;
	float rcpInputHeight = 1.0f / inputHeight;

	con[0]  = inputViewportWidth * rcpOutputWidth;
	con[1]  = inputViewportHeight * rcpOutputHeight;
	con[2]  = 0.5f * inputViewportWidth * rcpOutputWidth - 0.5f;
	con[3]  = 0.5f * inputViewportHeight * rcpOutputHeight - 0.5f;

	con[4]  = rcpInputWidth;
	con[5]  = rcpInputHeight;
	con[6]  = rcpInputWidth;
	con[7]  = -rcpInputHeight;

	con[8]  = -rcpInputWidth;
	con[9]  = 2.0f * rcpInputHeight;
	con[10] = rcpInputWidth;
	con[11] = 2.0f * rcpInputHeight;

	con[12] = 0.0f;
	con[13] = 4.0f * rcpInputHeight;
	con[14] = 0.0f;
	con[15] = 0.0f;
}

// RCAS is given the number of stops the sharpening is reduced by, so it runs
// the other way round from the percentage the user picks.
inline float fsrRcasCon(int sharpness)
{
	float stops = (100.0f - (float)sharpness) / 50.0f;

	return exp2f(-stops);
}

// Fills the whole block for a frame of inputWidth x inputHeight that is
// reconstructed at outputWidth x outputHeight.  The EASU and RCAS values go in
// as the floats they are and come back out of the shaders as uint4s unchanged,
// since only the bits matter; the size is the size of the pass before RCAS,
// which the RCAS shader reads as integers to clamp its taps with, and is
// therefore written as integers rather than as the bits of floats.
inline void fsrWriteConstants(float *constants, float inputWidth, float inputHeight,
                              float outputWidth, float outputHeight, int sharpness)
{
	ZeroMemory(constants, FSR_CONSTANT_FLOATS * sizeof(float));

	fsrEasuCon(constants, inputWidth, inputHeight, inputWidth, inputHeight,
	           outputWidth, outputHeight);
	constants[16] = fsrRcasCon(sharpness);

	((unsigned int *)constants)[20] = (unsigned int)outputWidth;
	((unsigned int *)constants)[21] = (unsigned int)outputHeight;
}

#endif // VBA_WIN32_UPSCALER_FSR_H
