/**
 * Null engine + host policy simulation (CI / default_engine).
 *
 * No real WASM: deterministic meter-only stand-in until Wasmtime is linked
 * (ADR-0002; HIVE_NATIVE_WITH_WASMTIME). Evaluators still treat results the
 * same way: storage_writes are staged here and committed only on success.
 *
 * Interprets a tiny meta-protocol in args for tests:
 *   "WRITE:key:value"  — storage write (fuel charged via host_fuel_weight;
 *                        size-checked; staged in storage_writes only after burn)
 *   "READ:key"         — storage read fuel (value not returned in stub)
 *   "DENY:transfer"    — attempts denied host (host_allowed == false → fail)
 *   "BURN:N"           — burn N fuel; cannot exceed host_limits.fuel (no bypass)
 *   "OK" / empty       — success no-op after base call fuel
 *
 * All paths meter through burn(); out-of-fuel → success=false, no new writes.
 * Task-ID: phase-3 / 3a, 3e
 */
#include "hive_native/contracts/engine.hpp"
#include <sstream>

namespace hive_native {
namespace contracts {

static bool burn(call_result& r, host_limits& lim, uint64_t amount) {
   if(r.fuel_used + amount > lim.fuel) {
      r.success = false;
      r.error = "out of fuel";
      r.fuel_used = lim.fuel;
      return false;
   }
   r.fuel_used += amount;
   return true;
}

call_result null_engine::deploy_and_init(const std::vector<uint8_t>& code,
                                         const std::vector<uint8_t>&,
                                         host_limits limits) {
   call_result r;
   if(code.empty()) {
      r.error = "empty code";
      return r;
   }
   // base deploy fuel
   if(!burn(r, limits, 100 + code.size() / 64)) return r;
   r.success = true;
   return r;
}

call_result null_engine::call(const std::vector<uint8_t>& code,
                              const std::string& export_name,
                              const std::vector<uint8_t>& args,
                              const std::map<std::string, std::vector<uint8_t>>& storage,
                              host_limits limits,
                              const account_name_type& /*caller*/) {
   call_result r;
   if(code.empty()) {
      r.error = "empty code";
      return r;
   }
   if(export_name.empty()) {
      r.error = "empty export";
      return r;
   }
   if(!burn(r, limits, 50)) return r;

   std::string cmd(args.begin(), args.end());
   if(cmd.empty() || cmd == "OK") {
      r.success = true;
      return r;
   }
   if(cmd.rfind("BURN:", 0) == 0) {
      uint64_t n = std::stoull(cmd.substr(5));
      if(!burn(r, limits, n)) return r;
      r.success = true;
      return r;
   }
   if(cmd.rfind("DENY:", 0) == 0) {
      std::string which = cmd.substr(5);
      if(which == "transfer" && !host_allowed(host_fn::transfer)) {
         r.denied_hosts.push_back("transfer");
         r.error = "host function denied: transfer";
         r.success = false;
         return r;
      }
      r.error = "unknown deny target";
      return r;
   }
   if(cmd.rfind("WRITE:", 0) == 0) {
      auto rest = cmd.substr(6);
      auto pos = rest.find(':');
      if(pos == std::string::npos) {
         r.error = "bad WRITE";
         return r;
      }
      std::string key = rest.substr(0, pos);
      std::string val = rest.substr(pos + 1);
      if(key.size() > limits.max_storage_key || val.size() > limits.max_storage_value) {
         r.error = "storage size limit";
         return r;
      }
      if(!burn(r, limits, host_fuel_weight(host_fn::write_storage, val.size()))) return r;
      r.storage_writes[key] = std::vector<uint8_t>(val.begin(), val.end());
      r.success = true;
      return r;
   }
   if(cmd.rfind("READ:", 0) == 0) {
      std::string key = cmd.substr(5);
      if(!burn(r, limits, host_fuel_weight(host_fn::read_storage, 0))) return r;
      (void)storage;
      r.success = true;
      return r;
   }

   // default: treat as generic call cost proportional to args
   if(!burn(r, limits, 20 + args.size())) return r;
   r.success = true;
   return r;
}

static null_engine g_null;

engine& default_engine() {
   return g_null;
}

} // namespace contracts
} // namespace hive_native
