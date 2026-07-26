/**
 * HTLC evaluators — redeem requires `to` active auth (ADR-0001).
 * Edge: redeem fails when head_time >= expiration; refund only after.
 * Task-ID: phase-2
 */
#include "hive_native/chain/evaluators.hpp"
#include <sstream>

namespace hive_native {
namespace chain {

void apply(database& db, const protocol::htlc_create_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_HTLC);
   db.require_full_htlc_state();
   op.validate(db.head_time);
   if(!db.account_exists(op.from) || !db.account_exists(op.to))
      throw protocol_error("account missing");
   if(db.open_htlc_count(op.from) >= HTLC_MAX_OPEN_PER_ACCOUNT)
      throw protocol_error("too many open htlcs");

   db.last_rc_charged = rc::cost_htlc_create(op);
   db.adjust_balance(op.from, asset{-op.amount.amount, op.amount.symbol});

   htlc_object h;
   h.id = db.next_htlc_id++;
   h.from = op.from;
   h.to = op.to;
   h.amount = op.amount;
   h.preimage_hash = op.preimage_hash;
   h.preimage_size = op.preimage_size;
   h.expiration = op.expiration;
   h.created = db.head_time;
   h.memo = op.memo;
   h.status = htlc_status::open;
   db.htlcs[h.id] = h;

   std::ostringstream p;
   p << "{\"htlc_id\":" << h.id << ",\"from\":\"" << h.from << "\",\"to\":\"" << h.to << "\"}";
   db.push_virtual("htlc_created", p.str());
}

void apply(database& db, const protocol::htlc_redeem_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_HTLC);
   db.require_full_htlc_state();
   op.validate();
   auto it = db.htlcs.find(op.htlc_id);
   if(it == db.htlcs.end()) throw protocol_error("unknown htlc");
   auto& h = it->second;
   if(h.status != htlc_status::open) throw protocol_error("htlc not open");
   if(h.to != op.to) throw protocol_error("redeem requires to authority"); // ADR-0001
   if(db.head_time >= h.expiration) throw protocol_error("htlc expired");
   if(op.preimage.size() != h.preimage_size)
      throw protocol_error("preimage size mismatch");

   auto dig = digest_of(h.preimage_hash.algo, op.preimage);
   if(!constant_time_equal(dig.bytes, h.preimage_hash.bytes))
      throw protocol_error("preimage hash mismatch");

   db.last_rc_charged = rc::cost_htlc_redeem(op);
   db.adjust_balance(h.to, h.amount);
   h.status = htlc_status::redeemed;

   std::ostringstream p;
   p << "{\"htlc_id\":" << h.id << ",\"to\":\"" << h.to << "\"}";
   db.push_virtual("htlc_redeemed", p.str());
   // prune closed from hot map after irreversible in production; keep status for tests
}

void apply(database& db, const protocol::htlc_refund_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_HTLC);
   db.require_full_htlc_state();
   op.validate();
   auto it = db.htlcs.find(op.htlc_id);
   if(it == db.htlcs.end()) throw protocol_error("unknown htlc");
   auto& h = it->second;
   if(h.status != htlc_status::open) throw protocol_error("htlc not open");
   if(h.from != op.from) throw protocol_error("refund requires from authority");
   if(db.head_time < h.expiration) throw protocol_error("htlc not yet expired");

   db.last_rc_charged = rc::cost_htlc_refund(op);
   db.adjust_balance(h.from, h.amount);
   h.status = htlc_status::refunded;

   std::ostringstream p;
   p << "{\"htlc_id\":" << h.id << ",\"from\":\"" << h.from << "\"}";
   db.push_virtual("htlc_refunded", p.str());
}

} // namespace chain
} // namespace hive_native
