# Checkbox Protocol Audit

**Auditor role:** Checkbox Protocol auditor  
**Workspace:** `/tmp/hive-native-features`  
**Git HEAD:** `f3fe296` (`docs: swarm reports and upstream/migration guides`)  
**Audit UTC:** 2026-07-26T03:13:17Z  
**Method:** 120s wait for peer agents, then one checklist item at a time with command/file evidence.

**Final score: 10/10** (all primary boxes pass; residual risks documented under **V**)

---

## ☐ → ☑ 1 Source tree has protocol / chain / rc / contracts / api

**Result: ☑ PASS**

**Evidence:** Both `include/hive_native/{protocol,chain,rc,contracts,api}` and matching `src/{protocol,chain,rc,contracts,api}` exist.

```
include/hive_native/
  api/         database_api_stubs.hpp
  chain/       database.hpp, evaluators.hpp
  contracts/   engine.hpp
  protocol/    contract_operations.hpp, htlc_operations.hpp, nft_operations.hpp
  rc/          costs.hpp
  util/        types.hpp

src/
  api/         database_api_stubs.cpp
  chain/       database.cpp, evaluators_{nft,htlc,contracts}.cpp
  contracts/   engine.cpp
  protocol/    validate.cpp
  rc/          costs.cpp
  util/        crypto.cpp
```

Commands:

```bash
ls include/hive_native/
# api chain contracts protocol rc util
ls src/
# api chain contracts protocol rc util
find include/hive_native src -type f | sort
# 18 sources under protocol/chain/rc/contracts/api (+ util)
```

---

## ☐ → ☑ 2 CMake builds

**Result: ☑ PASS**

**Evidence:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# -- Configuring done / -- Generating done
cmake --build build -j$(sysctl -n hw.ncpu)
# [100%] Built target hive_native_bench
# [100%] Built target hive_native_tests
# exit 0
```

Artifacts: `build/libhive_native.a`, `build/hive_native_tests`, `build/hive_native_bench`.  
Root `CMakeLists.txt` targets: static lib `hive_native`, test exe, optional bench (`HIVE_NATIVE_BUILD_BENCH=ON` default).

---

## ☐ → ☑ 3 Tests pass failed=0

**Result: ☑ PASS**

**Evidence:**

```bash
cd build && ctest --output-on-failure
# 1/1 Test #1: hive_native_tests ................   Passed    0.29 sec
# 100% tests passed out of 1

