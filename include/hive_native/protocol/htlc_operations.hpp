#pragma once
/**
 * HTLC atomic swap operations — Phase 2.
 * Task-ID: phase-2 / protocol
 * Redeem authority: `to` only (ADR-0001).
 */
#include "hive_native/util/types.hpp"
#include <vector>

namespace hive_native {
namespace protocol {

using htlc_id_type = uint64_t;

struct htlc_create_operation {
   account_name_type from;
   account_name_type to;
   asset             amount;
   hash_digest       preimage_hash;
   uint16_t          preimage_size = 0; // exact size required at redeem
   time_point_sec    expiration = 0;
   std::string       memo;

   void validate(time_point_sec now) const;
   account_name_type fee_payer() const { return from; }
   std::vector<account_name_type> get_required_active_authorities() const { return {from}; }
};

struct htlc_redeem_operation {
   account_name_type to; // must match HTLC.to and sign (ADR-0001)
   htlc_id_type      htlc_id = 0;
   std::vector<uint8_t> preimage;

   void validate() const;
   account_name_type fee_payer() const { return to; }
   std::vector<account_name_type> get_required_active_authorities() const { return {to}; }
};

struct htlc_refund_operation {
   account_name_type from; // locker; funds always return to HTLC.from
   htlc_id_type      htlc_id = 0;

   void validate() const;
   account_name_type fee_payer() const { return from; }
   std::vector<account_name_type> get_required_active_authorities() const { return {from}; }
};

// Virtual
struct htlc_created_operation {
   htlc_id_type      htlc_id = 0;
   account_name_type from;
   account_name_type to;
   asset             amount;
   time_point_sec    expiration = 0;
};

struct htlc_redeemed_operation {
   htlc_id_type      htlc_id = 0;
   account_name_type from;
   account_name_type to;
   asset             amount;
};

struct htlc_refunded_operation {
   htlc_id_type      htlc_id = 0;
   account_name_type from;
   asset             amount;
};

} // namespace protocol
} // namespace hive_native
