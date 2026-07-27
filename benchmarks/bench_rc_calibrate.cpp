/**
 * #891 — RC calibration from real portable microbench samples.
 * Runs synthetic transfer / nft_transfer / htlc_create timed loops,
 * feeds hive_native::perf::rc_calibrator, prints calibrated_rc vs placeholders.
 *
 * Target: hive_native_bench_rc (optional; not a ctest hard-fail gate).
 */
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/perf/rc_calibrator.hpp"
#include "hive_native/rc/costs.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace hive_native;
using namespace hive_native::protocol;
using namespace hive_native::chain;
using clock_type = std::chrono::steady_clock;

/** Mean wall µs over `iters` after a short warmup. */
template<typename F>
double bench_us(F&& f, int iters) {
   for(int i = 0; i < 50; ++i) f();
   auto t0 = clock_type::now();
   for(int i = 0; i < iters; ++i) f();
   auto t1 = clock_type::now();
   return std::chrono::duration<double, std::micro>(t1 - t0).count() / iters;
}

/** Synthetic transfer-class baseline (matches benchmarks/bench_ops.cpp). */
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

static database primed_nft() {
   database db;
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db.create_account("alice", 1e15, 1e15);
   db.create_account("bob", 1e15, 1e15);
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "RCAL";
   cc.name = "RcCal";
   cc.max_supply = 0;
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "alice";
   apply(db, m);
   return db;
}

int main() {
   constexpr int ROUNDS = 21;   // odd → stable median
   constexpr int ITERS  = 400;  // per-round loop length

   hive_native::perf::rc_calibrator cal;

   // ---- transfer samples ----
   database dbt;
   dbt.head_time = 1'700'000'000;
   dbt.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   dbt.create_account("alice", 1e15, 0);
   dbt.create_account("bob", 0, 0);
   for(int r = 0; r < ROUNDS; ++r) {
      double us = bench_us([&]{
         synthetic_transfer(dbt, "alice", "bob", 1);
         synthetic_transfer(dbt, "bob", "alice", 1);
      }, ITERS);
      // bench_us averages a round-trip pair; charge half as one transfer sample
      cal.add("transfer", us / 2.0);
   }

   // ---- nft_transfer samples (ping-pong one NFT) ----
   auto dbn = primed_nft();
   {
      nft_transfer_operation warm;
      warm.from = "alice";
      warm.to = "bob";
      warm.nft_id = 1;
      apply(dbn, warm);
   }
   bool to_bob = false;
   for(int r = 0; r < ROUNDS; ++r) {
      double us = bench_us([&]{
         nft_transfer_operation t;
         if(to_bob) { t.from = "alice"; t.to = "bob"; }
         else       { t.from = "bob";   t.to = "alice"; }
         t.nft_id = 1;
         apply(dbn, t);
         to_bob = !to_bob;
      }, ITERS);
      cal.add("nft_transfer", us);
   }

   // ---- htlc_create samples (time create only; refund+prune outside timer) ----
   std::vector<uint8_t> preimage = {1, 2, 3, 4, 5, 6, 7, 8};
   auto dig = digest_of(hash_algo::sha256, preimage);
   constexpr int H_PER_ROUND = 200;
   constexpr int BATCH = 200;
   for(int r = 0; r < ROUNDS; ++r) {
      // Fresh DB each round so map/virtual-op growth does not dominate median.
      database dbh;
      dbh.head_time = 1'700'000'000;
      dbh.config.hardfork = HIVE_HARDFORK_CONTRACTS;
      dbh.create_account("alice", 1e15, 0);
      dbh.create_account("bob", 0, 0);
      double create_us_accum = 0;
      int created = 0;
      while(created < H_PER_ROUND) {
         const int n = std::min(BATCH, H_PER_ROUND - created);
         uint64_t first_id = dbh.next_htlc_id;
         auto t0 = clock_type::now();
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
         auto t1 = clock_type::now();
         create_us_accum += std::chrono::duration<double, std::micro>(t1 - t0).count();
         // recycle open slots + prune closed objects outside create measurement
         dbh.head_time += 3600;
         for(uint64_t id = first_id; id < first_id + uint64_t(n); ++id) {
            htlc_refund_operation rf;
            rf.from = "alice";
            rf.htlc_id = id;
            apply(dbh, rf);
            dbh.htlcs.erase(id);
         }
         dbh.pending_virtual_ops.clear();
         created += n;
      }
      cal.add("htlc_create", create_us_accum / H_PER_ROUND);
   }

   // Placeholder costs (empty memo / typical portable ops)
   const uint64_t ph_transfer = rc::TRANSFER_BASE;
   const uint64_t ph_nft = rc::cost_nft_transfer(nft_transfer_operation{});
   const uint64_t ph_htlc = rc::cost_htlc_create(htlc_create_operation{});

   const double med_transfer = cal.median_us("transfer");
   const double med_nft = cal.median_us("nft_transfer");
   const double med_htlc = cal.median_us("htlc_create");

   const uint64_t cal_transfer = cal.calibrated_rc("transfer");
   const uint64_t cal_nft = cal.calibrated_rc("nft_transfer");
   const uint64_t cal_htlc = cal.calibrated_rc("htlc_create");

   auto ratio = [](double num, double den) {
      return (den > 0) ? (num / den) : 0.0;
   };

   std::cout << "{\n";
   std::cout << "  \"catalogue\": 891,\n";
   std::cout << "  \"transfer_base\": " << rc::TRANSFER_BASE << ",\n";
   std::cout << "  \"rounds\": " << ROUNDS << ",\n";
   std::cout << "  \"iters_per_round\": " << ITERS << ",\n";
   std::cout << "  \"samples\": {\n";
   std::cout << "    \"transfer\": { \"median_us\": " << med_transfer
             << ", \"calibrated_rc\": " << cal_transfer
             << ", \"placeholder_rc\": " << ph_transfer
             << ", \"wall_ratio_vs_transfer\": 1.0 },\n";
   std::cout << "    \"nft_transfer\": { \"median_us\": " << med_nft
             << ", \"calibrated_rc\": " << cal_nft
             << ", \"placeholder_rc\": " << ph_nft
             << ", \"wall_ratio_vs_transfer\": " << ratio(med_nft, med_transfer) << " },\n";
   std::cout << "    \"htlc_create\": { \"median_us\": " << med_htlc
             << ", \"calibrated_rc\": " << cal_htlc
             << ", \"placeholder_rc\": " << ph_htlc
             << ", \"wall_ratio_vs_transfer\": " << ratio(med_htlc, med_transfer) << " }\n";
   std::cout << "  },\n";
   std::cout << "  \"calibrated_rc\": {\n";
   std::cout << "    \"transfer\": " << cal_transfer << ",\n";
   std::cout << "    \"nft_transfer\": " << cal_nft << ",\n";
   std::cout << "    \"htlc_create\": " << cal_htlc << "\n";
   std::cout << "  },\n";
   std::cout << "  \"placeholder_rc\": {\n";
   std::cout << "    \"transfer\": " << ph_transfer << ",\n";
   std::cout << "    \"nft_transfer\": " << ph_nft << ",\n";
   std::cout << "    \"htlc_create\": " << ph_htlc << "\n";
   std::cout << "  },\n";
   std::cout << "  \"formula\": \"calibrated_rc = TRANSFER_BASE * median(op) / median(transfer)\",\n";
   std::cout << "  \"note\": \"portable in-memory apply; htlc refunds only recycle open-cap outside create timer\"\n";
   std::cout << "}\n";

   // Informational only — never hard-fail CI (target not in ctest).
   return 0;
}