./hive_native_tests
# passed=165 failed=0
# exit 0
```

No fix cycle required.

---

## ☐ → ☑ 4 Bench exit 0 and nft ratio ≤5

**Result: ☑ PASS**

**Evidence:**

```bash
./build/hive_native_bench
# exit 0
```

Sample output (portable in-memory):

```json
{
  "synthetic_transfer_us": 0.388813,
  "nft_transfer_us": 0.476875,
  "nft_transfer_ratio": 1.22649,
  "htlc_create_us": 2.96823,
  "budget_nft_p50_ratio_max": 1.5,
  "budget_nft_hard_fail_ratio": 5.0,
  "nft_within_hard_fail": true,
  "note": "portable in-memory; ratio vs synthetic transfer-class op"
}
```

- **Process exit:** 0  
- **`nft_transfer_ratio`:** ~1.12–1.23 across two runs (both **≤ 5**; within hard-fail budget)  
- Flag: `"nft_within_hard_fail": true`

---

## ☐ → ☑ 5 ADR-0001 and ADR-0002 exist

**Result: ☑ PASS**

**Evidence:**

| File | Status | Topic |
|------|--------|--------|
| `docs/decisions/ADR-0001-phase-0-human-gates.md` | Accepted | Phase 0 human gates (feature order, public repo, Wasmtime, HTLC `to` auth, NFT approval-for-all, PR #1 merge) |
| `docs/decisions/ADR-0002-wasmtime.md` | Accepted | WASM runtime = Wasmtime; null engine without link; 3h still gated |

```bash
ls docs/decisions/
# ADR-0001-phase-0-human-gates.md
# ADR-0002-wasmtime.md
```

---

## ☐ → ☑ 6 HAF SQL files exist

**Result: ☑ PASS**

**Evidence:**

```bash
ls -la haf/sql/
# 001_nft_tables.sql       (119 lines)
# 002_htlc_tables.sql      (111 lines)
# 003_contracts_tables.sql (105 lines)
# total 335 lines
```

Headers reference Task-IDs phase-1/1.8, phase-2/HAF, phase-3/HAF.

---

## ☐ → ☑ 7 SDK or plugin stubs exist

**Result: ☑ PASS** (both present)

**SDK (TypeScript stubs):**

```
sdk/typescript/
  package.json, tsconfig.json, README.md
  src/index.ts   (9 lines — package entry)
  src/ops.ts     (392 lines — op builders)
  src/types.ts   (366 lines — field types aligned to protocol/*.hpp)
```

**Plugin stub:**

```
plugins/hive_contracts/
  README.md
  include/hive_contracts_plugin.hpp  (80 lines)
  src/hive_contracts_plugin.cpp      (68 lines)
```

Stubs explicitly non-network / non-consensus (SDK comment: “Stubs only — no network, signing, or consensus serialization”).

---

## ☐ → ☑ 8 No secrets in tree (BEGIN PRIVATE | api_key | ghp_)

**Result: ☑ PASS** (no live secrets; only CI documentation of scan patterns)

**Evidence:**

```bash
rg -n --hidden -g '!.git/*' -g '!node_modules/*' -e 'BEGIN PRIVATE|api_key|ghp_' .
# ./docs/swarm/05-ci.md:57: - `BEGIN PRIVATE KEY`
```

Context (docs only — pattern list for CI secret scan, not a key material):

```text
Scans **tracked** files with `git grep` for PEM private-key headers, including:
- `BEGIN PRIVATE KEY`
- `BEGIN RSA PRIVATE KEY`
- ...
```

No matches for `-----BEGIN` PEM bodies, `ghp_` tokens, or assigned `api_key=` values in tracked source. **No remediation needed.**

---

## ☐ → ☑ 9 PROGRESS.md exists and mentions Phase 0

**Result: ☑ PASS**

**Evidence:**

```bash
ls -la PROGRESS.md
# -rw-r--r-- ... 6411 ... PROGRESS.md

rg -n "Phase 0" PROGRESS.md
# multiple hits, including:
# | Phase 0 – Design | **Complete** — PR #1 merged to `main` |
# ### Phase 0 (merged)
# - [x] Phase 0 marked complete
# - [x] PROGRESS.md shows Phase 0 complete
```

---

## ☐ → ☑ V Known failure modes — residual risks documented

**Result: ☑ PASS** (risks acknowledged here with file citations; not product defects for this audit gate)

### V1 — Untested `max_supply` race / concurrent mint

- **What works:** Sequential cap is enforced and unit-tested.
  - Check: `src/chain/evaluators_nft.cpp` — `if(col.max_supply != 0 && col.supply >= col.max_supply) throw ...` then `col.supply += 1`.
  - Test: `tests/test_runner.cpp` `test_nft_max_supply()` — mint once with `max_supply=1`, second mint throws.
- **Gap:** Portable `database` is single-threaded in-memory maps; no concurrent/interleaved mint stress, no parallel-apply annotation test proving serialization on `collection_id`. Design doc (`docs/01-nft-design.md`) notes mint serialization on collection id for supply races — **not exercised under concurrency**.
- **Risk:** Over-mint past `max_supply` if future multi-threaded apply does not lock/serialize per collection.
- **Mitigation path:** Collection-level apply lock / serial mint queue; concurrent property tests before mainnet HF.

### V2 — WASM is not real (null engine)

- **Evidence:** `src/contracts/engine.cpp` header:
  > “No real WASM: deterministic meter-only stand-in until Wasmtime is linked (ADR-0002; HIVE_NATIVE_WITH_WASMTIME).”
- Default path is `null_engine` with meta-protocol args (`WRITE:`, `BURN:`, `DENY:`, …). Optional Wasmtime is compile-flagged and **not** required for CI.
- **Risk:** Fuel, host deny, and storage commit semantics may diverge from real Wasmtime Cranelift execution; consensus-path WASM (Phase **3h**) remains human-gated (`PROGRESS.md`, ADR-0002).
- **Mitigation path:** Link Wasmtime behind flag; golden tests vs null engine; never enable consensus activation without 3h approval.

### V3 — Hardfork number placeholders

- **Evidence:** `include/hive_native/util/types.hpp`:

```cpp
// ---- Hardfork placeholder (human-set before mainnet) ----
inline constexpr uint32_t HIVE_HARDFORK_NFT   = 9001; // TBD upstream number
inline constexpr uint32_t HIVE_HARDFORK_HTLC  = 9002;
inline constexpr uint32_t HIVE_HARDFORK_CONTRACTS = 9003; // Phase 3h only
```

- Evaluators call `db.require_hardfork(HIVE_HARDFORK_*)` correctly for gates, but **9001/9002/9003 are not real upstream HF IDs**.
- **Risk:** Accidental mainnet/testnet activation with wrong numbers; docs/schedule mismatch.
- **Mitigation path:** Replace placeholders with upstream-assigned HF numbers only after maintainer coordination (PROGRESS: “HF numbers TBD upstream”).

### Additional residual notes (informational)

| Area | Note |
|------|------|
| RC costs | Placeholder costs with `TODO – measure` policy (`docs/08-rc-cost-model.md`) — not production-tuned |
| SDK / plugin | Stubs only; no signing, network, or consensus serialization |
| Bench | In-memory synthetic transfer class; not full node / RocksDB / witness hardware |
| HAF SQL | Schema proposals; not applied to a live HAF instance in this audit |

---

## Scorecard

| # | Item | Result |
|---|------|--------|
| 1 | Source tree protocol/chain/rc/contracts/api | ☑ |
| 2 | CMake builds | ☑ |
| 3 | Tests `failed=0` (165 passed) | ☑ |
| 4 | Bench exit 0, nft ratio ≤5 (~1.23) | ☑ |
| 5 | ADR-0001 + ADR-0002 | ☑ |
| 6 | HAF SQL files | ☑ |
| 7 | SDK or plugin stubs | ☑ |
| 8 | No secrets in tree | ☑ |
| 9 | PROGRESS.md + Phase 0 | ☑ |
| V | Residual risks documented | ☑ |

### **Final score: 10/10**

**Fix cycles:** 0 (tests green first pass).

### Residual risks (summary)

1. **`max_supply` concurrent mint race untested** under parallel apply.  
2. **No real WASM** — `null_engine` stand-in only; Wasmtime optional / 3h gated.  
3. **HF placeholders** `9001/9002/9003` TBD with upstream.  
4. RC placeholders, stub SDK/plugin, synthetic bench, unapplied HAF schemas (lower severity for Phase-portable pack).
