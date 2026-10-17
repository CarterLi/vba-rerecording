/*
 * hq4x filter (16-bit and 32-bit).
 *
 * Ported from VBA-M.  Uses the shared hq implementation
 * (hq_shared.h + hq_base.h + hq4x_pattern.h) which generates hq3x/hq4x
 * from one source.  Only the HQ4x variants are generated here because the
 * project already ships its own hq3x implementation (hq3x32.cpp).
 *
 * hq_base.h produces hq4x16 (16bpp) / hq4x32 (32bpp).  hq4x() below is the
 * 16-bit entry point matching the project's hqNx naming convention.
 */
#include <cstdint>

#include "hq_shared.h"

#define _16BIT
	#define _HQ4X
	#include "hq_base.h"
	#undef _HQ4X
#undef _16BIT

#define _32BIT
	#define _HQ4X
	#include "hq_base.h"
	#undef _HQ4X
#undef _32BIT

#undef GMASK
#undef RBMASK
#undef GSHIFT1MASK
#undef RBSHIFT1MASK
#undef GSHIFT2MASK
#undef RBSHIFT2MASK
#undef GSHIFT3MASK
#undef RBSHIFT3MASK
#undef GSHIFT4MASK
#undef RBSHIFT4MASK

// 16-bit entry point (VBA filter ABI)
void hq4x(uint8_t *srcPtr, uint32_t srcPitch, uint8_t *deltaPtr,
          uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
	hq4x16(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height);
}
