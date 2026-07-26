#include "hive_native/protocol/nft_operations.hpp"
#include "hive_native/protocol/htlc_operations.hpp"
#include "hive_native/protocol/contract_operations.hpp"

namespace hive_native {
namespace protocol {

void nft_create_collection_operation::validate() const {
   assert_account_name(creator);
   assert_len(symbol, MAX_NFT_SYMBOL_LEN, "symbol");
   assert_len(name, MAX_NFT_NAME_LEN, "name");
   if(symbol.empty()) throw protocol_error("symbol required");
   for(char c : symbol) {
      const bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
      if(!ok) throw protocol_error("symbol must be A-Z0-9");
   }
}

void nft_mint_operation::validate() const {
   assert_account_name(creator);
   assert_account_name(to);
   assert_len(uri, MAX_NFT_URI_LEN, "uri");
   if(collection == 0) throw protocol_error("collection required");
}

void nft_transfer_operation::validate() const {
   assert_account_name(from);
   assert_account_name(to);
   assert_len(memo, MAX_MEMO_LEN, "memo");
   if(nft_id == 0) throw protocol_error("nft_id required");
   if(from == to) throw protocol_error("cannot transfer to self");
}

void nft_approve_operation::validate() const {
   assert_account_name(owner);
   if(nft_id == 0) throw protocol_error("nft_id required");
   if(!approved.empty()) assert_account_name(approved);
}

void nft_set_approval_for_all_operation::validate() const {
   assert_account_name(owner);
   assert_account_name(operator_account);
   if(owner == operator_account) throw protocol_error("cannot approve self as operator");
}

void nft_burn_operation::validate() const {
   assert_account_name(owner);
   if(nft_id == 0) throw protocol_error("nft_id required");
}

void htlc_create_operation::validate(time_point_sec now) const {
   assert_account_name(from);
   assert_account_name(to);
   assert_len(memo, MAX_MEMO_LEN, "memo");
   if(!amount.is_positive()) throw protocol_error("amount must be positive");
   if(amount.symbol != asset_symbol::HIVE && amount.symbol != asset_symbol::HBD)
      throw protocol_error("only HIVE/HBD in MVP");
   if(preimage_size == 0 || preimage_size > MAX_HTLC_PREIMAGE_LEN)
      throw protocol_error("invalid preimage_size");
   const size_t expect = (preimage_hash.algo == hash_algo::sha256) ? 32 : 20;
   if(preimage_hash.bytes.size() != expect)
      throw protocol_error("preimage_hash length mismatch");
   if(expiration < now + HTLC_MIN_DURATION_SEC)
      throw protocol_error("expiration too soon");
   if(expiration > now + HTLC_MAX_DURATION_SEC)
      throw protocol_error("expiration too far");
}

void htlc_redeem_operation::validate() const {
   assert_account_name(to);
   if(htlc_id == 0) throw protocol_error("htlc_id required");
   if(preimage.empty() || preimage.size() > MAX_HTLC_PREIMAGE_LEN)
      throw protocol_error("invalid preimage");
}

void htlc_refund_operation::validate() const {
   assert_account_name(from);
   if(htlc_id == 0) throw protocol_error("htlc_id required");
}

void contract_deploy_operation::validate() const {
   assert_account_name(owner);
   if(fuel_limit == 0) throw protocol_error("fuel_limit required");
   if(code.size() > MAX_CODE_BYTES) throw protocol_error("code too large");
   if(code.empty()) throw protocol_error("code required in portable deploy");
   if(init_args.size() > MAX_CONTRACT_ARGS) throw protocol_error("init_args too large");
   auto h = sha256(code);
   // code_hash must match if non-zero
   bool zero = true;
   for(auto b : code_hash) if(b) { zero = false; break; }
   if(!zero && h != code_hash) throw protocol_error("code_hash mismatch");
}

void contract_call_operation::validate() const {
   assert_account_name(caller);
   if(contract_id == 0) throw protocol_error("contract_id required");
   if(fuel_limit == 0) throw protocol_error("fuel_limit required");
   assert_len(export_name, MAX_CONTRACT_EXPORT, "export_name");
   if(export_name.empty()) throw protocol_error("export_name required");
   if(args.size() > MAX_CONTRACT_ARGS) throw protocol_error("args too large");
}

} // namespace protocol
} // namespace hive_native
