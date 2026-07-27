# perf-20 — Checkbox Protocol Audit (swarm-perf-p0-impl)

**Auditor role:** Checkbox Protocol auditor (P0 portable perf swarm)  
**Workspace:** `/tmp/hive-native-features`  
**Branch:** `swarm-perf-p0-impl`  
**Git HEAD:** `f4bbdaa` (`f4bbdaa75affd8be003811ee5030ed4184d8bcb8`)  
**Commit subject:** `docs: mark P0 portable catalogue items as portable-prototype`  
**Audit UTC:** 2026-07-27T05:57:49Z  
**Host:** macOS arm64, cmake 4.4.0, Apple clang 21.0.0  
**Method:** Wait **150s** for peer agents, then audit **one checklist item at a time** with live RUN evidence. No CMake fix cycle required (all build/tests green on first pass).

**Final score: 10/10** (boxes 1–9 ☑ + residual risks documented under **V**)

---

## Precondition — wait for other agents

```text
WAIT_DONE 2026-07-27T05:57:01Z
## swarm-perf-p0-impl...origin/swarm-perf-p0-impl
swarm-perf-p0-impl
f4bbdaa
```

---

## ☐ → ☑ 1 cmake configure

**Result: ☑ PASS**

**Command:**

```bash
cmake -S . -B build
```

**RUN evidence:**

```text
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /tmp/hive-native-features/build
EXIT=0
```

Log: `/tmp/checkbox1-cmake-configure.log`

---

## ☐ → ☑ 2 cmake build all targets

**Result: ☑ PASS**

**Command:**

```bash
cmake --build build --target all -j$(sysctl -n hw.ncpu)
```

**RUN evidence (summary):**

```text
[ 26%] Built target hive_native
...
[ 97%] Built target hive_native_tests
[100%] Built target hive_native_apply_scheduler_tests
EXIT=0
```

All targets linked, including:

| Target | Kind |
|--------|------|
| `hive_native` | static library |
| `hive_native_tests` | unit suite |
| `hive_native_perf_tests` | perf suite |
| `hive_native_rocksdb_preset_tests` | new |
| `hive_native_selective_undo_tests` | new |
| `hive_native_compact_block_tests` | new |
| `hive_native_account_cache_tests` | new |
| `hive_native_apply_scheduler_tests` | new |
| `hive_native_simd_math_tests` | new |
| `hive_native_known_tx_tests` | new |
| `hive_native_dep_stress_tests` | new |
| `hive_native_node_profiles_tests` | new |
| `hive_native_fuzz_htlc` | fuzz harness |
| `hive_native_bench` / `hive_native_bench_rc` | benches |

Log: `/tmp/checkbox2-cmake-build.log`

---

## ☐ → ☑ 3 ctest all pass

**Result: ☑ PASS** — **12/12** tests, 100%

**Command:**

```bash
cd build && ctest --output-on-failure
```

**RUN evidence:**

```text
Test project /tmp/hive-native-features/build
 1/12 Test  #1: hive_native_tests ...................   Passed    0.38 sec
 2/12 Test  #2: hive_native_perf_tests ..............   Passed    0.27 sec
 3/12 Test  #3: hive_native_rocksdb_preset_tests ....   Passed    0.07 sec
 4/12 Test  #4: hive_native_selective_undo_tests ....   Passed    0.07 sec
 5/12 Test  #5: hive_native_compact_block_tests .....   Passed    0.07 sec
 6/12 Test  #6: hive_native_account_cache_tests .....   Passed    0.07 sec
 7/12 Test  #7: hive_native_apply_scheduler_tests ...   Passed    0.07 sec
 8/12 Test  #8: hive_native_simd_math_tests .........   Passed    0.07 sec
 9/12 Test  #9: hive_native_known_tx_tests ..........   Passed    0.07 sec
10/12 Test #10: hive_native_dep_stress_tests ........   Passed    0.07 sec
11/12 Test #11: hive_native_node_profiles_tests .....   Passed    0.01 sec
12/12 Test #12: hive_native_fuzz_htlc ...............   Passed    0.39 sec

100% tests passed out of 12
Total Test time (real) =   1.60 sec
EXIT=0
```

Log: `/tmp/checkbox3-ctest.log`

---

## ☐ → ☑ 4 hive_native_tests failed=0

**Result: ☑ PASS** — `passed=165 failed=0`

**Command:**

```bash
./build/hive_native_tests
```

**RUN evidence:**

```text
passed=165 failed=0
EXIT4=0
```

Log: `/tmp/checkbox4-hive_native_tests.log`

---

## ☐ → ☑ 5 hive_native_perf_tests failed=0

**Result: ☑ PASS** — `perf_passed=27 failed=0`

**Command:**

```bash
./build/hive_native_perf_tests
```

**RUN evidence:**

```text
perf_passed=27 failed=0
EXIT5=0
```

Log: `/tmp/checkbox5-hive_native_perf_tests.log`

---

## ☐ → ☑ 6 any new test binaries that exist also pass

