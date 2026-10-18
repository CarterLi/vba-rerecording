/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

/*
 * Alternatively at the previous license terms, you are allowed to use this
 * code in your program with these conditions:
 * - the program is not used in commercial activities.
 * - the whole source code of the program is released with the binary.
 */

#include <cstdint>

#include "simd_common.h"

// ============================================================================
// SSE2 Implementations
// ============================================================================

#ifdef USE_SSE2

// Scale2x algorithm: Edge-detection based scaling
// For each pixel, compare with neighbors and select based on edge patterns
static void internal_scale2x_32_sse2_single(uint32_t* dst,
                                             const uint32_t* src0,  // row above
                                             const uint32_t* src1,  // current row
                                             const uint32_t* src2,  // row below
                                             unsigned count) {
    // First pixel (special case - no left neighbor)
    dst[0] = src1[0];
    if (src1[1] == src0[0] && src2[0] != src0[0])
        dst[1] = src0[0];
    else
        dst[1] = src1[0];

    ++src0; ++src1; ++src2;
    dst += 2;
    count -= 2;

    // Central pixels - vectorized
    unsigned vecCount = count / 4;
    for (unsigned i = 0; i < vecCount; i++) {
        // Load current 4 pixels
        __m128i curr = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1));

        // Load left-shifted (prev pixels): [src1[-1], src1[0], src1[1], src1[2]]
        __m128i prev_part = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 - 1));
        __m128i prev = prev_part;

        // Load right-shifted (next pixels): [src1[1], src1[2], src1[3], src1[4]]
        __m128i next = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 + 1));

        // Load top and bottom
        __m128i top = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src0));
        __m128i bot = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2));

        // Condition: top != bottom && left != right
        __m128i top_ne_bot = _mm_andnot_si128(_mm_cmpeq_epi32(top, bot),
                                               _mm_set1_epi32(-1));
        __m128i prev_ne_next = _mm_andnot_si128(_mm_cmpeq_epi32(prev, next),
                                                 _mm_set1_epi32(-1));
        __m128i condition = _mm_and_si128(top_ne_bot, prev_ne_next);

        // Left output: if (left == top) ? top : current
        __m128i prev_eq_top = _mm_cmpeq_epi32(prev, top);
        __m128i left_select = _mm_and_si128(condition, prev_eq_top);
        __m128i left_out = _mm_or_si128(_mm_and_si128(left_select, top),
                                         _mm_andnot_si128(left_select, curr));

        // Right output: if (right == top) ? top : current
        __m128i next_eq_top = _mm_cmpeq_epi32(next, top);
        __m128i right_select = _mm_and_si128(condition, next_eq_top);
        __m128i right_out = _mm_or_si128(_mm_and_si128(right_select, top),
                                          _mm_andnot_si128(right_select, curr));

        // Interleave left and right outputs
        __m128i out_lo = _mm_unpacklo_epi32(left_out, right_out);
        __m128i out_hi = _mm_unpackhi_epi32(left_out, right_out);

        // Store 8 output pixels
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), out_lo);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 4), out_hi);

        src0 += 4; src1 += 4; src2 += 4;
        dst += 8;
    }

    // Remaining central pixels (scalar)
    count -= vecCount * 4;
    while (count > 0) {
        if (src0[0] != src2[0] && src1[-1] != src1[1]) {
            dst[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
            dst[1] = src1[1] == src0[0] ? src0[0] : src1[0];
        } else {
            dst[0] = src1[0];
            dst[1] = src1[0];
        }
        ++src0; ++src1; ++src2;
        dst += 2;
        --count;
    }

    // Last pixel (special case - no right neighbor)
    if (src1[-1] == src0[0] && src2[0] != src0[0])
        dst[0] = src0[0];
    else
        dst[0] = src1[0];
    dst[1] = src1[0];
}

static void internal_scale2x_32_sse2(uint32_t* dst0, uint32_t* dst1,
                                      const uint32_t* src0,
                                      const uint32_t* src1,
                                      const uint32_t* src2,
                                      unsigned count) {
    internal_scale2x_32_sse2_single(dst0, src0, src1, src2, count);
    internal_scale2x_32_sse2_single(dst1, src2, src1, src0, count);
}

