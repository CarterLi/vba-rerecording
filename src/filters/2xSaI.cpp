#include "../common/System.h"
#include "simd_common.h"

extern unsigned int RGB_LOW_BITS_MASK;

static uint32_t redblueMask = 0xF81F;
static uint32_t greenMask = 0x7E0;

uint32_t qRGB_COLOR_MASK[2] = { 0xF7DEF7DE, 0xF7DEF7DE };

// Masks used by the 16bpp SIMD path.  These are the same values that the old
// MMX code stored in Init_2xSaIMMX().
static uint16_t s_colorMask16     = 0xF7DE;
static uint16_t s_lowPixelMask16  = 0x0821;
static uint16_t s_qcolorMask16    = 0xE79C;
static uint16_t s_qlowPixelMask16 = 0x1863;

extern void hq2x_init(unsigned);

int Init_2xSaI(uint32_t BitFormat)
{
  if(systemColorDepth == 16) {
    if (BitFormat == 565) {
      redblueMask = 0xF81F;
      greenMask = 0x7E0;
      qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0xF7DEF7DE;
      hq2x_init(16);
      RGB_LOW_BITS_MASK = 0x0821;
      s_colorMask16     = 0xF7DE;
      s_lowPixelMask16  = 0x0821;
      s_qcolorMask16    = 0xE79C;
      s_qlowPixelMask16 = 0x1863;
    } else if (BitFormat == 555) {
      redblueMask = 0x7C1F;
      greenMask = 0x3E0;
      qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0x7BDE7BDE;
      hq2x_init(15);
      RGB_LOW_BITS_MASK = 0x0421;
      s_colorMask16     = 0x7BDE;
      s_lowPixelMask16  = 0x0421;
      s_qcolorMask16    = 0x739C;
      s_qlowPixelMask16 = 0x0C63;
    } else {
      return 0;
    }
  } else if(systemColorDepth == 32) {
    qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0xfefefe;
    hq2x_init(32);
    RGB_LOW_BITS_MASK = 0x010101;
  } else
    return 0;

  return 1;
}

static inline int GetResult1 (uint32_t A, uint32_t B, uint32_t C, uint32_t D,
                              uint32_t /* E */)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
      x += 1;
    else if (B == C)
      y += 1;
    if (A == D)
      x += 1;
    else if (B == D)
      y += 1;
    if (x <= 1)
      r += 1;
    if (y <= 1)
      r -= 1;
    return r;
}

static inline int GetResult2 (uint32_t A, uint32_t B, uint32_t C, uint32_t D,
                              uint32_t /* E */)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r -= 1;
  if (y <= 1)
    r += 1;
  return r;
}

static inline int GetResult (uint32_t A, uint32_t B, uint32_t C, uint32_t D)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

static inline uint32_t INTERPOLATE(uint32_t A, uint32_t B) {
    if (A == B) {
        return A;
    }
    
    // Extract each 8-bit channel by reading the full byte at each color position
    uint32_t r_a = (A >> 16) & 0xFF;
    uint32_t g_a = (A >> 8) & 0xFF;
    uint32_t b_a = A & 0xFF;
    
    uint32_t r_b = (B >> 16) & 0xFF;
    uint32_t g_b = (B >> 8) & 0xFF;
    uint32_t b_b = B & 0xFF;
    
    // Average properly
    uint32_t r_avg = (r_a + r_b) >> 1;
    uint32_t g_avg = (g_a + g_b) >> 1;
    uint32_t b_avg = (b_a + b_b) >> 1;
    
    // Repack
    return (r_avg << 16) | (g_avg << 8) | b_avg;
}

static inline uint32_t Q_INTERPOLATE(uint32_t A, uint32_t B, uint32_t C, uint32_t D) {
    // Extract each 8-bit channel
    uint32_t r_a = (A >> 16) & 0xFF;
    uint32_t g_a = (A >> 8) & 0xFF;
    uint32_t b_a = A & 0xFF;
    
    uint32_t r_b = (B >> 16) & 0xFF;
    uint32_t g_b = (B >> 8) & 0xFF;
    uint32_t b_b = B & 0xFF;
    
    uint32_t r_c = (C >> 16) & 0xFF;
    uint32_t g_c = (C >> 8) & 0xFF;
    uint32_t b_c = C & 0xFF;
    
    uint32_t r_d = (D >> 16) & 0xFF;
    uint32_t g_d = (D >> 8) & 0xFF;
    uint32_t b_d = D & 0xFF;
    
    // Average across 4 pixels
    uint32_t r_avg = (r_a + r_b + r_c + r_d) >> 2;
    uint32_t g_avg = (g_a + g_b + g_c + g_d) >> 2;
    uint32_t b_avg = (b_a + b_b + b_c + b_d) >> 2;
    
    // Repack
    return (r_avg << 16) | (g_avg << 8) | b_avg;
}

// 16 bit variants of the two interpolation helpers.  The generic versions
// above treat a pixel as 8 bit per channel, which is only valid for the 32bpp
// path; 16bpp pixels need the channel masked 2xSaI arithmetic.
static inline uint32_t INTERPOLATE16 (uint32_t A, uint32_t B)
{
  if (A == B)
    return A;
  return ((A & s_colorMask16) >> 1) + ((B & s_colorMask16) >> 1)
       + (A & B & s_lowPixelMask16);
}

static inline uint32_t Q_INTERPOLATE16 (uint32_t A, uint32_t B,
                                        uint32_t C, uint32_t D)
{
  uint32_t x = ((A & s_qcolorMask16) >> 2) + ((B & s_qcolorMask16) >> 2)
             + ((C & s_qcolorMask16) >> 2) + ((D & s_qcolorMask16) >> 2);
  uint32_t y = (A & s_qlowPixelMask16) + (B & s_qlowPixelMask16)
             + (C & s_qlowPixelMask16) + (D & s_qlowPixelMask16);
  return x + ((y >> 2) & s_qlowPixelMask16);
}

static inline int GetResult1_32 (uint32_t A, uint32_t B, uint32_t C, uint32_t D,
                                 uint32_t /* E */)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
      x += 1;
    else if (B == C)
      y += 1;
    if (A == D)
      x += 1;
    else if (B == D)
      y += 1;
    if (x <= 1)
      r += 1;
    if (y <= 1)
      r -= 1;
    return r;
}

static inline int GetResult2_32 (uint32_t A, uint32_t B, uint32_t C, uint32_t D,
                                 uint32_t /* E */)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r -= 1;
  if (y <= 1)
    r += 1;
  return r;
}

#define BLUE_MASK565 0x001F001F
#define RED_MASK565 0xF800F800
#define GREEN_MASK565 0x07E007E0

#define BLUE_MASK555 0x001F001F
#define RED_MASK555 0x7C007C00
#define GREEN_MASK555 0x03E003E0

// Helper functions for safe pixel access with boundary clamping
static inline int ClampX(int x, int width) {
    return (x < 0) ? 0 : (x >= width) ? width - 1 : x;
}

static inline int ClampY(int y, int height) {
    return (y < 0) ? 0 : (y >= height) ? height - 1 : y;
}

// Safe 16-bit pixel access
static inline uint16_t GetPixel16(const uint16_t* src, int srcPitch16, int x, int y, int width, int height) {
    x = ClampX(x, width);
    y = ClampY(y, height);
    return src[y * srcPitch16 + x];
}

// Safe 32-bit pixel access
static inline uint32_t GetPixel32(const uint32_t* src, int srcPitch32, int x, int y, int width, int height) {
    x = ClampX(x, width);
    y = ClampY(y, height);
    return src[y * srcPitch32 + x];
}

// ============================================================================
// 16bpp SSE2 path
//
// Direct translation of the old MMX assembly (2xSaImmx.asm) into SSE2
// intrinsics.  The MMX code processes four pixels per iteration in 64 bit
// registers; this code processes eight pixels per iteration in 128 bit
// registers and four pixels for the trailing block.  Every comparison,
// interpolation, mask and memory access is kept in the same order as the
// assembly, so the produced pixels are byte identical to the old MMX output.
//
// Note: the MMX code (like the original 2xSaI C reference it was derived
// from) reads one pixel left and two pixels right of the block plus two rows
// below the source; the caller guarantees a padded source surface.
// ============================================================================

#if defined(USE_SSE2)

template<int N> struct SaIBlock16;

template<> struct SaIBlock16<8>
{
  static inline __m128i load(const uint16_t* p)
  {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
  }
  // delta buffer: one entry per source pixel
  static inline void storeN(uint16_t* p, __m128i v)
  {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
  }
  // destination: one entry per output pixel
  static inline void storeOut(uint16_t* p, __m128i v)
  {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
  }
  static inline __m128i lo(__m128i a, __m128i b) { return _mm_unpacklo_epi16(a, b); }
  static inline __m128i hi(__m128i a, __m128i b) { return _mm_unpackhi_epi16(a, b); }
  // Write both destination rows, in the same order as the MMX code: all of the
  // first four pixels, then all of the last four pixels.
  static inline void storePair(uint16_t* dst, int dstPitch16,
                               __m128i lo1, __m128i lo2, __m128i hi1, __m128i hi2,
                               int changed)
  {
    if (changed & 1)
    {
      storeOut(dst, lo1);
      storeOut(dst + dstPitch16, lo2);
    }
    if (changed & 2)
    {
      storeOut(dst + 8, hi1);
      storeOut(dst + dstPitch16 + 8, hi2);
    }
  }
};

template<> struct SaIBlock16<4>
{
  static inline __m128i load(const uint16_t* p)
  {
    return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p));
  }
  static inline void storeN(uint16_t* p, __m128i v)
  {
    _mm_storel_epi64(reinterpret_cast<__m128i*>(p), v);
  }
  static inline void storeOut(uint16_t* p, __m128i v)
  {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
  }
  static inline __m128i lo(__m128i a, __m128i b) { return _mm_unpacklo_epi16(a, b); }
  static inline __m128i hi(__m128i, __m128i) { return _mm_setzero_si128(); }
  static inline void storePair(uint16_t* dst, int dstPitch16,
                               __m128i lo1, __m128i lo2, __m128i, __m128i,
                               int changed)
  {
    if (changed & 1)
    {
      storeOut(dst, lo1);
      storeOut(dst + dstPitch16, lo2);
    }
  }
};

static inline __m128i SaINot(__m128i v)
{
  return _mm_xor_si128(v, _mm_set1_epi16(static_cast<short>(0xFFFF)));
}

