# perf-01 — Apply scheduler (#151 integration)

**Task-ID:** catalogue #151 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`

---

## Goal

Wire `op_dependency.hpp` into a real portable apply scheduler so NFT / HTLC /
contract / global ops are classified into `op_touch`, layered by
`build_parallel_schedule`, and applied with measurable `parallel_width`.

---

## Files

| Path | Role |
|------|------|
| `include/hive_native/perf/apply_scheduler.hpp` | Header-only scheduler (classify + plan + apply) |
| `include/hive_native/perf/op_dependency.hpp` | Existing greedy layer scheduler (unchanged API) |
| `tests/test_apply_scheduler.cpp` | Integration tests |
| `CMakeLists.txt` | Target `hive_native_apply_scheduler_tests` |

No `.cpp` needed — implementation is header-only and links via `hive_native`
evaluators.

---

## API summary

```cpp
// Classify portable ops → op_touch
op_touch classify_nft_transfer(size_t, const nft_transfer_operation&);
op_touch classify_htlc_redeem(size_t, const htlc_redeem_operation&);
op_touch classify_contract_call(size_t, const contract_call_operation&);
op_touch classify_global(size_t, const global_marker_operation&);

// Plan only
apply_schedule_stats plan_schedule(const std::vector<scheduled_op>&);

// Plan + apply: layers serial; ops within layer sequential (determinism)
// parallel_width = max ops in any layer
apply_schedule_stats apply_scheduled(database&, const std::vector<scheduled_op>&);
```

### Conflict model

| Op | `op_class` | Touches |
|----|------------|---------|
| NFT transfer | `nft_transfer` | `from`, `to`, `nft:{id}` |
| HTLC redeem | `htlc_redeem` | `to`, `htlc:{id}` |
| Contract call | `custom_json` | `caller`, `contract:{id}` |
| Global marker | `global` | `touches_global = true` (serial barrier) |

Layers from `build_parallel_schedule`: account-set intersection or any global
touch ⇒ cannot co-reside in the same layer.

---

## Build & test commands

```bash
cmake -S . -B build
cmake --build build --target hive_native_apply_scheduler_tests -j
./build/hive_native_apply_scheduler_tests
ctest --test-dir build --output-on-failure -R hive_native_apply_scheduler
```

---

## Evidence (pass counts)

### Direct binary

```
apply_scheduler_passed=47 failed=0
```

Exit code: **0**

### CTest (scoped)

```
Test project /tmp/hive-native-features/build
    Start 1: hive_native_tests
1/3 Test #1: hive_native_tests ...................   Passed
    Start 2: hive_native_perf_tests
2/3 Test #2: hive_native_perf_tests ..............   Passed
    Start 7: hive_native_apply_scheduler_tests
3/3 Test #7: hive_native_apply_scheduler_tests ...   Passed

100% tests passed out of 3
```

### Scenario results

| Scenario | Expectation | Result |
|----------|-------------|--------|
| 4 independent NFT transfers (owners o1–o4) | `parallelism_factor >= 2` (actual ~4.0, width 4, 1 layer) | PASS |
| 2 transfers same account (`alice`) | serial layers (`layer_count == 2`, factor 1.0, width 1) | PASS |
| Independent pair + global marker | worse than free schedule; global forces separation; both transfers apply | PASS |
| HTLC redeem / contract call classify | same-contract serial; distinct contracts parallel | PASS |

State mutations verified after `apply_scheduled` (NFT owners updated; global
virtual op emitted).

---

## Design notes

1. **Determinism first:** within a layer, ops run sequentially so portable
   `database` maps stay single-threaded; `parallel_width` still records
   theoretical concurrency for upstream worker-pool wiring (#151 / #152).
2. **Object keys as pseudo-accounts:** `nft:N`, `htlc:N`, `contract:N` ensure
   object-level conflicts even when account sets would otherwise look disjoint.
3. **Global barrier:** any `touches_global` op cannot share a layer with any
   other op (matches `op_dependency` greedy list-scheduler).

---

## Upstream mapping

Portable sketch for hived:

`perf(#151): dependency-aware apply schedule for independent NFT/HTLC ops`

Target: expand `blockchain_worker_thread_pool` beyond P2P prework to evaluate
independent ops after authority pre-check, falling back to serial on conflict
detection (#520).
