#pragma once
/**
 * Evaluators: validate → charge RC → mutate → virtual ops.
 * Parallelism notes annotated per evaluator.
 * Task-ID: phase-1 / 1.3, phase-2, phase-3b
 */
#include "hive_native/chain/database.hpp"
#include "hive_native/rc/costs.hpp"

namespace hive_native {
namespace chain {

// NFT
void apply(database& db, const protocol::nft_create_collection_operation& op);
void apply(database& db, const protocol::nft_mint_operation& op);
void apply(database& db, const protocol::nft_transfer_operation& op);
void apply(database& db, const protocol::nft_approve_operation& op);
void apply(database& db, const protocol::nft_set_approval_for_all_operation& op);
void apply(database& db, const protocol::nft_burn_operation& op);

// HTLC
void apply(database& db, const protocol::htlc_create_operation& op);
void apply(database& db, const protocol::htlc_redeem_operation& op);
void apply(database& db, const protocol::htlc_refund_operation& op);

// Contracts (plugin / HF path)
void apply(database& db, const protocol::contract_deploy_operation& op);
void apply(database& db, const protocol::contract_call_operation& op);

} // namespace chain
} // namespace hive_native