// Same as above for 16 bpp: eight pixels per iteration, tail handled scalar.
static void internal_scale2x_16_sse2_single(uint16_t* dst,
                                            const uint16_t* src0,  // row above
                                            const uint16_t* src1,  // current row
                                            const uint16_t* src2,  // row below
                                            unsigned count) {
    // First pixel (special case - no left neighbor)
    dst[0] = src1[0];
    if (src1[1] == src0[0] && src2[0] != src0[0])
        dst[1] = src0[0];
    else
        dst[1] = src1[0];

    ++src0; ++src1; ++src2;
    dst += 2;
    count -= 2;

    const __m128i all = _mm_set1_epi16(static_cast<short>(-1));

    // Central pixels - vectorized
    unsigned vecCount = count / 8;
    for (unsigned i = 0; i < vecCount; i++) {
        const __m128i curr = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1));
        const __m128i prev = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 - 1));
        const __m128i next = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 + 1));
        const __m128i top = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src0));
        const __m128i bot = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2));

        // Condition: top != bottom && left != right
        const __m128i condition = _mm_and_si128(
            _mm_xor_si128(_mm_cmpeq_epi16(top, bot), all),
            _mm_xor_si128(_mm_cmpeq_epi16(prev, next), all));

        // Left output: if (left == top) ? top : current
        const __m128i left_select = _mm_and_si128(condition, _mm_cmpeq_epi16(prev, top));
        const __m128i left_out = _mm_or_si128(_mm_and_si128(left_select, top),
                                              _mm_andnot_si128(left_select, curr));

        // Right output: if (right == top) ? top : current
        const __m128i right_select = _mm_and_si128(condition, _mm_cmpeq_epi16(next, top));
        const __m128i right_out = _mm_or_si128(_mm_and_si128(right_select, top),
                                               _mm_andnot_si128(right_select, curr));

        // Interleave left and right outputs: 16 output pixels
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst),
                         _mm_unpacklo_epi16(left_out, right_out));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + 8),
                         _mm_unpackhi_epi16(left_out, right_out));

        src0 += 8; src1 += 8; src2 += 8;
        dst += 16;
    }

    // Remaining central pixels (scalar)
    count -= vecCount * 8;
    while (count > 0) {
        if (src0[0] != src2[0] && src1[-1] != src1[1]) {
            dst[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
            dst[1] = src1[1] == src0[0] ? src0[0] : src1[0];
        } else {
            dst[0] = src1[0];
            dst[1] = src1[0];
        }
        ++src0; ++src1; ++src2;
        dst += 2;
        --count;
    }

    // Last pixel (special case - no right neighbor)
    if (src1[-1] == src0[0] && src2[0] != src0[0])
        dst[0] = src0[0];
    else
        dst[0] = src1[0];
    dst[1] = src1[0];
}

static void internal_scale2x_16_sse2(uint16_t* dst0, uint16_t* dst1,
                                     const uint16_t* src0,
                                     const uint16_t* src1,
                                     const uint16_t* src2,
                                     unsigned count) {
    internal_scale2x_16_sse2_single(dst0, src0, src1, src2, count);
    internal_scale2x_16_sse2_single(dst1, src2, src1, src0, count);
}

#endif  // USE_SSE2

#if !defined(USE_SSE2)
static void internal_scale2x_16_def(uint16_t *dst, const uint16_t* src0, const uint16_t* src1, const uint16_t* src2, unsigned count) {
  /* first pixel */
  dst[0] = src1[0];
  if (src1[1] == src0[0] && src2[0] != src0[0])
    dst[1] = src0[0];
  else
    dst[1] = src1[0];
  ++src0;
  ++src1;
  ++src2;
  dst += 2;

  /* central pixels */
  count -= 2;
  while (count) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
      dst[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
      dst[1] = src1[1] == src0[0] ? src0[0] : src1[0];
    } else {
      dst[0] = src1[0];
      dst[1] = src1[0];
    }

    ++src0;
    ++src1;
    ++src2;
    dst += 2;
    --count;
  }

  /* last pixel */
  if (src1[-1] == src0[0] && src2[0] != src0[0])
    dst[0] = src0[0];
  else
    dst[0] = src1[0];
  dst[1] = src1[0];
}