**Result: ☑ PASS** — all 10 additional CTest-registered binaries exit 0 with `failed=0` (or fuzz `failures=0`)

**Command pattern:**

```bash
cd build
for bin in hive_native_rocksdb_preset_tests hive_native_selective_undo_tests \
  hive_native_compact_block_tests hive_native_account_cache_tests \
  hive_native_apply_scheduler_tests hive_native_simd_math_tests \
  hive_native_known_tx_tests hive_native_dep_stress_tests \
  hive_native_node_profiles_tests hive_native_fuzz_htlc; do
  ./"$bin"
done
```

**RUN evidence:**

| Binary | Result line | Exit |
|--------|-------------|------|
| `hive_native_rocksdb_preset_tests` | `rocksdb_presets_passed=48 failed=0` | 0 |
| `hive_native_selective_undo_tests` | `selective_undo_passed=23 failed=0` | 0 |
| `hive_native_compact_block_tests` | `compact_block_passed=45 failed=0` | 0 |
| `hive_native_account_cache_tests` | `account_cache_passed=142 failed=0` | 0 |
| `hive_native_apply_scheduler_tests` | `apply_scheduler_passed=47 failed=0` | 0 |
| `hive_native_simd_math_tests` | `simd_math_backend=neon passed=13 failed=0` | 0 |
| `hive_native_known_tx_tests` | `known_tx_passed=1222 failed=0` | 0 |
| `hive_native_dep_stress_tests` | `dep_stress_passed=37 failed=0` | 0 |
| `hive_native_node_profiles_tests` | `node_profiles_passed=41 failed=0` | 0 |
| `hive_native_fuzz_htlc` | `iterations=5000 … failures=0` / `PASSED` | 0 |

Log: `/tmp/checkbox6-new-binaries.log`

**Note:** Benches (`hive_native_bench`, `hive_native_bench_rc`) are not CTest tests; they were built successfully under box 2 but are out of scope for “test binaries” pass/fail counts.

---

## ☐ → ☑ 7 no secrets (git grep BEGIN PRIVATE)

**Result: ☑ PASS** — no live PEM private keys; matches are **documentation of scan patterns only**

**Command:**

```bash
git grep -n "BEGIN PRIVATE"
# plus PEM block probe:
git grep -n "BEGIN PRIVATE KEY-----" || true
```

**RUN evidence (all hits are docs describing CI secret-scan patterns):**

```text
docs/swarm/05-ci.md:57:- `BEGIN PRIVATE KEY`
docs/swarm/20-checkbox-audit.md:186:## ☐ → ☑ 8 No secrets in tree (BEGIN PRIVATE | api_key | ghp_)
docs/swarm/20-checkbox-audit.md:193:rg -n ... -e 'BEGIN PRIVATE|api_key|ghp_' .
docs/swarm/20-checkbox-audit.md:194:# ./docs/swarm/05-ci.md:57: - `BEGIN PRIVATE KEY`
docs/swarm/20-checkbox-audit.md:201:- `BEGIN PRIVATE KEY`
docs/swarm/perf-12-ci.md:82:- `BEGIN PRIVATE KEY`
```

No `-----BEGIN … PRIVATE KEY-----` PEM blocks found in the tree.  
(Aligned with prior audit interpretation in `docs/swarm/20-checkbox-audit.md` and `perf-12-ci.md` secret-scan policy.)

---

## ☐ → ☑ 8 docs/swarm/perf-* reports exist (>=5)

**Result: ☑ PASS** — **14** files (`>= 5` required)

**Command:**

```bash
ls -1 docs/swarm/perf-*
```

**RUN evidence:**

```text
docs/swarm/perf-00-orchestrator.md
docs/swarm/perf-01-apply-scheduler.md
docs/swarm/perf-02-account-cache.md
docs/swarm/perf-03-simd.md
docs/swarm/perf-04-known-tx.md
docs/swarm/perf-05-light-profiles.md
docs/swarm/perf-06-rc-cal.md
docs/swarm/perf-08-rocksdb.md
docs/swarm/perf-09-undo.md
docs/swarm/perf-10-compact-block.md
docs/swarm/perf-11-dep-stress.md
docs/swarm/perf-12-ci.md
docs/swarm/perf-13-fuzz.md
docs/swarm/perf-19-gitops.md
COUNT=14
```

**Gaps (informational, not FAIL):** `perf-07-*` and `perf-14`–`perf-18` absent at orchestrator close (see `perf-00-orchestrator.md`). Count still far exceeds threshold of 5.

---

## ☐ → ☑ 9 P0 portable modules still present

**Result: ☑ PASS** — full `include/hive_native/perf/` inventory + related chain headers present

**Command:**

```bash
ls include/hive_native/perf/
# plus explicit presence checks for P0 modules
```

**RUN evidence — `include/hive_native/perf/`:**

