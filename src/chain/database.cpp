#include "hive_native/chain/database.hpp"

namespace hive_native {
namespace chain {

void database::create_account(const account_name_type& name, share_type hive, share_type hbd) {
   assert_account_name(name);
   balances[name] = account_balance{hive, hbd};
}

bool database::account_exists(const account_name_type& name) const {
   return balances.find(name) != balances.end();
}

share_type database::get_balance(const account_name_type& name, asset_symbol sym) const {
   auto it = balances.find(name);
   if(it == balances.end()) throw protocol_error("account missing");
   return sym == asset_symbol::HIVE ? it->second.hive : it->second.hbd;
}

void database::adjust_balance(const account_name_type& name, asset delta) {
   if(!account_exists(name)) throw protocol_error("account missing");
   auto& b = balances[name];
   share_type& field = (delta.symbol == asset_symbol::HIVE) ? b.hive : b.hbd;
   if(delta.amount < 0 && field < -delta.amount)
      throw protocol_error("insufficient balance");
   field += delta.amount;
}

bool database::is_operator_approved(const account_name_type& owner,
                                    const account_name_type& op,
                                    protocol::collection_id_type collection) const {
   // collection-specific
   auto k1 = std::make_tuple(owner, op, collection);
   auto it = operators.find(k1);
   if(it != operators.end() && it->second.approved) return true;
   // all-collections (collection=0)
   auto k0 = std::make_tuple(owner, op, uint64_t(0));
   it = operators.find(k0);
   return it != operators.end() && it->second.approved;
}

uint32_t database::open_htlc_count(const account_name_type& from) const {
   uint32_t n = 0;
   for(const auto& [id, h] : htlcs) {
      (void)id;
      if(h.from == from && h.status == htlc_status::open) ++n;
   }
   return n;
}

void database::push_virtual(std::string name, std::string payload) {
   pending_virtual_ops.push_back(virtual_op{std::move(name), std::move(payload)});
}

void database::clear_virtual() { pending_virtual_ops.clear(); }

void database::require_hardfork(uint32_t hf) const {
   if(config.hardfork < hf)
      throw protocol_error("hardfork not active");
}

void database::require_full_nft_state() const {
   if(config.is_consensus_node && config.nft_skip_state)
      throw protocol_error("consensus node cannot skip NFT state");
   if(config.nft_skip_state)
      throw protocol_error("NFT state skipped on this node");
}

void database::require_full_htlc_state() const {
   if(config.is_consensus_node && config.htlc_skip_state)
      throw protocol_error("consensus node cannot skip HTLC state");
   if(config.htlc_skip_state)
      throw protocol_error("HTLC state skipped on this node");
}

} // namespace chain
} // namespace hive_native
