#pragma once
/**
 * Metered contract operations — Phase 3 (plugin-first; HF-gated for consensus).
 * Task-ID: phase-3 / 3b
 * Runtime: Wasmtime (ADR-0002)
 */
#include "hive_native/util/types.hpp"
#include <vector>

namespace hive_native {
namespace protocol {

using contract_id_type = uint64_t;

struct contract_deploy_operation {
   account_name_type owner;
   std::vector<uint8_t> code; // WASM bytes (or empty if code_hash-only publish)
   sha256_t          code_hash{};
   uint64_t          fuel_limit = 0;
   std::vector<uint8_t> init_args;

   void validate() const;
   account_name_type fee_payer() const { return owner; }
   std::vector<account_name_type> get_required_active_authorities() const { return {owner}; }
};

struct contract_call_operation {
   account_name_type caller;
   contract_id_type  contract_id = 0;
   std::string       export_name; // e.g. "call"
   std::vector<uint8_t> args;
   uint64_t          fuel_limit = 0;

   void validate() const;
   account_name_type fee_payer() const { return caller; }
   std::vector<account_name_type> get_required_active_authorities() const { return {caller}; }
};

struct contract_deployed_operation {
   contract_id_type  contract_id = 0;
   account_name_type owner;
   sha256_t          code_hash{};
};

struct contract_called_operation {
   contract_id_type  contract_id = 0;
   account_name_type caller;
   uint64_t          fuel_used = 0;
   bool              success = false;
};

} // namespace protocol
} // namespace hive_native
