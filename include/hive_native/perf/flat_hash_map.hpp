#pragma once
/** #4 #63 #424 — open-addressing string→T map for hot account lookups. */
#include "hive_native/perf/xxhash64.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hive_native {
namespace perf {

template<typename T>
class flat_hash_map {
public:
   explicit flat_hash_map(size_t cap = 16) { rehash(cap < 8 ? 8 : cap); }

   void clear() {
      for(auto& s : slots_) { s.st = empty; }
      size_ = 0;
   }

   size_t size() const { return size_; }
   size_t capacity() const { return slots_.size(); }

   bool insert(std::string key, T value) {
      if(size_ * 10 >= slots_.size() * 7) rehash(slots_.size() * 2);
      const size_t mask = slots_.size() - 1;
      size_t i = xxhash64(key) & mask;
      for(;;) {
         auto& s = slots_[i];
         if(s.st == empty || s.st == tomb) {
            s.st = full; s.key = std::move(key); s.value = std::move(value);
            ++size_;
            return true;
         }
         if(s.st == full && s.key == key) {
            s.value = std::move(value);
            return false; // updated
         }
         i = (i + 1) & mask;
      }
   }

   T* find(std::string_view key) {
      if(slots_.empty()) return nullptr;
      const size_t mask = slots_.size() - 1;
      size_t i = xxhash64(key) & mask;
      for(size_t n = 0; n < slots_.size(); ++n) {
         auto& s = slots_[i];
         if(s.st == empty) return nullptr;
         if(s.st == full && s.key == key) return &s.value;
         i = (i + 1) & mask;
      }
      return nullptr;
   }

   const T* find(std::string_view key) const {
      return const_cast<flat_hash_map*>(this)->find(key);
   }

   bool erase(std::string_view key) {
      if(slots_.empty()) return false;
      const size_t mask = slots_.size() - 1;
      size_t i = xxhash64(key) & mask;
      for(size_t n = 0; n < slots_.size(); ++n) {
         auto& s = slots_[i];
         if(s.st == empty) return false;
         if(s.st == full && s.key == key) {
            s.st = tomb; s.key.clear();
            --size_;
            return true;
         }
         i = (i + 1) & mask;
      }
      return false;
   }

private:
   enum state : uint8_t { empty = 0, full = 1, tomb = 2 };
   struct slot { state st = empty; std::string key; T value; };

   void rehash(size_t n) {
      size_t cap = 1;
      while(cap < n) cap <<= 1;
      std::vector<slot> old = std::move(slots_);
      slots_.assign(cap, slot{});
      size_ = 0;
      for(auto& s : old) {
         if(s.st == full) insert(std::move(s.key), std::move(s.value));
      }
   }

   std::vector<slot> slots_;
   size_t size_ = 0;
};

} // namespace perf
} // namespace hive_native
