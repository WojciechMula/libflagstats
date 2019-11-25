#ifndef POPCNT
#define POPCNT
#include <x86intrin.h>

#if POSPOPCNT_SIMD_VERSION >= 5
static inline void CSA256(__m256i* h, __m256i* l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = _mm256_xor_si256(a, b);
  *h = _mm256_or_si256(_mm256_and_si256(a, b), _mm256_and_si256(u, c));
  *l = _mm256_xor_si256(u, c);
}

static inline __m256i popcnt256(__m256i v)
{
  __m256i lookup1 = _mm256_setr_epi8(
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8,
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8
  );

  __m256i lookup2 = _mm256_setr_epi8(
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0,
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0
  );

  __m256i low_mask = _mm256_set1_epi8(0x0f);
  __m256i lo = _mm256_and_si256(v, low_mask);
  __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_mask);
  __m256i popcnt1 = _mm256_shuffle_epi8(lookup1, lo);
  __m256i popcnt2 = _mm256_shuffle_epi8(lookup2, hi);

  return _mm256_sad_epu8(popcnt1, popcnt2);
}

static inline uint64_t popcnt_avx2(const __m256i* data, uint64_t size) {
    __m256i cnt = _mm256_setzero_si256();
    __m256i ones = _mm256_setzero_si256();
    __m256i twos = _mm256_setzero_si256();
    __m256i fours = _mm256_setzero_si256();
    __m256i eights = _mm256_setzero_si256();
    __m256i sixteens = _mm256_setzero_si256();
    __m256i twosA, twosB, foursA, foursB, eightsA, eightsB;

    uint64_t i = 0;
    uint64_t limit = size - size % 16;
    uint64_t* cnt64;

    for (/**/; i < limit; i += 16) {
        CSA256(&twosA, &ones, ones, data[i+0], data[i+1]);
        CSA256(&twosB, &ones, ones, data[i+2], data[i+3]);
        CSA256(&foursA, &twos, twos, twosA, twosB);
        CSA256(&twosA, &ones, ones, data[i+4], data[i+5]);
        CSA256(&twosB, &ones, ones, data[i+6], data[i+7]);
        CSA256(&foursB, &twos, twos, twosA, twosB);
        CSA256(&eightsA, &fours, fours, foursA, foursB);
        CSA256(&twosA, &ones, ones, data[i+8], data[i+9]);
        CSA256(&twosB, &ones, ones, data[i+10], data[i+11]);
        CSA256(&foursA, &twos, twos, twosA, twosB);
        CSA256(&twosA, &ones, ones, data[i+12], data[i+13]);
        CSA256(&twosB, &ones, ones, data[i+14], data[i+15]);
        CSA256(&foursB, &twos, twos, twosA, twosB);
        CSA256(&eightsB, &fours, fours, foursA, foursB);
        CSA256(&sixteens, &eights, eights, eightsA, eightsB);

        cnt = _mm256_add_epi64(cnt, popcnt256(sixteens));
    }

    cnt = _mm256_slli_epi64(cnt, 4);
    cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(eights), 3));
    cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(fours), 2));
    cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(twos), 1));
    cnt = _mm256_add_epi64(cnt, popcnt256(ones));

    for(/**/; i < limit; ++i)
        cnt = _mm256_add_epi64(cnt, popcnt256(data[i]));

    cnt64 = (uint64_t*)&cnt;

    return cnt64[0] +
            cnt64[1] +
            cnt64[2] +
            cnt64[3];
}

static __m256i avx2_popcount(const __m256i vec) {
    const __m256i lookup = _mm256_setr_epi8(
        /* 0 */ 0, /* 1 */ 1, /* 2 */ 1, /* 3 */ 2,
        /* 4 */ 1, /* 5 */ 2, /* 6 */ 2, /* 7 */ 3,
        /* 8 */ 1, /* 9 */ 2, /* a */ 2, /* b */ 3,
        /* c */ 2, /* d */ 3, /* e */ 3, /* f */ 4,

        /* 0 */ 0, /* 1 */ 1, /* 2 */ 1, /* 3 */ 2,
        /* 4 */ 1, /* 5 */ 2, /* 6 */ 2, /* 7 */ 3,
        /* 8 */ 1, /* 9 */ 2, /* a */ 2, /* b */ 3,
        /* c */ 2, /* d */ 3, /* e */ 3, /* f */ 4
    );

    const __m256i low_mask = _mm256_set1_epi8(0x0f);

    const __m256i lo  = _mm256_and_si256(vec, low_mask);
    const __m256i hi  = _mm256_and_si256(_mm256_srli_epi16(vec, 4), low_mask);
    const __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo);
    const __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi);

    return _mm256_add_epi8(popcnt1, popcnt2);
}