// true when at least one 16 bit lane is non zero (movemask_epi8 only looks at
// the sign bit of every byte, so it cannot be used directly on a counter).
static inline bool SaIAnyLane(__m128i v)
{
  return _mm_movemask_epi8(_mm_cmpeq_epi16(v, _mm_setzero_si128())) != 0xFFFF;
}

// INTERPOLATE(): ((A & colorMask) >> 1) + ((B & colorMask) >> 1) +
//                ((A & B) & lowPixelMask)
static inline __m128i SaIInterp(__m128i a, __m128i b, __m128i cMask, __m128i lMask)
{
  return _mm_add_epi16(
      _mm_add_epi16(_mm_srli_epi16(_mm_and_si128(a, cMask), 1),
                    _mm_srli_epi16(_mm_and_si128(b, cMask), 1)),
      _mm_and_si128(_mm_and_si128(a, b), lMask));
}

// Q_INTERPOLATE(): (A + B + C + D) / 4 with the 2xSaI rounding scheme
static inline __m128i SaIInterp4(__m128i a, __m128i b, __m128i c, __m128i d,
                                 __m128i qcMask, __m128i qlMask)
{
  __m128i hi = _mm_add_epi16(
      _mm_add_epi16(_mm_srli_epi16(_mm_and_si128(a, qcMask), 2),
                    _mm_srli_epi16(_mm_and_si128(b, qcMask), 2)),
      _mm_add_epi16(_mm_srli_epi16(_mm_and_si128(c, qcMask), 2),
                    _mm_srli_epi16(_mm_and_si128(d, qcMask), 2)));
  __m128i lo = _mm_srli_epi16(
      _mm_add_epi16(_mm_add_epi16(_mm_and_si128(a, qlMask), _mm_and_si128(b, qlMask)),
                    _mm_add_epi16(_mm_and_si128(c, qlMask), _mm_and_si128(d, qlMask))), 2);
  return _mm_add_epi16(hi, _mm_and_si128(lo, qlMask));
}

// One GetResult() pair: two neighbour pixels compared against color5/color6.
static inline __m128i SaIGetResult(__m128i p0, __m128i p1, __m128i c5, __m128i c6,
                                   __m128i sel, __m128i zero)
{
  __m128i x = _mm_add_epi16(_mm_and_si128(_mm_cmpeq_epi16(p0, c5), sel),
                            _mm_and_si128(_mm_cmpeq_epi16(p1, c5), sel));
  __m128i y = _mm_add_epi16(_mm_and_si128(_mm_cmpeq_epi16(p0, c6), sel),
                            _mm_and_si128(_mm_cmpeq_epi16(p1, c6), sel));
  __m128i rx = _mm_and_si128(_mm_cmpeq_epi16(_mm_cmpgt_epi16(x, sel), zero), sel);
  __m128i ry = _mm_and_si128(_mm_cmpeq_epi16(_mm_cmpgt_epi16(y, sel), zero), sel);
  return _mm_sub_epi16(rx, ry);
}

// Delta test of the original code: the four source rows are compared against
// the (one pixel shifted) delta buffer.  Returns a bit mask of the four pixel
// halves that changed (bit 0: pixels 0..3, bit 1: pixels 4..7), mirroring the
// per group skip decisions of the assembly.
template<int N>
static inline int SaIChangedHalves(const uint16_t* B, const uint16_t* A,
                                   const uint16_t* C, const uint16_t* D,
                                   uint16_t* dlt, int srcPitch16)
{
  if (!dlt)
    return 3;

  const uint16_t* d0 = dlt;
  const uint16_t* d1 = dlt + srcPitch16;
  const uint16_t* d2 = dlt + 2 * srcPitch16;
  const uint16_t* d3 = dlt + 3 * srcPitch16;

  __m128i eq = _mm_cmpeq_epi16(SaIBlock16<N>::load(B - 1), SaIBlock16<N>::load(d0));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(B + 2), SaIBlock16<N>::load(d0 + 3)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(A - 1), SaIBlock16<N>::load(d1)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(A + 2), SaIBlock16<N>::load(d1 + 3)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(C - 1), SaIBlock16<N>::load(d2)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(C + 2), SaIBlock16<N>::load(d2 + 3)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(D - 1), SaIBlock16<N>::load(d3)));
  eq = _mm_and_si128(eq, _mm_cmpeq_epi16(SaIBlock16<N>::load(D + 2), SaIBlock16<N>::load(d3 + 3)));

  SaIBlock16<N>::storeN(dlt, SaIBlock16<N>::load(B - 1));

  const int mm = _mm_movemask_epi8(eq);
  int changed = 0;
  if ((mm & 0xFF) != 0xFF)
    changed |= 1;
  if (N == 8 && ((mm >> 8) & 0xFF) != 0xFF)
    changed |= 2;
  return changed;
}

// ---------------------------------------------------------------------------
// GetResult (A, B, C, D) of the reference implementation: A/B are the two
// centre pixels, C/D the neighbour pair.  Deliberately NOT the variant the old
// MMX line filter used (that one tested A/B against C/D the other way round,
// which is one of the reasons the MMX SuperEagle output differed from C).
// ---------------------------------------------------------------------------
static inline __m128i SaIResultPair(__m128i a, __m128i b, __m128i n1, __m128i n2,
                                    __m128i one, __m128i zero)
{
  const __m128i eAC = _mm_cmpeq_epi16(a, n1);
  const __m128i eAD = _mm_cmpeq_epi16(a, n2);
  const __m128i eBC = _mm_andnot_si128(eAC, _mm_cmpeq_epi16(b, n1));
  const __m128i eBD = _mm_andnot_si128(eAD, _mm_cmpeq_epi16(b, n2));
  const __m128i x = _mm_add_epi16(_mm_and_si128(eAC, one), _mm_and_si128(eAD, one));
  const __m128i y = _mm_add_epi16(_mm_and_si128(eBC, one), _mm_and_si128(eBD, one));
  const __m128i px = _mm_and_si128(_mm_cmpeq_epi16(_mm_cmpgt_epi16(x, one), zero), one);
  const __m128i py = _mm_and_si128(_mm_cmpeq_epi16(_mm_cmpgt_epi16(y, one), zero), one);
  return _mm_sub_epi16(px, py);
}

static inline __m128i SaISel(__m128i mask, __m128i whenTrue, __m128i whenFalse)
{
  return _mm_or_si128(_mm_and_si128(mask, whenTrue),
                      _mm_andnot_si128(mask, whenFalse));
}

