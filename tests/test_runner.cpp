/**
 * Portable test runner — no external deps.
 * Task-ID: phase-1 / 1.6, phase-2 tests, phase-3 tests
 */
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/contracts/engine.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

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

static database make_db() {
   database db;
   db.head_time = 1'700'000'000;
   db.config.hardfork = HIVE_HARDFORK_CONTRACTS;
   db.create_account("alice", 1'000'000, 1'000'000);
   db.create_account("bob", 1'000'000, 1'000'000);
   db.create_account("carol", 0, 0);
   db.create_account("op", 0, 0);
   return db;
}

static void check_hex(const uint8_t* dig, size_t n, const char* hex) {
   for(size_t i = 0; i < n; ++i) {
      unsigned int byte;
      std::sscanf(hex + i*2, "%2x", &byte);
      CHECK(dig[i] == uint8_t(byte));
   }
}

static void test_crypto() {
   // FIPS 180-4 SHA-256("abc")
   auto h = sha256(std::string("abc"));
   check_hex(h.data(), 32, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

   // FIPS 180-4 SHA-256("") empty string
   auto h_empty = sha256(std::string(""));
   check_hex(h_empty.data(), 32,
             "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

   // RIPEMD-160("abc") known vector
   const char* abc = "abc";
   auto r = ripemd160(reinterpret_cast<const uint8_t*>(abc), 3);
   check_hex(r.data(), 20, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}

static void test_nft_happy_path() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "ART";
   cc.name = "Art";
   cc.max_supply = 10;
   apply(db, cc);
   CHECK(db.collections.size() == 1);

   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   m.metadata_hash = sha256(std::string("meta"));
   apply(db, m);
   CHECK(db.nfts.size() == 1);
   CHECK(db.nfts[1].owner == "bob");

   nft_approve_operation ap;
   ap.owner = "bob";
   ap.nft_id = 1;
   ap.approved = "op";
   apply(db, ap);

   nft_transfer_operation t;
   t.from = "op"; // approved operator
   t.to = "carol";
   t.nft_id = 1;
   apply(db, t);
   CHECK(db.nfts[1].owner == "carol");
   CHECK(db.nfts[1].approved.empty());
}

static void test_nft_approval_for_all() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "GAME";
   cc.name = "Game";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   apply(db, m);

   nft_set_approval_for_all_operation sa;
   sa.owner = "bob";
   sa.operator_account = "op";
   sa.collection = 0; // all
   sa.approved = true;
   apply(db, sa);
   CHECK(db.is_operator_approved("bob", "op", 1));

   nft_transfer_operation t;
   t.from = "op";
   t.to = "carol";
   t.nft_id = 1;
   apply(db, t);
   CHECK(db.nfts[1].owner == "carol");
}

static void test_nft_soulbound_and_auth() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "SB";
   cc.name = "Soul";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   m.soulbound = true;
   apply(db, m);
   nft_transfer_operation t;
   t.from = "bob";
   t.to = "carol";
   t.nft_id = 1;
   CHECK_THROW(apply(db, t));

   // stranger cannot transfer
   auto db2 = make_db();
   apply(db2, cc);
   m.soulbound = false;
   m.collection = 1;
   apply(db2, m);
   t.from = "carol";
   CHECK_THROW(apply(db2, t));
}

static void test_nft_max_supply() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "ONE";
   cc.name = "One";
   cc.max_supply = 1;
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   apply(db, m);
   CHECK_THROW(apply(db, m));
}

static void test_nft_light_skip() {
   auto db = make_db();
   db.config.is_consensus_node = false;
   db.config.nft_skip_state = true;
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "X";
   cc.name = "X";
   CHECK_THROW(apply(db, cc));

   db.config.is_consensus_node = true;
   db.config.nft_skip_state = true;
   CHECK_THROW(apply(db, cc)); // consensus cannot skip
}

static void test_nft_burn_reduces_supply() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "BRN";
   cc.name = "Burn";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   apply(db, m);
   CHECK(db.collections[1].supply == 1);
   CHECK(db.nfts.size() == 1);

   nft_burn_operation b;
   b.owner = "bob";
   b.nft_id = 1;
   apply(db, b);
   CHECK(db.nfts.empty());
   CHECK(db.collections[1].supply == 0);
}

