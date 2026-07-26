#pragma once
/** #204 — compact bloom for is_known_transaction style sets. */
#include "hive_native/perf/xxhash64.hpp"
#include <cstdint>
#include <string_view>
#include <vector>

namespace hive_native {
namespace perf {

class bloom_filter {
public:
   bloom_filter(size_t bits = 1 << 20, size_t hashes = 4)
      : bits_(bits < 64 ? 64 : bits), hashes_(hashes < 1 ? 1 : hashes),
        data_((bits_ + 63) / 64, 0) {}

   void clear() { std::fill(data_.begin(), data_.end(), 0); }

   void add(std::string_view key) {
      for(size_t i = 0; i < hashes_; ++i) {
         uint64_t h = xxhash64(key, 0x9e3779b97f4a7c15ULL + i * 0x100000001b3ULL);
         size_t bit = size_t(h % bits_);
         data_[bit >> 6] |= (uint64_t(1) << (bit & 63));
      }
   }

   bool maybe_contains(std::string_view key) const {
      for(size_t i = 0; i < hashes_; ++i) {
         uint64_t h = xxhash64(key, 0x9e3779b97f4a7c15ULL + i * 0x100000001b3ULL);
         size_t bit = size_t(h % bits_);
         if((data_[bit >> 6] & (uint64_t(1) << (bit & 63))) == 0) return false;
      }
      return true;
   }

   size_t bit_size() const { return bits_; }

private:
   size_t bits_;
   size_t hashes_;
   std::vector<uint64_t> data_;
};

} // namespace perf
} // namespace hive_native
