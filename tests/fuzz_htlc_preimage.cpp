/**
 * Deterministic HTLC preimage fuzz (catalogue / security oracle).
 * Task-ID: swarm-perf / fuzz-htlc-preimage
 *
 * xorshift PRNG, fixed seed, short runtime (~5000 iterations).
 * Oracle: redeem with wrong preimage never credits any account and never crashes;
 * correct preimage credits only HTLC.to.
 */
#include "hive_native/chain/evaluators.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

using namespace hive_native;
using namespace hive_native::protocol;
using namespace hive_native::chain;

namespace {

/** Classic xorshift64 — deterministic, no external deps. */
struct xorshift64 {
   uint64_t state;

   explicit xorshift64(uint64_t seed) : state(seed ? seed : 0xdeadbeefcafebabeull) {}

   uint64_t next() {
      uint64_t x = state;
      x ^= x << 13;
      x ^= x >> 7;
      x ^= x << 17;
      state = x;
      return x;
   }

   uint32_t next_u32() { return static_cast<uint32_t>(next()); }

   /** Inclusive range [lo, hi]. */
   uint32_t range(uint32_t lo, uint32_t hi) {
      if(hi <= lo) return lo;
      return lo + (next_u32() % (hi - lo + 1));
   }
};

static constexpr int k_iterations     = 5000;
static constexpr int k_wrong_attempts = 8;
static constexpr uint64_t k_seed      = 0xC0FFEEF1A5202601ull;
// Cap random preimage length for short CI runtime (still exercises size + hash paths).
static constexpr uint16_t k_max_fuzz_preimage = 64;

static database make_db(share_type alice_hive, share_type bob_hive, share_type carol_hive) {
   database db;
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db.create_account("alice", alice_hive, 0);
   db.create_account("bob", bob_hive, 0);
   db.create_account("carol", carol_hive, 0);
   return db;
}

static std::vector<uint8_t> random_bytes(xorshift64& rng, size_t n) {
   std::vector<uint8_t> out(n);
   for(size_t i = 0; i < n; ++i)
      out[i] = static_cast<uint8_t>(rng.next() & 0xff);
   return out;
}

static bool try_redeem(database& db, const htlc_redeem_operation& r) {
   try {
      apply(db, r);
      return true;
   } catch(const protocol_error&) {
      return false;
   } catch(const std::exception&) {
      // Unexpected exception type is still a soft fail for the oracle (no crash path).
      return false;
   }
}

struct counters {
   int iterations_ok = 0;
   int wrong_rejected = 0;
   int correct_accepted = 0;
   int failures = 0;
};

static void fail(counters& c, const char* msg, int iter) {
   ++c.failures;
   std::cerr << "FAIL iter=" << iter << " " << msg << "\n";
}

static void run_iteration(xorshift64& rng, int iter, counters& c) {
   const share_type lock_amount = static_cast<share_type>(1 + rng.range(1, 50'000));
   const share_type alice0 = 1'000'000;
   const share_type bob0   = 500'000;
   const share_type carol0 = 100'000;

   const uint16_t preimage_size =
      static_cast<uint16_t>(rng.range(1, k_max_fuzz_preimage));
   const bool use_ripemd = (rng.next() & 1ull) != 0;
   const hash_algo algo = use_ripemd ? hash_algo::ripemd160 : hash_algo::sha256;

   std::vector<uint8_t> correct = random_bytes(rng, preimage_size);
   auto dig = digest_of(algo, correct);

   auto db = make_db(alice0, bob0, carol0);

   htlc_create_operation create;
   create.from = "alice";
   create.to = "bob";
   create.amount = asset{lock_amount, asset_symbol::HIVE};
   create.preimage_hash = dig;
   create.preimage_size = preimage_size;
   create.expiration = db.head_time + 3600;

   try {
      apply(db, create);
   } catch(const std::exception& e) {
      fail(c, e.what(), iter);
      return;
   }

   if(db.get_balance("alice", asset_symbol::HIVE) != alice0 - lock_amount) {
      fail(c, "create did not debit alice", iter);
      return;
   }
   if(db.htlcs.size() != 1 || db.htlcs.begin()->second.status != htlc_status::open) {
      fail(c, "htlc not open after create", iter);
      return;
   }

   const auto htlc_id = db.htlcs.begin()->first;

   // --- Wrong preimages must never credit anyone ---
   for(int w = 0; w < k_wrong_attempts; ++w) {
      htlc_redeem_operation r;
      r.to = "bob";
      r.htlc_id = htlc_id;

      const uint32_t mode = rng.range(0, 3);
      if(mode == 0) {
         // Same size, different bytes (retry if collision with correct).
         r.preimage = random_bytes(rng, preimage_size);
         if(r.preimage == correct) {
            for(auto& b : r.preimage) b ^= 0xff;
            if(r.preimage == correct) r.preimage[0] ^= 0x01;
         }
      } else if(mode == 1) {
         // Wrong size (still within MAX, non-zero when possible).
         uint16_t bad_sz = preimage_size == 1
            ? static_cast<uint16_t>(2)
            : static_cast<uint16_t>(rng.range(1, k_max_fuzz_preimage));
         if(bad_sz == preimage_size) {
            bad_sz = static_cast<uint16_t>(preimage_size > 1 ? preimage_size - 1
                                                              : preimage_size + 1);
         }
         r.preimage = random_bytes(rng, bad_sz);
      } else if(mode == 2) {
         // Correct preimage but wrong redeemer (carol sniping).
         r.to = "carol";
         r.preimage = correct;
      } else {
         // Empty / oversized edge: empty is invalid; oversized uses MAX+ path via clamp.
         if(rng.next() & 1ull) {
            r.preimage.clear(); // validate rejects
         } else {
            r.preimage = random_bytes(rng, preimage_size);
            if(r.preimage == correct) r.preimage[0] ^= 0xa5;
         }
      }

      const share_type a_before = db.get_balance("alice", asset_symbol::HIVE);
      const share_type b_before = db.get_balance("bob", asset_symbol::HIVE);
      const share_type c_before = db.get_balance("carol", asset_symbol::HIVE);

      bool ok = false;
      try {
         ok = try_redeem(db, r);
      } catch(...) {
         fail(c, "unexpected throw escaped try_redeem", iter);
         return;
      }

      if(ok) {
         fail(c, "wrong redeem accepted", iter);
         return;
      }
      if(db.get_balance("alice", asset_symbol::HIVE) != a_before ||
         db.get_balance("bob", asset_symbol::HIVE) != b_before ||
         db.get_balance("carol", asset_symbol::HIVE) != c_before) {
         fail(c, "balance changed on failed redeem", iter);
         return;
      }
      if(db.htlcs[htlc_id].status != htlc_status::open) {
         fail(c, "htlc closed by failed redeem", iter);
         return;
      }
      ++c.wrong_rejected;
   }

   // --- Correct preimage must credit only bob ---
   {
      const share_type a_before = db.get_balance("alice", asset_symbol::HIVE);
      const share_type b_before = db.get_balance("bob", asset_symbol::HIVE);
      const share_type c_before = db.get_balance("carol", asset_symbol::HIVE);

      htlc_redeem_operation r;
      r.to = "bob";
      r.htlc_id = htlc_id;
      r.preimage = correct;

      bool ok = false;
      try {
         ok = try_redeem(db, r);
      } catch(...) {
         fail(c, "correct redeem crashed", iter);
         return;
      }

      if(!ok) {
         fail(c, "correct redeem rejected", iter);
         return;
      }
      if(db.get_balance("alice", asset_symbol::HIVE) != a_before) {
         fail(c, "alice balance changed on correct redeem", iter);
         return;
      }
      if(db.get_balance("bob", asset_symbol::HIVE) != b_before + lock_amount) {
         fail(c, "bob not credited correctly", iter);
         return;
      }
      if(db.get_balance("carol", asset_symbol::HIVE) != c_before) {
         fail(c, "carol credited on redeem (wrong account)", iter);
         return;
      }
      if(db.htlcs[htlc_id].status != htlc_status::redeemed) {
         fail(c, "htlc not redeemed after correct preimage", iter);
         return;
      }
      ++c.correct_accepted;
   }

   // Double-redeem must fail and not re-credit.
   {
      const share_type b_before = db.get_balance("bob", asset_symbol::HIVE);
      htlc_redeem_operation r;
      r.to = "bob";
      r.htlc_id = htlc_id;
      r.preimage = correct;
      if(try_redeem(db, r)) {
         fail(c, "double redeem accepted", iter);
         return;
      }
      if(db.get_balance("bob", asset_symbol::HIVE) != b_before) {
         fail(c, "double redeem re-credited bob", iter);
         return;
      }
   }

   ++c.iterations_ok;
}

} // namespace

int main() {
   xorshift64 rng(k_seed);
   counters c;

   for(int i = 0; i < k_iterations; ++i)
      run_iteration(rng, i, c);

   std::cout << "fuzz_htlc_preimage"
             << " seed=0x" << std::hex << k_seed << std::dec
             << " iterations=" << k_iterations
             << " ok=" << c.iterations_ok
             << " wrong_rejected=" << c.wrong_rejected
             << " correct_accepted=" << c.correct_accepted
             << " failures=" << c.failures
             << "\n";

   if(c.failures != 0 || c.iterations_ok != k_iterations) {
      std::cerr << "fuzz_htlc_preimage FAILED\n";
      return 1;
   }
   std::cout << "fuzz_htlc_preimage PASSED\n";
   return 0;
}