template<int N>
static inline void Super2xSaI_16_block(uint16_t* dst, int dstPitch16,
                                       const uint16_t* B, const uint16_t* A,
                                       const uint16_t* C, const uint16_t* D,
                                       uint16_t* dlt, int srcPitch16)
{
  const int changed = SaIChangedHalves<N>(B, A, C, D, dlt, srcPitch16);
  if (!changed)
    return;

  const __m128i zero = _mm_setzero_si128();
  const __m128i cMask = _mm_set1_epi16(static_cast<short>(s_colorMask16));
  const __m128i lMask = _mm_set1_epi16(static_cast<short>(s_lowPixelMask16));

  const __m128i c5 = SaIBlock16<N>::load(A);
  const __m128i c6 = SaIBlock16<N>::load(A + 1);
  const __m128i c2 = SaIBlock16<N>::load(C);
  const __m128i c3 = SaIBlock16<N>::load(C + 1);

  const __m128i i56   = SaIInterp(c5, c6, cMask, lMask);
  const __m128i i5556 = SaIInterp(i56, c5, cMask, lMask);
  const __m128i i5666 = SaIInterp(i56, c6, cMask, lMask);
  const __m128i i23   = SaIInterp(c2, c3, cMask, lMask);
  const __m128i i2223 = SaIInterp(i23, c2, cMask, lMask);
  const __m128i i2333 = SaIInterp(i23, c3, cMask, lMask);

  const __m128i eq53 = _mm_cmpeq_epi16(c5, c3);
  const __m128i eq62 = _mm_cmpeq_epi16(c6, c2);
  const __m128i eq56 = _mm_cmpeq_epi16(c5, c6);

  const __m128i both = _mm_and_si128(eq53, eq62);
  __m128i mask26 = _mm_andnot_si128(eq53, eq62);                     // (5!=3) && (6==2)
  __m128i mask35 = _mm_or_si128(_mm_andnot_si128(eq62, eq53),        // (5==3) && (6!=2)
                                _mm_and_si128(eq56, both));          // plus 5==6 and both
  const __m128i ambiguous = _mm_andnot_si128(eq56, both);

  if (_mm_movemask_epi8(ambiguous) != 0)
  {
    const __m128i sel = _mm_and_si128(ambiguous, _mm_set1_epi16(1));
    __m128i r = zero;
    r = _mm_add_epi16(r, SaIGetResult(SaIBlock16<N>::load(B),     SaIBlock16<N>::load(A - 1), c5, c6, sel, zero)); // E,G
    r = _mm_add_epi16(r, SaIGetResult(SaIBlock16<N>::load(B + 1), SaIBlock16<N>::load(A + 2), c5, c6, sel, zero)); // F,K
    r = _mm_add_epi16(r, SaIGetResult(SaIBlock16<N>::load(C - 1), SaIBlock16<N>::load(D),     c5, c6, sel, zero)); // H,N
    r = _mm_add_epi16(r, SaIGetResult(SaIBlock16<N>::load(C + 2), SaIBlock16<N>::load(D + 1), c5, c6, sel, zero)); // L,O

    if (SaIAnyLane(r))
    {
      mask35 = _mm_or_si128(mask35, _mm_cmpgt_epi16(r, zero));
      mask26 = _mm_or_si128(mask26, _mm_cmpgt_epi16(zero, r));
    }
  }

  const __m128i c1 = SaIBlock16<N>::load(C - 1);
  const __m128i c4 = SaIBlock16<N>::load(A - 1);
  const __m128i b0 = SaIBlock16<N>::load(B - 1);
  const __m128i b2 = SaIBlock16<N>::load(B + 1);
  const __m128i a0 = SaIBlock16<N>::load(D - 1);
  const __m128i a1 = SaIBlock16<N>::load(D);
  const __m128i a2 = SaIBlock16<N>::load(D + 1);
  const __m128i a3 = SaIBlock16<N>::load(D + 2);
  const __m128i b1 = SaIBlock16<N>::load(B);
  const __m128i b3 = SaIBlock16<N>::load(B + 2);

  const __m128i i52 = SaIInterp(c5, c2, cMask, lMask);

  // product1a
  __m128i m1a = _mm_and_si128(mask26,
      _mm_and_si128(_mm_cmpeq_epi16(c1, c2), SaINot(_mm_cmpeq_epi16(b2, c2))));
  m1a = _mm_or_si128(m1a,
      _mm_and_si128(
          _mm_and_si128(_mm_cmpeq_epi16(c3, c2), _mm_cmpeq_epi16(c4, c2)),
          _mm_and_si128(SaINot(_mm_cmpeq_epi16(c5, c1)), SaINot(_mm_cmpeq_epi16(b0, c2)))));
  __m128i f1a = _mm_or_si128(_mm_and_si128(m1a, i52),
                                   _mm_andnot_si128(m1a, c5));

  // product2a
  __m128i m2a = _mm_and_si128(mask35,
      _mm_and_si128(_mm_cmpeq_epi16(c4, c5), SaINot(_mm_cmpeq_epi16(a2, c5))));
  m2a = _mm_or_si128(m2a,
      _mm_and_si128(
          _mm_and_si128(_mm_cmpeq_epi16(c6, c5), _mm_cmpeq_epi16(c1, c5)),
          _mm_and_si128(SaINot(_mm_cmpeq_epi16(c2, c4)), SaINot(_mm_cmpeq_epi16(a0, c5)))));
  __m128i f2a = _mm_or_si128(_mm_and_si128(m2a, i52),
                                   _mm_andnot_si128(m2a, c2));

  // product2b
  __m128i cnd1 = _mm_and_si128(_mm_cmpeq_epi16(c6, c3), _mm_cmpeq_epi16(a1, c3));
  cnd1 = _mm_and_si128(cnd1,
      _mm_and_si128(SaINot(_mm_cmpeq_epi16(c2, a2)), SaINot(_mm_cmpeq_epi16(a0, c3))));
  __m128i cnd2 = _mm_and_si128(_mm_cmpeq_epi16(c5, c2), _mm_cmpeq_epi16(a2, c2));
  cnd2 = _mm_and_si128(cnd2,
      _mm_and_si128(SaINot(_mm_cmpeq_epi16(a1, c3)), SaINot(_mm_cmpeq_epi16(a3, c2))));

  const __m128i anyQ = _mm_or_si128(cnd1, cnd2);
  const __m128i m35q = _mm_andnot_si128(anyQ, mask35);
  const __m128i m26q = _mm_andnot_si128(anyQ, mask26);

  __m128i f2b = _mm_or_si128(_mm_and_si128(cnd1, i2333), _mm_and_si128(cnd2, i2223));
  f2b = _mm_or_si128(f2b, _mm_and_si128(m35q, c3));
  f2b = _mm_or_si128(f2b, _mm_and_si128(m26q, c2));
  f2b = _mm_or_si128(f2b, _mm_andnot_si128(
      _mm_or_si128(_mm_or_si128(cnd1, m35q), _mm_or_si128(m26q, cnd2)), i23));

  // product1b
  __m128i d1 = _mm_and_si128(_mm_cmpeq_epi16(c3, c6), _mm_cmpeq_epi16(b1, c6));
  d1 = _mm_and_si128(d1,
      _mm_and_si128(SaINot(_mm_cmpeq_epi16(c5, b2)), SaINot(_mm_cmpeq_epi16(b0, c6))));
  __m128i d2 = _mm_and_si128(_mm_cmpeq_epi16(c2, c5), _mm_cmpeq_epi16(b2, c5));
  d2 = _mm_and_si128(d2,
      _mm_and_si128(SaINot(_mm_cmpeq_epi16(b1, c6)), SaINot(_mm_cmpeq_epi16(b3, c5))));

  const __m128i anyP = _mm_or_si128(d1, d2);
  const __m128i m35p = _mm_andnot_si128(anyP, mask35);
  const __m128i m26p = _mm_andnot_si128(anyP, mask26);

  __m128i f1b = _mm_or_si128(_mm_and_si128(d1, i5666), _mm_and_si128(d2, i5556));
  f1b = _mm_or_si128(f1b, _mm_and_si128(m35p, c5));
  f1b = _mm_or_si128(f1b, _mm_and_si128(m26p, c6));
  f1b = _mm_or_si128(f1b, _mm_andnot_si128(
      _mm_or_si128(_mm_or_si128(d1, m35p), _mm_or_si128(m26p, d2)), i56));

  SaIBlock16<N>::storePair(dst, dstPitch16,
                           SaIBlock16<N>::lo(f1a, f1b), SaIBlock16<N>::lo(f2a, f2b),
                           SaIBlock16<N>::hi(f1a, f1b), SaIBlock16<N>::hi(f2a, f2b),
                           changed);
}

// SuperEagle, 16bpp, N pixels per block.
//
// Translation of the reference C implementation (the same one SuperEagle32
// uses): all four branches are evaluated for every lane and the result is
// picked by per lane masks, since the branch conditions are mutually
// exclusive.  No delta shortcut: the reference recomputes every pixel, so the
// block writes delta[y][x] = color5 exactly like the C code does.
template<int N>
static inline void SuperEagle_16_block(uint16_t* dst, int dstPitch16,
                                       const uint16_t* B, const uint16_t* A,
                                       const uint16_t* C, const uint16_t* D,
                                       uint16_t* dlt, int /*srcPitch16*/)
{
  const __m128i zero = _mm_setzero_si128();
  const __m128i one = _mm_set1_epi16(1);
  const __m128i cMask = _mm_set1_epi16(static_cast<short>(s_colorMask16));
  const __m128i lMask = _mm_set1_epi16(static_cast<short>(s_lowPixelMask16));
  const __m128i qcMask = _mm_set1_epi16(static_cast<short>(s_qcolorMask16));
  const __m128i qlMask = _mm_set1_epi16(static_cast<short>(s_qlowPixelMask16));

  const __m128i c1 = SaIBlock16<N>::load(C - 1);   // color1
  const __m128i c2 = SaIBlock16<N>::load(C);       // color2
  const __m128i c3 = SaIBlock16<N>::load(C + 1);   // color3
  const __m128i s1 = SaIBlock16<N>::load(C + 2);   // colorS1
  const __m128i c4 = SaIBlock16<N>::load(A - 1);   // color4
  const __m128i c5 = SaIBlock16<N>::load(A);       // color5
  const __m128i c6 = SaIBlock16<N>::load(A + 1);   // color6
  const __m128i s2 = SaIBlock16<N>::load(A + 2);   // colorS2
  const __m128i b1 = SaIBlock16<N>::load(B);       // colorB1
  const __m128i b2 = SaIBlock16<N>::load(B + 1);   // colorB2
  const __m128i a1 = SaIBlock16<N>::load(D);       // colorA1
  const __m128i a2 = SaIBlock16<N>::load(D + 1);   // colorA2

  const __m128i eq53 = _mm_cmpeq_epi16(c5, c3);
  const __m128i eq62 = _mm_cmpeq_epi16(c6, c2);
  // color2 == color6 && color5 != color3
  const __m128i mA = _mm_andnot_si128(eq53, eq62);
  // color5 == color3 && color2 != color6
  const __m128i mB = _mm_andnot_si128(eq62, eq53);
  // color5 == color3 && color2 == color6
  const __m128i mC = _mm_and_si128(eq53, eq62);
  // everything else
  const __m128i mD = SaINot(_mm_or_si128(_mm_or_si128(mA, mB), mC));

  const __m128i i56 = SaIInterp(c5, c6, cMask, lMask);
  const __m128i i23 = SaIInterp(c2, c3, cMask, lMask);

  // ---- color2 == color6 && color5 != color3 -------------------------------
  // product1b = product2a = color2
  // product1a = (color1 == color2 || color6 == colorB2) ? I(color2, I(color2, color5))
  //                                                     : I(color5, color6)
  // product2b = (color6 == colorS2 || color2 == colorA1) ? I(color2, I(color2, color3))
  //                                                      : I(color2, color3)
  const __m128i condA = _mm_or_si128(_mm_cmpeq_epi16(c1, c2),
                                     _mm_cmpeq_epi16(c6, b2));
  const __m128i p1aA = SaISel(condA,
                              SaIInterp(c2, SaIInterp(c2, c5, cMask, lMask), cMask, lMask),
                              i56);
  const __m128i condA2 = _mm_or_si128(_mm_cmpeq_epi16(c6, s2),
                                      _mm_cmpeq_epi16(c2, a1));
  const __m128i p2bA = SaISel(condA2, SaIInterp(c2, i23, cMask, lMask), i23);

  // ---- color5 == color3 && color2 != color6 -------------------------------
  // product2b = product1a = color5
  // product1b = (colorB1 == color5 || color3 == colorS1) ? I(color5, I(color5, color6))
  //                                                      : I(color5, color6)
  // product2a = (color3 == colorA2 || color4 == color5)  ? I(color5, I(color5, color2))
  //                                                      : I(color2, color3)
  const __m128i condB1 = _mm_or_si128(_mm_cmpeq_epi16(b1, c5),
                                      _mm_cmpeq_epi16(c3, s1));
  const __m128i p1bB = SaISel(condB1, SaIInterp(c5, i56, cMask, lMask), i56);
  const __m128i condB2 = _mm_or_si128(_mm_cmpeq_epi16(c3, a2),
                                      _mm_cmpeq_epi16(c4, c5));
  const __m128i p2aB = SaISel(condB2,
                              SaIInterp(c5, SaIInterp(c5, c2, cMask, lMask), cMask, lMask),
                              i23);

  // ---- color5 == color3 && color2 == color6 -------------------------------
  // r = GetResult(color6, color5, color1, colorA1)
  //   + GetResult(color6, color5, color4, colorB1)
  //   + GetResult(color6, color5, colorA2, colorS1)
  //   + GetResult(color6, color5, colorB2, colorS2)
  // r > 0: product1b = product2a = color2, product1a = product2b = I(color5, color6)
  // r < 0: product2b = product1a = color5, product1b = product2a = I(color5, color6)
  // r == 0: product2b = product1a = color5, product1b = product2a = color2
  __m128i r = zero;
  r = _mm_add_epi16(r, SaIResultPair(c6, c5, c1, a1, one, zero));
  r = _mm_add_epi16(r, SaIResultPair(c6, c5, c4, b1, one, zero));
  r = _mm_add_epi16(r, SaIResultPair(c6, c5, a2, s1, one, zero));
  r = _mm_add_epi16(r, SaIResultPair(c6, c5, b2, s2, one, zero));

  const __m128i pos = _mm_cmpgt_epi16(r, zero);
  const __m128i neg = _mm_cmpgt_epi16(zero, r);
  const __m128i p1aC = SaISel(pos, i56, c5);   // also product2b
  const __m128i p1bC = SaISel(neg, i56, c2);   // also product2a

  // ---- general case -------------------------------------------------------
  // t1 = I(color2, color6);
  //   product1a = Q(color5, color5, color5, t1)
  //   product2b = Q(color3, color3, color3, t1)
  // t2 = I(color5, color3);
  //   product2a = Q(color2, color2, color2, t2)
  //   product1b = Q(color6, color6, color6, t2)
  const __m128i t1 = SaIInterp(c2, c6, cMask, lMask);
  const __m128i p1aD = SaIInterp4(c5, c5, c5, t1, qcMask, qlMask);
  const __m128i p2bD = SaIInterp4(c3, c3, c3, t1, qcMask, qlMask);
  const __m128i t2 = SaIInterp(c5, c3, cMask, lMask);
  const __m128i p1bD = SaIInterp4(c6, c6, c6, t2, qcMask, qlMask);
  const __m128i p2aD = SaIInterp4(c2, c2, c2, t2, qcMask, qlMask);

  const __m128i f1a = _mm_or_si128(_mm_or_si128(_mm_and_si128(mA, p1aA),
                                                 _mm_and_si128(mB, c5)),
                                   _mm_or_si128(_mm_and_si128(mC, p1aC),
                                                _mm_and_si128(mD, p1aD)));
  const __m128i f1b = _mm_or_si128(_mm_or_si128(_mm_and_si128(mA, c2),
                                                 _mm_and_si128(mB, p1bB)),
                                   _mm_or_si128(_mm_and_si128(mC, p1bC),
                                                _mm_and_si128(mD, p1bD)));
  const __m128i f2a = _mm_or_si128(_mm_or_si128(_mm_and_si128(mA, c2),
                                                 _mm_and_si128(mB, p2aB)),
                                   _mm_or_si128(_mm_and_si128(mC, p1bC),
                                                _mm_and_si128(mD, p2aD)));
  const __m128i f2b = _mm_or_si128(_mm_or_si128(_mm_and_si128(mA, p2bA),
                                                 _mm_and_si128(mB, c5)),
                                   _mm_or_si128(_mm_and_si128(mC, p1aC),
                                                _mm_and_si128(mD, p2bD)));

  if (dlt)
    SaIBlock16<N>::storeN(dlt, c5);

  SaIBlock16<N>::storePair(dst, dstPitch16,
                           SaIBlock16<N>::lo(f1a, f1b), SaIBlock16<N>::lo(f2a, f2b),
                           SaIBlock16<N>::hi(f1a, f1b), SaIBlock16<N>::hi(f2a, f2b),
                           3);
}

