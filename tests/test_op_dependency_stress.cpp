/**
 * #151 dependency-graph stress — random transfer schedule + worst-case serial.
 * Portable: uses only hive_native::perf::op_dependency (header-only model).
 */
#include "hive_native/perf/op_dependency.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static int g_failed = 0, g_passed = 0;
#define CHECK(c)                                                                                   \
   do {                                                                                            \
      if(c)                                                                                        \
         ++g_passed;                                                                               \
      else {                                                                                       \
         ++g_failed;                                                                               \
         std::cerr << "FAIL " << __LINE__ << " " #c "\n";                                          \
      }                                                                                            \
   } while(0)

namespace {

constexpr int k_accounts = 20;
constexpr int k_ops = 100;
constexpr int k_trials = 32;
constexpr double k_min_avg_factor = 1.5;

std::vector<std::string> make_accounts(int n) {
   std::vector<std::string> a;
   a.reserve(static_cast<size_t>(n));
   for(int i = 0; i < n; ++i)
      a.push_back("acct" + std::to_string(i));
   return a;
}

/** Build k_ops transfer ops: each touches two accounts chosen from the pool. */
std::vector<hive_native::perf::op_touch>
random_transfers(const std::vector<std::string>& accounts, int n_ops, std::mt19937_64& rng) {
   using hive_native::perf::op_class;
   using hive_native::perf::op_touch;
   std::uniform_int_distribution<int> dist(0, static_cast<int>(accounts.size()) - 1);
   std::vector<op_touch> ops;
   ops.reserve(static_cast<size_t>(n_ops));
   for(int i = 0; i < n_ops; ++i) {
      int from = dist(rng);
      int to = dist(rng);
      // Prefer distinct counterparty when possible (still a valid transfer model if same).
      if(to == from && accounts.size() > 1)
         to = (from + 1) % static_cast<int>(accounts.size());
      op_touch t;
      t.index = static_cast<size_t>(i);
      t.cls = op_class::transfer;
      t.accounts = {accounts[static_cast<size_t>(from)], accounts[static_cast<size_t>(to)]};
      t.touches_global = false;
      ops.push_back(std::move(t));
   }
   return ops;
}

/**
 * 100 random transfer ops over 20 accounts; average parallelism_factor across
 * trials must exceed 1.5 (disjoint account sets schedule into concurrent layers).
 */
void test_random_transfer_parallelism() {
   using hive_native::perf::parallelism_factor;
   const auto accounts = make_accounts(k_accounts);
   double sum = 0.0;
   double min_f = 1e300;
   double max_f = 0.0;

   for(int trial = 0; trial < k_trials; ++trial) {
      std::mt19937_64 rng(0x151ull + static_cast<uint64_t>(trial) * 0x9E3779B97F4A7C15ull);
      auto ops = random_transfers(accounts, k_ops, rng);
      CHECK(ops.size() == static_cast<size_t>(k_ops));
      double f = parallelism_factor(ops);
      sum += f;
      if(f < min_f) min_f = f;
      if(f > max_f) max_f = f;
   }

   const double avg = sum / static_cast<double>(k_trials);
   std::cout << "dep_stress_random_trials=" << k_trials << " ops=" << k_ops
             << " accounts=" << k_accounts << " avg_parallelism_factor=" << avg
             << " min=" << min_f << " max=" << max_f << "\n";
   CHECK(avg > k_min_avg_factor);
   // Single deterministic seed smoke: 100 ops / 20 accounts still parallelizable.
   {
      std::mt19937_64 rng(42);
      auto ops = random_transfers(accounts, k_ops, rng);
      double f = parallelism_factor(ops);
      std::cout << "dep_stress_seed42_factor=" << f << "\n";
      CHECK(f > k_min_avg_factor);
   }
}

/**
 * Worst case: every op touches the same account → fully serial schedule,
 * parallelism_factor must be exactly 1.0.
 */
void test_worst_case_same_account() {
   using hive_native::perf::op_class;
   using hive_native::perf::op_touch;
   using hive_native::perf::parallelism_factor;
   using hive_native::perf::build_parallel_schedule;

   std::vector<op_touch> ops;
   ops.reserve(static_cast<size_t>(k_ops));
   for(int i = 0; i < k_ops; ++i) {
      op_touch t;
      t.index = static_cast<size_t>(i);
      t.cls = op_class::transfer;
      t.accounts = {"hot_account", "counterparty"};
      // All share "hot_account" → pairwise conflict chain.
      t.touches_global = false;
      ops.push_back(std::move(t));
   }

   auto sch = build_parallel_schedule(ops);
   const double f = parallelism_factor(ops);
   std::cout << "dep_stress_worst_layers=" << sch.size() << " factor=" << f << "\n";
   CHECK(sch.size() == static_cast<size_t>(k_ops));
   CHECK(f == 1.0);
   // Tolerance form for floating point identity of 100/100.
   CHECK(std::fabs(f - 1.0) < 1e-12);
}

} // namespace

int main() {
   test_random_transfer_parallelism();
   test_worst_case_same_account();
   std::cout << "dep_stress_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
