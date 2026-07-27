/**
 * #301 — compact-block relay portable unit tests.
 */
#include "hive_native/perf/compact_block.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace hive_native;
using namespace hive_native::perf;

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

static void test_short_tx_id_from_sha256() {
   // FIPS 180-4 SHA-256("abc") = ba7816bf8f01cfea...
   auto dig = sha256(std::string("abc"));
   auto sid = make_short_tx_id(dig);
   CHECK(sid.size() == 6);
   CHECK(sid[0] == 0xba);
   CHECK(sid[1] == 0x78);
   CHECK(sid[2] == 0x16);
   CHECK(sid[3] == 0xbf);
   CHECK(sid[4] == 0x8f);
   CHECK(sid[5] == 0x01);

   // From raw bytes: short_id(tx) == first 6 of sha256(tx)
   auto sid2 = make_short_tx_id(std::string_view("abc"));
   CHECK(sid2 == sid);
}

static void test_reconstruct_full_hit() {
   std::vector<std::string> body = {
      "tx-alice-pays-bob",
      "tx-carol-vote",
      "tx-dave-custom",
   };
   sha256_t hdr{};
   hdr[0] = 0x11;

   auto cb = make_compact_block(hdr, body); // no prefills
   CHECK(cb.short_ids.size() == 3);
   CHECK(cb.prefilled.empty());
   CHECK(cb.tx_count() == 3);
   CHECK(cb.header_hash[0] == 0x11);

   mempool_map pool;
   for(const auto& tx : body) pool.emplace(make_short_tx_id(tx), tx);

   auto r = reconstruct(cb, pool);
   CHECK(r.ok());
   CHECK(r.missing.empty());
   CHECK(r.txs.size() == 3);
   CHECK(r.txs[0] == body[0]);
   CHECK(r.txs[1] == body[1]);
   CHECK(r.txs[2] == body[2]);

   auto opt = r.full_list();
   CHECK(opt.has_value());
   CHECK(opt->size() == 3);
}

static void test_reconstruct_with_prefilled() {
   std::vector<std::string> body = {
      "coinbase-or-witness",
      "tx-known-1",
      "tx-known-2",
      "tx-priority",
   };
   // Prefill first (often coinbase) and last (high-priority).
   auto cb = make_compact_block(sha256_t{}, body, {0, 3});
   CHECK(cb.prefilled.size() == 2);
   CHECK(cb.short_ids.size() == 2);
   CHECK(cb.prefilled[0].index == 0);
   CHECK(cb.prefilled[1].index == 3);
   CHECK(cb.prefilled[0].tx == body[0]);
   CHECK(cb.prefilled[1].tx == body[3]);

   // Mempool only has the middle two (short-id path).
   mempool_map pool;
   pool.emplace(make_short_tx_id(body[1]), body[1]);
   pool.emplace(make_short_tx_id(body[2]), body[2]);

   auto r = reconstruct(cb, pool);
   CHECK(r.ok());
   CHECK(r.txs == body);
}

static void test_reconstruct_missing_ids() {
   std::vector<std::string> body = {
      "tx-a",
      "tx-b-missing-from-mempool",
      "tx-c",
   };
   auto cb = make_compact_block(sha256_t{}, body);
   mempool_map pool;
   pool.emplace(make_short_tx_id(body[0]), body[0]);
   pool.emplace(make_short_tx_id(body[2]), body[2]);
   // intentionally omit body[1]

   auto r = reconstruct(cb, pool);
   CHECK(!r.ok());
   CHECK(r.txs.empty());
   CHECK(r.missing.size() == 1);
   CHECK(r.missing[0] == make_short_tx_id(body[1]));
   CHECK(!r.full_list().has_value());
}

static void test_reconstruct_partial_then_fill() {
   std::vector<std::string> body = {
      "tx-1", "tx-2", "tx-3", "tx-4",
   };
   auto cb = make_compact_block(sha256_t{}, body, {0}); // prefill only first

   mempool_map pool;
   pool.emplace(make_short_tx_id(body[1]), body[1]);
   // missing body[2] and body[3]

   auto r1 = reconstruct(cb, pool);
   CHECK(!r1.ok());
   CHECK(r1.missing.size() == 2);
   CHECK(r1.missing[0] == make_short_tx_id(body[2]));
   CHECK(r1.missing[1] == make_short_tx_id(body[3]));

   // Simulate fill response: add missing txs to mempool and retry.
   for(const auto& sid : r1.missing) {
      for(const auto& tx : body) {
         if(make_short_tx_id(tx) == sid) pool.emplace(sid, tx);
      }
   }

   auto r2 = reconstruct(cb, pool);
   CHECK(r2.ok());
   CHECK(r2.txs == body);
}

static void test_empty_block() {
   compact_block cb;
   cb.header_hash = sha256(std::string("hdr"));
   auto r = reconstruct(cb, mempool_map{});
   CHECK(r.ok());
   CHECK(r.txs.empty());
   CHECK(r.missing.empty());
   CHECK(r.full_list().has_value());
}

static void test_malformed_prefilled_index() {
   compact_block cb;
   cb.short_ids.push_back(make_short_tx_id(std::string("a")));
   // index 5 is out of range (tx_count == 1 + 1 = 2, but wait — prefilled adds to count)
   // short_ids=1 + prefilled=1 → n=2; index 5 >= 2 → malformed
   cb.prefilled.push_back({5, "bad"});
   auto r = reconstruct(cb, mempool_map{});
   CHECK(!r.ok());
   CHECK(!r.missing.empty() || r.txs.empty());
}

int main() {
   test_short_tx_id_from_sha256();
   test_reconstruct_full_hit();
   test_reconstruct_with_prefilled();
   test_reconstruct_missing_ids();
   test_reconstruct_partial_then_fill();
   test_empty_block();
   test_malformed_prefilled_index();
   std::cout << "compact_block_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