template<int N>
static inline void _2xSaI_16_block(uint16_t* dst, int dstPitch16,
                                   const uint16_t* B, const uint16_t* A,
                                   const uint16_t* C, const uint16_t* D,
                                   uint16_t* dlt, int srcPitch16)
{
  const int changed = SaIChangedHalves<N>(B, A, C, D, dlt, srcPitch16);
  if (!changed)
    return;

  const __m128i zero = _mm_setzero_si128();
  const __m128i cMask = _mm_set1_epi16(static_cast<short>(s_colorMask16));
  const __m128i lMask = _mm_set1_epi16(static_cast<short>(s_lowPixelMask16));

  const __m128i cA = SaIBlock16<N>::load(A);
  const __m128i cB = SaIBlock16<N>::load(A + 1);
  const __m128i cC = SaIBlock16<N>::load(C);
  const __m128i cD = SaIBlock16<N>::load(C + 1);
  const __m128i cE = SaIBlock16<N>::load(B);
  const __m128i cF = SaIBlock16<N>::load(B + 1);
  const __m128i cG = SaIBlock16<N>::load(A - 1);
  const __m128i cH = SaIBlock16<N>::load(C - 1);
  const __m128i cI = SaIBlock16<N>::load(B - 1);
  const __m128i cJ = SaIBlock16<N>::load(B + 2);
  const __m128i cK = SaIBlock16<N>::load(A + 2);
  const __m128i cL = SaIBlock16<N>::load(C + 2);
  const __m128i cM = SaIBlock16<N>::load(D - 1);
  const __m128i cN = SaIBlock16<N>::load(D);
  const __m128i cO = SaIBlock16<N>::load(D + 1);

  // ---- Mask1 / Mask2 of the upper destination line ----
  __m128i m = _mm_and_si128(_mm_cmpeq_epi16(cA, cD), _mm_cmpeq_epi16(cA, cE));
  m = _mm_and_si128(m, _mm_cmpeq_epi16(cB, cL));
  m = _mm_and_si128(m, SaINot(_mm_cmpeq_epi16(cB, cC)));

  __m128i m2 = _mm_and_si128(_mm_cmpeq_epi16(cA, cC), _mm_cmpeq_epi16(cA, cF));
  m2 = _mm_and_si128(m2, _mm_cmpeq_epi16(cB, cJ));
  m2 = _mm_and_si128(m2, SaINot(_mm_cmpeq_epi16(cB, cE)));
  __m128i mask1 = _mm_or_si128(m, m2);

  m = _mm_and_si128(_mm_cmpeq_epi16(cB, cC), _mm_cmpeq_epi16(cB, cF));
  m = _mm_and_si128(m, _mm_cmpeq_epi16(cA, cH));
  m = _mm_and_si128(m, SaINot(_mm_cmpeq_epi16(cA, cD)));

  m2 = _mm_and_si128(_mm_cmpeq_epi16(cB, cD), _mm_cmpeq_epi16(cB, cE));
  m2 = _mm_and_si128(m2, _mm_cmpeq_epi16(cA, cI));
  m2 = _mm_and_si128(m2, SaINot(_mm_cmpeq_epi16(cA, cF)));
  __m128i mask2 = _mm_or_si128(m, m2);

  const __m128i iab = SaIInterp(cA, cB, cMask, lMask);
  const __m128i anyAB = _mm_or_si128(mask1, mask2);
  const __m128i outAB = _mm_or_si128(
      _mm_or_si128(_mm_and_si128(mask1, cA), _mm_and_si128(mask2, cB)),
      _mm_andnot_si128(anyAB, iab));

  // ---- Mask1 / Mask2 of the lower destination line ----
  m = _mm_and_si128(_mm_cmpeq_epi16(cA, cD), _mm_cmpeq_epi16(cA, cG));
  m = _mm_and_si128(m, _mm_cmpeq_epi16(cC, cO));
  m = _mm_and_si128(m, SaINot(_mm_cmpeq_epi16(cB, cC)));

  m2 = _mm_and_si128(_mm_cmpeq_epi16(cA, cB), _mm_cmpeq_epi16(cA, cH));
  m2 = _mm_and_si128(m2, _mm_cmpeq_epi16(cC, cM));
  m2 = _mm_and_si128(m2, SaINot(_mm_cmpeq_epi16(cG, cC)));
  mask1 = _mm_or_si128(m, m2);

  m = _mm_and_si128(_mm_cmpeq_epi16(cB, cC), _mm_cmpeq_epi16(cC, cH));
  m = _mm_and_si128(m, _mm_cmpeq_epi16(cA, cF));
  m = _mm_and_si128(m, SaINot(_mm_cmpeq_epi16(cA, cD)));

  m2 = _mm_and_si128(_mm_cmpeq_epi16(cC, cG), _mm_cmpeq_epi16(cC, cD));
  m2 = _mm_and_si128(m2, _mm_cmpeq_epi16(cA, cI));
  m2 = _mm_and_si128(m2, SaINot(_mm_cmpeq_epi16(cA, cH)));
  mask2 = _mm_or_si128(m, m2);

  const __m128i iac = SaIInterp(cA, cC, cMask, lMask);
  const __m128i anyAC = _mm_or_si128(mask1, mask2);
  const __m128i acPixel = _mm_or_si128(
      _mm_or_si128(_mm_and_si128(mask1, cA), _mm_and_si128(mask2, cC)),
      _mm_andnot_si128(anyAC, iac));

  // ---- branch selection ----
  const __m128i eqAD = _mm_cmpeq_epi16(cA, cD);
  const __m128i eqBC = _mm_cmpeq_epi16(cB, cC);
  const __m128i eqAB = _mm_cmpeq_epi16(cA, cB);

  const __m128i both = _mm_and_si128(eqAD, eqBC);
  mask2 = _mm_and_si128(eqBC, SaINot(eqAD));                       // (A!=D) && (B==C)
  mask1 = _mm_or_si128(_mm_and_si128(eqAD, SaINot(eqBC)),          // (A==D) && (B!=C)
                       _mm_and_si128(eqAB, both));
  const __m128i ambiguous = _mm_andnot_si128(eqAB, both);

  if (_mm_movemask_epi8(ambiguous) != 0)
  {
    const __m128i sel = _mm_and_si128(ambiguous, _mm_set1_epi16(1));
    __m128i r = zero;
    r = _mm_add_epi16(r, SaIGetResult(cE, cG, cA, cB, sel, zero));   // E,G
    r = _mm_add_epi16(r, SaIGetResult(cF, cK, cA, cB, sel, zero));   // F,K
    r = _mm_add_epi16(r, SaIGetResult(cH, cN, cA, cB, sel, zero));   // H,N
    r = _mm_add_epi16(r, SaIGetResult(cL, cO, cA, cB, sel, zero));   // L,O

    if (SaIAnyLane(r))
    {
      mask1 = _mm_or_si128(mask1, _mm_cmpgt_epi16(r, zero));
      mask2 = _mm_or_si128(mask2, _mm_cmpgt_epi16(zero, r));
    }
  }

  const __m128i qcMask = _mm_set1_epi16(static_cast<short>(s_qcolorMask16));
  const __m128i qlMask = _mm_set1_epi16(static_cast<short>(s_qlowPixelMask16));
  const __m128i q = SaIInterp4(cA, cB, cC, cD, qcMask, qlMask);

  const __m128i anyQ = _mm_or_si128(mask1, mask2);
  const __m128i diag = _mm_or_si128(
      _mm_or_si128(_mm_and_si128(mask1, cA), _mm_and_si128(mask2, cB)),
      _mm_andnot_si128(anyQ, q));

  SaIBlock16<N>::storePair(dst, dstPitch16,
                           SaIBlock16<N>::lo(cA, outAB), SaIBlock16<N>::lo(acPixel, diag),
                           SaIBlock16<N>::hi(cA, outAB), SaIBlock16<N>::hi(acPixel, diag),
                           changed);
}

