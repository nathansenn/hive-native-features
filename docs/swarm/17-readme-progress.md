# Swarm note 17 — README + PROGRESS refresh

**Task-ID:** docs / readme-progress  
**Date:** 2026-07-26  
**Branch:** `phase-1-nft`  
**Workdir:** `/tmp/hive-native-features`

## What changed

### `README.md`

Rewrote from design-only bootstrap to a **builder/developer entry point**:

- **Build instructions** — CMake configure/build (`cmake -S . -B build`, `cmake --build build -j`), C++17 / CMake ≥ 3.16.
- **Options** — `HIVE_NATIVE_BUILD_BENCH`, `HIVE_NATIVE_WITH_WASMTIME` + `WASMTIME_ROOT`; null engine without Wasmtime.
- **Test / bench commands** — `./build/hive_native_tests`, `ctest --test-dir build`, `./build/hive_native_bench`; expected smoke results.
- **Phase status** — Phase 0 **done** (PR #1 merged); Phases 1–3 **portable code in-tree**; Phase **3h still human-gated**.
- **ADRs** — table linking ADR-0001 (human gates) and ADR-0002 (Wasmtime).
- **Layout** — tree + table for `include/`, `src/`, `tests/`, `haf/`, `sdk/`, `plugins/`, `docs/`, `benchmarks/`.
- Kept goals, constraints, documentation map, workflow rules, upstream targets.

### `PROGRESS.md`

Comprehensive status rewrite:

- Phase 0 complete / merged; decisions **locked** (ADR-0001 / ADR-0002).
- Portable implementation checklist for NFT, HTLC, contracts (null engine).
- Blocked section limited to **3h** + HF numbers.
- Next steps: portable polish, plugin/Wasmtime non-consensus work; **3h not started**.
- Verification log extended with **2026-07-26** entries: cmake build, 63 tests pass, bench within hard-fail budget.
- Human gates split into closed vs still open (3h + HF schedule).

### This file

- `docs/swarm/17-readme-progress.md` — change log only (no code).

## Out of scope

- No source/code changes in this task.
- No Phase 3h design started.
- Did not commit or open PRs (docs task only).

## Verification

Docs-only edit. Portable suite previously verified (see `docs/swarm/01-build-verify.md` and PROGRESS verification log):

```
passed=63 failed=0
nft_within_hard_fail: true
```