static void test_nft_approval_collection_scope() {
   auto db = make_db();
   nft_create_collection_operation cc1;
   cc1.creator = "alice";
   cc1.symbol = "C1";
   cc1.name = "Col1";
   apply(db, cc1);
   nft_create_collection_operation cc2;
   cc2.creator = "alice";
   cc2.symbol = "C2";
   cc2.name = "Col2";
   apply(db, cc2);

   nft_mint_operation m1;
   m1.creator = "alice";
   m1.collection = 1;
   m1.to = "bob";
   apply(db, m1);
   nft_mint_operation m2;
   m2.creator = "alice";
   m2.collection = 2;
   m2.to = "bob";
   apply(db, m2);

   // Collection-specific operator (collection=1 only)
   nft_set_approval_for_all_operation sa;
   sa.owner = "bob";
   sa.operator_account = "op";
   sa.collection = 1;
   sa.approved = true;
   apply(db, sa);
   CHECK(db.is_operator_approved("bob", "op", 1));
   CHECK(!db.is_operator_approved("bob", "op", 2));

   nft_transfer_operation t;
   t.from = "op";
   t.to = "carol";
   t.nft_id = 1;
   apply(db, t);
   CHECK(db.nfts[1].owner == "carol");

   // Collection-1 approval must not authorize collection-2 transfer
   t.nft_id = 2;
   CHECK_THROW(apply(db, t));
   CHECK(db.nfts[2].owner == "bob");

   // collection=0 means all collections
   sa.collection = 0;
   sa.approved = true;
   apply(db, sa);
   CHECK(db.is_operator_approved("bob", "op", 2));
   apply(db, t);
   CHECK(db.nfts[2].owner == "carol");
}

static void test_nft_revoke_approval_for_all() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "REV";
   cc.name = "Revoke";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   apply(db, m);

   nft_set_approval_for_all_operation sa;
   sa.owner = "bob";
   sa.operator_account = "op";
   sa.collection = 0;
   sa.approved = true;
   apply(db, sa);
   CHECK(db.is_operator_approved("bob", "op", 1));

   sa.approved = false;
   apply(db, sa);
   CHECK(!db.is_operator_approved("bob", "op", 1));

   nft_transfer_operation t;
   t.from = "op";
   t.to = "carol";
   t.nft_id = 1;
   CHECK_THROW(apply(db, t));
   CHECK(db.nfts[1].owner == "bob");
}

static void test_nft_mint_non_creator_fails() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "NC";
   cc.name = "NonCreator";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "bob"; // not collection creator
   m.collection = 1;
   m.to = "bob";
   CHECK_THROW(apply(db, m));
   CHECK(db.nfts.empty());
   CHECK(db.collections[1].supply == 0);
}

static void test_nft_symbol_duplicate_fails() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "DUP";
   cc.name = "First";
   apply(db, cc);
   CHECK(db.collections.size() == 1);

   nft_create_collection_operation cc2;
   cc2.creator = "bob";
   cc2.symbol = "DUP";
   cc2.name = "Second";
   CHECK_THROW(apply(db, cc2));
   CHECK(db.collections.size() == 1);
   CHECK(db.collection_by_symbol["DUP"] == 1);
}

static void test_nft_transfer_to_self_fails_validate() {
   auto db = make_db();
   nft_create_collection_operation cc;
   cc.creator = "alice";
   cc.symbol = "SELF";
   cc.name = "Self";
   apply(db, cc);
   nft_mint_operation m;
   m.creator = "alice";
   m.collection = 1;
   m.to = "bob";
   apply(db, m);

   nft_transfer_operation t;
   t.from = "bob";
   t.to = "bob";
   t.nft_id = 1;
   CHECK_THROW(t.validate());
   CHECK_THROW(apply(db, t));
   CHECK(db.nfts[1].owner == "bob");
}

static void test_htlc_redeem() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {'s','e','c','r','e','t'};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{5000, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   c.expiration = db.head_time + 3600;
   apply(db, c);
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000 - 5000);

   htlc_redeem_operation r;
   r.to = "bob";
   r.htlc_id = 1;
   r.preimage = preimage;
   apply(db, r);
   CHECK(db.get_balance("bob", asset_symbol::HIVE) == 1'000'000 + 5000);
   CHECK(db.htlcs[1].status == htlc_status::redeemed);
}

