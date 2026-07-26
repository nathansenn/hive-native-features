#pragma once
/**
 * Contract engine interface — Wasmtime preferred (ADR-0002).
 * Task-ID: phase-3 / 3a
 *
 * NullEngine always available for CI.
 * WasmtimeEngine enabled with HIVE_NATIVE_WITH_WASMTIME.
 */
#include "hive_native/util/types.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hive_native {
namespace contracts {

struct host_limits {
   uint64_t fuel = 0;
   size_t   max_memory_bytes = 4 * 1024 * 1024;
   size_t   max_storage_key = 256;
   size_t   max_storage_value = 4096;
   size_t   max_storage_total = 64 * 1024;
};

struct call_result {
   bool success = false;
   uint64_t fuel_used = 0;
   std::string error;
   std::map<std::string, std::vector<uint8_t>> storage_writes; // applied on success only
   std::vector<std::string> denied_hosts; // audit trail
};

class engine {
public:
   virtual ~engine() = default;
   virtual const char* name() const = 0;
   virtual call_result deploy_and_init(const std::vector<uint8_t>& code,
                                       const std::vector<uint8_t>& init_args,
                                       host_limits limits) = 0;
   virtual call_result call(const std::vector<uint8_t>& code,
                            const std::string& export_name,
                            const std::vector<uint8_t>& args,
                            const std::map<std::string, std::vector<uint8_t>>& storage,
                            host_limits limits,
                            const account_name_type& caller) = 0;
};

/** Deterministic meter-only engine: no real WASM; simulates fuel & storage host rules. */
class null_engine : public engine {
public:
   const char* name() const override { return "null"; }
   call_result deploy_and_init(const std::vector<uint8_t>& code,
                               const std::vector<uint8_t>& init_args,
                               host_limits limits) override;
   call_result call(const std::vector<uint8_t>& code,
                    const std::string& export_name,
                    const std::vector<uint8_t>& args,
                    const std::map<std::string, std::vector<uint8_t>>& storage,
                    host_limits limits,
                    const account_name_type& caller) override;
};

/**
 * Host allow-list (static weights). Transfer host disallowed in v1.
 * Task-ID: phase-3 / 3e
 */
enum class host_fn : uint8_t {
   read_storage = 1,
   write_storage,
   remove_storage,
   get_caller,
   get_contract_id,
   get_block_time,
   get_balance,
   sha256_host,
   log_msg,
   abort_call,
   // DENIED:
   transfer, net, fs, random, wall_clock
};

inline bool host_allowed(host_fn f) {
   switch(f) {
      case host_fn::transfer:
      case host_fn::net:
      case host_fn::fs:
      case host_fn::random:
      case host_fn::wall_clock:
         return false;
      default:
         return true;
   }
}

inline uint64_t host_fuel_weight(host_fn f, size_t bytes = 0) {
   switch(f) {
      case host_fn::read_storage:  return 50 + bytes;
      case host_fn::write_storage: return 200 + bytes * 2;
      case host_fn::remove_storage: return 80;
      case host_fn::get_caller:
      case host_fn::get_contract_id:
      case host_fn::get_block_time: return 10;
      case host_fn::get_balance: return 40;
      case host_fn::sha256_host: return 30 + bytes;
      case host_fn::log_msg: return 20 + bytes;
      case host_fn::abort_call: return 1;
      default: return 1000000; // denied paths shouldn't run
   }
}

engine& default_engine();

} // namespace contracts
} // namespace hive_native
