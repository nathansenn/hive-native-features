# perf-02 — Primary-key account cache (#63)

**Task-ID:** catalogue #63 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Workdir:** `/tmp/hive-native-features`  
**Branch:** `swarm-perf-p0-impl`  
**Catalogue:** [Primary-key caching for get_account / find_account](../performance/HIVE_1000_OPTIMIZATIONS.md)

---

## Goal

Hot `get_account` / `find_account` style primary-key lookups should not walk the
portable `std::map` balances index on every call. Layer an open-addressing
`flat_hash_map` cache by account name, with explicit invalidation on
`adjust_balance`.

Prefer a **wrapper** so existing `database` tests stay untouched.

---

## Implementation

| Path | Role |
|------|------|
| [`include/hive_native/perf/account_cache.hpp`](../../include/hive_native/perf/account_cache.hpp) | `perf::account_cache` — name → `account_balance` via `flat_hash_map`; hit/miss/invalidation counters; `hit_rate()` |
| [`include/hive_native/chain/account_index.hpp`](../../include/hive_native/chain/account_index.hpp) | `chain::account_index` — cache-aside over `database`: `find` / `create` / `adjust_balance` / `get_balance` |
| [`tests/test_account_cache.cpp`](../../tests/test_account_cache.cpp) | Hit rate after repeated finds; invalidation after balance change; cache disable path |
| `CMakeLists.txt` | Target `hive_native_account_cache_tests` + `add_test` |

### Semantics

1. **`create(name, hive, hbd)`** — write-through to `database`, seed cache.
2. **`find(name)`** — cache hit returns snapshot; miss loads `db.balances` and fills cache.
3. **`adjust_balance(name, delta)`** — write-through to `database`, **invalidate** cache entry (next `find` reloads).
4. **Cache optional** — `account_index(db, /*enable_cache=*/false)` bypasses map entirely.

Writers that mutate `database` balances directly must call
`cache().invalidate(name)` or use `account_index::adjust_balance`.

Upstream mapping (when porting to hived chainbase): mirror this as a
thread-local or apply-session primary-key pointer cache on
`database::get_account` / `find_account`, cleared on undo/session pop and on
object modification — same invalidation contract.

---

## Build / test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_account_cache_tests
./build/hive_native_account_cache_tests
ctest --test-dir build -R account_cache --output-on-failure
```

### Evidence (this run)

```text
$ ./build/hive_native_account_cache_tests
account_cache_passed=142 failed=0

$ ctest --test-dir build -R account_cache --output-on-failure
Test project /tmp/hive-native-features/build
    Start 3: hive_native_account_cache_tests
1/1 Test #3: hive_native_account_cache_tests ...   Passed    0.00 sec
100% tests passed out of 1

# regression (existing targets still green)
$ ./build/hive_native_tests && ./build/hive_native_perf_tests
passed=165 failed=0
perf_passed=27 failed=0
```

---

## Why not mutate `database.hpp`?

Integrating the cache into `database` would require every existing test and
evaluator path to respect invalidation. The wrapper keeps:

- `tests/test_runner.cpp` / evaluators unchanged
- Opt-in caching for hot lookup loops and future API layers
- Clear ownership of invalidation at the write path

---

## Related catalogue IDs

| ID | Module | Note |
|----|--------|------|
| #4 #424 | `flat_hash_map.hpp` | Open-addressing backend |
| #63 | `account_cache.hpp` + `account_index.hpp` | This item — primary-key cache |
| #32 | `xxhash64.hpp` | Hash used by `flat_hash_map` |

---

## Constraints

- No consensus change  
- Portable only (in-memory stand-in)  
- Invalidation required on balance mutation  
