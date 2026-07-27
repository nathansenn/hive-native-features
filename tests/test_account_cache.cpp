/**
 * Catalogue #63 — Primary-key account cache tests.
 * Hit rate after repeated finds; invalidation after balance change.
 */
#include "hive_native/chain/account_index.hpp"
#include "hive_native/chain/database.hpp"
#include "hive_native/perf/account_cache.hpp"
#include <iostream>

using hive_native::asset;
using hive_native::asset_symbol;
using hive_native::chain::account_index;
using hive_native::chain::database;
using hive_native::perf::account_cache;

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

static void test_hit_rate_repeated_finds() {
   account_cache c;
   c.put("alice", {1'000'000, 0});
   // First find: hit
   CHECK(c.find("alice") != nullptr);
   CHECK(c.hits() == 1 && c.misses() == 0);
   // Nine more finds
   for(int i = 0; i < 9; ++i)
      CHECK(c.find("alice") != nullptr);
   CHECK(c.hits() == 10);
   CHECK(c.misses() == 0);
   CHECK(c.hit_rate() == 1.0);

   // Misses pull rate down
   CHECK(c.find("ghost") == nullptr);
   CHECK(c.misses() == 1);
   CHECK(c.hit_rate() > 0.90); // 10/11
}

static void test_invalidate() {
   account_cache c;
   c.put("alice", {100, 0});
   CHECK(c.invalidate("alice"));
   CHECK(c.find("alice") == nullptr);
   CHECK(c.invalidations() == 1);
   CHECK(!c.invalidate("alice")); // already gone
}

static void test_account_index_find_hit_rate() {
   database db;
   account_index idx(db);
   idx.create("alice", 1'000'000, 100);
   idx.create("bob", 500'000, 0);

   // create seeds cache — first find should hit
   auto a0 = idx.find("alice");
   CHECK(a0.has_value() && a0->hive == 1'000'000);
   CHECK(idx.cache().hits() >= 1);

   const auto hits_before = idx.cache().hits();
   for(int i = 0; i < 100; ++i) {
      auto a = idx.find("alice");
      CHECK(a && a->hive == 1'000'000);
   }
   CHECK(idx.cache().hits() >= hits_before + 100);
   CHECK(idx.cache().hit_rate() > 0.95);
}

static void test_account_index_invalidation_on_adjust() {
   database db;
   account_index idx(db);
   idx.create("alice", 1'000, 0);
   idx.create("bob", 0, 0);

   CHECK(idx.get_balance("alice", asset_symbol::HIVE) == 1'000);

   // Mutate via index — must invalidate
   idx.adjust_balance("alice", asset{-100, asset_symbol::HIVE});
   idx.adjust_balance("bob", asset{100, asset_symbol::HIVE});

   auto a = idx.find("alice");
   CHECK(a.has_value());
   CHECK(a->hive == 900);
   CHECK(idx.get_balance("bob", asset_symbol::HIVE) == 100);
   CHECK(idx.cache().invalidations() >= 2);

   // Direct database mutation is not observed until invalidate —
   // writers must use account_index::adjust_balance.
   db.adjust_balance("alice", asset{-50, asset_symbol::HIVE});
   auto maybe_stale = idx.cache().find("alice");
   if(maybe_stale) {
      idx.cache().invalidate("alice");
   }
   auto fresh = idx.find("alice");
   CHECK(fresh && fresh->hive == 850);
   CHECK(idx.cache().invalidations() >= 1);
}

static void test_cache_disabled() {
   database db;
   account_index idx(db, /*enable_cache=*/false);
   idx.create("alice", 42, 0);
   CHECK(idx.cache().size() == 0);
   auto a = idx.find("alice");
   CHECK(a && a->hive == 42);
   CHECK(idx.cache().size() == 0); // still disabled
   idx.adjust_balance("alice", asset{8, asset_symbol::HIVE});
   CHECK(idx.get_balance("alice", asset_symbol::HIVE) == 50);
}

static void test_miss_then_fill() {
   database db;
   db.create_account("carol", 7, 3); // bypass index — cache cold
   account_index idx(db);
   CHECK(idx.cache().size() == 0);
   auto c = idx.find("carol");
   CHECK(c && c->hive == 7 && c->hbd == 3);
   CHECK(idx.cache().size() == 1);
   // Second find hits
   const auto misses = idx.cache().misses();
   const auto hits = idx.cache().hits();
   auto c2 = idx.find("carol");
   CHECK(c2 && c2->hive == 7);
   CHECK(idx.cache().hits() == hits + 1);
   CHECK(idx.cache().misses() == misses); // no new miss
}

int main() {
   test_hit_rate_repeated_finds();
   test_invalidate();
   test_account_index_find_hit_rate();
   test_account_index_invalidation_on_adjust();
   test_cache_disabled();
   test_miss_then_fill();
   std::cout << "account_cache_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
