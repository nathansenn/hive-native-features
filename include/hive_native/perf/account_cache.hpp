#pragma once
/**
 * Catalogue #63 — Primary-key caching for get_account / find_account.
 *
 * Open-addressing name → account_balance cache layered over the portable
 * database. Invalidation is explicit (callers must drop entries when
 * balances mutate via adjust_balance / create paths).
 */
#include "hive_native/chain/database.hpp"
#include "hive_native/perf/flat_hash_map.hpp"
#include "hive_native/util/types.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace hive_native {
namespace perf {

/** Cached primary-key view for account balance lookups by name. */
class account_cache {
public:
   explicit account_cache(size_t cap = 64) : map_(cap) {}

   void clear() {
      map_.clear();
      hits_ = misses_ = inserts_ = invalidations_ = 0;
   }

   size_t size() const { return map_.size(); }
   uint64_t hits() const { return hits_; }
   uint64_t misses() const { return misses_; }
   uint64_t inserts() const { return inserts_; }
   uint64_t invalidations() const { return invalidations_; }

   /** Lookups that hit / (hits+misses). 0 when no finds yet. */
   double hit_rate() const {
      const uint64_t t = hits_ + misses_;
      return t == 0 ? 0.0 : double(hits_) / double(t);
   }

   /** Insert or overwrite a cached balance snapshot. */
   void put(const account_name_type& name, chain::account_balance bal) {
      map_.insert(name, bal);
      ++inserts_;
   }

   /**
    * Find cached balance. Records hit/miss counters.
    * Returns nullptr on miss (does not load from database).
    */
   const chain::account_balance* find(std::string_view name) const {
      if(auto* p = map_.find(name)) {
         ++hits_;
         return p;
      }
      ++misses_;
      return nullptr;
   }

   chain::account_balance* find_mut(std::string_view name) {
      if(auto* p = map_.find(name)) {
         ++hits_;
         return p;
      }
      ++misses_;
      return nullptr;
   }

   /** Drop a single name from the cache (e.g. after adjust_balance). */
   bool invalidate(std::string_view name) {
      if(map_.erase(name)) {
         ++invalidations_;
         return true;
      }
      return false;
   }

private:
   mutable flat_hash_map<chain::account_balance> map_;
   mutable uint64_t hits_ = 0;
   mutable uint64_t misses_ = 0;
   uint64_t inserts_ = 0;
   uint64_t invalidations_ = 0;
};

} // namespace perf
} // namespace hive_native
