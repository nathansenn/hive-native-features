/**
 * Microbenchmark skeleton vs synthetic transfer baseline.
 * Task-ID: phase-1 / 1.10
 */
#include "hive_native/chain/evaluators.hpp"
#include <chrono>
#include <iostream>
#include <vector>

using namespace hive_native;
using namespace hive_native::protocol;
using namespace hive_native::chain;
using clock_type = std::chrono::steady_clock;

template<typename F>
double bench_us(F&& f, int iters) {
   // warmup
   for(int i = 0; i < 100; ++i) f();
   auto t0 = clock_type::now();
   for(int i = 0; i < iters; ++i) f();
   auto t1 = clock_type::now();
   return std::chrono::duration<double, std::micro>(t1 - t0).count() / iters;
}

static database primed() {
   database db;
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db.create_account("alice", 1e15, 1e15);
   db.create_account("bob", 1e15, 1e15);
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "BENCH";
   cc.name = "Bench";
   cc.max_supply = 0;
   apply(db, cc);
   return db;
}

/** Synthetic transfer-class op: validate names + dual balance move + virtual op.
 *  Fairer baseline than raw adjust_balance for ratio budgets in docs/04. */
static void synthetic_transfer(database& db, const char* from, const char* to, share_type amt) {
   assert_account_name(from);
   assert_account_name(to);
   if(!db.account_exists(from) || !db.account_exists(to))
      throw protocol_error("account missing");
   db.adjust_balance(from, asset{-amt, asset_symbol::HIVE});
   db.adjust_balance(to, asset{amt, asset_symbol::HIVE});
   db.push_virtual("transfer", std::string(from) + "->" + to);
   db.last_rc_charged = rc::TRANSFER_BASE;
}

int main() {
   const int N = 2000;

   database db0;
   db0.head_time = 1'700'000'000;
   db0.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db0.create_account("alice", 1e15, 0);
   db0.create_account("bob", 0, 0);
   double transfer_us = bench_us([&]{
      synthetic_transfer(db0, "alice", "bob", 1);
      synthetic_transfer(db0, "bob", "alice", 1);
   }, N);

   auto db = primed();
   for(int i = 0; i < N + 10; ++i) {
      nft_mint_operation m;
      m.creator = "alice";
      m.collection = 1;
      m.to = "alice";
      apply(db, m);
   }
   // Steady-state ping-pong on a single NFT (amortizes mint; measures transfer apply)
   {
      nft_transfer_operation warm;
      warm.from = "alice";
      warm.to = "bob";
      warm.nft_id = 1;
      apply(db, warm);
   }
   bool to_bob = false;
   double nft_transfer_us = bench_us([&]{
      nft_transfer_operation t;
      if(to_bob) { t.from = "alice"; t.to = "bob"; }
      else       { t.from = "bob";   t.to = "alice"; }
      t.nft_id = 1;
      apply(db, t);
      to_bob = !to_bob;
   }, N);

   std::vector<uint8_t> preimage = {1,2,3,4,5,6,7,8};
   auto dig = digest_of(hash_algo::sha256, preimage);
   database dbh;
   dbh.head_time = 1'700'000'000;
   dbh.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   dbh.create_account("alice", 1e15, 0);
   dbh.create_account("bob", 0, 0);
   // Stay under HTLC_MAX_OPEN_PER_ACCOUNT (256): create batch, expire+refund, repeat.
   double htlc_create_us = 0;
   {
      const int H = 400;
      const int BATCH = 200;
      auto t0 = clock_type::now();
      int done = 0;
      while(done < H) {
         const int n = std::min(BATCH, H - done);
         uint64_t first_id = dbh.next_htlc_id;
         for(int i = 0; i < n; ++i) {
            htlc_create_operation c;
            c.from = "alice";
            c.to = "bob";
            c.amount = asset{1, asset_symbol::HIVE};
            c.preimage_hash = dig;
            c.preimage_size = 8;
            c.expiration = dbh.head_time + 3600;
            apply(dbh, c);
         }
         dbh.head_time += 3600;
         for(uint64_t id = first_id; id < first_id + uint64_t(n); ++id) {
            htlc_refund_operation rf;
            rf.from = "alice";
            rf.htlc_id = id;
            apply(dbh, rf);
         }
         done += n;
      }
      auto t1 = clock_type::now();
      htlc_create_us = std::chrono::duration<double, std::micro>(t1 - t0).count() / H;
   }

   const double ratio = nft_transfer_us / transfer_us;
   std::cout << "{\n";
   std::cout << "  \"synthetic_transfer_us\": " << transfer_us << ",\n";
   std::cout << "  \"nft_transfer_us\": " << nft_transfer_us << ",\n";
   std::cout << "  \"nft_transfer_ratio\": " << ratio << ",\n";
   std::cout << "  \"htlc_create_us\": " << htlc_create_us << ",\n";
   std::cout << "  \"budget_nft_p50_ratio_max\": 1.5,\n";
   std::cout << "  \"budget_nft_hard_fail_ratio\": 5.0,\n";
   std::cout << "  \"nft_within_hard_fail\": " << (ratio <= 5.0 ? "true" : "false") << ",\n";
   std::cout << "  \"note\": \"portable in-memory; ratio vs synthetic transfer-class op\"\n";
   std::cout << "}\n";
   // Soft target 1.5x; hard fail 5x per docs/04. Exit non-zero only on hard fail.
   return ratio > 5.0 ? 2 : 0;
}