static void internal_scale2x_32_def(uint32_t* dst,
                                    const uint32_t* src0,
                                    const uint32_t* src1,
                                    const uint32_t* src2,
                                    unsigned count)
{
  /* first pixel */
  dst[0] = src1[0];
  if (src1[1] == src0[0] && src2[0] != src0[0])
    dst[1] = src0[0];
  else
    dst[1] = src1[0];
  ++src0;
  ++src1;
  ++src2;
  dst += 2;

  /* central pixels */
  count -= 2;
  while (count) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
      dst[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
      dst[1] = src1[1] == src0[0] ? src0[0] : src1[0];
    } else {
      dst[0] = src1[0];
      dst[1] = src1[0];
    }

    ++src0;
    ++src1;
    ++src2;
    dst += 2;
    --count;
  }

  /* last pixel */
  if (src1[-1] == src0[0] && src2[0] != src0[0])
    dst[0] = src0[0];
  else
    dst[0] = src1[0];
  dst[1] = src1[0];
}
#endif  // !USE_SSE2

void AdMame2x(uint8_t *srcPtr, uint32_t srcPitch, uint8_t * /* deltaPtr */,
              uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
  uint16_t *dst0 = (uint16_t *)dstPtr;
  uint16_t *dst1 = dst0 + (dstPitch >> 1);

  uint16_t *src0 = (uint16_t *)srcPtr;
  uint16_t *src1 = src0 + (srcPitch >> 1);
  uint16_t *src2 = src1 + (srcPitch >> 1);

#ifdef USE_SSE2
  // SSE2 path (64-bit builds and SSE2-capable 32-bit)
  internal_scale2x_16_sse2(dst0, dst1, src0, src0, src1, width);

  int count = height;
  count -= 2;
  while(count) {
    dst0 += dstPitch;     // two output rows
    dst1 += dstPitch;
    internal_scale2x_16_sse2(dst0, dst1, src0, src1, src2, width);
    src0 = src1;
    src1 = src2;
    src2 += srcPitch >> 1;
    --count;
  }
  dst0 += dstPitch;
  dst1 += dstPitch;
  internal_scale2x_16_sse2(dst0, dst1, src0, src1, src1, width);

#else
  {
    internal_scale2x_16_def(dst0, src0, src0, src1, width);
    internal_scale2x_16_def(dst1, src1, src0, src0, width);

    int count = height;

    count -= 2;
    while(count) {
      dst0 += dstPitch;
      dst1 += dstPitch;
      internal_scale2x_16_def(dst0, src0, src1, src2, width);
      internal_scale2x_16_def(dst1, src2, src1, src0, width);
      src0 = src1;
      src1 = src2;
      src2 += srcPitch >> 1;
      --count;
    }
    dst0 += dstPitch;
    dst1 += dstPitch;
    internal_scale2x_16_def(dst0, src0, src1, src1, width);
    internal_scale2x_16_def(dst1, src1, src1, src0, width);
  }
#endif
}

void AdMame2x32(uint8_t *srcPtr, uint32_t srcPitch, uint8_t * /* deltaPtr */,
                uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
  uint32_t *dst0 = (uint32_t *)dstPtr;
  uint32_t *dst1 = dst0 + (dstPitch >> 2);

  uint32_t *src0 = (uint32_t *)srcPtr;
  uint32_t *src1 = src0 + (srcPitch >> 2);
  uint32_t *src2 = src1 + (srcPitch >> 2);

#ifdef USE_SSE2
  // SSE2 path (64-bit builds and SSE2-capable 32-bit)
  internal_scale2x_32_sse2(dst0, dst1, src0, src0, src1, width);

  int count = height;
  count -= 2;
  while(count) {
    dst0 += dstPitch >> 1;
    dst1 += dstPitch >> 1;
    internal_scale2x_32_sse2(dst0, dst1, src0, src1, src2, width);
    src0 = src1;
    src1 = src2;
    src2 += srcPitch >> 2;
    --count;
  }
  dst0 += dstPitch >> 1;
  dst1 += dstPitch >> 1;
  internal_scale2x_32_sse2(dst0, dst1, src0, src1, src1, width);

#else
  // Scalar fallback
  internal_scale2x_32_def(dst0, src0, src0, src1, width);
  internal_scale2x_32_def(dst1, src1, src0, src0, width);

  int count = height;

  count -= 2;
  while(count) {
    dst0 += dstPitch >> 1;
    dst1 += dstPitch >> 1;
    internal_scale2x_32_def(dst0, src0, src1, src2, width);
    internal_scale2x_32_def(dst1, src2, src1, src0, width);
    src0 = src1;
    src1 = src2;
    src2 += srcPitch >> 2;
    --count;
  }
  dst0 += dstPitch >> 1;
  dst1 += dstPitch >> 1;
  internal_scale2x_32_def(dst0, src0, src1, src1, width);
  internal_scale2x_32_def(dst1, src1, src1, src0, width);
#endif
}
