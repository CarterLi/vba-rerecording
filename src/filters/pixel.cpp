#include <cstdint>

#include "simd_common.h"
#include "../Port.h"

extern unsigned int RGB_LOW_BITS_MASK;

// ============================================================================
// SSE2 Implementations
// ============================================================================

#ifdef USE_SSE2

// Pixelate32: 2x scale with grid effect
// Row 0: [original, darkened]
// Row 1: [darkened, darkened]
// Darkened = color * 0.25 = (color >> 2) with proper masking
static void Pixelate32_SSE2(uint8_t *srcPtr, uint32_t srcPitch,
                             uint8_t *dstPtr, uint32_t dstPitch, int width, int height) {
    const __m128i mask = _mm_set1_epi32(0x00FCFCFC);  // For >> 2 without overflow

    for (int y = 0; y < height; y++) {
        uint32_t *src = reinterpret_cast<uint32_t*>(srcPtr + y * srcPitch);
        uint32_t *dst0 = reinterpret_cast<uint32_t*>(dstPtr + (y * 2) * dstPitch);
        uint32_t *dst1 = reinterpret_cast<uint32_t*>(dstPtr + (y * 2 + 1) * dstPitch);

        int x = 0;
        // Process 4 pixels at a time, output 8 pixels
        for (; x + 4 <= width; x += 4) {
            __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[x]));

            // Calculate darkened = ((color & mask) >> 1 & mask) >> 1 = color * 0.25
            __m128i masked = _mm_and_si128(pixels, mask);
            __m128i dark = _mm_srli_epi32(masked, 2);

            // Row 0: [A dark(A) B dark(B) C dark(C) D dark(D)]
            __m128i row0_lo = _mm_unpacklo_epi32(pixels, dark);  // [A dark(A) B dark(B)]
            __m128i row0_hi = _mm_unpackhi_epi32(pixels, dark);  // [C dark(C) D dark(D)]

            // Row 1: [dark(A) dark(A) dark(B) dark(B) ...]
            __m128i row1_lo = _mm_unpacklo_epi32(dark, dark);
            __m128i row1_hi = _mm_unpackhi_epi32(dark, dark);

            // Store row 0
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst0[x * 2]), row0_lo);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst0[x * 2 + 4]), row0_hi);

            // Store row 1
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst1[x * 2]), row1_lo);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst1[x * 2 + 4]), row1_hi);
        }

        // Scalar tail
        uint32_t colorMask = ~RGB_LOW_BITS_MASK;
        for (; x < width; x++) {
            uint32_t color = src[x];
            uint32_t product = (((color & colorMask) >> 1) & colorMask) >> 1;

            dst0[x * 2] = color;
            dst0[x * 2 + 1] = product;
            dst1[x * 2] = product;
            dst1[x * 2 + 1] = product;
        }
    }
}

