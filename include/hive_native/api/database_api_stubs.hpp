#pragma once
/**
 * database_api method stubs — signatures for upstream plugins.
 * Task-ID: phase-1 / 1.9, phase-2, phase-3
 *
 * Light-mode rules (all list_* methods):
 *   - limit clamped to [1, 100]
 *   - light=true omits heavy fields (uri, memo, code)
 *   - skip-state config returns empty / nullopt without scanning
 *
 * SQL mapping: docs/swarm/07-haf-api.md
 */
#include "hive_native/chain/database.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hive_native {
namespace api {

/** Shared pagination / light-mode args for list methods. */
struct list_args {
   uint64_t start = 0;   // exclusive lower bound on primary id (or key)
   uint32_t limit = 100; // light max 100
   bool light = false;
};

inline uint32_t clamp_limit(uint32_t limit) {
   if(limit == 0) return 1;
   if(limit > 100) return 100;
   return limit;
}

// ---- NFT (phase-1) ----

std::optional<chain::nft_object>
get_nft(const chain::database& db, protocol::nft_id_type id, bool light = false);

std::optional<chain::nft_collection_object>
get_nft_collection(const chain::database& db, protocol::collection_id_type id);

std::optional<chain::nft_collection_object>
get_nft_collection_by_symbol(const chain::database& db, const std::string& symbol);

std::vector<chain::nft_object>
list_nfts_by_owner(const chain::database& db, const account_name_type& owner, list_args args);

std::vector<chain::nft_object>
list_nfts_by_collection(const chain::database& db, protocol::collection_id_type c, list_args args);

// ---- HTLC (phase-2) ----

std::optional<chain::htlc_object>
get_htlc(const chain::database& db, protocol::htlc_id_type id, bool light = false);

std::vector<chain::htlc_object>
list_htlcs_by_from(const chain::database& db, const account_name_type& from, list_args args);

std::vector<chain::htlc_object>
list_htlcs_by_to(const chain::database& db, const account_name_type& to, list_args args);

/** List open HTLCs with expiration >= start (start interpreted as time_point_sec). */
std::vector<chain::htlc_object>
list_htlcs_by_expiration(const chain::database& db, time_point_sec min_expiration, list_args args);

// ---- Contracts (phase-3) ----

std::optional<chain::contract_object>
get_contract(const chain::database& db, protocol::contract_id_type id, bool light = true);

std::vector<chain::contract_object>
list_contracts_by_owner(const chain::database& db, const account_name_type& owner, list_args args);

/** Read one storage key; nullopt if missing or contracts_skip. */
std::optional<std::vector<uint8_t>>
get_storage_key(const chain::database& db,
                protocol::contract_id_type id,
                const std::string& key);

} // namespace api
} // namespace hive_native
