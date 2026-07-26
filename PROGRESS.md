# Progress – hive-native-features

**Current Phase:** Phases 1–3 portable implementation (plugin-first); **3h still gated**  
**Last Updated:** 2026-07-26  
**Active Branch:** `phase-1-nft`  
**Open PRs:** none tracked for portable pack (Phase 0 PR #1 **merged**)  
**Repo:** https://github.com/nathansenn/hive-native-features  

## Status summary

| Area | State |
|------|--------|
| Phase 0 – Design | **Complete** — PR #1 merged to `main` |
| Human-gate decisions | **Locked** in [ADR-0001](./docs/decisions/ADR-0001-phase-0-human-gates.md) + [ADR-0002](./docs/decisions/ADR-0002-wasmtime.md) |
| Phase 1 – Native NFT | **Portable code in-tree** (protocol, evaluators, RC, HAF SQL, API stubs, tests, bench) |
| Phase 2 – HTLC | **Portable code in-tree** (create / redeem / refund; redeem = `to` active only) |
| Phase 3a–3g – Contracts | **Portable code in-tree** (ops, null engine, host allow-list; optional Wasmtime flag) |
| Phase 3h – Consensus activation | **Blocked** — explicit human approval required; do not design/ship consensus WASM yet |

## Completed

### Phase 0 (merged)

- [x] Repo created (`nathansenn/hive-native-features`, public)
- [x] Identity confirmed (`nathansenn`)
- [x] Branch `phase-0-design` + design pack docs 00–05
- [x] WORKFLOW.md, README.md, PROGRESS.md, .gitignore
- [x] Draft PR #1 “Phase 0 – Design complete” opened and **merged**
- [x] Milestones M1–M4 + tracking issues #2–#5 created
- [x] Human review / Phase 0 gates closed via ADR-0001
- [x] Phase 0 marked complete

### Decisions locked (ADR-0001 / ADR-0002)

- [x] Feature order: NFT → HTLC → contracts
- [x] Repository visibility: **public**
- [x] WASM runtime: **Wasmtime** (ADR-0002)
- [x] HTLC redeem: **`to` account active authority only**
- [x] NFT MVP: **approval-for-all** included
- [x] Phase 0 PR #1: **merged**

### Portable implementation (Phases 1–3 sketch)

- [x] CMake project (`hive_native` static lib, tests, optional bench)
- [x] NFT protocol + evaluators + RC placeholders
- [x] HTLC protocol + evaluators (`to`-only redeem, `from`-only refund)
- [x] Contract ops + null engine + host deny allow-list tests
- [x] Optional `HIVE_NATIVE_WITH_WASMTIME` compile path
- [x] HAF SQL proposals under `haf/sql/`
- [x] `database_api` stubs
- [x] Portable test suite + microbench skeleton
- [x] Design docs updated for accepted status + portable code

## In Progress

- Task-ID: phase-1+ portable polish / upstream readiness  
  Owner: Architect + specialists  
  Status: portable suite green; SDK/plugin dirs are placeholders; HF numbers TBD upstream

## Blocked

- **Phase 3h only:** consensus activation design and any consensus-forcing WASM path — wait for explicit human approval.
- Hardfork numbers / activation timing for NFT, HTLC, contracts — TBD with upstream maintainers (placeholders in code).

## Next 3 Tasks

1. Keep portable suite green; expand tests / RC measurement as needed for Phase 1 PR readiness.
2. Flesh `plugins/hive_contracts` and optional Wasmtime integration behind `HIVE_NATIVE_WITH_WASMTIME` (still non-consensus).
3. **Do not start Phase 3h** until human gate; when ready: consensus activation design only after explicit approval.

## Phase checklist

### Phase 0 acceptance

- [x] All design docs written
- [x] Performance budgets written
- [x] Light-node rules written
- [x] Design docs reviewed by ≥ 2 agent roles
- [x] PROGRESS.md shows Phase 0 complete
- [x] PR #1 merged

### Phase 1–3 portable (in-repo)

- [x] NFT / HTLC / contract portable ops + evaluators
- [x] Unit tests pass (`failed=0`)
- [x] Microbench skeleton runs; NFT ratio within hard-fail budget
- [ ] Upstream Hive integration PR (not yet)
- [ ] Phase 3h consensus activation (human-gated)

## Milestones & issues (live)

| Milestone | Issues |
|-----------|--------|
| [Phase 0 – Design](https://github.com/nathansenn/hive-native-features/milestone/1) | [#2 Human review](https://github.com/nathansenn/hive-native-features/issues/2) — **resolved via ADR-0001 / merge** |
| [Phase 1 – Native NFT](https://github.com/nathansenn/hive-native-features/milestone/2) | [#3 Protocol definitions 1.1](https://github.com/nathansenn/hive-native-features/issues/3) |
| [Phase 2 – HTLC](https://github.com/nathansenn/hive-native-features/milestone/3) | [#4 HTLC epic](https://github.com/nathansenn/hive-native-features/issues/4) |
| [Phase 3 – Contracts](https://github.com/nathansenn/hive-native-features/milestone/4) | [#5 WASM 3a skeleton](https://github.com/nathansenn/hive-native-features/issues/5) |

## Verification Log (last entries)

| Task-ID | Result | Notes | Date | Agent |
|---------|--------|-------|------|-------|
| phase-0-bootstrap | PASS | Identity nathansenn; repo public; no secrets; docs syntax OK | 2026-07-24 | GitOps + Reviewer |
| phase-0-docs-pack | PASS | Design docs 00–05 complete | 2026-07-24 | Architect + Reviewer |
| phase-0-pr-1 | PASS | Draft PR #1 opened; milestones + issues created | 2026-07-24 | GitOps |
| phase-0-reviewer | PASS | Design consistency, light-node rules, human gates explicit | 2026-07-24 | Reviewer |
| phase-0-merge | PASS | PR #1 merged; decisions locked ADR-0001 / ADR-0002 | 2026-07-26 | Human + Architect |
| portable-cmake-build | PASS | `cmake -S . -B build && cmake --build build -j` | 2026-07-26 | Test & Verification |
| portable-unit-tests | PASS | `./build/hive_native_tests` → **passed=63 failed=0** | 2026-07-26 | Test & Verification |
| portable-bench | PASS | `./build/hive_native_bench` → `nft_within_hard_fail: true` (ratio ~1.0–1.2 vs transfer baseline) | 2026-07-26 | Performance Benchmarker |

### Commands used (2026-07-26)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hive_native_tests    # passed=63 failed=0
./build/hive_native_bench    # JSON budgets; nft_within_hard_fail true
```

## Human gates

### Closed (ADR-0001)

1. ~~Approve Phase 0 design direction~~ — **approved**
2. ~~Confirm public visibility~~ — **public**
3. ~~WASM runtime~~ — **Wasmtime** (ADR-0002)
4. ~~HTLC redeem authority~~ — **`to`-only**
5. ~~NFT approval-for-all~~ — **included in MVP**
6. ~~Merge design PR~~ — **merged**

### Still open

1. **Phase 3h** — consensus activation design and shipping consensus WASM: **requires explicit human approval**.
2. Hardfork numbers / activation schedule for each feature: TBD with upstream.

## Performance program (2026-07-26)

- Catalogue: `docs/performance/HIVE_1000_OPTIMIZATIONS.md` (1000 items)
- P0 roadmap: `docs/performance/P0_ROADMAP.md`
- Portable prototypes: `include/hive_native/perf/*` + `hive_native_perf_tests` (27 checks)
- Verification: feature tests 165 pass; perf tests 27 pass; ctest 2/2
