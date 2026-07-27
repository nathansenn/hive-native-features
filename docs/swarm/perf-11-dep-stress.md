# perf-11 — Operation dependency stress (#151)

**Task-ID:** swarm / perf-11-dep-stress  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Catalogue:** #151 — dependency analysis for parallel apply  

---

## Goal

Stress-test the portable `op_dependency` scheduler:

1. **Random load** — 100 transfer ops over 20 accounts; **average `parallelism_factor` > 1.5**
2. **Worst case** — all ops touch the same account; **`parallelism_factor == 1.0`** (fully serial)

`parallelism_factor = total_ops / num_schedule_layers` (1.0 = fully serial).

---

## Files

| Path | Role |
|------|------|
| `include/hive_native/perf/op_dependency.hpp` | Header-only greedy list scheduler (unchanged) |
| `tests/test_op_dependency_stress.cpp` | **New** stress harness |
| `tests/test_perf.cpp` | Existing unit checks — **unchanged** |
| `CMakeLists.txt` | Adds `hive_native_dep_stress_tests` + ctest |

---

## Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_dep_stress_tests hive_native_perf_tests
./build/hive_native_dep_stress_tests
./build/hive_native_perf_tests
ctest --test-dir build --output-on-failure -R 'hive_native_perf_tests|hive_native_dep_stress'
```

---

## Results (this host)

### Dependency stress (`./build/hive_native_dep_stress_tests`)

```
dep_stress_random_trials=32 ops=100 accounts=20 avg_parallelism_factor=6.15313 min=5.26316 max=7.14286
dep_stress_seed42_factor=5.88235
dep_stress_worst_layers=100 factor=1
dep_stress_passed=37 failed=0
```

| Case | Metric | Observed | Gate | Result |
|------|--------|---------:|------|--------|
| Random transfers (32 trials × 100 ops / 20 accts) | avg `parallelism_factor` | **6.153** | > 1.5 | **PASS** |
| Seed-42 single batch | `parallelism_factor` | **5.882** | > 1.5 | **PASS** |
| All ops share one account | `parallelism_factor` | **1.0** | == 1.0 | **PASS** |
| Worst-case layers | `schedule.size()` | **100** | == 100 ops | **PASS** |

Exit code: **0**

### Existing perf suite (not broken)

```
perf_passed=27 failed=0
```

Exit code: **0**

### ctest

```
Start 2: hive_native_perf_tests ........... Passed
Start 4: hive_native_dep_stress_tests ..... Passed
100% tests passed out of 2
```

---

## Method notes

- **Random:** each op is `transfer` touching two accounts drawn uniformly from `acct0`…`acct19` (prefer distinct pair). Fixed trial seeds (`0x151 + trial·golden`) for reproducibility; report mean/min/max factor over 32 trials.
- **Worst-case:** 100 transfers all include `"hot_account"` → every pair conflicts → greedy scheduler emits one op per layer → factor `100/100 = 1.0`.
- No change to `build_parallel_schedule` / `parallelism_factor` implementation — stress only.

---

## Interpretation

With 20 accounts, random transfer pairs leave large disjoint subsets, so the scheduler packs ~5–7 ops per layer on average (factor ≈ 6). Hot-account contention correctly collapses to serial apply. This supports #151 portable prototype: independent account sets are eligible for concurrent layers; shared accounts force serialization.