static void test_htlc_edge_cases() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {1,2,3,4};
   auto dig = digest_of(hash_algo::sha256, preimage);
   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{100, asset_symbol::HBD};
   c.preimage_hash = dig;
   c.preimage_size = 4;
   c.expiration = db.head_time + 3600;
   apply(db, c);

   // wrong redeemer
   htlc_redeem_operation r;
   r.to = "carol";
   r.htlc_id = 1;
   r.preimage = preimage;
   CHECK_THROW(apply(db, r));

   // bad preimage
   r.to = "bob";
   r.preimage = {9,9,9,9};
   CHECK_THROW(apply(db, r));

   // refund before expiry
   htlc_refund_operation rf;
   rf.from = "alice";
   rf.htlc_id = 1;
   CHECK_THROW(apply(db, rf));

   // expire then redeem fails, refund ok
   db.head_time = c.expiration;
   r.preimage = preimage;
   CHECK_THROW(apply(db, r));
   apply(db, rf);
   CHECK(db.htlcs[1].status == htlc_status::refunded);
   CHECK(db.get_balance("alice", asset_symbol::HBD) == 1'000'000);
}

static void test_htlc_ripemd() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {'x'};
   auto dig = digest_of(hash_algo::ripemd160, preimage);
   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{10, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = 1;
   c.expiration = db.head_time + 120;
   apply(db, c);
   htlc_redeem_operation r;
   r.to = "bob";
   r.htlc_id = 1;
   r.preimage = preimage;
   apply(db, r);
   CHECK(db.htlcs[1].status == htlc_status::redeemed);
}

/** Phase-2 negative / edge matrix: double-spend, duration bounds, balance, size. */
static void test_htlc_double_redeem_fails() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {'s','e','c','r','e','t'};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{1000, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   c.expiration = db.head_time + 3600;
   apply(db, c);

   htlc_redeem_operation r;
   r.to = "bob";
   r.htlc_id = 1;
   r.preimage = preimage;
   apply(db, r);
   CHECK(db.htlcs[1].status == htlc_status::redeemed);
   CHECK(db.get_balance("bob", asset_symbol::HIVE) == 1'000'000 + 1000);

   // Second redeem must fail (htlc not open); bob balance unchanged
   CHECK_THROW(apply(db, r));
   CHECK(db.htlcs[1].status == htlc_status::redeemed);
   CHECK(db.get_balance("bob", asset_symbol::HIVE) == 1'000'000 + 1000);
}

static void test_htlc_double_refund_fails() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {1, 2, 3, 4};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{2000, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   c.expiration = db.head_time + 3600;
   apply(db, c);
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000 - 2000);

   db.head_time = c.expiration;
   htlc_refund_operation rf;
   rf.from = "alice";
   rf.htlc_id = 1;
   apply(db, rf);
   CHECK(db.htlcs[1].status == htlc_status::refunded);
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000);

   // Second refund must fail (htlc not open); alice not double-credited
   CHECK_THROW(apply(db, rf));
   CHECK(db.htlcs[1].status == htlc_status::refunded);
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000);
}

static void test_htlc_expiration_too_soon_fails() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {7, 7, 7};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{100, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   // strictly less than MIN duration from head_time
   c.expiration = db.head_time + HTLC_MIN_DURATION_SEC - 1;
   CHECK_THROW(apply(db, c));
   CHECK(db.htlcs.empty());
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000);
}

static void test_htlc_expiration_too_far_fails() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {8, 8, 8};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{100, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   // strictly greater than MAX duration from head_time
   c.expiration = db.head_time + HTLC_MAX_DURATION_SEC + 1;
   CHECK_THROW(apply(db, c));
   CHECK(db.htlcs.empty());
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000);
}

static void test_htlc_insufficient_balance_fails() {
   auto db = make_db();
   std::vector<uint8_t> preimage = {9, 9, 9, 9};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{1'000'000 + 1, asset_symbol::HIVE}; // one more than alice has
   c.preimage_hash = dig;
   c.preimage_size = uint16_t(preimage.size());
   c.expiration = db.head_time + 3600;
   CHECK_THROW(apply(db, c));
   CHECK(db.htlcs.empty());
   CHECK(db.get_balance("alice", asset_symbol::HIVE) == 1'000'000);
}

