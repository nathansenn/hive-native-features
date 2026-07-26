#include "hive_native/rc/costs.hpp"

namespace hive_native {
namespace rc {

static uint64_t bytes_cost(size_t n) { return n * 2; }

uint64_t cost_nft_create_collection(const protocol::nft_create_collection_operation& op) {
   return TRANSFER_BASE * 3 + bytes_cost(op.symbol.size() + op.name.size()) + 500; // state
}

uint64_t cost_nft_mint(const protocol::nft_mint_operation& op) {
   return TRANSFER_BASE * 2 + bytes_cost(op.uri.size()) + 800;
}

uint64_t cost_nft_transfer(const protocol::nft_transfer_operation& op) {
   return TRANSFER_BASE + bytes_cost(op.memo.size());
}

uint64_t cost_nft_approve(const protocol::nft_approve_operation&) {
   return TRANSFER_BASE / 2;
}

uint64_t cost_nft_set_approval_for_all(const protocol::nft_set_approval_for_all_operation&) {
   return TRANSFER_BASE / 2 + 100;
}

uint64_t cost_nft_burn(const protocol::nft_burn_operation&) {
   return TRANSFER_BASE;
}

uint64_t cost_htlc_create(const protocol::htlc_create_operation& op) {
   return TRANSFER_BASE * 2 + bytes_cost(op.memo.size()) + 600;
}

uint64_t cost_htlc_redeem(const protocol::htlc_redeem_operation& op) {
   return TRANSFER_BASE + bytes_cost(op.preimage.size()) + 200; // hash
}

uint64_t cost_htlc_refund(const protocol::htlc_refund_operation&) {
   return TRANSFER_BASE;
}

uint64_t cost_contract_deploy(const protocol::contract_deploy_operation& op) {
   return TRANSFER_BASE * 10 + bytes_cost(op.code.size()) + (op.fuel_limit * FUEL_TO_RC_NUM) / FUEL_TO_RC_DEN;
}

uint64_t cost_contract_call(const protocol::contract_call_operation& op, uint64_t fuel_used) {
   const uint64_t fuel = fuel_used ? fuel_used : op.fuel_limit;
   return TRANSFER_BASE * 2 + bytes_cost(op.args.size()) + (fuel * FUEL_TO_RC_NUM) / FUEL_TO_RC_DEN;
}

} // namespace rc
} // namespace hive_native
