/**
 * hive_contracts plugin stubs — delegate to hive_native::contracts::default_engine().
 * Task-ID: phase-3 / 3a
 *
 * Non-consensus. Wasmtime when HIVE_NATIVE_WITH_WASMTIME; else null_engine.
 * Phase 3h consensus activation is human-gated (see docs/swarm/11-plugin.md).
 */
#include "hive_contracts_plugin.hpp"

namespace hive_contracts {

void hive_contracts_plugin::init(const plugin_options& opts) {
   if(initialized_)
      return;
   opts_ = opts;
   // default_engine(): null_engine, or future WasmtimeEngine under HIVE_NATIVE_WITH_WASMTIME
   engine_ = &hive_native::contracts::default_engine();
   initialized_ = true;
}

void hive_contracts_plugin::shutdown() {
   engine_ = nullptr;
   initialized_ = false;
   opts_ = plugin_options{};
}

const char* hive_contracts_plugin::engine_name() const {
   if(!initialized_ || engine_ == nullptr)
      return "uninitialized";
   return engine_->name();
}

hive_native::contracts::call_result hive_contracts_plugin::deploy(
   const std::vector<uint8_t>& code,
   const std::vector<uint8_t>& init_args,
   hive_native::contracts::host_limits limits) {
   hive_native::contracts::call_result r;
   if(!initialized_ || engine_ == nullptr) {
      r.error = "plugin not initialized";
      return r;
   }
   if(!opts_.enabled || !opts_.execute) {
      r.error = "plugin execution disabled";
      return r;
   }
   return engine_->deploy_and_init(code, init_args, limits);
}

hive_native::contracts::call_result hive_contracts_plugin::call(
   const std::vector<uint8_t>& code,
   const std::string& export_name,
   const std::vector<uint8_t>& args,
   const std::map<std::string, std::vector<uint8_t>>& storage,
   hive_native::contracts::host_limits limits,
   const hive_native::account_name_type& caller) {
   hive_native::contracts::call_result r;
   if(!initialized_ || engine_ == nullptr) {
      r.error = "plugin not initialized";
      return r;
   }
   if(!opts_.enabled || !opts_.execute) {
      r.error = "plugin execution disabled";
      return r;
   }
   return engine_->call(code, export_name, args, storage, limits, caller);
}

} // namespace hive_contracts
