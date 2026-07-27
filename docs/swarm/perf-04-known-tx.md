# perf-04 — Known-transaction set (#204)

**Task-ID:** catalogue #204 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  

## Goal

Portable **mempool known-transaction set** for hived-style `is_known_transaction` fast paths:

1. **Bloom filter** as the first filter (`maybe_seen`) — rejects novel txids without a table probe; may false-positive.
2. **`std::unordered_set`** as ground truth (`definitely_seen`) — exact confirm; no false positives.
3. **`add`** inserts into both layers.

## Files

| Path | Role |
|------|------|
| [`include/hive_native/perf/known_tx_set.hpp`](../../include/hive_native/perf/known_tx_set.hpp) | API: `add`, `maybe_seen`, `definitely_seen`, `clear`, `size` |
| [`include/hive_native/perf/bloom.hpp`](../../include/hive_native/perf/bloom.hpp) | Existing #204 bloom primitive (reused) |
| [`tests/test_known_tx.cpp`](../../tests/test_known_tx.cpp) | Unit tests |
| [`CMakeLists.txt`](../../CMakeLists.txt) | Target `hive_native_known_tx_tests` |

## API sketch

```cpp
hive_native::perf::known_tx_set k(/*bloom_bits*/ 1<<20, /*hashes*/ 4);
k.add("txid…");
if (k.maybe_seen(id)) {
  if (k.definitely_seen(id)) { /* duplicate / known */ }
  else { /* bloom FP — novel */ }
} else {
  /* definitely novel (no FNs) */
}
```

### Invariants

| Query | False positive | False negative |
|-------|----------------|----------------|
| `maybe_seen` | possible | **never** |
| `definitely_seen` | **never** | **never** |

## Build / verify

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_known_tx_tests
./build/hive_native_known_tx_tests
ctest --test-dir build -R known_tx --output-on-failure
```

## Evidence

| Check | Result |
|-------|--------|
| Branch | `swarm-perf-p0-impl` |
| Host | Apple arm64 |
| Base commit (pre-change) | `d02792c` |
| Build | `cmake --build build -j --target hive_native_known_tx_tests` → **success** |
| Unit binary | `./build/hive_native_known_tx_tests` → `known_tx_passed=1222 failed=0` |
| ctest | `hive_native_known_tx_tests` **Passed**; `hive_native_perf_tests` **Passed** (no regression) |

```text
$ ./build/hive_native_known_tx_tests
known_tx_passed=1222 failed=0

$ ctest --test-dir build -R 'known_tx|perf' --output-on-failure
100% tests passed out of 2
```

## Upstream mapping

Catalogue: **#204** — *Bloom / compact-set for is_known_transaction*  
(`docs/performance/HIVE_1000_OPTIMIZATIONS.md`, P0 apply path)

Suggested hived integration points:

- `node::is_known_transaction` / mempool admission
- P2P inventory filter before full-body request
- Optional: keep bloom for wire / discard exact set when memory-bound (not in this portable prototype)

Constraints: non-consensus; no HF required; pure admission / mempool bookkeeping.