// Pixelate (16-bit): 2x scale with grid effect
static void Pixelate_SSE2(uint8_t *srcPtr, uint32_t srcPitch,
                           uint8_t *deltaPtr, uint8_t *dstPtr,
                           uint32_t dstPitch, int width, int height) {
    const __m128i mask = _mm_set1_epi16(static_cast<int16_t>(0xFCFC & ~RGB_LOW_BITS_MASK));

    for (int y = 0; y < height; y++) {
        uint16_t *src = reinterpret_cast<uint16_t*>(srcPtr + y * srcPitch);
        uint16_t *delta = reinterpret_cast<uint16_t*>(deltaPtr + y * srcPitch);
        uint16_t *dst0 = reinterpret_cast<uint16_t*>(dstPtr + (y * 2) * dstPitch);
        uint16_t *dst1 = reinterpret_cast<uint16_t*>(dstPtr + (y * 2 + 1) * dstPitch);

        int x = 0;
        // Process 8 pixels at a time
        for (; x + 8 <= width; x += 8) {
            __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[x]));

            // Update delta buffer
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&delta[x]), pixels);

            // Calculate darkened = color * 0.25
            __m128i masked = _mm_and_si128(pixels, mask);
            __m128i dark = _mm_srli_epi16(masked, 2);

            // Row 0: [A dark(A) B dark(B) ...]
            __m128i row0_lo = _mm_unpacklo_epi16(pixels, dark);
            __m128i row0_hi = _mm_unpackhi_epi16(pixels, dark);

            // Row 1: [dark(A) dark(A) dark(B) dark(B) ...]
            __m128i row1_lo = _mm_unpacklo_epi16(dark, dark);
            __m128i row1_hi = _mm_unpackhi_epi16(dark, dark);

            // Store row 0
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst0[x * 2]), row0_lo);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst0[x * 2 + 8]), row0_hi);

            // Store row 1
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst1[x * 2]), row1_lo);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dst1[x * 2 + 8]), row1_hi);
        }

        // Scalar tail
        for (; x < width; x++) {
            uint16_t color = src[x];
            delta[x] = color;
            uint16_t product = static_cast<uint16_t>(
                (((color & ~RGB_LOW_BITS_MASK) >> 1) & ~RGB_LOW_BITS_MASK) >> 1);

            dst0[x * 2] = color;
            dst0[x * 2 + 1] = product;
            dst1[x * 2] = product;
            dst1[x * 2 + 1] = product;
        }
    }
}

#endif  // USE_SSE2

void Pixelate2x16(uint8_t *srcPtr, uint32_t srcPitch, uint8_t *deltaPtr,
          uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
#ifdef USE_SSE2
  Pixelate_SSE2(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height);
#else
  uint8_t *nextLine, *finish;
  uint32_t colorMask = ~(RGB_LOW_BITS_MASK | (RGB_LOW_BITS_MASK << 16));

  nextLine = dstPtr + dstPitch;

  do {
    uint32_t *bP = (uint32_t *) srcPtr;
    uint32_t *xP = (uint32_t *) deltaPtr;
    uint32_t *dP = (uint32_t *) dstPtr;
    uint32_t *nL = (uint32_t *) nextLine;
    uint32_t currentPixel;
    uint32_t nextPixel;
    uint32_t currentDelta;
    uint32_t nextDelta;

    finish = (uint8_t *) bP + ((width+2) << 1);
    nextPixel = *bP++;
    nextDelta = *xP++;

    do {
      currentPixel = nextPixel;
      currentDelta = nextDelta;
      nextPixel = *bP++;
      nextDelta = *xP++;

      if ((nextPixel != nextDelta) || (currentPixel != currentDelta)) {
        uint32_t colorA, colorB, product;

        *(xP - 2) = currentPixel;
#ifdef WORDS_BIGENDIAN
        colorA = currentPixel >> 16;
        colorB = currentPixel & 0xffff;
#else
        colorA = currentPixel & 0xffff;
        colorB = currentPixel >> 16;
#endif
        product = (((colorA & colorMask) >> 1) & colorMask) >> 1;

#ifdef WORDS_BIGENDIAN
        *(nL) = (product << 16) | (product);
        *(dP) = (colorA << 16) | product;
#else
        *(nL) = product | (product << 16);
        *(dP) = colorA | (product << 16);
#endif

#ifdef WORDS_BIGENDIAN
        colorA = nextPixel >> 16;
#else
        colorA = nextPixel & 0xffff;
#endif
        product = (((colorB & colorMask) >> 1) & colorMask) >> 1;
#ifdef WORDS_BIGENDIAN
        *(nL + 1) = (product << 16) | (product);
        *(dP + 1) = (colorB << 16) | (product);
#else
        *(nL + 1) = (product) | (product << 16);
        *(dP + 1) = (colorB) | (product << 16);
#endif
      }

      dP += 2;
      nL += 2;
    } while ((uint8_t *) bP < finish);

    deltaPtr += srcPitch;
    srcPtr += srcPitch;
    dstPtr += dstPitch << 1;
    nextLine += dstPitch << 1;
  }
  while (--height);
#endif
}