typedef void (*SaI16BlockFn)(uint16_t*, int, const uint16_t*, const uint16_t*,
                             const uint16_t*, const uint16_t*, uint16_t*, int);

static void SaI16_filter(uint8_t* srcPtr, uint32_t srcPitch, uint8_t* deltaPtr,
                         uint8_t* dstPtr, uint32_t dstPitch, int width, int height,
                         SaI16BlockFn block8, SaI16BlockFn block4)
{
  const int srcPitch16 = static_cast<int>(srcPitch >> 1);
  const int dstPitch16 = static_cast<int>(dstPitch >> 1);
  const int count = (width + 3) & ~3;

  const uint16_t* src = reinterpret_cast<const uint16_t*>(srcPtr);
  uint16_t* dst = reinterpret_cast<uint16_t*>(dstPtr);
  uint16_t* delta = reinterpret_cast<uint16_t*>(deltaPtr);

  for (int y = 0; y < height; ++y)
  {
    const uint16_t* rowB = src + (y - 1) * srcPitch16;
    const uint16_t* rowA = src + y * srcPitch16;
    const uint16_t* rowC = src + (y + 1) * srcPitch16;
    const uint16_t* rowD = src + (y + 2) * srcPitch16;
    uint16_t* dlt = delta ? delta + y * srcPitch16 : 0;

    int x = 0;
    for (; x + 8 <= count; x += 8)
      block8(dst + 2 * x, dstPitch16, rowB + x, rowA + x, rowC + x, rowD + x,
             dlt ? dlt + x : 0, srcPitch16);
    if (x < count)
      block4(dst + 2 * x, dstPitch16, rowB + x, rowA + x, rowC + x, rowD + x,
             dlt ? dlt + x : 0, srcPitch16);

    dst += 2 * dstPitch16;
  }
}

static void Super2xSaI_16_sse2(uint8_t* srcPtr, uint32_t srcPitch, uint8_t* deltaPtr,
                               uint8_t* dstPtr, uint32_t dstPitch, int width, int height)
{
  SaI16_filter(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height,
               &Super2xSaI_16_block<8>, &Super2xSaI_16_block<4>);
}

static void SuperEagle_16_sse2(uint8_t* srcPtr, uint32_t srcPitch, uint8_t* deltaPtr,
                               uint8_t* dstPtr, uint32_t dstPitch, int width, int height)
{
  SaI16_filter(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height,
               &SuperEagle_16_block<8>, &SuperEagle_16_block<4>);
}

static void _2xSaI_16_sse2(uint8_t* srcPtr, uint32_t srcPitch, uint8_t* deltaPtr,
                           uint8_t* dstPtr, uint32_t dstPitch, int width, int height)
{
  SaI16_filter(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height,
               &_2xSaI_16_block<8>, &_2xSaI_16_block<4>);
}

#endif  // USE_SSE2


void Super2xSaI (uint8_t *srcPtr, uint32_t srcPitch,
                 uint8_t *deltaPtr, uint8_t *dstPtr, uint32_t dstPitch,
                 int width, int height)
{
#if defined(USE_SSE2)
  if (width > 0 && height > 0 && systemColorDepth == 16) {
    Super2xSaI_16_sse2(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height);
    return;
  }
#endif
  uint8_t  *dP;
  uint32_t Nextline = srcPitch >> 1;
  {

      const uint16_t* src = reinterpret_cast<const uint16_t*>(srcPtr);
      int srcPitch16 = static_cast<int>(Nextline);
      int originalHeight = height;

      for (int y = 0; y < originalHeight; y++) {
        dP = dstPtr + y * (dstPitch << 1);

        for (int x = 0; x < width; x++) {
          uint32_t color4, color5, color6;
          uint32_t color1, color2, color3;
          uint32_t colorA0, colorA1, colorA2, colorA3,
            colorB0, colorB1, colorB2, colorB3, colorS1, colorS2;
          uint32_t product1a, product1b, product2a, product2b;

          //---------------------------------------    B1 B2
          //                                         4  5  6 S2
          //                                         1  2  3 S1
          //                                           A1 A2

          colorB0 = GetPixel16(src, srcPitch16, x - 1, y - 1, width, originalHeight);
          colorB1 = GetPixel16(src, srcPitch16, x,     y - 1, width, originalHeight);
          colorB2 = GetPixel16(src, srcPitch16, x + 1, y - 1, width, originalHeight);
          colorB3 = GetPixel16(src, srcPitch16, x + 2, y - 1, width, originalHeight);

          color4 = GetPixel16(src, srcPitch16, x - 1, y, width, originalHeight);
          color5 = GetPixel16(src, srcPitch16, x,     y, width, originalHeight);
          color6 = GetPixel16(src, srcPitch16, x + 1, y, width, originalHeight);
          colorS2 = GetPixel16(src, srcPitch16, x + 2, y, width, originalHeight);

          color1 = GetPixel16(src, srcPitch16, x - 1, y + 1, width, originalHeight);
          color2 = GetPixel16(src, srcPitch16, x,     y + 1, width, originalHeight);
          color3 = GetPixel16(src, srcPitch16, x + 1, y + 1, width, originalHeight);
          colorS1 = GetPixel16(src, srcPitch16, x + 2, y + 1, width, originalHeight);

          colorA0 = GetPixel16(src, srcPitch16, x - 1, y + 2, width, originalHeight);
          colorA1 = GetPixel16(src, srcPitch16, x,     y + 2, width, originalHeight);
          colorA2 = GetPixel16(src, srcPitch16, x + 1, y + 2, width, originalHeight);
          colorA3 = GetPixel16(src, srcPitch16, x + 2, y + 2, width, originalHeight);

          //--------------------------------------
          if (color2 == color6 && color5 != color3) {
            product2b = product1b = color2;
          } else if (color5 == color3 && color2 != color6) {
            product2b = product1b = color5;
          } else if (color5 == color3 && color2 == color6) {
            int r = 0;

            r += GetResult (color6, color5, color1, colorA1);
            r += GetResult (color6, color5, color4, colorB1);
            r += GetResult (color6, color5, colorA2, colorS1);
            r += GetResult (color6, color5, colorB2, colorS2);

            if (r > 0)
              product2b = product1b = color6;
            else if (r < 0)
              product2b = product1b = color5;
            else {
              product2b = product1b = INTERPOLATE16 (color5, color6);
            }
          } else {
            if (color6 == color3 && color3 == colorA1
                && color2 != colorA2 && color3 != colorA0)
              product2b =
                Q_INTERPOLATE16 (color3, color3, color3, color2);
            else if (color5 == color2 && color2 == colorA2
                     && colorA1 != color3 && color2 != colorA3)
              product2b =
                Q_INTERPOLATE16 (color2, color2, color2, color3);
            else
              product2b = INTERPOLATE16 (color2, color3);

            if (color6 == color3 && color6 == colorB1
                && color5 != colorB2 && color6 != colorB0)
              product1b =
                Q_INTERPOLATE16 (color6, color6, color6, color5);
            else if (color5 == color2 && color5 == colorB2
                     && colorB1 != color6 && color5 != colorB3)
              product1b =
                Q_INTERPOLATE16 (color6, color5, color5, color5);
            else
              product1b = INTERPOLATE16 (color5, color6);
          }

          if (color5 == color3 && color2 != color6 && color4 == color5
              && color5 != colorA2)
            product2a = INTERPOLATE16 (color2, color5);
          else
            if (color5 == color1 && color6 == color5
                && color4 != color2 && color5 != colorA0)
              product2a = INTERPOLATE16 (color2, color5);
            else
              product2a = color2;

          if (color2 == color6 && color5 != color3 && color1 == color2
              && color2 != colorB2)
            product1a = INTERPOLATE16 (color2, color5);
          else
            if (color4 == color2 && color3 == color2
                && color1 != color5 && color2 != colorB0)
              product1a = INTERPOLATE16 (color2, color5);
            else
              product1a = color5;

#ifdef WORDS_BIGENDIAN
          product1a = (product1a << 16) | product1b;
          product2a = (product2a << 16) | product2b;
#else
          product1a = product1a | (product1b << 16);
          product2a = product2a | (product2b << 16);
#endif

          *((uint32_t *) dP) = product1a;
          *((uint32_t *) (dP + dstPitch)) = product2a;

          dP += sizeof (uint32_t);
        }                       // end of for ( x < width )
      }                 // endof: for (y < originalHeight)
    }
}

