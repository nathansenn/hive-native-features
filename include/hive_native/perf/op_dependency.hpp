#pragma once
/**
 * #151 — operation dependency analysis for parallel apply.
 * Portable model: ops that touch disjoint account sets can run in parallel.
 */
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace hive_native {
namespace perf {

enum class op_class : uint8_t {
   transfer,
   nft_transfer,
   htlc_redeem,
   vote,
   custom_json,
   global, // serial — witness, hardfork, props
   other
};

struct op_touch {
   size_t index = 0;
   op_class cls = op_class::other;
   std::vector<std::string> accounts; // accounts whose state is read/written
   bool touches_global = false;
};

struct schedule_layer {
   std::vector<size_t> op_indices; // can run concurrently within layer
};

/**
 * Greedy list-scheduling: assign each op to earliest layer that does not
 * conflict with any op already in that layer.
 * Conflict if: either touches_global, or account sets intersect.
 */
inline std::vector<schedule_layer> build_parallel_schedule(const std::vector<op_touch>& ops) {
   std::vector<schedule_layer> layers;
   std::vector<std::unordered_set<std::string>> layer_accounts;
   std::vector<bool> layer_global;

   for(const auto& op : ops) {
      size_t place = layers.size();
      for(size_t L = 0; L < layers.size(); ++L) {
         if(op.touches_global || layer_global[L]) continue;
         bool conflict = false;
         for(const auto& a : op.accounts) {
            if(layer_accounts[L].count(a)) { conflict = true; break; }
         }
         if(!conflict) { place = L; break; }
      }
      if(place == layers.size()) {
         layers.push_back({});
         layer_accounts.emplace_back();
         layer_global.push_back(false);
      }
      layers[place].op_indices.push_back(op.index);
      for(const auto& a : op.accounts) layer_accounts[place].insert(a);
      if(op.touches_global) layer_global[place] = true;
   }
   return layers;
}

/** Parallelism factor = total_ops / num_layers (1.0 = fully serial). */
inline double parallelism_factor(const std::vector<op_touch>& ops) {
   if(ops.empty()) return 1.0;
   auto sch = build_parallel_schedule(ops);
   return double(ops.size()) / double(sch.size());
}

} // namespace perf
} // namespace hive_native