| Header | Lines | Catalogue (from orchestrator) |
|--------|------:|-------------------------------|
| `account_cache.hpp` | 89 | #63 |
| `apply_scheduler.hpp` | 221 | #151 |
| `arena.hpp` | 62 | #72 #219 |
| `bloom.hpp` | 45 | #204 |
| `compact_block.hpp` | 181 | #301 |
| `flat_hash_map.hpp` | 99 | #4 #63 #424 |
| `known_tx_set.hpp` | 57 | #204 |
| `op_dependency.hpp` | 75 | #151 |
| `rc_calibrator.hpp` | 60 | #891 #894 |
| `rocksdb_presets.hpp` | 193 | #23–#28, #103–#105 |
| `selective_undo.hpp` | 88 | #43 |
| `simd_math.hpp` | 118 | #209 |
| `worker_pool.hpp` | 109 | #152 #153 #155 |
| `xxhash64.hpp` | 76 | #32 #84 |

**Related chain headers:**

| Header | Lines | Role |
|--------|------:|------|
| `include/hive_native/chain/account_index.hpp` | 92 | #63 primary-key account index |
| `include/hive_native/chain/node_profiles.hpp` | 143 | #8 #691 light profiles |

All 16 checked paths: **OK / present**.

---

## ☐ → ☑ V residual risks listed

**Result: ☑ PASS** — residual risks captured below (and cross-linked to worker reports)

### Residual risks (V)

1. **Portable prototype, not production wire-up**  
   Modules are header-only sketches / unit-tested portable code. Upstream hived apply path, RocksDB plugin runtime, and network relay integration remain separate tasks (`perf-08-rocksdb`, `perf-10-compact-block`).

2. **Non-consensus / no hardfork**  
   Compact-block short IDs use bare SHA-256 truncation for determinism; mainnet should prefer salted BIP152-style SipHash before any consensus-adjacent use (`perf-10-compact-block`). Selective undo needs HF/integration review and a compile/runtime gate (`perf-09-undo`).

3. **RocksDB presets are configuration-only**  
   `rocksdb_presets.hpp` does **not** link RocksDB; it encodes tuning tables only. Real I/O benefits need host RocksDB + CF mapping (`perf-08-rocksdb`).

4. **RC cost placeholders**  
   `htlc_create` calibrated ~7.6× transfer / ~2.9× placeholder (2600); catalogue placeholders remain **TODO – measure** before consensus RC weights (`perf-06-rc-cal`).

5. **Parallel apply is model-level**  
   Dependency scheduler proves layer packing / hot-account serialization in unit + stress tests; true multi-threaded apply + chainbase isolation is not proven in this repo (`perf-01`, `perf-11`).

6. **SIMD lanes are independent only**  
   Batch mul/add assumes independent lanes; serial cumulative reductions need care (`perf-03-simd`). Backend here: **NEON** (arm64).

7. **Light profiles are config flags**  
   `node_profiles` / `HIVE_LIGHT_NODE` set skip/feature bools; full plugin/index enablement mapping under light node remains future work (`perf-05-light-profiles`).

8. **Report gaps**  
   Swarm docs missing `perf-07` and `perf-14`–`perf-18` slots; not a test failure, but coverage of those agent roles is incomplete (`perf-00-orchestrator`).

9. **Secret-scan documentation false positives**  
   Docs intentionally mention `BEGIN PRIVATE KEY` as the scan pattern. Future secret-scan workflows must exclude or allowlist doc pattern lists (already noted in `perf-12-ci` / `secret-scan.yml` design).

10. **Fuzz depth is shallow**  
    `hive_native_fuzz_htlc` ran 5000 iterations with fixed seed in ctest; not a full libFuzzer/AFL campaign.

---

## Scorecard

| # | Item | Result |
|---|------|--------|
| 1 | cmake configure | ☑ PASS |
| 2 | cmake build all targets | ☑ PASS |
| 3 | ctest all pass (12/12) | ☑ PASS |
| 4 | `hive_native_tests` failed=0 (165 pass) | ☑ PASS |
| 5 | `hive_native_perf_tests` failed=0 (27 pass) | ☑ PASS |
| 6 | new test binaries all pass (10/10) | ☑ PASS |
| 7 | no secrets (`BEGIN PRIVATE` → docs only) | ☑ PASS |
| 8 | `docs/swarm/perf-*` ≥5 (14 present) | ☑ PASS |
| 9 | P0 portable modules present | ☑ PASS |
| V | residual risks listed | ☑ PASS |

### **Score: 10/10**

**CMake fix cycle:** not invoked (no configure/build/test failure).

---

## Reproduction (local parity)

```bash
cd /tmp/hive-native-features
git checkout swarm-perf-p0-impl
cmake -S . -B build
cmake --build build --target all -j$(sysctl -n hw.ncpu)
ctest --test-dir build --output-on-failure
./build/hive_native_tests          # expect failed=0
./build/hive_native_perf_tests     # expect failed=0
# plus remaining test binaries from box 6
git grep -n "BEGIN PRIVATE" || true
ls docs/swarm/perf-* | wc -l      # expect >= 5
ls include/hive_native/perf/
```
