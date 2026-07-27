/**
 * Catalogue #204 — known_tx_set: bloom first filter + exact set confirms.
 */
#include "hive_native/perf/known_tx_set.hpp"
#include <iostream>
#include <string>

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

static void test_empty() {
   hive_native::perf::known_tx_set k(1 << 14, 4);
   CHECK(k.size() == 0);
   CHECK(!k.maybe_seen("never"));
   CHECK(!k.definitely_seen("never"));
}

static void test_add_and_seen() {
   hive_native::perf::known_tx_set k(1 << 14, 4);
   CHECK(k.add("txid_aaa"));
   CHECK(k.add("txid_bbb"));
   CHECK(!k.add("txid_aaa")); // duplicate

   CHECK(k.size() == 2);
   CHECK(k.maybe_seen("txid_aaa"));
   CHECK(k.maybe_seen("txid_bbb"));
   CHECK(k.definitely_seen("txid_aaa"));
   CHECK(k.definitely_seen("txid_bbb"));

   CHECK(!k.definitely_seen("txid_ccc"));
   // Novel key: bloom should usually miss; exact must miss.
   CHECK(!k.definitely_seen("definitely-not-inserted-zzzz"));
   CHECK(!k.maybe_seen("definitely-not-inserted-zzzz"));
}

static void test_string_view_add() {
   hive_native::perf::known_tx_set k(1 << 12, 3);
   std::string_view sv = "sv_txid_01";
   CHECK(k.add(sv));
   CHECK(k.definitely_seen(sv));
   CHECK(k.maybe_seen(sv));
}

static void test_clear() {
   hive_native::perf::known_tx_set k(1 << 12, 3);
   k.add("x1");
   k.add("x2");
   CHECK(k.size() == 2);
   k.clear();
   CHECK(k.size() == 0);
   CHECK(!k.definitely_seen("x1"));
   CHECK(!k.maybe_seen("x1"));
}

static void test_bulk_exact_invariant() {
   // After many inserts, every added txid must be definitely_seen;
   // every non-added id that blooms miss is definitely not present.
   hive_native::perf::known_tx_set k(1 << 16, 4);
   for(int i = 0; i < 500; ++i)
      k.add("tx" + std::to_string(i));
   CHECK(k.size() == 500);
   for(int i = 0; i < 500; ++i) {
      auto id = "tx" + std::to_string(i);
      CHECK(k.maybe_seen(id));
      CHECK(k.definitely_seen(id));
   }
   for(int i = 1000; i < 1100; ++i) {
      auto id = "tx" + std::to_string(i);
      CHECK(!k.definitely_seen(id));
      // If bloom says no, exact must also say no (no false negatives).
      if(!k.maybe_seen(id))
         CHECK(!k.definitely_seen(id));
   }
}

int main() {
   test_empty();
   test_add_and_seen();
   test_string_view_add();
   test_clear();
   test_bulk_exact_invariant();
   std::cout << "known_tx_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