void Super2xSaI32 (uint8_t *srcPtr, uint32_t srcPitch,
                   uint8_t * /* deltaPtr */, uint8_t *dstPtr, uint32_t dstPitch,
                   int width, int height)
{
  uint32_t *dP;
  uint32_t Nextline = srcPitch >> 2;

  const uint32_t* src = reinterpret_cast<const uint32_t*>(srcPtr);
  int srcPitch32 = static_cast<int>(Nextline);

  for (int y = 0; y < height; y++) {
    dP = reinterpret_cast<uint32_t*>(dstPtr + y * (dstPitch << 1));

    for (int x = 0; x < width; x++) {
      uint32_t color4, color5, color6;
      uint32_t color1, color2, color3;
      uint32_t colorA0, colorA1, colorA2, colorA3,
        colorB0, colorB1, colorB2, colorB3, colorS1, colorS2;
      uint32_t product1a, product1b, product2a, product2b;

      //---------------------------------------    B1 B2
      //                                         4  5  6 S2
      //                                         1  2  3 S1
      //                                           A1 A2

      colorB0 = GetPixel32(src, srcPitch32, x - 1, y - 1, width, height);
      colorB1 = GetPixel32(src, srcPitch32, x,     y - 1, width, height);
      colorB2 = GetPixel32(src, srcPitch32, x + 1, y - 1, width, height);
      colorB3 = GetPixel32(src, srcPitch32, x + 2, y - 1, width, height);

      color4 = GetPixel32(src, srcPitch32, x - 1, y, width, height);
      color5 = GetPixel32(src, srcPitch32, x,     y, width, height);
      color6 = GetPixel32(src, srcPitch32, x + 1, y, width, height);
      colorS2 = GetPixel32(src, srcPitch32, x + 2, y, width, height);

      color1 = GetPixel32(src, srcPitch32, x - 1, y + 1, width, height);
      color2 = GetPixel32(src, srcPitch32, x,     y + 1, width, height);
      color3 = GetPixel32(src, srcPitch32, x + 1, y + 1, width, height);
      colorS1 = GetPixel32(src, srcPitch32, x + 2, y + 1, width, height);

      colorA0 = GetPixel32(src, srcPitch32, x - 1, y + 2, width, height);
      colorA1 = GetPixel32(src, srcPitch32, x,     y + 2, width, height);
      colorA2 = GetPixel32(src, srcPitch32, x + 1, y + 2, width, height);
      colorA3 = GetPixel32(src, srcPitch32, x + 2, y + 2, width, height);

      //--------------------------------------
      if (color2 == color6 && color5 != color3) {
        product2b = product1b = color2;
      } else if (color5 == color3 && color2 != color6) {
        product2b = product1b = color5;
      } else if (color5 == color3 && color2 == color6) {
        int r = 0;

        r += GetResult (color6, color5, color1, colorA1);
        r += GetResult (color6, color5, color4, colorB1);
        r += GetResult (color6, color5, colorA2, colorS1);
        r += GetResult (color6, color5, colorB2, colorS2);

        if (r > 0)
          product2b = product1b = color6;
        else if (r < 0)
          product2b = product1b = color5;
        else {
          product2b = product1b = INTERPOLATE (color5, color6);
        }
      } else {
        if (color6 == color3 && color3 == colorA1
            && color2 != colorA2 && color3 != colorA0)
          product2b =
            Q_INTERPOLATE (color3, color3, color3, color2);
        else if (color5 == color2 && color2 == colorA2
                 && colorA1 != color3 && color2 != colorA3)
          product2b =
            Q_INTERPOLATE (color2, color2, color2, color3);
        else
          product2b = INTERPOLATE (color2, color3);

        if (color6 == color3 && color6 == colorB1
            && color5 != colorB2 && color6 != colorB0)
          product1b =
            Q_INTERPOLATE (color6, color6, color6, color5);
        else if (color5 == color2 && color5 == colorB2
                 && colorB1 != color6 && color5 != colorB3)
          product1b =
            Q_INTERPOLATE (color6, color5, color5, color5);
        else
          product1b = INTERPOLATE (color5, color6);
      }

      if (color5 == color3 && color2 != color6 && color4 == color5
          && color5 != colorA2)
        product2a = INTERPOLATE (color2, color5);
      else
        if (color5 == color1 && color6 == color5
            && color4 != color2 && color5 != colorA0)
          product2a = INTERPOLATE (color2, color5);
        else
          product2a = color2;

      if (color2 == color6 && color5 != color3 && color1 == color2
          && color2 != colorB2)
        product1a = INTERPOLATE (color2, color5);
      else
        if (color4 == color2 && color3 == color2
            && color1 != color5 && color2 != colorB0)
          product1a = INTERPOLATE (color2, color5);
        else
          product1a = color5;
      *(dP) = product1a;
      *(dP+1) = product1b;
      *(dP + (dstPitch >> 2)) = product2a;
      *(dP + (dstPitch >> 2) + 1) = product2b;

      dP += 2;
    }                       // end of for ( x < width )
  }                 // endof: for (y < height)
}

void SuperEagle (uint8_t *srcPtr, uint32_t srcPitch, uint8_t *deltaPtr,
                 uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
#if defined(USE_SSE2)
  if (width > 0 && height > 0 && systemColorDepth == 16) {
    SuperEagle_16_sse2(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height);
    return;
  }
#endif
  uint8_t  *dP;
  uint16_t *xP;
  bool hasDelta = (deltaPtr != nullptr);

  {
    uint32_t Nextline = srcPitch >> 1;
    const uint16_t* src = reinterpret_cast<const uint16_t*>(srcPtr);
    int srcPitch16 = static_cast<int>(Nextline);

    for (int y = 0; y < height; y++) {
      xP = hasDelta ? reinterpret_cast<uint16_t*>(deltaPtr + y * srcPitch) : nullptr;
      dP = dstPtr + y * (dstPitch << 1);

      for (int x = 0; x < width; x++) {
        uint32_t color4, color5, color6;
        uint32_t color1, color2, color3;
        uint32_t colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
        uint32_t product1a, product1b, product2a, product2b;

        // Safe boundary-clamped pixel access
        colorB1 = GetPixel16(src, srcPitch16, x,     y - 1, width, height);
        colorB2 = GetPixel16(src, srcPitch16, x + 1, y - 1, width, height);

        color4 = GetPixel16(src, srcPitch16, x - 1, y, width, height);
        color5 = GetPixel16(src, srcPitch16, x,     y, width, height);
        color6 = GetPixel16(src, srcPitch16, x + 1, y, width, height);
        colorS2 = GetPixel16(src, srcPitch16, x + 2, y, width, height);

        color1 = GetPixel16(src, srcPitch16, x - 1, y + 1, width, height);
        color2 = GetPixel16(src, srcPitch16, x,     y + 1, width, height);
        color3 = GetPixel16(src, srcPitch16, x + 1, y + 1, width, height);
        colorS1 = GetPixel16(src, srcPitch16, x + 2, y + 1, width, height);

        colorA1 = GetPixel16(src, srcPitch16, x,     y + 2, width, height);
        colorA2 = GetPixel16(src, srcPitch16, x + 1, y + 2, width, height);

        // --------------------------------------
        if (color2 == color6 && color5 != color3) {
          product1b = product2a = color2;
          if ((color1 == color2) || (color6 == colorB2)) {
            product1a = INTERPOLATE16 (color2, color5);
            product1a = INTERPOLATE16 (color2, product1a);
            //                       product1a = color2;
          } else {
            product1a = INTERPOLATE16 (color5, color6);
          }

          if ((color6 == colorS2) || (color2 == colorA1)) {
            product2b = INTERPOLATE16 (color2, color3);
            product2b = INTERPOLATE16 (color2, product2b);
            //                       product2b = color2;
          } else {
            product2b = INTERPOLATE16 (color2, color3);
          }
        } else if (color5 == color3 && color2 != color6) {
          product2b = product1a = color5;

          if ((colorB1 == color5) || (color3 == colorS1)) {
            product1b = INTERPOLATE16 (color5, color6);
            product1b = INTERPOLATE16 (color5, product1b);
            //                       product1b = color5;
          } else {
            product1b = INTERPOLATE16 (color5, color6);
          }

          if ((color3 == colorA2) || (color4 == color5)) {
            product2a = INTERPOLATE16 (color5, color2);
            product2a = INTERPOLATE16 (color5, product2a);
            //                       product2a = color5;
          } else {
            product2a = INTERPOLATE16 (color2, color3);
          }

        } else if (color5 == color3 && color2 == color6) {
          int r = 0;

          r += GetResult (color6, color5, color1, colorA1);
          r += GetResult (color6, color5, color4, colorB1);
          r += GetResult (color6, color5, colorA2, colorS1);
          r += GetResult (color6, color5, colorB2, colorS2);

          if (r > 0) {
            product1b = product2a = color2;
            product1a = product2b = INTERPOLATE16 (color5, color6);
          } else if (r < 0) {
            product2b = product1a = color5;
            product1b = product2a = INTERPOLATE16 (color5, color6);
          } else {
            product2b = product1a = color5;
            product1b = product2a = color2;
          }
        } else {
          product2b = product1a = INTERPOLATE16 (color2, color6);
          product2b =
            Q_INTERPOLATE16 (color3, color3, color3, product2b);
          product1a =
            Q_INTERPOLATE16 (color5, color5, color5, product1a);

          product2a = product1b = INTERPOLATE16 (color5, color3);
          product2a =
            Q_INTERPOLATE16 (color2, color2, color2, product2a);
          product1b =
            Q_INTERPOLATE16 (color6, color6, color6, product1b);

          //                    product1a = color5;
          //                    product1b = color6;
          //                    product2a = color2;
          //                    product2b = color3;
        }
#ifdef WORDS_BIGENDIAN
        product1a = (product1a << 16) | product1b;
        product2a = (product2a << 16) | product2b;
#else
        product1a = product1a | (product1b << 16);
        product2a = product2a | (product2b << 16);
#endif

        *((uint32_t *) dP) = product1a;
        *((uint32_t *) (dP + dstPitch)) = product2a;
        if (xP) { *xP = (uint16_t)color5; xP++; }

        dP += sizeof (uint32_t);
      }                 // end of for ( x < width )
    }                   // endof: for (y < height)
  }
}

