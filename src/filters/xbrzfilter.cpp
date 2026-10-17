/*
 * xBRZ filter wrappers.
 *
 * Bridges the VBA filter ABI
 *     void f(u8* src, u32 srcPitch, u8* delta, u8* dst, u32 dstPitch, int w, int h)
 * to the upstream xBRZ scaler (see xbrz.cpp / xbrz.h).
 *
 * Only the 2x-5x factors are exposed because the internal frame buffer is
 * sized for up to 5x scaling (4 MB @ 32-bit color depth, 240x160 -> 1200x800).
 * 32-bit color depth only.
 */
#include <cstdint>

#include "../Port.h"
#include "xbrz.h"

void xbrz2x32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	xbrz::scale(2, (const uint32_t *)srcPtr, (uint32_t *)dstPtr, width, height, xbrz::ColorFormat::rgb, srcPitch, dstPitch);
}

void xbrz3x32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	xbrz::scale(3, (const uint32_t *)srcPtr, (uint32_t *)dstPtr, width, height, xbrz::ColorFormat::rgb, srcPitch, dstPitch);
}

void xbrz4x32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	xbrz::scale(4, (const uint32_t *)srcPtr, (uint32_t *)dstPtr, width, height, xbrz::ColorFormat::rgb, srcPitch, dstPitch);
}

void xbrz5x32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	xbrz::scale(5, (const uint32_t *)srcPtr, (uint32_t *)dstPtr, width, height, xbrz::ColorFormat::rgb, srcPitch, dstPitch);
}
