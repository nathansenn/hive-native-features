#pragma once
/** #72 #219 — bump-pointer arena for temporary apply / full_transaction data. */
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

namespace hive_native {
namespace perf {

class arena {
public:
   explicit arena(size_t chunk = 64 * 1024) : chunk_size_(chunk) { grow(); }

   void* allocate(size_t n, size_t align = alignof(std::max_align_t)) {
      size_t pad = (align - (offset_ % align)) % align;
      if(offset_ + pad + n > chunks_.back().size()) {
         size_t need = n + align;
         if(need > chunk_size_) chunk_size_ = need;
         grow();
         pad = (align - (offset_ % align)) % align;
      }
      offset_ += pad;
      void* p = chunks_.back().data() + offset_;
      offset_ += n;
      return p;
   }

   template<typename T, typename... Args>
   T* create(Args&&... args) {
      void* mem = allocate(sizeof(T), alignof(T));
      return new (mem) T(std::forward<Args>(args)...);
   }

   /** Reset for next block — does not free OS memory. */
   void reset() {
      if(chunks_.empty()) return;
      // keep first chunk, drop extras to bound growth
      if(chunks_.size() > 1) chunks_.resize(1);
      offset_ = 0;
   }

   size_t bytes_used() const {
      if(chunks_.empty()) return 0;
      return (chunks_.size() - 1) * chunk_size_ + offset_;
   }

   size_t chunks() const { return chunks_.size(); }

private:
   void grow() {
      chunks_.emplace_back(chunk_size_);
      offset_ = 0;
   }

   size_t chunk_size_;
   size_t offset_ = 0;
   std::vector<std::vector<uint8_t>> chunks_;
};

} // namespace perf
} // namespace hive_native
