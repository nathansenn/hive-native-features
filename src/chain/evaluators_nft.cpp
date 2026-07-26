/**
 * NFT evaluators
 * Parallelism:
 *  - create_collection: independent by symbol (unique constraint serializes)
 *  - mint: DEPENDENT on collection.supply
 *  - transfer: independent by nft_id; couples account auth
 *  - approve / approval_for_all: independent by object key
 *  - burn: independent by nft_id
 * Task-ID: phase-1 / 1.3
 */
#include "hive_native/chain/evaluators.hpp"
#include <sstream>

namespace hive_native {
namespace chain {

void apply(database& db, const protocol::nft_create_collection_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   if(!db.account_exists(op.creator)) throw protocol_error("creator account missing");
   if(db.collection_by_symbol.count(op.symbol)) throw protocol_error("symbol taken");

   db.last_rc_charged = rc::cost_nft_create_collection(op);

   nft_collection_object c;
   c.id = db.next_collection_id++;
   c.creator = op.creator;
   c.symbol = op.symbol;
   c.name = op.name;
   c.max_supply = op.max_supply;
   c.supply = 0;
   c.transferable = op.transferable;
   c.created = db.head_time;
   db.collections[c.id] = c;
   db.collection_by_symbol[c.symbol] = c.id;

   std::ostringstream p;
   p << "{\"id\":" << c.id << ",\"symbol\":\"" << c.symbol << "\"}";
   db.push_virtual("nft_collection_created", p.str());
}

void apply(database& db, const protocol::nft_mint_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   if(!db.account_exists(op.creator) || !db.account_exists(op.to))
      throw protocol_error("account missing");
   auto cit = db.collections.find(op.collection);
   if(cit == db.collections.end()) throw protocol_error("unknown collection");
   auto& col = cit->second;
   if(col.creator != op.creator) throw protocol_error("not collection creator");
   if(col.max_supply != 0 && col.supply >= col.max_supply)
      throw protocol_error("max supply reached");

   db.last_rc_charged = rc::cost_nft_mint(op);

   nft_object n;
   n.id = db.next_nft_id++;
   n.collection = op.collection;
   n.token_serial = col.supply + 1;
   n.owner = op.to;
   n.metadata_hash = op.metadata_hash;
   n.uri = op.uri;
   n.soulbound = op.soulbound || !col.transferable;
   n.minted = db.head_time;
   db.nfts[n.id] = n;
   col.supply += 1;

   std::ostringstream p;
   p << "{\"nft_id\":" << n.id << ",\"to\":\"" << n.owner << "\"}";
   db.push_virtual("nft_minted", p.str());
}

void apply(database& db, const protocol::nft_transfer_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   if(!db.account_exists(op.to)) throw protocol_error("to missing");
   auto it = db.nfts.find(op.nft_id);
   if(it == db.nfts.end()) throw protocol_error("unknown nft");
   auto& n = it->second;

   const bool is_owner = (n.owner == op.from);
   const bool is_token_approved = (n.approved == op.from);
   const bool is_op_all = db.is_operator_approved(n.owner, op.from, n.collection);
   if(!is_owner && !is_token_approved && !is_op_all)
      throw protocol_error("not authorized to transfer");
   if(n.soulbound) throw protocol_error("soulbound");

   // When operator transfers, from must be current owner for fee_payer pattern:
   // allow op.from as operator; owner field changes from n.owner → to
   db.last_rc_charged = rc::cost_nft_transfer(op);

   const auto prev_owner = n.owner;
   n.owner = op.to;
   n.approved.clear(); // clear per-token approve on transfer

   std::ostringstream p;
   p << "{\"nft_id\":" << n.id << ",\"from\":\"" << prev_owner << "\",\"to\":\"" << n.owner << "\"}";
   db.push_virtual("nft_transferred", p.str());
}

void apply(database& db, const protocol::nft_approve_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   auto it = db.nfts.find(op.nft_id);
   if(it == db.nfts.end()) throw protocol_error("unknown nft");
   if(it->second.owner != op.owner) throw protocol_error("not owner");
   if(!op.approved.empty() && !db.account_exists(op.approved))
      throw protocol_error("approved account missing");

   db.last_rc_charged = rc::cost_nft_approve(op);
   it->second.approved = op.approved;

   std::ostringstream p;
   p << "{\"nft_id\":" << op.nft_id << ",\"approved\":\"" << op.approved << "\"}";
   db.push_virtual("nft_approved", p.str());
}

void apply(database& db, const protocol::nft_set_approval_for_all_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   if(!db.account_exists(op.owner) || !db.account_exists(op.operator_account))
      throw protocol_error("account missing");
   if(op.collection != 0 && !db.collections.count(op.collection))
      throw protocol_error("unknown collection");

   db.last_rc_charged = rc::cost_nft_set_approval_for_all(op);

   auto key = std::make_tuple(op.owner, op.operator_account, op.collection);
   if(!op.approved) {
      db.operators.erase(key);
   } else {
      nft_operator_object o;
      o.owner = op.owner;
      o.operator_account = op.operator_account;
      o.collection = op.collection;
      o.approved = true;
      db.operators[key] = o;
   }

   std::ostringstream p;
   p << "{\"owner\":\"" << op.owner << "\",\"operator\":\"" << op.operator_account
     << "\",\"collection\":" << op.collection << ",\"approved\":" << (op.approved ? "true" : "false") << "}";
   db.push_virtual("nft_approval_for_all", p.str());
}

void apply(database& db, const protocol::nft_burn_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_NFT);
   db.require_full_nft_state();
   op.validate();
   auto it = db.nfts.find(op.nft_id);
   if(it == db.nfts.end()) throw protocol_error("unknown nft");
   if(it->second.owner != op.owner) throw protocol_error("not owner");

   db.last_rc_charged = rc::cost_nft_burn(op);
   const auto collection = it->second.collection;
   db.nfts.erase(it);
   auto cit = db.collections.find(collection);
   if(cit != db.collections.end() && cit->second.supply > 0)
      cit->second.supply -= 1;

   std::ostringstream p;
   p << "{\"nft_id\":" << op.nft_id << ",\"owner\":\"" << op.owner << "\"}";
   db.push_virtual("nft_burned", p.str());
}

} // namespace chain
} // namespace hive_native
