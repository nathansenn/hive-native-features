#pragma once
/**
 * In-memory chainbase stand-in for portable verification.
 * Task-ID: phase-1 / 1.2 + phase-2 state + phase-3 storage
 *
 * Models undo sessions lightly for tests. Upstream will use real chainbase.
 */
#include "hive_native/protocol/nft_operations.hpp"
#include "hive_native/protocol/htlc_operations.hpp"
#include "hive_native/protocol/contract_operations.hpp"
#include "hive_native/util/types.hpp"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace hive_native {
namespace chain {

struct account_balance {
   share_type hive = 0;
   share_type hbd  = 0;
};

struct nft_collection_object {
   protocol::collection_id_type id = 0;
   account_name_type creator;
   std::string symbol;
   std::string name;
   uint64_t max_supply = 0;
   uint64_t supply = 0;
   bool transferable = true;
   time_point_sec created = 0;
};

struct nft_object {
   protocol::nft_id_type id = 0;
   protocol::collection_id_type collection = 0;
   uint64_t token_serial = 0;
   account_name_type owner;
   account_name_type approved; // per-token
   sha256_t metadata_hash{};
   std::string uri;
   bool soulbound = false;
   time_point_sec minted = 0;
};

/** ADR-0001 approval-for-all */
struct nft_operator_object {
   account_name_type owner;
   account_name_type operator_account;
   protocol::collection_id_type collection = 0; // 0 = all
   bool approved = true;
};

enum class htlc_status : uint8_t { open = 0, redeemed = 1, refunded = 2 };

struct htlc_object {
   protocol::htlc_id_type id = 0;
   account_name_type from;
   account_name_type to;
   asset amount;
   hash_digest preimage_hash;
   uint16_t preimage_size = 0;
   time_point_sec expiration = 0;
   time_point_sec created = 0;
   std::string memo;
   htlc_status status = htlc_status::open;
};

struct contract_object {
   protocol::contract_id_type id = 0;
   account_name_type owner;
   sha256_t code_hash{};
   std::vector<uint8_t> code;
   time_point_sec created = 0;
};

struct virtual_op {
   std::string name;
   std::string payload_json; // simple debug form
};

struct node_config {
   bool nft_skip_state = false;   // light/mobile non-consensus
   bool htlc_skip_state = false;
   bool contracts_skip = false;
   bool is_consensus_node = true; // witnesses must be true
   uint32_t hardfork = HIVE_HARDFORK_CONTRACTS; // tests enable all
   // Optional named presets: see node_profiles.hpp (full / api_pruned / mobile_light).
   // Call apply_profile(config, node_profile::...) or apply_default_profile(config)
   // at startup. CMake -DHIVE_LIGHT_NODE=ON selects mobile_light as default_profile().
};

class database {
public:
   time_point_sec head_time = 0;
   node_config    config;
   uint64_t       next_collection_id = 1;
   uint64_t       next_nft_id = 1;
   uint64_t       next_htlc_id = 1;
   uint64_t       next_contract_id = 1;

   std::map<account_name_type, account_balance> balances;
   std::map<protocol::collection_id_type, nft_collection_object> collections;
   std::map<std::string, protocol::collection_id_type> collection_by_symbol;
   std::map<protocol::nft_id_type, nft_object> nfts;
   // key: owner|operator|collection
   std::map<std::tuple<std::string,std::string,uint64_t>, nft_operator_object> operators;
   std::map<protocol::htlc_id_type, htlc_object> htlcs;
   std::map<protocol::contract_id_type, contract_object> contracts;
   // contract_id -> key -> value
   std::map<protocol::contract_id_type, std::map<std::string, std::vector<uint8_t>>> contract_storage;

   std::vector<virtual_op> pending_virtual_ops;
   uint64_t last_rc_charged = 0;

   void create_account(const account_name_type& name, share_type hive = 0, share_type hbd = 0);
   bool account_exists(const account_name_type& name) const;
   share_type get_balance(const account_name_type& name, asset_symbol sym) const;
   void adjust_balance(const account_name_type& name, asset delta);

   bool is_operator_approved(const account_name_type& owner,
                             const account_name_type& op,
                             protocol::collection_id_type collection) const;

   uint32_t open_htlc_count(const account_name_type& from) const;

   void push_virtual(std::string name, std::string payload);
   void clear_virtual();

   void require_hardfork(uint32_t hf) const;
   void require_full_nft_state() const;
   void require_full_htlc_state() const;
};

} // namespace chain
} // namespace hive_native
