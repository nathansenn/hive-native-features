/**
 * Contract deploy/call evaluators — plugin path; HF for consensus later.
 * Task-ID: phase-3 / 3b-3d
 */
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/contracts/engine.hpp"
#include <sstream>

namespace hive_native {
namespace chain {

void apply(database& db, const protocol::contract_deploy_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_CONTRACTS);
   if(db.config.contracts_skip && !db.config.is_consensus_node)
      throw protocol_error("contracts skipped");
   op.validate();
   if(!db.account_exists(op.owner)) throw protocol_error("account missing");

   contracts::host_limits lim;
   lim.fuel = op.fuel_limit;
   auto& eng = contracts::default_engine();
   auto res = eng.deploy_and_init(op.code, op.init_args, lim);
   if(!res.success) throw protocol_error(res.error.empty() ? "deploy failed" : res.error);

   db.last_rc_charged = rc::cost_contract_deploy(op);

   contract_object c;
   c.id = db.next_contract_id++;
   c.owner = op.owner;
   c.code = op.code;
   c.code_hash = sha256(op.code);
   c.created = db.head_time;
   db.contracts[c.id] = c;
   db.contract_storage[c.id] = {};

   std::ostringstream p;
   p << "{\"contract_id\":" << c.id << ",\"owner\":\"" << c.owner << "\"}";
   db.push_virtual("contract_deployed", p.str());
}

void apply(database& db, const protocol::contract_call_operation& op) {
   db.require_hardfork(HIVE_HARDFORK_CONTRACTS);
   if(db.config.contracts_skip && !db.config.is_consensus_node)
      throw protocol_error("contracts skipped");
   op.validate();
   if(!db.account_exists(op.caller)) throw protocol_error("account missing");
   auto it = db.contracts.find(op.contract_id);
   if(it == db.contracts.end()) throw protocol_error("unknown contract");

   contracts::host_limits lim;
   lim.fuel = op.fuel_limit;
   auto& eng = contracts::default_engine();
   auto& storage = db.contract_storage[op.contract_id];
   auto res = eng.call(it->second.code, op.export_name, op.args, storage, lim, op.caller);

   db.last_rc_charged = rc::cost_contract_call(op, res.fuel_used);

   if(!res.success) {
      std::ostringstream p;
      p << "{\"contract_id\":" << op.contract_id << ",\"success\":false,\"fuel\":" << res.fuel_used << "}";
      db.push_virtual("contract_called", p.str());
      throw protocol_error(res.error.empty() ? "call failed" : res.error);
   }

   // Commit storage writes only on success (no partial mutation)
   for(const auto& [k, v] : res.storage_writes)
      storage[k] = v;

   std::ostringstream p;
   p << "{\"contract_id\":" << op.contract_id << ",\"success\":true,\"fuel\":" << res.fuel_used << "}";
   db.push_virtual("contract_called", p.str());
}

} // namespace chain
} // namespace hive_native
