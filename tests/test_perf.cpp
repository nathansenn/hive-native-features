/**
 * Perf catalogue portable prototypes — items #4 #32 #72 #151 #152 #204 #219 #891
 */
#include "hive_native/perf/arena.hpp"
#include "hive_native/perf/bloom.hpp"
#include "hive_native/perf/flat_hash_map.hpp"
#include "hive_native/perf/op_dependency.hpp"
#include "hive_native/perf/rc_calibrator.hpp"
#include "hive_native/perf/worker_pool.hpp"
#include "hive_native/perf/xxhash64.hpp"
#include <atomic>
#include <iostream>
#include <string>

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

static void test_xxhash() {
   // Known xxHash64("", 0) = 0xEF46DB3751D8E999
   auto h0 = hive_native::perf::xxhash64(std::string_view(""));
   CHECK(h0 == 0xEF46DB3751D8E999ULL);
   auto h1 = hive_native::perf::xxhash64(std::string_view("abc"));
   CHECK(h1 != h0);
   CHECK(hive_native::perf::xxhash64(std::string_view("abc")) == h1);
}

static void test_flat_hash() {
   hive_native::perf::flat_hash_map<int> m;
   CHECK(m.insert("alice", 1));
   CHECK(m.insert("bob", 2));
   CHECK(!m.insert("alice", 3)); // update
   CHECK(m.find("alice") && *m.find("alice") == 3);
   CHECK(m.find("bob") && *m.find("bob") == 2);
   CHECK(m.find("carol") == nullptr);
   CHECK(m.erase("bob"));
   CHECK(m.find("bob") == nullptr);
   // stress rehash
   for(int i = 0; i < 1000; ++i) m.insert("u" + std::to_string(i), i);
   CHECK(m.size() >= 1000);
   CHECK(m.find("u500") && *m.find("u500") == 500);
}

static void test_arena() {
   hive_native::perf::arena a(1024);
   auto* p = a.create<int>(42);
   CHECK(p && *p == 42);
   auto* s = static_cast<char*>(a.allocate(100));
   CHECK(s != nullptr);
   size_t used = a.bytes_used();
   CHECK(used >= sizeof(int) + 100);
   a.reset();
   CHECK(a.bytes_used() == 0);
}

static void test_bloom() {
   hive_native::perf::bloom_filter b(1 << 14, 4);
   b.add("txid1");
   b.add("txid2");
   CHECK(b.maybe_contains("txid1"));
   CHECK(b.maybe_contains("txid2"));
   // Not a guarantee of false, but empty should usually miss
   CHECK(!b.maybe_contains("definitely-not-inserted-zzzz"));
}

static void test_dependency() {
   using namespace hive_native::perf;
   std::vector<op_touch> ops = {
      {0, op_class::transfer, {"alice","bob"}, false},
      {1, op_class::transfer, {"carol","dave"}, false},
      {2, op_class::transfer, {"alice","erin"}, false}, // conflicts 0
      {3, op_class::global, {}, true},
   };
   auto sch = build_parallel_schedule(ops);
   // ops 0 and 1 can share a layer; 2 cannot with 0; 3 alone-ish
   CHECK(sch.size() >= 2);
   CHECK(parallelism_factor(ops) > 1.0);
   // fully independent
   std::vector<op_touch> indep;
   for(int i = 0; i < 8; ++i)
      indep.push_back({size_t(i), op_class::transfer, {"a"+std::to_string(i)}, false});
   CHECK(parallelism_factor(indep) >= 7.0); // ideally 8 layers=1 → factor 8
}

static void test_pool() {
   hive_native::perf::worker_pool pool(4);
   std::atomic<int> n{0};
   for(int i = 0; i < 100; ++i) {
      auto p = (i % 4 == 0) ? hive_native::perf::task_priority::apply
                            : hive_native::perf::task_priority::low;
      pool.submit(p, [&]{ n.fetch_add(1); });
   }
   pool.wait_idle();
   CHECK(n.load() == 100);
   CHECK(pool.completed() == 100);
}

static void test_rc_cal() {
   hive_native::perf::rc_calibrator c;
   c.add("transfer", 100);
   c.add("transfer", 120);
   c.add("transfer", 110);
   c.add("nft_transfer", 220);
   c.add("nft_transfer", 230);
   CHECK(c.median_us("transfer") == 110);
   auto rc = c.calibrated_rc("nft_transfer");
   // ~2x transfer → ~2000
   CHECK(rc >= 1800 && rc <= 2500);
}

int main() {
   test_xxhash();
   test_flat_hash();
   test_arena();
   test_bloom();
   test_dependency();
   test_pool();
   test_rc_cal();
   std::cout << "perf_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
