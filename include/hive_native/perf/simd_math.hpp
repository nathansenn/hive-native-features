#pragma once
/** #209 — portable vote-weight / arithmetic batch helper (scalar + optional NEON/SSE2).
 *  Task: perf-03-simd / catalogue #209
 *
 *  batch_mul_u64 / batch_add_u64 compute element-wise operations with wrapping
 *  uint64_t arithmetic. SIMD paths accelerate the common case; scalar fallback
 *  is always correct and used for remainders and non-SIMD builds.
 */
#include <cstddef>
#include <cstdint>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#  include <arm_neon.h>
#  define HIVE_NATIVE_SIMD_NEON 1
#elif defined(__SSE2__)
#  include <emmintrin.h>
#  define HIVE_NATIVE_SIMD_SSE2 1
#endif

namespace hive_native {
namespace perf {

/** Scalar reference: out[i] = a[i] * b[i] (wrapping). Always correct. */
inline void batch_mul_u64_scalar(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n) {
   for(size_t i = 0; i < n; ++i)
      out[i] = a[i] * b[i];
}

/** Scalar reference: out[i] = a[i] + b[i] (wrapping). Always correct. */
inline void batch_add_u64_scalar(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n) {
   for(size_t i = 0; i < n; ++i)
      out[i] = a[i] + b[i];
}

/**
 * Element-wise product of n uint64_t lanes (wrapping).
 * Uses NEON/SSE2 when available for loads/stores + add-style vectorization
 * patterns; 64-bit multiply is performed per-lane (no portable vector mullo
 * on SSE2/NEON). Remainder lanes always use the scalar path.
 */
inline void batch_mul_u64(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n) {
   size_t i = 0;

#if defined(HIVE_NATIVE_SIMD_NEON)
   // 2-wide: load pairs, scalar mul (no vmulq_u64), store.
   for(; i + 2 <= n; i += 2) {
      uint64x2_t va = vld1q_u64(a + i);
      uint64x2_t vb = vld1q_u64(b + i);
      uint64_t r0 = vgetq_lane_u64(va, 0) * vgetq_lane_u64(vb, 0);
      uint64_t r1 = vgetq_lane_u64(va, 1) * vgetq_lane_u64(vb, 1);
      uint64x2_t vr = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1));
      vst1q_u64(out + i, vr);
   }
#elif defined(HIVE_NATIVE_SIMD_SSE2)
   // 2-wide: load pairs, scalar mul (no _mm_mullo_epi64 until later ISAs), store.
   for(; i + 2 <= n; i += 2) {
      __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
      __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
      // Extract as uint64_t for portable wrapping mul.
#if defined(__x86_64__) || defined(_M_X64)
      uint64_t a0 = static_cast<uint64_t>(_mm_cvtsi128_si64(va));
      uint64_t a1 = static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(va, 8)));
      uint64_t b0 = static_cast<uint64_t>(_mm_cvtsi128_si64(vb));
      uint64_t b1 = static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(vb, 8)));
#else
      alignas(16) uint64_t ta[2], tb[2];
      _mm_store_si128(reinterpret_cast<__m128i*>(ta), va);
      _mm_store_si128(reinterpret_cast<__m128i*>(tb), vb);
      uint64_t a0 = ta[0], a1 = ta[1], b0 = tb[0], b1 = tb[1];
#endif
      __m128i vr = _mm_set_epi64x(static_cast<long long>(a1 * b1),
                                  static_cast<long long>(a0 * b0));
      _mm_storeu_si128(reinterpret_cast<__m128i*>(out + i), vr);
   }
#endif

   for(; i < n; ++i)
      out[i] = a[i] * b[i];
}

/**
 * Element-wise sum of n uint64_t lanes (wrapping).
 * NEON: vaddq_u64; SSE2: _mm_add_epi64; else pure scalar.
 */
inline void batch_add_u64(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n) {
   size_t i = 0;

#if defined(HIVE_NATIVE_SIMD_NEON)
   for(; i + 2 <= n; i += 2) {
      uint64x2_t va = vld1q_u64(a + i);
      uint64x2_t vb = vld1q_u64(b + i);
      vst1q_u64(out + i, vaddq_u64(va, vb));
   }
#elif defined(HIVE_NATIVE_SIMD_SSE2)
   for(; i + 2 <= n; i += 2) {
      __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
      __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
      _mm_storeu_si128(reinterpret_cast<__m128i*>(out + i), _mm_add_epi64(va, vb));
   }
#endif

   for(; i < n; ++i)
      out[i] = a[i] + b[i];
}

/** Which backend the accelerated loops compile to (for diagnostics / docs). */
inline const char* simd_math_backend() {
#if defined(HIVE_NATIVE_SIMD_NEON)
   return "neon";
#elif defined(HIVE_NATIVE_SIMD_SSE2)
   return "sse2";
#else
   return "scalar";
#endif
}

} // namespace perf
} // namespace hive_native
