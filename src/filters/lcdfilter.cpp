/*
 * LCD color-correction filters for the GBA (AGB) and GB/GBC (CGB) panels.
 *
 * Ported from VBA-M (src/components/filters_agb and src/components/filters_cgb),
 * which adapt the "Color Mangler" shaders by Pokefan531 / hunterk (public
 * domain).  The original shaders run per pixel on the GPU; here the same color
 * transform is baked into the palette translation tables instead, so every
 * renderer (GDI, DirectDraw, Direct3D, OpenGL, ...) picks it up for free.
 */

#include "lcdfilter.h"

#include <math.h>

#include "../common/System.h"

extern u16 systemColorMap16[0x10000];
extern u32 systemColorMap32[0x10000];

extern int systemColorDepth;
extern int systemRedShift;
extern int systemGreenShift;
extern int systemBlueShift;

namespace {

struct LcdProfile {
	float m[4][4];       // Column 0-2 = R/G/B, column 3 = black lift + luminance
	float in_gamma[3];   // Per-channel input gamma
	float out_gamma;     // Output gamma; the pipeline raises to 1/out_gamma
	float slider_scale;  // Darken (AGB) / lighten (CGB) multiplier, 0 = no control
};

struct LcdKernel {
	float gamma[3];   // Input gamma with the slider value folded in
	float mat[3][3];  // mat[out][in], transposed from LcdProfile
	float off[3];     // Black lift, scaled by the alpha channel
	float lum;
	float out_recip;  // 1 / out_gamma
};

void resolveKernel(const LcdProfile &p, float slider, LcdKernel &k)
{
	k.lum = p.m[3][3];
	k.out_recip = 1.0f / p.out_gamma;

	for (int row = 0; row < 3; row++)
	{
		k.gamma[row] = p.in_gamma[row] + slider * p.slider_scale;

		// The shader lift column is scaled by the clamped alpha channel, which
		// is lum for an opaque framebuffer.
		k.off[row] = p.m[3][row] * k.lum;

		for (int col = 0; col < 3; col++)
			k.mat[row][col] = p.m[col][row];
	}
}

void applyKernel(const LcdKernel &k, float &r, float &g, float &b)
{
	// 1. Apply initial gamma (including the slider) to convert to linear space.
	r = powf(r, k.gamma[0]);
	g = powf(g, k.gamma[1]);
	b = powf(b, k.gamma[2]);

	// 2. Apply luminance factor and clamp.
	r = fmaxf(0.0f, fminf(1.0f, r * k.lum));
	g = fmaxf(0.0f, fminf(1.0f, g * k.lum));
	b = fmaxf(0.0f, fminf(1.0f, b * k.lum));

	// 3. Apply the color profile matrix.
	const float tr = k.mat[0][0] * r + k.mat[0][1] * g + k.mat[0][2] * b + k.off[0];
	const float tg = k.mat[1][0] * r + k.mat[1][1] * g + k.mat[1][2] * b + k.off[1];
	const float tb = k.mat[2][0] * r + k.mat[2][1] * g + k.mat[2][2] * b + k.off[2];

	// 4. Apply the display gamma. Mirror through the origin; the matrices carry
	// negative cross terms and powf() rejects a negative base.
	r = copysignf(powf(fabsf(tr), k.out_recip), tr);
	g = copysignf(powf(fabsf(tg), k.out_recip), tg);
	b = copysignf(powf(fabsf(tb), k.out_recip), tb);

	// Final clamp.
	r = fmaxf(0.0f, fminf(1.0f, r));
	g = fmaxf(0.0f, fminf(1.0f, g));
	b = fmaxf(0.0f, fminf(1.0f, b));
}

// Every AGB-family shader uses a flat 2.2 in and out.
#define AGB_GAMMA 2.2f, 2.2f, 2.2f
#define AGB_OUT   2.2f

// One profile per (panel variant, color mode), taken from the matching shader.
// Matrices use the column-major order of GLSL mat4.
const LcdProfile kAgbProfiles[kLcdGbaVariantCount][3] = {
	// gba-color. darken_screen is applied 1:1.
	{
		{{{0.905f, 0.10f,   0.1575f, 0.0f},
		  {0.195f, 0.65f,   0.1425f, 0.0f},
		  {-0.10f, 0.25f,   0.70f,   0.0f},
		  {0.0f,   0.0f,    0.0f,    0.91f}}, {AGB_GAMMA}, AGB_OUT, 1.0f},
		{{{0.76f,  0.125f,  0.16f,   0.0f},
		  {0.27f,  0.6375f, 0.18f,   0.0f},
		  {-0.03f, 0.2375f, 0.66f,   0.0f},
		  {0.0f,   0.0f,    0.0f,    0.97f}}, {AGB_GAMMA}, AGB_OUT, 1.0f},
		{{{0.61f,  0.155f,  0.16f,   0.0f},
		  {0.345f, 0.615f,  0.1875f, 0.0f},
		  {0.045f, 0.23f,   0.6525f, 0.0f},
		  {0.0f,   0.0f,    0.0f,    1.0f}},  {AGB_GAMMA}, AGB_OUT, 1.0f},
	},
	// sp101-color
	{
		{{{0.96f,   0.0325f, 0.001f,  0.0f},
		  {0.11f,   0.89f,   -0.03f,  0.0f},
		  {-0.07f,  0.0775f, 1.029f,  0.0f},
		  {0.0f,    0.0f,    0.0f,    0.935f}}, {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.805f,  0.0675f, 0.017f,  0.0f},
		  {0.24f,   0.86f,   0.02f,   0.0f},
		  {-0.045f, 0.0725f, 0.963f,  0.0f},
		  {0.0f,    0.0f,    0.0f,    0.955f}}, {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.625f,  0.10f,   0.015f,  0.0f},
		  {0.35f,   0.82f,   0.0325f, 0.0f},
		  {0.025f,  0.08f,   0.9525f, 0.0f},
		  {0.0f,    0.0f,    0.0f,    1.0f}},  {AGB_GAMMA}, AGB_OUT, 0.0f},
	},
	// gbMicro-color
	{
		{{{0.8025f,  0.10f,   0.1225f, 0.0f},
		  {0.31f,    0.6875f, 0.1125f, 0.0f},
		  {-0.1125f, 0.2125f, 0.765f,  0.0f},
		  {0.0f,     0.0f,    0.0f,    0.9f}},  {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.6675f,  0.125f,  0.13f,   0.0f},
		  {0.3825f,  0.675f,  0.1475f, 0.0f},
		  {-0.05f,   0.20f,   0.7225f, 0.0f},
		  {0.0f,     0.0f,    0.0f,    0.96f}}, {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.525f,   0.15f,   0.13f,   0.0f},
		  {0.43f,    0.65f,   0.155f,  0.0f},
		  {0.045f,   0.20f,   0.715f,  0.0f},
		  {0.0f,     0.0f,    0.0f,    1.0f}},  {AGB_GAMMA}, AGB_OUT, 0.0f},
	},
	// nds-color
	{
		{{{0.835f,  0.10f,    0.105f,  0.0f},
		  {0.27f,   0.6375f,  0.175f,  0.0f},
		  {-0.105f, 0.2625f,  0.72f,   0.0f},
		  {0.0f,    0.0f,     0.0f,    0.905f}}, {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.70f,   0.125f,   0.12f,   0.0f},
		  {0.34f,   0.625f,   0.20f,   0.0f},
		  {-0.04f,  0.25f,    0.68f,   0.0f},
		  {0.0f,    0.0f,     0.0f,    0.96f}},  {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.555f,  0.1475f,  0.1175f, 0.0f},
		  {0.39f,   0.6075f,  0.2075f, 0.0f},
		  {0.055f,  0.245f,   0.675f,  0.0f},
		  {0.0f,    0.0f,     0.0f,    1.0f}},   {AGB_GAMMA}, AGB_OUT, 0.0f},
	},
	// dslite-color
	{
		{{{0.93f,   0.025f,  0.008f,  0.0f},
		  {0.14f,   0.90f,   -0.03f,  0.0f},
		  {-0.07f,  0.075f,  1.022f,  0.0f},
		  {0.0f,    0.0f,    0.0f,    0.935f}}, {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.76f,   0.055f,  0.0225f, 0.0f},
		  {0.27f,   0.875f,  0.0225f, 0.0f},
		  {-0.03f,  0.07f,   0.955f,  0.0f},
		  {0.0f,    0.0f,    0.0f,    0.97f}},  {AGB_GAMMA}, AGB_OUT, 0.0f},
		{{{0.585f,  0.09f,   0.0225f, 0.0f},
		  {0.3725f, 0.825f,  0.035f,  0.0f},
		  {0.0425f, 0.085f,  0.9425f, 0.0f},
		  {0.0f,    0.0f,    0.0f,    1.0f}},   {AGB_GAMMA}, AGB_OUT, 0.0f},
	},
	// NSO-gba-color (darken applied 1:1; shader default is 0.8)
	{
		{{{0.865f,  0.0575f, 0.0575f, 0.0f},
		  {0.1225f, 0.925f,  0.1225f, 0.0f},
		  {0.0125f, 0.0125f, 0.82f,   0.0f},
		  {0.0f,    0.0f,    0.0f,    1.0f}}, {AGB_GAMMA}, AGB_OUT, 1.0f},
		{{{0.72f,   0.0875f, 0.0725f, 0.0f},
		  {0.2675f, 0.9f,    0.185f,  0.0f},
		  {0.0125f, 0.0125f, 0.7425f, 0.0f},
		  {0.0f,    0.0f,    0.0f,    1.0f}}, {AGB_GAMMA}, AGB_OUT, 1.0f},
		{{{0.57f,   0.115f,  0.0725f, 0.0f},
		  {0.3825f, 0.8625f, 0.195f,  0.0f},
		  {0.0475f, 0.0225f, 0.7325f, 0.0f},
		  {0.0f,    0.0f,    0.0f,    1.0f}}, {AGB_GAMMA}, AGB_OUT, 1.0f},
	},
};

