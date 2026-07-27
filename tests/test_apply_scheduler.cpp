/**
 * #151 apply_scheduler integration tests.
 * - independent NFT transfers → parallelism_factor >= 2
 * - conflicting same-account transfers → serial layers
 * - global op forces separation
 */
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/perf/apply_scheduler.hpp"
#include "hive_native/util/types.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace hive_native;
using namespace hive_native::protocol;
using namespace hive_native::chain;
using namespace hive_native::perf;

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond) do { \
  if(cond) { ++g_passed; } else { \
    ++g_failed; \
    std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n"; \
  } \
} while(0)

static database make_db_with_owners(const std::vector<std::string>& owners) {
   database db;
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   for(const auto& o : owners) {
      db.create_account(o, 1'000'000, 0);
   }
   // Shared receiver pool (account names: a-z0-9.- only)
   db.create_account("recva", 0, 0);
   db.create_account("recvb", 0, 0);
   db.create_account("recvc", 0, 0);
   db.create_account("recvd", 0, 0);
   return db;
}

static void mint_to(database& db, const std::string& creator,
                    const std::string& symbol, const std::string& to) {
   if(!db.collection_by_symbol.count(symbol)) {
      nft_create_collection_operation cc;
      cc.creator = creator;
      cc.symbol = symbol;
      cc.name = symbol;
      cc.max_supply = 0;
      apply(db, cc);
   }
   nft_mint_operation m;
   m.creator = creator;
   m.collection = db.collection_by_symbol[symbol];
   m.to = to;
   m.metadata_hash = sha256(std::string("meta-") + to);
   apply(db, m);
}

/** 4 independent NFT transfers from different owners → parallelism_factor >= 2 */
static void test_independent_nft_transfers() {
   auto db = make_db_with_owners({"o1", "o2", "o3", "o4", "alice"});
   mint_to(db, "alice", "IND", "o1");
   mint_to(db, "alice", "IND", "o2");
   mint_to(db, "alice", "IND", "o3");
   mint_to(db, "alice", "IND", "o4");
   CHECK(db.nfts.size() == 4);

   std::vector<scheduled_op> ops;
   {
      nft_transfer_operation t;
      t.from = "o1"; t.to = "recva"; t.nft_id = 1;
      ops.push_back(t);
   }
   {
      nft_transfer_operation t;
      t.from = "o2"; t.to = "recvb"; t.nft_id = 2;
      ops.push_back(t);
   }
   {
      nft_transfer_operation t;
      t.from = "o3"; t.to = "recvc"; t.nft_id = 3;
      ops.push_back(t);
   }
   {
      nft_transfer_operation t;
      t.from = "o4"; t.to = "recvd"; t.nft_id = 4;
      ops.push_back(t);
   }

   // Plan first: all four touch disjoint accounts/nft ids → one layer, factor 4
   auto plan = plan_schedule(ops);
   CHECK(plan.layer_count == 1);
   CHECK(plan.parallel_width == 4);
   CHECK(plan.parallelism_factor >= 2.0);
   CHECK(plan.parallelism_factor >= 3.9); // expect ~4.0

   auto stats = apply_scheduled(db, ops);
   CHECK(stats.ops_applied == 4);
   CHECK(stats.parallelism_factor >= 2.0);
   CHECK(stats.parallel_width >= 2);
   CHECK(db.nfts[1].owner == "recva");
   CHECK(db.nfts[2].owner == "recvb");
   CHECK(db.nfts[3].owner == "recvc");
   CHECK(db.nfts[4].owner == "recvd");
}

/** 2 conflicting transfers same account → serial layers (factor == 1) */
static void test_conflicting_same_account() {
   auto db = make_db_with_owners({"alice", "bob"});
   mint_to(db, "alice", "CON", "alice");
   mint_to(db, "alice", "CON", "alice");
   CHECK(db.nfts.size() == 2);
   CHECK(db.nfts[1].owner == "alice");
   CHECK(db.nfts[2].owner == "alice");

   std::vector<scheduled_op> ops;
   {
      nft_transfer_operation t;
      t.from = "alice"; t.to = "recva"; t.nft_id = 1;
      ops.push_back(t);
   }
   {
      nft_transfer_operation t;
      t.from = "alice"; t.to = "recvb"; t.nft_id = 2;
      ops.push_back(t);
   }

   auto plan = plan_schedule(ops);
   // Same account "alice" → cannot share a layer
   CHECK(plan.layer_count == 2);
   CHECK(plan.parallel_width == 1);
   CHECK(plan.parallelism_factor == 1.0);

   auto stats = apply_scheduled(db, ops);
   CHECK(stats.ops_applied == 2);
   CHECK(stats.layer_count == 2);
   CHECK(stats.parallelism_factor == 1.0);
   CHECK(db.nfts[1].owner == "recva");
   CHECK(db.nfts[2].owner == "recvb");
}

/** Mixed with global op forces separation of independent work */
static void test_global_forces_separation() {
   auto db = make_db_with_owners({"o1", "o2", "alice"});
   mint_to(db, "alice", "GLB", "o1");
   mint_to(db, "alice", "GLB", "o2");

   std::vector<scheduled_op> ops;
   {
      nft_transfer_operation t;
      t.from = "o1"; t.to = "recva"; t.nft_id = 1;
      ops.push_back(t);
   }
   {
      // Global between two otherwise independent transfers
      ops.push_back(global_marker_operation{"witness-set-properties"});
   }
   {
      nft_transfer_operation t;
      t.from = "o2"; t.to = "recvb"; t.nft_id = 2;
      ops.push_back(t);
   }

   auto plan = plan_schedule(ops);
   // Global conflicts with every layer it would join; greedy places each
   // transfer and the global in separate layers when global is present.
   // Expected: op0 alone or with nothing global; global alone; op2 alone or
   // after — at least 2 layers, and global's layer has width 1.
   CHECK(plan.layer_count >= 2);
   CHECK(plan.parallelism_factor < 3.0); // not fully parallel (would be 3)

   bool found_global_alone = false;
   auto touches = classify_all(ops);
   CHECK(touches[1].touches_global == true);
   CHECK(touches[1].cls == op_class::global);
   CHECK(touches[0].cls == op_class::nft_transfer);
   CHECK(kind_of(ops[1]) == scheduled_op_kind::global);

   for(const auto& layer : plan.layers) {
      if(layer.op_indices.size() == 1 && layer.op_indices[0] == 1)
         found_global_alone = true;
   }
   // Global may share a layer only if nothing else is there; greedy assigns
   // global to its own layer when prior layers already have non-global ops
   // OR when any layer is marked global. With op0 first, layer0 gets op0;
   // global cannot join layer0 → new layer; op2 cannot join global layer →
   // may join layer0 if accounts disjoint.
   // So possible schedules:
   //   L0: [0,2]  L1: [1]     factor 1.5  width 2  — global alone
   //   L0: [0]    L1: [1]  L2:[2] — if op2 couldn't join (shouldn't)
   CHECK(found_global_alone || plan.layer_count >= 2);

   // Without global, the two transfers would be one layer (factor 2)
   std::vector<scheduled_op> no_global = {ops[0], ops[2]};
   auto free = plan_schedule(no_global);
   CHECK(free.layer_count == 1);
   CHECK(free.parallel_width == 2);
   CHECK(free.parallelism_factor >= 2.0);

   // With global present, factor must be strictly worse than pure independent pair
   CHECK(plan.parallelism_factor < free.parallelism_factor
         || plan.layer_count > free.layer_count);

   auto stats = apply_scheduled(db, ops);
   CHECK(stats.ops_applied == 3);
   CHECK(db.nfts[1].owner == "recva");
   CHECK(db.nfts[2].owner == "recvb");
   // Global marker recorded as virtual
   bool saw_global = false;
   for(const auto& v : db.pending_virtual_ops) {
      if(v.name == "global_marker") saw_global = true;
   }
   CHECK(saw_global);
}

/** Classification coverage for HTLC redeem + contract call */
static void test_classify_htlc_and_contract() {
   htlc_redeem_operation r;
   r.to = "bob";
   r.htlc_id = 7;
   r.preimage = {1, 2, 3};
   auto th = classify_htlc_redeem(0, r);
   CHECK(th.cls == op_class::htlc_redeem);
   CHECK(th.touches_global == false);
   CHECK(th.accounts.size() >= 2); // to + htlc:id

   contract_call_operation c;
   c.caller = "alice";
   c.contract_id = 3;
   c.export_name = "call";
   c.fuel_limit = 1000;
   auto tc = classify_contract_call(1, c);
   CHECK(tc.cls == op_class::custom_json);
   CHECK(tc.accounts.size() >= 2); // caller + contract:id

   // Two calls same contract conflict
   scheduled_op a = c;
   scheduled_op b = c;
   auto plan = plan_schedule({a, b});
   CHECK(plan.layer_count == 2);
   CHECK(plan.parallelism_factor == 1.0);

   // Different contracts independent (different callers)
   contract_call_operation c2;
   c2.caller = "carol";
   c2.contract_id = 9;
   c2.export_name = "call";
   c2.fuel_limit = 1000;
   auto plan2 = plan_schedule({scheduled_op{c}, scheduled_op{c2}});
   CHECK(plan2.layer_count == 1);
   CHECK(plan2.parallel_width == 2);
}

int main() {
   test_independent_nft_transfers();
   test_conflicting_same_account();
   test_global_forces_separation();
   test_classify_htlc_and_contract();
   std::cout << "apply_scheduler_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
