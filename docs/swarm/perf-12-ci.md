# perf-12 – CI: full ctest + benches

**Task-ID:** swarm / perf-12-ci  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Catalogue:** #794 continuous micro-benchmarks + regression gates in CI  
**Related:** [docs/swarm/05-ci.md](./05-ci.md), [docs/04-performance-budgets.md](../04-performance-budgets.md)

---

## 1. Purpose

Keep GitHub Actions green for the portable library while the P0 swarm lands more
`add_test` targets and the optional RC calibration harness:

1. After configure/build, run **all** CTest tests (no `-R` filter).
2. Always run `./build/hive_native_bench` (ratio hard-fail gate).
3. If `./build/hive_native_bench_rc` was built, run it; **skip only when missing**
   (do not `|| true` over a real non-zero exit).
4. Leave [`.github/workflows/secret-scan.yml`](../../.github/workflows/secret-scan.yml)
   unchanged so private-key markers still fail CI.

---

## 2. Deliverables

| Artifact | Role |
|----------|------|
| [`.github/workflows/ci.yml`](../../.github/workflows/ci.yml) | Build → all ctest → bench → optional bench_rc |
| [`.github/workflows/secret-scan.yml`](../../.github/workflows/secret-scan.yml) | Unchanged; still scans tracked PEM headers |
| `docs/swarm/perf-12-ci.md` | This report |

CMake (other swarm tasks) already registers:

| CMake / binary | Role in CI |
|----------------|------------|
| `add_test(...)` suite | Exercised by bare `ctest --test-dir build` |
| `hive_native_bench` | Required step when built (default `HIVE_NATIVE_BUILD_BENCH=ON`) |
| `hive_native_bench_rc` | Optional step; present when bench option is ON |

---

## 3. Workflow: `ci.yml` (updated)

**Triggers:** `push`, `pull_request`  
**Runner:** `ubuntu-latest`

| Step | Action | Fail policy |
|------|--------|-------------|
| Install | `cmake`, `g++`, `make` | fail |
| Configure | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` | fail |
| Build | `cmake --build build --parallel` | fail |
| **Test (all ctest)** | `ctest --test-dir build --output-on-failure` | **any test fail → job fail** |
| **Benchmark** | `./build/hive_native_bench` | non-zero / crash → job fail |
| **RC calibration** | if `[ -x ./build/hive_native_bench_rc ]` then run it; else `true` | binary **missing** → skip; binary **fails** → job fail |

### `hive_native_bench_rc` soft-skip (missing only)

```bash
set -euo pipefail
if [ -x ./build/hive_native_bench_rc ]; then
  echo "Running hive_native_bench_rc"
  ./build/hive_native_bench_rc
else
  echo "hive_native_bench_rc not found; skipping (optional target)"
  true
fi
```

**Not** `./build/hive_native_bench_rc || true` — that would hide calibration
failures when the binary exists.

---

## 4. Secret-scan (unchanged)

[`secret-scan.yml`](../../.github/workflows/secret-scan.yml) remains a separate
workflow on `push` / `pull_request`. It still `git grep`s tracked files for:

- `BEGIN PRIVATE KEY`
- `BEGIN RSA PRIVATE KEY`
- `BEGIN OPENSSH PRIVATE KEY`
- `BEGIN EC PRIVATE KEY`
- `BEGIN DSA PRIVATE KEY`

Any match outside the workflow’s own pattern exclusion fails the job. CI
changes in this task do not touch that file.

---

## 5. Failure policy (summary)

| Condition | Result |
|-----------|--------|
| Configure or compile error | CI job fails |
| Any CTest failure | CI job fails |
| `hive_native_bench` crash / non-zero | CI job fails |
| `hive_native_bench_rc` missing | skip (soft) |
| `hive_native_bench_rc` present and non-zero | CI job fails |
| Private-key marker in tracked files | Secret-scan job fails |

---

## 6. Local parity

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/hive_native_bench
if [ -x ./build/hive_native_bench_rc ]; then
  ./build/hive_native_bench_rc
fi
# secret-scan parity (same patterns as workflow):
git grep -nE 'BEGIN (RSA |OPENSSH |EC |DSA )?PRIVATE KEY|BEGIN RSA PRIVATE KEY' \
  -- . ':!.github/workflows/secret-scan.yml' && exit 1 || true
```

---

## 7. Out of scope

- Changing secret-scan patterns or third-party scanners  
- Enabling `HIVE_NATIVE_WITH_WASMTIME` in CI  
- Compiler matrix / sanitizers / coverage  
- Making `hive_native_bench_rc` a CTest hard-fail gate (it stays a separate binary)
