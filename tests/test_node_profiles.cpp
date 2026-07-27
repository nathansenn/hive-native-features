/**
 * Light node profile presets — catalogue #8 #691
 * Task: swarm-perf-p0 / perf-05-light-profiles
 */
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/chain/node_profiles.hpp"
#include <iostream>

using namespace hive_native;
using namespace hive_native::protocol;
using namespace hive_native::chain;

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond) do { \
  if(cond) { ++g_passed; } else { \
    ++g_failed; \
    std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n"; \
  } \
} while(0)

#define CHECK_THROW(expr) do { \
  bool threw = false; \
  try { expr; } catch(...) { threw = true; } \
  CHECK(threw); \
} while(0)

static database make_db(node_profile p = node_profile::full) {
   database db;
   apply_profile(db, p);
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db.create_account("alice", 1'000'000, 1'000'000);
   db.create_account("bob", 1'000'000, 1'000'000);
   return db;
}

static void test_to_string() {
   CHECK(to_string(node_profile::full) == "full");
   CHECK(to_string(node_profile::api_pruned) == "api_pruned");
   CHECK(to_string(node_profile::mobile_light) == "mobile_light");
}

static void test_full_profile_flags() {
   node_config cfg;
   // Pretend misconfigured then apply full
   cfg.nft_skip_state = true;
   cfg.htlc_skip_state = true;
   cfg.contracts_skip = true;
   cfg.is_consensus_node = false;
   apply_profile(cfg, node_profile::full);
   CHECK(cfg.is_consensus_node);
   CHECK(!cfg.nft_skip_state);
   CHECK(!cfg.htlc_skip_state);
   CHECK(!cfg.contracts_skip);
   CHECK(config_ok_for_role(cfg));
}

static void test_api_pruned_profile_flags() {
   auto cfg = make_node_config(node_profile::api_pruned);
   CHECK(!cfg.is_consensus_node);
   CHECK(!cfg.nft_skip_state);
   CHECK(!cfg.htlc_skip_state);
   CHECK(cfg.contracts_skip);
   CHECK(config_ok_for_role(cfg));
}

static void test_mobile_light_profile_flags() {
   auto cfg = make_node_config(node_profile::mobile_light);
   CHECK(!cfg.is_consensus_node);
   CHECK(cfg.nft_skip_state);
   CHECK(cfg.htlc_skip_state);
   CHECK(cfg.contracts_skip);
   CHECK(config_ok_for_role(cfg));
}

static void test_consensus_cannot_keep_nft_skip() {
   // Even if full path is applied after someone sets consensus+skip, sanitize/apply clears it.
   node_config cfg;
   cfg.is_consensus_node = true;
   cfg.nft_skip_state = true;
   cfg.htlc_skip_state = true;
   CHECK(!config_ok_for_role(cfg));
   CHECK(!sanitize_consensus_skips(cfg));
   CHECK(!cfg.nft_skip_state);
   CHECK(!cfg.htlc_skip_state);
   CHECK(config_ok_for_role(cfg));
}

static void test_apply_profile_never_leaves_consensus_with_nft_skip() {
   // apply_profile(full) is the only consensus profile; always clear skips.
   node_config cfg = make_node_config(node_profile::mobile_light);
   CHECK(cfg.nft_skip_state);
   apply_profile(cfg, node_profile::full);
   CHECK(cfg.is_consensus_node);
   CHECK(!cfg.nft_skip_state);
   CHECK(!cfg.htlc_skip_state);
}

static void test_require_config_ok_throws() {
   node_config bad;
   bad.is_consensus_node = true;
   bad.nft_skip_state = true;
   CHECK_THROW(require_config_ok_for_role(bad));

   node_config good = make_node_config(node_profile::full);
   require_config_ok_for_role(good); // no throw
   CHECK(true);
}

static void test_full_allows_nft_apply() {
   auto db = make_db(node_profile::full);
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "ART";
   cc.name = "Art";
   cc.max_supply = 10;
   apply(db, cc);
   CHECK(db.collections.size() == 1);
}

static void test_mobile_light_rejects_nft_apply() {
   auto db = make_db(node_profile::mobile_light);
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "X";
   cc.name = "X";
   CHECK_THROW(apply(db, cc));
}

static void test_api_pruned_allows_nft_rejects_contracts() {
   auto db = make_db(node_profile::api_pruned);
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "API";
   cc.name = "Api";
   apply(db, cc);
   CHECK(db.collections.size() == 1);

   contract_deploy_operation d;
   d.owner = "alice";
   d.code = {0x00, 0x61, 0x73, 0x6d}; // wasm magic; skip gate fires first
   d.fuel_limit = 10'000;
   // contracts_skip + !consensus → "contracts skipped"
   CHECK_THROW(apply(db, d));
}

static void test_default_profile_compile() {
   // Without HIVE_LIGHT_NODE, default is full.
#ifndef HIVE_LIGHT_NODE
   CHECK(default_profile() == node_profile::full);
#else
   CHECK(default_profile() == node_profile::mobile_light);
#endif
   node_config cfg;
   apply_default_profile(cfg);
   CHECK(config_ok_for_role(cfg));
#ifndef HIVE_LIGHT_NODE
   CHECK(cfg.is_consensus_node);
   CHECK(!cfg.nft_skip_state);
#else
   CHECK(!cfg.is_consensus_node);
   CHECK(cfg.nft_skip_state);
#endif
}

static void test_database_apply_profile_overload() {
   database db;
   apply_profile(db, node_profile::mobile_light);
   CHECK(db.config.nft_skip_state);
   CHECK(!db.config.is_consensus_node);
   apply_profile(db, node_profile::full);
   CHECK(db.config.is_consensus_node);
   CHECK(!db.config.nft_skip_state);
}

int main() {
   test_to_string();
   test_full_profile_flags();
   test_api_pruned_profile_flags();
   test_mobile_light_profile_flags();
   test_consensus_cannot_keep_nft_skip();
   test_apply_profile_never_leaves_consensus_with_nft_skip();
   test_require_config_ok_throws();
   test_full_allows_nft_apply();
   test_mobile_light_rejects_nft_apply();
   test_api_pruned_allows_nft_rejects_contracts();
   test_default_profile_compile();
   test_database_apply_profile_overload();
   std::cout << "node_profiles_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
