#pragma once
/**
 * #891 #894 — dynamic RC calibration from measured wall-time samples.
 * Maps measured microseconds → relative RC micro-units vs TRANSFER_BASE.
 */
#include "hive_native/rc/costs.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace hive_native {
namespace perf {

struct rc_sample {
   std::string op_name;
   double wall_us = 0;
};

class rc_calibrator {
public:
   void add(std::string op, double wall_us) {
      samples_[std::move(op)].push_back(wall_us);
   }

   /** Median wall_us for op, or 0 if none. */
   double median_us(const std::string& op) const {
      auto it = samples_.find(op);
      if(it == samples_.end() || it->second.empty()) return 0;
      auto v = it->second;
      size_t mid = v.size() / 2;
      std::nth_element(v.begin(), v.begin() + mid, v.end());
      return v[mid];
   }

   /**
    * Calibrated RC = TRANSFER_BASE * (median_op / median_transfer).
    * Falls back to placeholder if transfer baseline missing.
    */
   uint64_t calibrated_rc(const std::string& op) const {
      double base = median_us("transfer");
      double opm = median_us(op);
      if(base <= 0 || opm <= 0) return rc::TRANSFER_BASE;
      double ratio = opm / base;
      // clamp pathological ratios
      if(ratio < 0.1) ratio = 0.1;
      if(ratio > 100.0) ratio = 100.0;
      return uint64_t(std::llround(rc::TRANSFER_BASE * ratio));
   }

   size_t op_count() const { return samples_.size(); }

private:
   std::unordered_map<std::string, std::vector<double>> samples_;
};

} // namespace perf
} // namespace hive_native
