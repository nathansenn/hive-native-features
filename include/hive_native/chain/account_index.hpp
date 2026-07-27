#pragma once
/**
 * Catalogue #63 — AccountIndex: get_account-style wrapper over database
 * with optional primary-key cache (perf::account_cache / flat_hash_map).
 *
 * Prefer this over mutating database internals so existing tests that
 * use database directly remain unchanged.
 *
 * Semantics:
 *  - find / get_balance: cache-aside; miss loads from database and fills cache
 *  - create: writes through to database and seeds cache
 *  - adjust_balance: writes through to database and invalidates cache entry
 */
#include "hive_native/chain/database.hpp"
#include "hive_native/perf/account_cache.hpp"
#include "hive_native/util/types.hpp"
#include <optional>
#include <string_view>

namespace hive_native {
namespace chain {

class account_index {
public:
   explicit account_index(database& db, bool enable_cache = true)
      : db_(db), cache_enabled_(enable_cache) {}

   database&       db()       { return db_; }
   const database& db() const { return db_; }

   bool cache_enabled() const { return cache_enabled_; }
   void set_cache_enabled(bool on) {
      cache_enabled_ = on;
      if(!on) cache_.clear();
   }

   perf::account_cache&       cache()       { return cache_; }
   const perf::account_cache& cache() const { return cache_; }

   void create(const account_name_type& name, share_type hive = 0, share_type hbd = 0) {
      db_.create_account(name, hive, hbd);
      if(cache_enabled_)
         cache_.put(name, account_balance{hive, hbd});
   }

   bool exists(const account_name_type& name) const {
      return find(name).has_value();
   }

   /**
    * get_account / find_account style primary-key lookup.
    * Cache hit returns snapshot; miss loads from database and populates cache.
    */
   std::optional<account_balance> find(std::string_view name) const {
      if(cache_enabled_) {
         if(const auto* hit = cache_.find(name))
            return *hit;
      }
      auto it = db_.balances.find(std::string(name));
      if(it == db_.balances.end())
         return std::nullopt;
      if(cache_enabled_)
         cache_.put(std::string(name), it->second);
      return it->second;
   }

   share_type get_balance(const account_name_type& name, asset_symbol sym) const {
      auto bal = find(name);
      if(!bal) throw protocol_error("account missing");
      return sym == asset_symbol::HIVE ? bal->hive : bal->hbd;
   }

   /**
    * Mutating path: always goes through database, then invalidates the
    * primary-key cache entry so the next find reloads fresh balances.
    */
   void adjust_balance(const account_name_type& name, asset delta) {
      db_.adjust_balance(name, delta);
      if(cache_enabled_)
         cache_.invalidate(name);
   }

   void clear_cache() { cache_.clear(); }

private:
   database&           db_;
   bool                cache_enabled_ = true;
   mutable perf::account_cache cache_;
};

} // namespace chain
} // namespace hive_native