static uint64_t avx2_sum_epu64(const __m256i v) {
    return _mm256_extract_epi64(v, 0)
            + _mm256_extract_epi64(v, 1)
            + _mm256_extract_epi64(v, 2)
            + _mm256_extract_epi64(v, 3);
}
#endif

#if POSPOPCNT_SIMD_VERSION >= 6
static __m256i avx512_popcount2(const __m512i v)
{
    const __m256i lo = _mm512_extracti64x4_epi64(v, 0);
    const __m256i hi = _mm512_extracti64x4_epi64(v, 1);
    const __m256i s  = _mm256_add_epi8(avx2_popcount(lo), avx2_popcount(hi));

    return _mm256_sad_epu8(s, _mm256_setzero_si256());
}

static void CSA(__m512i* h, __m512i* l, __m512i a, __m512i b, __m512i c) {
  *l = _mm512_ternarylogic_epi32(c, b, a, 0x96);
  *h = _mm512_ternarylogic_epi32(c, b, a, 0xe8);
}


static uint64_t popcnt_harley_seal(const __m512i* data, const uint64_t size)
{
  __m256i total     = _mm256_setzero_si256();
  __m512i ones      = _mm512_setzero_si512();
  __m512i twos      = _mm512_setzero_si512();
  __m512i fours     = _mm512_setzero_si512();
  __m512i eights    = _mm512_setzero_si512();
  __m512i sixteens  = _mm512_setzero_si512();
  __m512i twosA, twosB, foursA, foursB, eightsA, eightsB;

  const uint64_t limit = size - size % 16;
  uint64_t i = 0;

  for(; i < limit; i += 16)
  {
    CSA(&twosA, &ones, ones, _mm512_loadu_si512(data+i+0), _mm512_loadu_si512(data+i+1));
    CSA(&twosB, &ones, ones, _mm512_loadu_si512(data+i+2), _mm512_loadu_si512(data+i+3));
    CSA(&foursA, &twos, twos, twosA, twosB);
    CSA(&twosA, &ones, ones, _mm512_loadu_si512(data+i+4), _mm512_loadu_si512(data+i+5));
    CSA(&twosB, &ones, ones, _mm512_loadu_si512(data+i+6), _mm512_loadu_si512(data+i+7));
    CSA(&foursB, &twos, twos, twosA, twosB);
    CSA(&eightsA,&fours, fours, foursA, foursB);
    CSA(&twosA, &ones, ones, _mm512_loadu_si512(data+i+8), _mm512_loadu_si512(data+i+9));
    CSA(&twosB, &ones, ones, _mm512_loadu_si512(data+i+10), _mm512_loadu_si512(data+i+11));
    CSA(&foursA, &twos, twos, twosA, twosB);
    CSA(&twosA, &ones, ones, _mm512_loadu_si512(data+i+12), _mm512_loadu_si512(data+i+13));
    CSA(&twosB, &ones, ones, _mm512_loadu_si512(data+i+14), _mm512_loadu_si512(data+i+15));
    CSA(&foursB, &twos, twos, twosA, twosB);
    CSA(&eightsB, &fours, fours, foursA, foursB);
    CSA(&sixteens, &eights, eights, eightsA, eightsB);

    total = _mm256_add_epi64(total, avx512_popcount2(sixteens));
  }

  total = _mm256_slli_epi64(total, 4);     // * 16
  total = _mm256_add_epi64(total, _mm256_slli_epi64(avx512_popcount2(eights), 3)); // += 8 * ...
  total = _mm256_add_epi64(total, _mm256_slli_epi64(avx512_popcount2(fours),  2)); // += 4 * ...
  total = _mm256_add_epi64(total, _mm256_slli_epi64(avx512_popcount2(twos),   1)); // += 2 * ...
  total = _mm256_add_epi64(total, avx512_popcount2(ones));

  for(; i < size; i++) {
    total = _mm256_add_epi64(total, avx512_popcount2(_mm512_loadu_si512(data+i)));
  }


  return avx2_sum_epu64(total);
}
#endif

#endif