void Pixelate2x32(uint8_t *srcPtr, uint32_t srcPitch, uint8_t * /* deltaPtr */,
                uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
#ifdef USE_SSE2
  Pixelate32_SSE2(srcPtr, srcPitch, dstPtr, dstPitch, width, height);
#else
  uint8_t *nextLine, *finish;
  uint32_t colorMask = ~RGB_LOW_BITS_MASK;

  nextLine = dstPtr + dstPitch;

  do {
    uint32_t *bP = (uint32_t *) srcPtr;
    //    uint32_t *xP = (uint32_t *) deltaPtr;
    uint32_t *dP = (uint32_t *) dstPtr;
    uint32_t *nL = (uint32_t *) nextLine;
    uint32_t currentPixel;
    uint32_t nextPixel;

    finish = (uint8_t *) bP + ((width+1) << 2);
    nextPixel = *bP++;

    do {
      currentPixel = nextPixel;
      nextPixel = *bP++;

      uint32_t colorA, colorB, product;

      colorA = currentPixel;
      colorB = nextPixel;

      product = (((colorA & colorMask) >> 1) & colorMask) >> 1;
      *(nL) = product;
      *(nL+1) = product;
      *(dP) = colorA;
      *(dP+1) = product;

      nextPixel = *bP++;
      colorA = nextPixel;
      product = (((colorB & colorMask) >> 1) & colorMask) >> 1;
      *(nL + 2) = product;
      *(nL + 3) = product;
      *(dP + 2) = colorB;
      *(dP + 3) = product;

      dP += 4;
      nL += 4;
    } while ((uint8_t *) bP < finish);

    srcPtr += srcPitch;
    dstPtr += dstPitch << 1;
    nextLine += dstPitch << 1;
  }
  while (--height);
#endif
}

// generic Pixelate Nx magnification filter (VBA-RR extension: 3x/4x)
template <int magnification, typename ColorType>
void PixelateNx(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
                u8 *dstPtr, u32 dstPitch, int width, int height)
{
	ColorType colorMask = ((ColorType)~RGB_LOW_BITS_MASK >> 2) & ((ColorType)~RGB_LOW_BITS_MASK >> 1);

	srcPitch      = srcPitch / sizeof(ColorType) - width;
	u32 dstNextP  = dstPitch / sizeof(ColorType);
	u32 dstNextL  = (dstNextP - width) * magnification; // skip to the next magnificated 'line'
	dstNextP     -= magnification;

	u32 offset    = (dstPitch + sizeof(ColorType)) * magnification - dstPitch;

	ColorType *src   = (ColorType *)srcPtr;
	ColorType *dst   = (ColorType *)dstPtr;

	do // per src line
	{
		u8 *finishP = (u8 *)dst + offset;
		for (int x = 0; x < width; ++x) // per pixel in line
		{
			ColorType col    = *src;
			ColorType *dst2  = dst;
			u8 *finishM = (u8 *)(dst + magnification);

			ColorType product = (col >> 2) & colorMask;
			do
			{
				*dst2 = product;
			} while ((u8 *)++dst2 < finishM);
			dst2    += dstNextP;
			finishM += dstPitch;
			do // dst magnificated pixel
			{
				*dst2++ = product;
				do
				{
					*dst2 = col;
				} while ((u8 *)++dst2 < finishM);
				dst2    += dstNextP;
				finishM += dstPitch;
			} while ((u8 *)dst2 < finishP);

			++src;
			dst     += magnification;
			finishP += magnification * sizeof(ColorType);
		}
		src += srcPitch;
		dst += dstNextL;
	} while (--height);
}

typedef void (*PixelateNxFP)(u8*, u32, u8*, u8*, u32, int, int);

PixelateNxFP Pixelate3x16 = PixelateNx<3, u16>;
PixelateNxFP Pixelate3x32 = PixelateNx<3, u32>;
PixelateNxFP Pixelate4x16 = PixelateNx<4, u16>;
PixelateNxFP Pixelate4x32 = PixelateNx<4, u32>;
