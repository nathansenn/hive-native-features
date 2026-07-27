# perf-03 — SIMD batch arithmetic (#209)

**Task-ID:** swarm / perf-03-simd  
**Catalogue:** #209 — SIMD / NEON acceleration for vote-weight and common arithmetic  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  

---

## Deliverable

Portable header-only batch helper for vote-weight style `uint64_t` mul/add:

| Symbol | Role |
|--------|------|
| `batch_mul_u64` | `out[i] = a[i] * b[i]` (wrapping) |
| `batch_add_u64` | `out[i] = a[i] + b[i]` (wrapping) |
| `batch_mul_u64_scalar` / `batch_add_u64_scalar` | Always-correct scalar reference |
| `simd_math_backend()` | `"neon"` / `"sse2"` / `"scalar"` |

**Header:** `include/hive_native/perf/simd_math.hpp`  
**Test:** `tests/test_simd_math.cpp` → target `hive_native_simd_math_tests`

---

## Backend selection (`#ifdef`)

| Macro | Path |
|-------|------|
| `__ARM_NEON` / `__ARM_NEON__` | 2-wide NEON (`vaddq_u64`, load/store pairs for mul) |
| `__SSE2__` | 2-wide SSE2 (`_mm_add_epi64`, load/store pairs for mul) |
| else | Pure scalar loops |

Notes:

- SSE2 / baseline NEON have no portable 64×64→64 vector multiply; mul uses vector loads/stores with per-lane scalar multiply so results stay bit-identical to scalar.
- Odd-length tails always take the scalar remainder path.
- Scalar fallback is **always correct** and used when SIMD is unavailable.

---

## Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_simd_math_tests
ctest --test-dir build -R hive_native_simd_math_tests --output-on-failure
./build/hive_native_simd_math_tests
```

Test sizes (batch vs scalar reference): **n = 1, 7, 64, 1000**.

### Recorded result (2026-07-27, Apple M3 Ultra arm64)

```
simd_math_backend=neon passed=13 failed=0
```

```
ctest --test-dir build -R hive_native_simd_math_tests --output-on-failure
# 1/1 Test #3: hive_native_simd_math_tests ......   Passed
# 100% tests passed out of 1
```

Exit code **0**.

---

## CMake

```cmake
add_executable(hive_native_simd_math_tests tests/test_simd_math.cpp)
target_link_libraries(hive_native_simd_math_tests PRIVATE hive_native)
add_test(NAME hive_native_simd_math_tests COMMAND hive_native_simd_math_tests)
```

---

## Upstream mapping

When porting into hived apply path (vote weight / RC arithmetic hot loops):

1. Keep bit-identical wrapping `uint64_t` semantics (no wider intermediate unless already consensus).
2. Prefer this helper only for **independent lane** batches (e.g. precomputed weight vectors), not for serial cumulative reduction without care.
3. Commit message style: `perf(#209): portable SIMD batch mul/add for vote-weight arithmetic`.

---

## Files

| Path | Change |
|------|--------|
| `include/hive_native/perf/simd_math.hpp` | new |
| `tests/test_simd_math.cpp` | new |
| `CMakeLists.txt` | add `hive_native_simd_math_tests` |
| `docs/swarm/perf-03-simd.md` | this report |
