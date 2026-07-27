/**
 * Catalogue #209 — simd_math batch_mul_u64 / batch_add_u64 vs scalar.
 * Sizes: n = 1, 7, 64, 1000 (odd, small, power-of-two, large).
 */
#include "hive_native/perf/simd_math.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

static int g_failed = 0, g_passed = 0;
#define CHECK(c)                                                                 \
   do {                                                                          \
      if(c)                                                                      \
         ++g_passed;                                                             \
      else {                                                                     \
         ++g_failed;                                                             \
         std::cerr << "FAIL " << __LINE__ << " " #c "\n";                        \
      }                                                                          \
   } while(0)

static void fill_inputs(std::vector<uint64_t>& a, std::vector<uint64_t>& b, size_t n) {
   a.resize(n);
   b.resize(n);
   for(size_t i = 0; i < n; ++i) {
      // Mix of small, large, and wrapping-prone values.
      a[i] = 0x9E3779B97F4A7C15ULL * (i + 1) + 0xDEADBEEFCAFEBABEULL;
      b[i] = 0xC2B2AE3D27D4EB4FULL * (i + 3) + 0x1234567890ABCDEFULL;
      if(i % 11 == 0) a[i] = UINT64_MAX - i;
      if(i % 13 == 0) b[i] = UINT64_MAX / 2 + i;
   }
}

static void verify_size(size_t n) {
   std::vector<uint64_t> a, b;
   fill_inputs(a, b, n);

   std::vector<uint64_t> mul_simd(n), mul_ref(n);
   std::vector<uint64_t> add_simd(n), add_ref(n);

   hive_native::perf::batch_mul_u64(a.data(), b.data(), mul_simd.data(), n);
   hive_native::perf::batch_mul_u64_scalar(a.data(), b.data(), mul_ref.data(), n);
   hive_native::perf::batch_add_u64(a.data(), b.data(), add_simd.data(), n);
   hive_native::perf::batch_add_u64_scalar(a.data(), b.data(), add_ref.data(), n);

   bool mul_ok = true, add_ok = true;
   for(size_t i = 0; i < n; ++i) {
      if(mul_simd[i] != mul_ref[i]) mul_ok = false;
      if(add_simd[i] != add_ref[i]) add_ok = false;
   }
   CHECK(mul_ok);
   CHECK(add_ok);

   // Null-length must be a no-op (does not write).
   if(n > 0) {
      uint64_t sentinel = 0xA5A5A5A5A5A5A5A5ULL;
      std::vector<uint64_t> empty_out(1, sentinel);
      hive_native::perf::batch_mul_u64(a.data(), b.data(), empty_out.data(), 0);
      hive_native::perf::batch_add_u64(a.data(), b.data(), empty_out.data(), 0);
      CHECK(empty_out[0] == sentinel);
   }
}

int main() {
   const size_t sizes[] = {1, 7, 64, 1000};
   for(size_t n : sizes)
      verify_size(n);

   // Backend string is non-empty (diagnostic only).
   const char* be = hive_native::perf::simd_math_backend();
   CHECK(be != nullptr && be[0] != '\0');

   std::cout << "simd_math_backend=" << be
             << " passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
