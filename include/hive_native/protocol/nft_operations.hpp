#pragma once
/**
 * Native NFT protocol operations — Phase 1.
 * Task-ID: phase-1 / 1.1
 *
 * Designed for later insertion into Hive static_variant operation lists.
 * Approval-for-all included per ADR-0001.
 */
#include "hive_native/util/types.hpp"
#include <optional>
#include <vector>

namespace hive_native {
namespace protocol {

using collection_id_type = uint64_t;
using nft_id_type        = uint64_t;

struct nft_create_collection_operation {
   account_name_type creator;
   std::string       symbol;      // unique, <= MAX_NFT_SYMBOL_LEN
   std::string       name;        // <= MAX_NFT_NAME_LEN
   uint64_t          max_supply = 0; // 0 = unlimited
   bool              transferable = true;

   void validate() const;
   account_name_type fee_payer() const { return creator; }
   std::vector<account_name_type> get_required_active_authorities() const { return {creator}; }
};

struct nft_mint_operation {
   account_name_type creator; // must be collection creator (v1)
   collection_id_type collection = 0;
   account_name_type to;
   sha256_t          metadata_hash{};
   std::string       uri; // optional, bounded
   bool              soulbound = false;

   void validate() const;
   account_name_type fee_payer() const { return creator; }
   std::vector<account_name_type> get_required_active_authorities() const { return {creator}; }
};

struct nft_transfer_operation {
   account_name_type from;
   account_name_type to;
   nft_id_type       nft_id = 0;
   std::string       memo;

   void validate() const;
   account_name_type fee_payer() const { return from; }
   // Auth resolved at apply: owner or approved or operator-for-all
   std::vector<account_name_type> get_required_active_authorities() const { return {from}; }
};

struct nft_approve_operation {
   account_name_type owner;
   nft_id_type       nft_id = 0;
   account_name_type approved; // empty clears

   void validate() const;
   account_name_type fee_payer() const { return owner; }
   std::vector<account_name_type> get_required_active_authorities() const { return {owner}; }
};

/** ADR-0001: approval-for-all in MVP */
struct nft_set_approval_for_all_operation {
   account_name_type owner;
   account_name_type operator_account;
   collection_id_type collection = 0; // 0 = all collections owned by owner
   bool approved = true;

   void validate() const;
   account_name_type fee_payer() const { return owner; }
   std::vector<account_name_type> get_required_active_authorities() const { return {owner}; }
};

struct nft_burn_operation {
   account_name_type owner;
   nft_id_type       nft_id = 0;

   void validate() const;
   account_name_type fee_payer() const { return owner; }
   std::vector<account_name_type> get_required_active_authorities() const { return {owner}; }
};

// ---- Virtual ops (HAF / indexers) ----
struct nft_collection_created_operation {
   collection_id_type collection = 0;
   account_name_type  creator;
   std::string        symbol;
};

struct nft_minted_operation {
   nft_id_type        nft_id = 0;
   collection_id_type collection = 0;
   account_name_type  to;
   sha256_t           metadata_hash{};
};

struct nft_transferred_operation {
   nft_id_type       nft_id = 0;
   account_name_type from;
   account_name_type to;
};

struct nft_approved_operation {
   nft_id_type       nft_id = 0;
   account_name_type owner;
   account_name_type approved;
};

struct nft_approval_for_all_operation {
   account_name_type  owner;
   account_name_type  operator_account;
   collection_id_type collection = 0;
   bool               approved = true;
};

struct nft_burned_operation {
   nft_id_type       nft_id = 0;
   account_name_type owner;
   collection_id_type collection = 0;
};

} // namespace protocol
} // namespace hive_native
