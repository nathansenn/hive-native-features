# Build & Verify — phase-1-nft

**Status:** PASS  
**Date:** 2026-07-25  
**Branch:** phase-1-nft  
**Workdir:** `/tmp/hive-native-features`

## Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hive_native_tests
./build/hive_native_bench
```

## Configure / Build

```
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /tmp/hive-native-features/build
[ 71%] Built target hive_native
[100%] Built target hive_native_bench
[100%] Built target hive_native_tests
```

- `CMakeLists.txt` already includes `src/api/database_api_stubs.cpp` (no change required).

## Unit tests (`./build/hive_native_tests`)

```
passed=63 failed=0
```

- Exit code: **0**
- **failed=0** ✓

## Benchmarks (`./build/hive_native_bench`)

```json
{
  "synthetic_transfer_us": 0.233563,
  "nft_transfer_us": 0.272396,
  "nft_transfer_ratio": 1.16627,
  "htlc_create_us": 1.5901,
  "budget_nft_p50_ratio_max": 1.5,
  "budget_nft_hard_fail_ratio": 5.0,
  "nft_within_hard_fail": true,
  "note": "portable in-memory; ratio vs synthetic transfer-class op"
}
```

- Exit code: **0**
- `nft_transfer_ratio`: **1.16627** ≤ 5 ✓ (`nft_within_hard_fail: true`)
- Also within soft budget p50 max 1.5 ✓

## Fixes applied

None — compile, link, tests, and bench all passed without code changes.

## Files changed

- `docs/swarm/01-build-verify.md` (this file only)
