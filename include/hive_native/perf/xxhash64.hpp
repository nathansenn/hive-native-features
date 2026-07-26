#pragma once
/** #32 #84 — portable xxHash64 (non-crypto integrity). Task: perf-1000 */
#include <cstdint>
#include <cstring>
#include <string_view>

namespace hive_native {
namespace perf {

inline uint64_t rotl64(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }

inline uint64_t xxhash64(const void* data, size_t len, uint64_t seed = 0) {
   static constexpr uint64_t PRIME1 = 11400714785074694791ULL;
   static constexpr uint64_t PRIME2 = 14029467366897019727ULL;
   static constexpr uint64_t PRIME3 = 1609587929392839161ULL;
   static constexpr uint64_t PRIME4 = 9650029242287828579ULL;
   static constexpr uint64_t PRIME5 = 2870177450012600261ULL;

   const auto* p = static_cast<const uint8_t*>(data);
   const auto* const end = p + len;
   uint64_t h;

   if(len >= 32) {
      const auto* const limit = end - 32;
      uint64_t v1 = seed + PRIME1 + PRIME2;
      uint64_t v2 = seed + PRIME2;
      uint64_t v3 = seed + 0;
      uint64_t v4 = seed - PRIME1;
      do {
         auto round = [&](uint64_t acc, uint64_t input) {
            acc += input * PRIME2;
            acc = rotl64(acc, 31);
            acc *= PRIME1;
            return acc;
         };
         uint64_t i1, i2, i3, i4;
         std::memcpy(&i1, p, 8); p += 8;
         std::memcpy(&i2, p, 8); p += 8;
         std::memcpy(&i3, p, 8); p += 8;
         std::memcpy(&i4, p, 8); p += 8;
         v1 = round(v1, i1); v2 = round(v2, i2);
         v3 = round(v3, i3); v4 = round(v4, i4);
      } while(p <= limit);
      h = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
      auto merge = [&](uint64_t acc, uint64_t v) {
         v *= PRIME2; v = rotl64(v, 31); v *= PRIME1;
         acc ^= v; acc = acc * PRIME1 + PRIME4;
         return acc;
      };
      h = merge(h, v1); h = merge(h, v2); h = merge(h, v3); h = merge(h, v4);
   } else {
      h = seed + PRIME5;
   }
   h += uint64_t(len);
   while(p + 8 <= end) {
      uint64_t k; std::memcpy(&k, p, 8); p += 8;
      k *= PRIME2; k = rotl64(k, 31); k *= PRIME1;
      h ^= k; h = rotl64(h, 27) * PRIME1 + PRIME4;
   }
   if(p + 4 <= end) {
      uint32_t k; std::memcpy(&k, p, 4); p += 4;
      h ^= uint64_t(k) * PRIME1; h = rotl64(h, 23) * PRIME2 + PRIME3;
   }
   while(p < end) {
      h ^= (*p++) * PRIME5; h = rotl64(h, 11) * PRIME1;
   }
   h ^= h >> 33; h *= PRIME2; h ^= h >> 29; h *= PRIME3; h ^= h >> 32;
   return h;
}

inline uint64_t xxhash64(std::string_view s, uint64_t seed = 0) {
   return xxhash64(s.data(), s.size(), seed);
}

} // namespace perf
} // namespace hive_native