#undef AGB_GAMMA
#undef AGB_OUT

const LcdProfile kCgbProfiles[kLcdGbcVariantCount][3] = {
	// gbc-color. Same matrices as gba-color; lighten_screen offsets the gamma.
	{
		{{{0.905f, 0.10f,   0.1575f, 0.0f},
		  {0.195f, 0.65f,   0.1425f, 0.0f},
		  {-0.10f, 0.25f,   0.70f,   0.0f},
		  {0.0f,   0.0f,    0.0f,    0.91f}}, {2.2f, 2.2f, 2.2f}, 2.2f, -1.0f},
		{{{0.76f,  0.125f,  0.16f,   0.0f},
		  {0.27f,  0.6375f, 0.18f,   0.0f},
		  {-0.03f, 0.2375f, 0.66f,   0.0f},
		  {0.0f,   0.0f,    0.0f,    0.97f}}, {2.2f, 2.2f, 2.2f}, 2.2f, -1.0f},
		{{{0.61f,  0.155f,  0.16f,   0.0f},
		  {0.345f, 0.615f,  0.1875f, 0.0f},
		  {0.045f, 0.23f,   0.6525f, 0.0f},
		  {0.0f,   0.0f,    0.0f,    1.0f}},  {2.2f, 2.2f, 2.2f}, 2.2f, -1.0f},
	},
	// NSO-gbc-color. Per-channel input gamma, no inverse display gamma, and a
	// non-zero lift column; all three modes share one matrix and differ only in
	// luminance. Upstream notes this is an approximation of a LUT filter.
	{
		{{{0.84f,  0.105f, 0.15f,  0.0f},
		  {0.265f, 0.67f,  0.30f,  0.0f},
		  {0.0f,   0.24f,  0.525f, 0.0f},
		  {0.175f, 0.18f,  0.18f,  0.85f}}, {1.24f, 0.8f, 0.7f}, 1.0f, 0.0f},
		{{{0.84f,  0.105f, 0.15f,  0.0f},
		  {0.265f, 0.67f,  0.30f,  0.0f},
		  {0.0f,   0.24f,  0.525f, 0.0f},
		  {0.175f, 0.18f,  0.18f,  1.0f}},  {1.24f, 0.8f, 0.7f}, 1.0f, 0.0f},
		{{{0.84f,  0.105f, 0.15f,  0.0f},
		  {0.265f, 0.67f,  0.30f,  0.0f},
		  {0.0f,   0.24f,  0.525f, 0.0f},
		  {0.175f, 0.18f,  0.18f,  1.0f}},  {1.24f, 0.8f, 0.7f}, 1.0f, 0.0f},
	},
};

