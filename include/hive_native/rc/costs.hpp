#pragma once
/**
 * RC cost functions — placeholders calibrated relatively.
 * Task-ID: phase-1 / 1.4, phase-2 RC, phase-3d fuel→RC
 *
 * Absolute RC units are Hive-internal; here we expose relative micro-units
 * for tests and microbenchmarks (1 transfer_base = 1000).
 */
#include "hive_native/protocol/nft_operations.hpp"
#include "hive_native/protocol/htlc_operations.hpp"
#include "hive_native/protocol/contract_operations.hpp"
#include <cstdint>

namespace hive_native {
namespace rc {

inline constexpr uint64_t TRANSFER_BASE = 1000;

uint64_t cost_nft_create_collection(const protocol::nft_create_collection_operation& op);
uint64_t cost_nft_mint(const protocol::nft_mint_operation& op);
uint64_t cost_nft_transfer(const protocol::nft_transfer_operation& op);
uint64_t cost_nft_approve(const protocol::nft_approve_operation& op);
uint64_t cost_nft_set_approval_for_all(const protocol::nft_set_approval_for_all_operation& op);
uint64_t cost_nft_burn(const protocol::nft_burn_operation& op);

uint64_t cost_htlc_create(const protocol::htlc_create_operation& op);
uint64_t cost_htlc_redeem(const protocol::htlc_redeem_operation& op);
uint64_t cost_htlc_refund(const protocol::htlc_refund_operation& op);

/** fuel → RC: linear with slope; state writes extra (contracts) */
uint64_t cost_contract_deploy(const protocol::contract_deploy_operation& op);
uint64_t cost_contract_call(const protocol::contract_call_operation& op, uint64_t fuel_used);

inline constexpr uint64_t FUEL_TO_RC_NUM = 1;
inline constexpr uint64_t FUEL_TO_RC_DEN = 10; // 10 fuel units → 1 RC micro

} // namespace rc
} // namespace hive_native