void SuperEagle32 (uint8_t *srcPtr, uint32_t srcPitch, uint8_t *deltaPtr,
                   uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
  uint32_t  *dP;
  uint32_t *xP;
  bool hasDelta = (deltaPtr != nullptr);

  uint32_t Nextline = srcPitch >> 2;
  const uint32_t* src = reinterpret_cast<const uint32_t*>(srcPtr);
  int srcPitch32 = static_cast<int>(Nextline);

  for (int y = 0; y < height; y++) {
    xP = hasDelta ? reinterpret_cast<uint32_t*>(deltaPtr + y * srcPitch) : nullptr;
    dP = reinterpret_cast<uint32_t*>(dstPtr + y * (dstPitch << 1));

    for (int x = 0; x < width; x++) {
      uint32_t color4, color5, color6;
      uint32_t color1, color2, color3;
      uint32_t colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
      uint32_t product1a, product1b, product2a, product2b;

      // Safe boundary-clamped pixel access
      colorB1 = GetPixel32(src, srcPitch32, x,     y - 1, width, height);
      colorB2 = GetPixel32(src, srcPitch32, x + 1, y - 1, width, height);

      color4 = GetPixel32(src, srcPitch32, x - 1, y, width, height);
      color5 = GetPixel32(src, srcPitch32, x,     y, width, height);
      color6 = GetPixel32(src, srcPitch32, x + 1, y, width, height);
      colorS2 = GetPixel32(src, srcPitch32, x + 2, y, width, height);

      color1 = GetPixel32(src, srcPitch32, x - 1, y + 1, width, height);
      color2 = GetPixel32(src, srcPitch32, x,     y + 1, width, height);
      color3 = GetPixel32(src, srcPitch32, x + 1, y + 1, width, height);
      colorS1 = GetPixel32(src, srcPitch32, x + 2, y + 1, width, height);

      colorA1 = GetPixel32(src, srcPitch32, x,     y + 2, width, height);
      colorA2 = GetPixel32(src, srcPitch32, x + 1, y + 2, width, height);

      // --------------------------------------
      // 32-bit mode uses INTERPOLATE for LCD filter compatibility
      if (color2 == color6 && color5 != color3) {
        product1b = product2a = color2;
        if ((color1 == color2) || (color6 == colorB2)) {
          product1a = INTERPOLATE (color2, color5);
          product1a = INTERPOLATE (color2, product1a);
          //                       product1a = color2;
        } else {
          product1a = INTERPOLATE (color5, color6);
        }

        if ((color6 == colorS2) || (color2 == colorA1)) {
          product2b = INTERPOLATE (color2, color3);
          product2b = INTERPOLATE (color2, product2b);
          //                       product2b = color2;
        } else {
          product2b = INTERPOLATE (color2, color3);
        }
      } else if (color5 == color3 && color2 != color6) {
        product2b = product1a = color5;

        if ((colorB1 == color5) || (color3 == colorS1)) {
          product1b = INTERPOLATE (color5, color6);
          product1b = INTERPOLATE (color5, product1b);
          //                       product1b = color5;
        } else {
          product1b = INTERPOLATE (color5, color6);
        }

        if ((color3 == colorA2) || (color4 == color5)) {
          product2a = INTERPOLATE (color5, color2);
          product2a = INTERPOLATE (color5, product2a);
          //                       product2a = color5;
        } else {
          product2a = INTERPOLATE (color2, color3);
        }

      } else if (color5 == color3 && color2 == color6) {
        int r = 0;

        r += GetResult (color6, color5, color1, colorA1);
        r += GetResult (color6, color5, color4, colorB1);
        r += GetResult (color6, color5, colorA2, colorS1);
        r += GetResult (color6, color5, colorB2, colorS2);

        if (r > 0) {
          product1b = product2a = color2;
          product1a = product2b = INTERPOLATE (color5, color6);
        } else if (r < 0) {
          product2b = product1a = color5;
          product1b = product2a = INTERPOLATE (color5, color6);
        } else {
          product2b = product1a = color5;
          product1b = product2a = color2;
        }
      } else {
        product2b = product1a = INTERPOLATE (color2, color6);
        product2b =
          Q_INTERPOLATE (color3, color3, color3, product2b);
        product1a =
          Q_INTERPOLATE (color5, color5, color5, product1a);

        product2a = product1b = INTERPOLATE (color5, color3);
        product2a =
          Q_INTERPOLATE (color2, color2, color2, product2a);
        product1b =
          Q_INTERPOLATE (color6, color6, color6, product1b);

        //                    product1a = color5;
        //                    product1b = color6;
        //                    product2a = color2;
        //                    product2b = color3;
      }
      
      *(dP) = product1a;
      *(dP+1) = product1b;
      *(dP + (dstPitch >> 2)) = product2a;
      *(dP + (dstPitch >> 2) +1) = product2b;
      if (xP) { *xP = color5; xP++; }

      dP += 2;
    }                 // end of for ( x < width )
  }                   // endof: for (y < height)
}

void _2xSaI (uint8_t *srcPtr, uint32_t srcPitch, uint8_t *deltaPtr,
             uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
#if defined(USE_SSE2)
  if (width > 0 && height > 0 && systemColorDepth == 16) {
    _2xSaI_16_sse2(srcPtr, srcPitch, deltaPtr, dstPtr, dstPitch, width, height);
    return;
  }
#endif
  uint8_t  *dP;

  {
    uint32_t Nextline = srcPitch >> 1;
    const uint16_t* src = reinterpret_cast<const uint16_t*>(srcPtr);
    int srcPitch16 = static_cast<int>(Nextline);

    for (int y = 0; y < height; y++) {
      dP = dstPtr + y * (dstPitch << 1);

      for (int x = 0; x < width; x++) {

        uint32_t colorA, colorB;
        uint32_t colorC, colorD,
          colorE, colorF, colorG, colorH,
          colorI, colorJ, colorK, colorL,

          colorM, colorN, colorO, colorP;
        uint32_t product, product1, product2;

        //---------------------------------------
        // Map of the pixels:                    I|E F|J
        //                                       G|A B|K
        //                                       H|C D|L
        //                                       M|N O|P
        colorI = GetPixel16(src, srcPitch16, x - 1, y - 1, width, height);
        colorE = GetPixel16(src, srcPitch16, x,     y - 1, width, height);
        colorF = GetPixel16(src, srcPitch16, x + 1, y - 1, width, height);
        colorJ = GetPixel16(src, srcPitch16, x + 2, y - 1, width, height);

        colorG = GetPixel16(src, srcPitch16, x - 1, y, width, height);
        colorA = GetPixel16(src, srcPitch16, x,     y, width, height);
        colorB = GetPixel16(src, srcPitch16, x + 1, y, width, height);
        colorK = GetPixel16(src, srcPitch16, x + 2, y, width, height);

        colorH = GetPixel16(src, srcPitch16, x - 1, y + 1, width, height);
        colorC = GetPixel16(src, srcPitch16, x,     y + 1, width, height);
        colorD = GetPixel16(src, srcPitch16, x + 1, y + 1, width, height);
        colorL = GetPixel16(src, srcPitch16, x + 2, y + 1, width, height);

        colorM = GetPixel16(src, srcPitch16, x - 1, y + 2, width, height);
        colorN = GetPixel16(src, srcPitch16, x,     y + 2, width, height);
        colorO = GetPixel16(src, srcPitch16, x + 1, y + 2, width, height);
        colorP = GetPixel16(src, srcPitch16, x + 2, y + 2, width, height);

        if ((colorA == colorD) && (colorB != colorC)) {
          if (((colorA == colorE) && (colorB == colorL)) ||
              ((colorA == colorC) && (colorA == colorF)
               && (colorB != colorE) && (colorB == colorJ))) {
            product = colorA;
          } else {
            product = INTERPOLATE16 (colorA, colorB);
          }

          if (((colorA == colorG) && (colorC == colorO)) ||
              ((colorA == colorB) && (colorA == colorH)
               && (colorG != colorC) && (colorC == colorM))) {
            product1 = colorA;
          } else {
            product1 = INTERPOLATE16 (colorA, colorC);
          }
          product2 = colorA;
        } else if ((colorB == colorC) && (colorA != colorD)) {
          if (((colorB == colorF) && (colorA == colorH)) ||
              ((colorB == colorE) && (colorB == colorD)
               && (colorA != colorF) && (colorA == colorI))) {
            product = colorB;
          } else {
            product = INTERPOLATE16 (colorA, colorB);
          }

          if (((colorC == colorH) && (colorA == colorF)) ||
              ((colorC == colorG) && (colorC == colorD)
               && (colorA != colorH) && (colorA == colorI))) {
            product1 = colorC;
          } else {
            product1 = INTERPOLATE16 (colorA, colorC);
          }
          product2 = colorB;
        } else if ((colorA == colorD) && (colorB == colorC)) {
          if (colorA == colorB) {
            product = colorA;
            product1 = colorA;
            product2 = colorA;
          } else {
            int r = 0;

            product1 = INTERPOLATE16 (colorA, colorC);
            product = INTERPOLATE16 (colorA, colorB);

            r +=
              GetResult1 (colorA, colorB, colorG, colorE,
                          colorI);
            r +=
              GetResult2 (colorB, colorA, colorK, colorF,
                          colorJ);
            r +=
              GetResult2 (colorB, colorA, colorH, colorN,
                          colorM);
            r +=
              GetResult1 (colorA, colorB, colorL, colorO,
                          colorP);

            if (r > 0)
              product2 = colorA;
            else if (r < 0)
              product2 = colorB;
            else {
              product2 =
                Q_INTERPOLATE16 (colorA, colorB, colorC,
                               colorD);
            }
          }
        } else {
          product2 = Q_INTERPOLATE16 (colorA, colorB, colorC, colorD);

          if ((colorA == colorC) && (colorA == colorF)
              && (colorB != colorE) && (colorB == colorJ)) {
            product = colorA;
          } else if ((colorB == colorE) && (colorB == colorD)
                     && (colorA != colorF) && (colorA == colorI)) {
            product = colorB;
          } else {
            product = INTERPOLATE16 (colorA, colorB);
          }

          if ((colorA == colorB) && (colorA == colorH)
              && (colorG != colorC) && (colorC == colorM)) {
            product1 = colorA;
          } else if ((colorC == colorG) && (colorC == colorD)
                     && (colorA != colorH) && (colorA == colorI)) {
            product1 = colorC;
          } else {
            product1 = INTERPOLATE16 (colorA, colorC);
          }
        }

#ifdef WORDS_BIGENDIAN
        product = (colorA << 16) | product ;
        product1 = (product1 << 16) | product2 ;
#else
        product = colorA | (product << 16);
        product1 = product1 | (product2 << 16);
#endif
        *((int32_t *) dP) = product;
        *((uint32_t *) (dP + dstPitch)) = product1;

        dP += sizeof (uint32_t);
      }                 // end of for ( x < width )
    }                   // endof: for (y < height)
  }
}