int clampMode(int colorMode)
{
	return (colorMode >= 0 && colorMode < 3) ? colorMode : 0;
}

// The NSO GBA shader exposes a darken slider that defaults to 0.8; every other
// AGB profile has darken_scale == 0, so the value is irrelevant for them.
float gbaSliderValue(int variant)
{
	return variant == kLcdGbaNso ? 0.8f : 0.0f;
}

// The CGB profiles use the slider at its upstream default of 0.0.
float gbcSliderValue(int /*variant*/)
{
	return 0.0f;
}

// Rebuild systemColorMap16/32 from the BGR555 source index without any
// correction.  Mirrors the loop the renderers used to run inline.
void buildPlainMaps()
{
	if (systemColorDepth == 16)
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap16[i] = (u16)(((i & 0x1f) << systemRedShift) |
			                            (((i & 0x3e0) >> 5) << systemGreenShift) |
			                            (((i & 0x7c00) >> 10) << systemBlueShift));
		}
	}
	else
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap32[i] = (u32)(((i & 0x1f) << systemRedShift) |
			                            (((i & 0x3e0) >> 5) << systemGreenShift) |
			                            (((i & 0x7c00) >> 10) << systemBlueShift));
		}
	}
}

// Rebuild the tables with the LCD correction applied.  The GBA/GB palette
// source is BGR555, which is the same layout the tables are indexed by.
void buildFilteredMaps(const LcdKernel &k)
{
	const bool depth16 = (systemColorDepth == 16);

	for (int i = 0; i < 0x10000; i++)
	{
		float r = (float)(i & 0x1f) / 31.0f;
		float g = (float)((i >> 5) & 0x1f) / 31.0f;
		float b = (float)((i >> 10) & 0x1f) / 31.0f;

		applyKernel(k, r, g, b);

		unsigned int r5 = (unsigned int)(r * 31.0f + 0.5f);
		unsigned int g5 = (unsigned int)(g * 31.0f + 0.5f);
		unsigned int b5 = (unsigned int)(b * 31.0f + 0.5f);
		if (r5 > 31) r5 = 31;
		if (g5 > 31) g5 = 31;
		if (b5 > 31) b5 = 31;

		const u32 packed = (r5 << systemRedShift) |
		                   (g5 << systemGreenShift) |
		                   (b5 << systemBlueShift);

		if (depth16)
			systemColorMap16[i] = (u16)packed;
		else
			systemColorMap32[i] = packed;
	}
}

} // namespace

void lcdFilterUpdateSystemColorMaps(int gbaVariant, int gbcVariant,
                                    int colorMode, bool isGba)
{
	const int mode = clampMode(colorMode);

	if (isGba)
	{
		if (gbaVariant < 0 || gbaVariant >= kLcdGbaVariantCount)
		{
			buildPlainMaps();
			return;
		}

		LcdKernel k;
		resolveKernel(kAgbProfiles[gbaVariant][mode], gbaSliderValue(gbaVariant), k);
		buildFilteredMaps(k);
	}
	else
	{
		if (gbcVariant < 0 || gbcVariant >= kLcdGbcVariantCount)
		{
			buildPlainMaps();
			return;
		}

		LcdKernel k;
		resolveKernel(kCgbProfiles[gbcVariant][mode], gbcSliderValue(gbcVariant), k);
		buildFilteredMaps(k);
	}
}