static void test_htlc_wrong_preimage_size_fails() {
   auto db = make_db();
   // Create locks exact preimage_size = 6 matching "secret"
   std::vector<uint8_t> preimage = {'s','e','c','r','e','t'};
   auto dig = digest_of(hash_algo::sha256, preimage);

   htlc_create_operation c;
   c.from = "alice";
   c.to = "bob";
   c.amount = asset{500, asset_symbol::HIVE};
   c.preimage_hash = dig;
   c.preimage_size = 6;
   c.expiration = db.head_time + 3600;
   apply(db, c);

   htlc_redeem_operation r;
   r.to = "bob";
   r.htlc_id = 1;

   // Too short (valid non-empty for op.validate, wrong vs create size)
   r.preimage = {'s','e','c','r','e'};
   CHECK_THROW(apply(db, r));

   // Too long (hash would also fail, but size check runs first)
   r.preimage = {'s','e','c','r','e','t','!'};
   CHECK_THROW(apply(db, r));

   // Correct size but wrong bytes still fails hash; size gate allows it through
   r.preimage = {'w','r','o','n','g','!'};
   CHECK_THROW(apply(db, r));

   // Correct size + correct preimage succeeds (control)
   r.preimage = preimage;
   apply(db, r);
   CHECK(db.htlcs[1].status == htlc_status::redeemed);
   CHECK(db.get_balance("bob", asset_symbol::HIVE) == 1'000'000 + 500);
}

static void test_contracts() {
   auto db = make_db();
   contract_deploy_operation d;
   d.owner = "alice";
   d.code = {0x00, 0x61, 0x73, 0x6d}; // fake wasm magic prefix
   d.fuel_limit = 10'000;
   apply(db, d);
   CHECK(db.contracts.size() == 1);

   contract_call_operation call;
   call.caller = "bob";
   call.contract_id = 1;
   call.export_name = "call";
   call.fuel_limit = 5'000;
   std::string w = "WRITE:k:v";
   call.args.assign(w.begin(), w.end());
   apply(db, call);
   CHECK(db.contract_storage[1]["k"] == std::vector<uint8_t>({'v'}));

   // denied transfer host
   std::string deny = "DENY:transfer";
   call.args.assign(deny.begin(), deny.end());
   CHECK_THROW(apply(db, call));

   // out of fuel
   call.fuel_limit = 10;
   std::string burn = "BURN:100000";
   call.args.assign(burn.begin(), burn.end());
   CHECK_THROW(apply(db, call));
}

static void test_rc_positive() {
   nft_transfer_operation t;
   t.from = "a";
   t.to = "b";
   t.nft_id = 1;
   CHECK(rc::cost_nft_transfer(t) >= rc::TRANSFER_BASE);
   CHECK(rc::cost_nft_mint(nft_mint_operation{}) > rc::cost_nft_transfer(t));
}

static void test_host_allow_list() {
   CHECK(contracts::host_allowed(contracts::host_fn::read_storage));
   CHECK(!contracts::host_allowed(contracts::host_fn::transfer));
   CHECK(!contracts::host_allowed(contracts::host_fn::net));
}

int main() {
   test_crypto();
   test_nft_happy_path();
   test_nft_approval_for_all();
   test_nft_soulbound_and_auth();
   test_nft_max_supply();
   test_nft_light_skip();
   test_nft_burn_reduces_supply();
   test_nft_approval_collection_scope();
   test_nft_revoke_approval_for_all();
   test_nft_mint_non_creator_fails();
   test_nft_symbol_duplicate_fails();
   test_nft_transfer_to_self_fails_validate();
   test_htlc_redeem();
   test_htlc_edge_cases();
   test_htlc_ripemd();
   test_htlc_double_redeem_fails();
   test_htlc_double_refund_fails();
   test_htlc_expiration_too_soon_fails();
   test_htlc_expiration_too_far_fails();
   test_htlc_insufficient_balance_fails();
   test_htlc_wrong_preimage_size_fails();
   test_contracts();
   test_rc_positive();
   test_host_allow_list();

   std::cout << "passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