void _2xSaI32 (uint8_t *srcPtr, uint32_t srcPitch, uint8_t * /* deltaPtr */,
               uint8_t *dstPtr, uint32_t dstPitch, int width, int height)
{
  uint32_t  *dP;

  uint32_t Nextline = srcPitch >> 2;
  const uint32_t* src = reinterpret_cast<const uint32_t*>(srcPtr);
  int srcPitch32 = static_cast<int>(Nextline);

  for (int y = 0; y < height; y++) {
    dP = reinterpret_cast<uint32_t*>(dstPtr + y * (dstPitch << 1));

    for (int x = 0; x < width; x++) {
      uint32_t colorA, colorB;
      uint32_t colorC, colorD,
        colorE, colorF, colorG, colorH,
        colorI, colorJ, colorK, colorL,

        colorM, colorN, colorO, colorP;
      uint32_t product, product1, product2;

      //---------------------------------------
      // Map of the pixels:                    I|E F|J
      //                                       G|A B|K
      //                                       H|C D|L
      //                                       M|N O|P
      colorI = GetPixel32(src, srcPitch32, x - 1, y - 1, width, height);
      colorE = GetPixel32(src, srcPitch32, x,     y - 1, width, height);
      colorF = GetPixel32(src, srcPitch32, x + 1, y - 1, width, height);
      colorJ = GetPixel32(src, srcPitch32, x + 2, y - 1, width, height);

      colorG = GetPixel32(src, srcPitch32, x - 1, y, width, height);
      colorA = GetPixel32(src, srcPitch32, x,     y, width, height);
      colorB = GetPixel32(src, srcPitch32, x + 1, y, width, height);
      colorK = GetPixel32(src, srcPitch32, x + 2, y, width, height);

      colorH = GetPixel32(src, srcPitch32, x - 1, y + 1, width, height);
      colorC = GetPixel32(src, srcPitch32, x,     y + 1, width, height);
      colorD = GetPixel32(src, srcPitch32, x + 1, y + 1, width, height);
      colorL = GetPixel32(src, srcPitch32, x + 2, y + 1, width, height);

      colorM = GetPixel32(src, srcPitch32, x - 1, y + 2, width, height);
      colorN = GetPixel32(src, srcPitch32, x,     y + 2, width, height);
      colorO = GetPixel32(src, srcPitch32, x + 1, y + 2, width, height);
      colorP = GetPixel32(src, srcPitch32, x + 2, y + 2, width, height);

      if ((colorA == colorD) && (colorB != colorC)) {
        if (((colorA == colorE) && (colorB == colorL)) ||
            ((colorA == colorC) && (colorA == colorF)
             && (colorB != colorE) && (colorB == colorJ))) {
          product = colorA;
        } else {
          product = INTERPOLATE (colorA, colorB);
        }

        if (((colorA == colorG) && (colorC == colorO)) ||
            ((colorA == colorB) && (colorA == colorH)
             && (colorG != colorC) && (colorC == colorM))) {
          product1 = colorA;
        } else {
          product1 = INTERPOLATE (colorA, colorC);
        }
        product2 = colorA;
      } else if ((colorB == colorC) && (colorA != colorD)) {
        if (((colorB == colorF) && (colorA == colorH)) ||
            ((colorB == colorE) && (colorB == colorD)
             && (colorA != colorF) && (colorA == colorI))) {
          product = colorB;
        } else {
          product = INTERPOLATE (colorA, colorB);
        }

        if (((colorC == colorH) && (colorA == colorF)) ||
            ((colorC == colorG) && (colorC == colorD)
             && (colorA != colorH) && (colorA == colorI))) {
          product1 = colorC;
        } else {
          product1 = INTERPOLATE (colorA, colorC);
        }
        product2 = colorB;
      } else if ((colorA == colorD) && (colorB == colorC)) {
        if (colorA == colorB) {
          product = colorA;
          product1 = colorA;
          product2 = colorA;
        } else {
          int r = 0;

          product1 = INTERPOLATE (colorA, colorC);
          product = INTERPOLATE (colorA, colorB);

          r +=
            GetResult1 (colorA, colorB, colorG, colorE,
                        colorI);
          r +=
            GetResult2 (colorB, colorA, colorK, colorF,
                        colorJ);
          r +=
            GetResult2 (colorB, colorA, colorH, colorN,
                           colorM);
          r +=
            GetResult1 (colorA, colorB, colorL, colorO,
                           colorP);

          if (r > 0)
            product2 = colorA;
          else if (r < 0)
            product2 = colorB;
          else {
            product2 =
              Q_INTERPOLATE (colorA, colorB, colorC,
                               colorD);
            }
        }
      } else {
        product2 = Q_INTERPOLATE (colorA, colorB, colorC, colorD);

        if ((colorA == colorC) && (colorA == colorF)
            && (colorB != colorE) && (colorB == colorJ)) {
          product = colorA;
        } else if ((colorB == colorE) && (colorB == colorD)
                   && (colorA != colorF) && (colorA == colorI)) {
          product = colorB;
        } else {
          product = INTERPOLATE (colorA, colorB);
        }

        if ((colorA == colorB) && (colorA == colorH)
            && (colorG != colorC) && (colorC == colorM)) {
          product1 = colorA;
        } else if ((colorC == colorG) && (colorC == colorD)
                   && (colorA != colorH) && (colorA == colorI)) {
          product1 = colorC;
        } else {
          product1 = INTERPOLATE (colorA, colorC);
        }
      }
      *(dP) = colorA;
      *(dP + 1) = product;
      *(dP + (dstPitch >> 2)) = product1;
      *(dP + (dstPitch >> 2) + 1) = product2;

      dP += 2;
    }                 // end of for ( x < width )
  }                   // endof: for (y < height)
}

static uint32_t Bilinear (uint32_t A, uint32_t B, uint32_t x)
{
  unsigned long areaA, areaB;
  unsigned long result;

  if (A == B)
    return A;

  areaB = (x >> 11) & 0x1f;     // reduce 16 bit fraction to 5 bits
  areaA = 0x20 - areaB;

  A = (A & redblueMask) | ((A & greenMask) << 16);
  B = (B & redblueMask) | ((B & greenMask) << 16);

  result = ((areaA * A) + (areaB * B)) >> 5;

  return (result & redblueMask) | ((result >> 16) & greenMask);
}

static uint32_t Bilinear4 (uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t x,
                         uint32_t y)
{
  unsigned long areaA, areaB, areaC, areaD;
  unsigned long result, xy;

  x = (x >> 11) & 0x1f;
  y = (y >> 11) & 0x1f;
  xy = (x * y) >> 5;

  A = (A & redblueMask) | ((A & greenMask) << 16);
  B = (B & redblueMask) | ((B & greenMask) << 16);
  C = (C & redblueMask) | ((C & greenMask) << 16);
  D = (D & redblueMask) | ((D & greenMask) << 16);

  areaA = 0x20 + xy - x - y;
  areaB = x - xy;
  areaC = y - xy;
  areaD = xy;

  result = ((areaA * A) + (areaB * B) + (areaC * C) + (areaD * D)) >> 5;

  return (result & redblueMask) | ((result >> 16) & greenMask);
}

void Scale_2xSaI (uint8_t *srcPtr, uint32_t srcPitch, uint8_t * /* deltaPtr */,
                  uint8_t *dstPtr, uint32_t dstPitch,
                  uint32_t dstWidth, uint32_t dstHeight, int width, int height)
{
  uint8_t  *dP;

  uint32_t w;
  uint32_t h;
  uint32_t dw;
  uint32_t dh;
  uint32_t hfinish;
  uint32_t wfinish;

  uint32_t Nextline = srcPitch >> 1;
  const uint16_t* src = reinterpret_cast<const uint16_t*>(srcPtr);
  int srcPitch16 = static_cast<int>(Nextline);

  wfinish = (width - 1) << 16;  // convert to fixed point
  dw = wfinish / (dstWidth - 1);
  hfinish = (height - 1) << 16; // convert to fixed point
  dh = hfinish / (dstHeight - 1);

  for (h = 0; h < hfinish; h += dh) {
    uint32_t y1, y2;

    y1 = h & 0xffff;    // fraction part of fixed point
    int srcY = static_cast<int>(h >> 16);
    dP = dstPtr;
    y2 = 0x10000 - y1;

    w = 0;

    for (; w < wfinish;) {
      uint32_t A, B, C, D;
      uint32_t E, F, G, H;
      uint32_t I, J, K, L;
      uint32_t x1, x2, a1, f1, f2;
      uint32_t product1 = 0;

      int srcX = static_cast<int>(w >> 16);

      // Safe boundary-clamped pixel access
      A = GetPixel16(src, srcPitch16, srcX,     srcY,     width, height);
      B = GetPixel16(src, srcPitch16, srcX + 1, srcY,     width, height);
      C = GetPixel16(src, srcPitch16, srcX,     srcY + 1, width, height);
      D = GetPixel16(src, srcPitch16, srcX + 1, srcY + 1, width, height);
      E = GetPixel16(src, srcPitch16, srcX,     srcY - 1, width, height);
      F = GetPixel16(src, srcPitch16, srcX + 1, srcY - 1, width, height);
      G = GetPixel16(src, srcPitch16, srcX - 1, srcY,     width, height);
      H = GetPixel16(src, srcPitch16, srcX - 1, srcY + 1, width, height);
      I = GetPixel16(src, srcPitch16, srcX + 2, srcY,     width, height);
      J = GetPixel16(src, srcPitch16, srcX + 2, srcY + 1, width, height);
      K = GetPixel16(src, srcPitch16, srcX,     srcY + 2, width, height);
      L = GetPixel16(src, srcPitch16, srcX + 1, srcY + 2, width, height);

      x1 = w & 0xffff;  // fraction part of fixed point
      x2 = 0x10000 - x1;

      /*0*/
      if (A == B && C == D && A == C)
        product1 = A;
      else /*1*/ if (A == D && B != C) {
        f1 = (x1 >> 1) + (0x10000 >> 2);
        f2 = (y1 >> 1) + (0x10000 >> 2);
        if (y1 <= f1 && A == J && A != E)       // close to B
          {
            a1 = f1 - y1;
            product1 = Bilinear (A, B, a1);
          } else if (y1 >= f1 && A == G && A != L)      // close to C
            {
              a1 = y1 - f1;
              product1 = Bilinear (A, C, a1);
            }
        else if (x1 >= f2 && A == E && A != J)  // close to B
          {
            a1 = x1 - f2;
            product1 = Bilinear (A, B, a1);
          }
        else if (x1 <= f2 && A == L && A != G)  // close to C
          {
            a1 = f2 - x1;
            product1 = Bilinear (A, C, a1);
          }
        else if (y1 >= x1)      // close to C
          {
            a1 = y1 - x1;
            product1 = Bilinear (A, C, a1);
          }
        else if (y1 <= x1)      // close to B
          {
            a1 = x1 - y1;
            product1 = Bilinear (A, B, a1);
          }
      }
      else
        /*2*/
        if (B == C && A != D)
          {
            f1 = (x1 >> 1) + (0x10000 >> 2);
            f2 = (y1 >> 1) + (0x10000 >> 2);
            if (y2 >= f1 && B == H && B != F)   // close to A
              {
                a1 = y2 - f1;
                product1 = Bilinear (B, A, a1);
              }
            else if (y2 <= f1 && B == I && B != K)      // close to D
              {
                a1 = f1 - y2;
                product1 = Bilinear (B, D, a1);
              }
            else if (x2 >= f2 && B == F && B != H)      // close to A
              {
                a1 = x2 - f2;
                product1 = Bilinear (B, A, a1);
              }
            else if (x2 <= f2 && B == K && B != I)      // close to D
              {
                a1 = f2 - x2;
                product1 = Bilinear (B, D, a1);
              }
            else if (y2 >= x1)  // close to A
              {
                a1 = y2 - x1;
                product1 = Bilinear (B, A, a1);
              }
            else if (y2 <= x1)  // close to D
              {
                a1 = x1 - y2;
                product1 = Bilinear (B, D, a1);
              }
          }
      /*3*/
        else
          {
            product1 = Bilinear4 (A, B, C, D, x1, y1);
          }

      //end First Pixel
      *(uint32_t *) dP = product1;
      dP += 2;
      w += dw;
    }
    dstPtr += dstPitch;
  }
}
