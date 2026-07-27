#pragma once
/**
 * #151 integration — wire op_dependency into a real portable apply scheduler.
 *
 * Classifies NFT transfer / HTLC redeem / contract call / global ops into
 * op_touch, builds a parallel schedule, then executes layers serially.
 * Ops within a layer may run sequential for determinism; parallel_width
 * records the theoretical concurrency of each layer.
 */
#include "hive_native/chain/database.hpp"
#include "hive_native/chain/evaluators.hpp"
#include "hive_native/perf/op_dependency.hpp"
#include "hive_native/protocol/contract_operations.hpp"
#include "hive_native/protocol/htlc_operations.hpp"
#include "hive_native/protocol/nft_operations.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace hive_native {
namespace perf {

/** Portable stand-in for witness / hardfork / props ops that touch global state. */
struct global_marker_operation {
   std::string name = "global";
};

/**
 * Lightweight op enum + payload for scheduler tests and portable apply.
 * Prefer classify_scheduled_op / apply_one over ad-hoc strings.
 */
enum class scheduled_op_kind : uint8_t {
   nft_transfer = 0,
   htlc_redeem  = 1,
   contract_call = 2,
   global       = 3,
};

using scheduled_op = std::variant<
   protocol::nft_transfer_operation,
   protocol::htlc_redeem_operation,
   protocol::contract_call_operation,
   global_marker_operation
>;

struct apply_schedule_stats {
   std::vector<schedule_layer> layers;
   /** Max op count in any single layer (theoretical parallel width). */
   size_t parallel_width = 0;
   /** total_ops / num_layers (1.0 = fully serial). */
   double parallelism_factor = 1.0;
   size_t ops_applied = 0;
   size_t layer_count = 0;
};

// ---- Classification into op_touch -----------------------------------------

inline op_touch classify_nft_transfer(size_t index,
                                      const protocol::nft_transfer_operation& op) {
   op_touch t;
   t.index = index;
   t.cls = op_class::nft_transfer;
   t.accounts = {op.from, op.to};
   // Also pin the NFT object so two transfers of the same token conflict.
   t.accounts.push_back(std::string("nft:") + std::to_string(op.nft_id));
   t.touches_global = false;
   return t;
}

inline op_touch classify_htlc_redeem(size_t index,
                                     const protocol::htlc_redeem_operation& op) {
   op_touch t;
   t.index = index;
   t.cls = op_class::htlc_redeem;
   t.accounts = {op.to};
   t.accounts.push_back(std::string("htlc:") + std::to_string(op.htlc_id));
   t.touches_global = false;
   return t;
}

inline op_touch classify_contract_call(size_t index,
                                       const protocol::contract_call_operation& op) {
   op_touch t;
   t.index = index;
   t.cls = op_class::custom_json; // contract execution is account + object keyed
   t.accounts = {op.caller};
   // Contract storage is shared — serialize concurrent calls on same contract.
   t.accounts.push_back(std::string("contract:") + std::to_string(op.contract_id));
   t.touches_global = false;
   return t;
}

inline op_touch classify_global(size_t index, const global_marker_operation& /*op*/) {
   op_touch t;
   t.index = index;
   t.cls = op_class::global;
   t.touches_global = true;
   return t;
}

inline op_touch classify_scheduled_op(size_t index, const scheduled_op& op) {
   return std::visit(
      [index](const auto& concrete) -> op_touch {
         using T = std::decay_t<decltype(concrete)>;
         if constexpr(std::is_same_v<T, protocol::nft_transfer_operation>)
            return classify_nft_transfer(index, concrete);
         else if constexpr(std::is_same_v<T, protocol::htlc_redeem_operation>)
            return classify_htlc_redeem(index, concrete);
         else if constexpr(std::is_same_v<T, protocol::contract_call_operation>)
            return classify_contract_call(index, concrete);
         else
            return classify_global(index, concrete);
      },
      op);
}

inline std::vector<op_touch> classify_all(const std::vector<scheduled_op>& ops) {
   std::vector<op_touch> out;
   out.reserve(ops.size());
   for(size_t i = 0; i < ops.size(); ++i)
      out.push_back(classify_scheduled_op(i, ops[i]));
   return out;
}

inline scheduled_op_kind kind_of(const scheduled_op& op) {
   return std::visit(
      [](const auto& concrete) -> scheduled_op_kind {
         using T = std::decay_t<decltype(concrete)>;
         if constexpr(std::is_same_v<T, protocol::nft_transfer_operation>)
            return scheduled_op_kind::nft_transfer;
         else if constexpr(std::is_same_v<T, protocol::htlc_redeem_operation>)
            return scheduled_op_kind::htlc_redeem;
         else if constexpr(std::is_same_v<T, protocol::contract_call_operation>)
            return scheduled_op_kind::contract_call;
         else
            return scheduled_op_kind::global;
      },
      op);
}

// ---- Execute one op -------------------------------------------------------

inline void apply_one(chain::database& db, const scheduled_op& op) {
   std::visit(
      [&db](const auto& concrete) {
         using T = std::decay_t<decltype(concrete)>;
         if constexpr(std::is_same_v<T, protocol::nft_transfer_operation>) {
            chain::apply(db, concrete);
         } else if constexpr(std::is_same_v<T, protocol::htlc_redeem_operation>) {
            chain::apply(db, concrete);
         } else if constexpr(std::is_same_v<T, protocol::contract_call_operation>) {
            chain::apply(db, concrete);
         } else {
            // Global marker: no consensus mutation in portable stand-in; record virtual.
            db.push_virtual("global_marker", std::string("{\"name\":\"") + concrete.name + "\"}");
         }
      },
      op);
}

// ---- Schedule + apply -----------------------------------------------------

/**
 * Build dependency schedule from classified touches, then apply:
 *   - layers execute serially (preserve causal order across conflicts)
 *   - ops within a layer execute sequentially for deterministic state,
 *     but parallel_width records how many could run concurrently
 */
inline apply_schedule_stats apply_scheduled(chain::database& db,
                                            const std::vector<scheduled_op>& ops) {
   apply_schedule_stats stats;
   if(ops.empty()) {
      stats.parallelism_factor = 1.0;
      return stats;
   }

   auto touches = classify_all(ops);
   stats.layers = build_parallel_schedule(touches);
   stats.layer_count = stats.layers.size();
   stats.parallelism_factor = double(ops.size()) / double(stats.layers.size());
   stats.parallel_width = 0;
   for(const auto& layer : stats.layers) {
      if(layer.op_indices.size() > stats.parallel_width)
         stats.parallel_width = layer.op_indices.size();
   }

   // Execute layers serially; within layer sequential (determinism).
   for(const auto& layer : stats.layers) {
      for(size_t idx : layer.op_indices) {
         apply_one(db, ops[idx]);
         ++stats.ops_applied;
      }
   }
   return stats;
}

/** Schedule-only helper (no database mutation) for analysis / tests. */
inline apply_schedule_stats plan_schedule(const std::vector<scheduled_op>& ops) {
   apply_schedule_stats stats;
   if(ops.empty()) {
      stats.parallelism_factor = 1.0;
      return stats;
   }
   auto touches = classify_all(ops);
   stats.layers = build_parallel_schedule(touches);
   stats.layer_count = stats.layers.size();
   stats.parallelism_factor = double(ops.size()) / double(stats.layers.size());
   stats.parallel_width = 0;
   for(const auto& layer : stats.layers) {
      if(layer.op_indices.size() > stats.parallel_width)
         stats.parallel_width = layer.op_indices.size();
   }
   return stats;
}

} // namespace perf
} // namespace hive_native
