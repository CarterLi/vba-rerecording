/*
 * Shared interpolation helpers for the HQ3x/HQ3xS filters.
 *
 * These used to be hand written MMX (and plain x86) inline assembly, which
 * pinned the whole filter set to 32-bit builds.  The arithmetic below mirrors
 * the old instruction sequences exactly - lane widths, shift kinds and the
 * saturating pack included - so the filter output is byte for byte what the
 * assembly produced.
 */

#include <cstdint>
#include <cstring>

#include "../Port.h"
#include "hq_shared32.h"
#include "interp.h"

namespace {

inline void StorePixel(unsigned char *pc, uint32_t value)
{
	memcpy(pc, &value, sizeof(value));
}

// packuswb: clamp a 16-bit lane into an unsigned byte.
inline unsigned char PackUnsignedByte(uint32_t value)
{
	return static_cast<unsigned char>(value > 0xFFu ? 0xFFu : value);
}

}  // namespace

// (c1 * 3 + c2) / 4, computed in 32-bit lanes like pslld/psubd/paddd/psrld.
void Interp1(unsigned char *pc, unsigned int c1, unsigned int c2)
{
	StorePixel(pc, (c1 * 3u + c2) >> 2);
}

// (c1 * 2 + c2 + c3) / 4, arithmetic shift like psrad.
void Interp2(unsigned char *pc, unsigned int c1, unsigned int c2, unsigned int c3)
{
	const uint32_t sum = c1 * 2u + c2 + c3;
	StorePixel(pc, static_cast<uint32_t>(static_cast<int32_t>(sum) >> 2));
}

// (c1 * 7 + c2) / 8, per byte like punpcklbw/pmullw/psrlw/packuswb.
void Interp3(unsigned char *pc, unsigned int c1, unsigned int c2)
{
	unsigned char out[4];

	for (int i = 0; i < 4; ++i)
	{
		const uint32_t a = (c1 >> (i * 8)) & 0xFFu;
		const uint32_t b = (c2 >> (i * 8)) & 0xFFu;
		out[i] = PackUnsignedByte((a * 7u + b) >> 3);
	}

	memcpy(pc, out, sizeof(out));
}

// (c1 * 2 + (c2 + c3) * 7) / 16, per byte like Interp3.
void Interp4(unsigned char *pc, unsigned int c1, unsigned int c2, unsigned int c3)
{
	unsigned char out[4];

	for (int i = 0; i < 4; ++i)
	{
		const uint32_t a = (c1 >> (i * 8)) & 0xFFu;
		const uint32_t b = (c2 >> (i * 8)) & 0xFFu;
		const uint32_t c = (c3 >> (i * 8)) & 0xFFu;
		out[i] = PackUnsignedByte((a * 2u + (b + c) * 7u) >> 4);
	}

	memcpy(pc, out, sizeof(out));
}

// (c1 + c2) / 2, arithmetic shift like paddd/psrad.
void Interp5(unsigned char *pc, unsigned int c1, unsigned int c2)
{
	const uint32_t sum = c1 + c2;
	StorePixel(pc, static_cast<uint32_t>(static_cast<int32_t>(sum) >> 1));
}

void Interp1_16(unsigned char *pc, unsigned short c1, unsigned short c2)
{
	*((unsigned short *)pc) = interp_16_31(c1, c2);
}

void Interp2_16(unsigned char *pc, unsigned short c1, unsigned short c2, unsigned short c3)
{
	*((unsigned short *)pc) = interp_16_211(c1, c2, c3);
}

void Interp3_16(unsigned char *pc, unsigned short c1, unsigned short c2)
{
	*((unsigned short *)pc) = interp_16_71(c1, c2);
}

void Interp4_16(unsigned char *pc, unsigned short c1, unsigned short c2, unsigned short c3)
{
	*((unsigned short *)pc) = interp_16_772(c2, c3, c1);
}

void Interp5_16(unsigned char *pc, unsigned short c1, unsigned short c2)
{
	*((unsigned short *)pc) = interp_16_11(c1, c2);
}

bool Diff(unsigned int c1, unsigned int c2)
{
	const unsigned int YUV1 = RGBtoYUV(c1);
	const unsigned int YUV2 = RGBtoYUV(c2);

	if (YUV1 == YUV2)
		return false;

	return
	    (abs32((YUV1 & Ymask) - (YUV2 & Ymask)) > trY) ||
	    (abs32((YUV1 & Umask) - (YUV2 & Umask)) > trU) ||
	    (abs32((YUV1 & Vmask) - (YUV2 & Vmask)) > trV);
}

// Mirrors the old paddd/psubd/pslld sequence lane for lane, wraparound
// included.  Note the channel masks are deliberately *not* shifted down: the
// assembly added the masked words straight together, so keeping them in place
// is what makes Diff() behave exactly as it did before.
unsigned int RGBtoYUV(unsigned int c)
{
	const uint32_t r = c & 0x000000FFu;
	const uint32_t g = c & 0x0000FF00u;
	const uint32_t b = c & 0x00FF0000u;

	const uint32_t y = (r + g + b) << 14;
	const uint32_t u = ((r - b) + 512u) << 6;
	const uint32_t v = (((g << 1) - r - b) >> 3) + 128u;

	return y + u + v;
}
