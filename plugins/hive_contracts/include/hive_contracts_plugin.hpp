#pragma once
/**
 * hive_contracts plugin skeleton — non-consensus wrapper around the contract engine.
 * Task-ID: phase-3 / 3a
 *
 * Runtime: Wasmtime preferred (ADR-0002); build flag HIVE_NATIVE_WITH_WASMTIME.
 * Consensus activation is Phase 3h and remains human-gated.
 */
#include "hive_native/contracts/engine.hpp"
#include "hive_native/util/types.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hive_contracts {

/**
 * Optional node plugin options (non-consensus).
 * Mirrors how an upstream appbase plugin would be configured.
 */
struct plugin_options {
   bool enabled = true;
   /// When false, init still succeeds but deploy/call return errors (skip path).
   bool execute = true;
   std::string engine_name; // empty → use default_engine().name()
};

/**
 * Thin plugin façade over hive_native::contracts::engine.
 *
 * Phases 3a–3g: experimental / local apply only.
 * Phase 3h (human-gated): may become a required consensus path — not automatic.
 */
class hive_contracts_plugin {
public:
   hive_contracts_plugin() = default;
   ~hive_contracts_plugin() = default;

   hive_contracts_plugin(const hive_contracts_plugin&) = delete;
   hive_contracts_plugin& operator=(const hive_contracts_plugin&) = delete;

   /** Select engine, apply options. Idempotent if already initialized. */
   void init(const plugin_options& opts = {});

   /** Release plugin state; safe if never initialized. */
   void shutdown();

   bool is_initialized() const { return initialized_; }
   const char* engine_name() const;

   /**
    * Deploy + init code under fuel/memory limits.
    * Delegates to hive_native::contracts::default_engine() (null or Wasmtime).
    */
   hive_native::contracts::call_result deploy(
      const std::vector<uint8_t>& code,
      const std::vector<uint8_t>& init_args,
      hive_native::contracts::host_limits limits);

   /**
    * Call an export against a storage snapshot.
    * Storage writes in the result apply only on success (caller commits).
    */
   hive_native::contracts::call_result call(
      const std::vector<uint8_t>& code,
      const std::string& export_name,
      const std::vector<uint8_t>& args,
      const std::map<std::string, std::vector<uint8_t>>& storage,
      hive_native::contracts::host_limits limits,
      const hive_native::account_name_type& caller);

private:
   bool initialized_ = false;
   plugin_options opts_{};
   hive_native::contracts::engine* engine_ = nullptr;
};

} // namespace hive_contracts
