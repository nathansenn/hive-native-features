# hive-native-features

Phased implementation of **native NFTs**, **HTLC atomic swaps**, and **metered smart contracts** for the [Hive](https://hive.blog) blockchain.

This repository is a **design-first, verification-gated** workspace. Code and documentation land only after checklist gates pass. The long-term goal is clean, upstream-friendly patches for Hive core (`gitlab.syncad.com/hive/hive` / `openhive-network/hive`) and related HAF / API layers.

Portable C++ sketches (protocol, evaluators, null contract engine, tests, benches) live **in-tree** and do not require a full Hive tree to build or run.

## Goals

| Feature | Intent |
|--------|--------|
| **Native NFTs** | First-class protocol objects (mint / transfer / burn / approve / approval-for-all), RC-metered, prune-aware, light-node safe |
| **HTLC Atomic Swaps** | Hash-time-locked contracts for trust-minimized asset exchange (`to`-only redeem) |
| **Metered Smart Contracts** | Plugin-first WASM (Wasmtime) with fuel → RC mapping; consensus activation only after human approval (Phase **3h**) |

## Non-negotiable constraints

1. **Preserve ~3-second block times** — no apply-path regressions without Architect + human waiver.
2. **Light / pruned / mobile safe** — every feature has an explicit skip or verify-only path.
3. **RC metering** — every new operation defines (or TODOs) resource-credit cost.
4. **Small, reviewable commits** — one logical unit per commit; draft PRs preferred.
5. **No secrets** in git history.
6. **Human gates** on architecture, HF numbers, consensus contracts (3h), repo policy, and merges to `main`.

## Phase status

| Phase | Branch | Status |
|-------|--------|--------|
| **0 – Design** | `phase-0-design` → merged to `main` | **Done** (PR #1 merged; decisions in ADRs) |
| **1 – Native NFT primitives** | `phase-1-nft` | **Portable code in-tree** (ops, evaluators, RC, HAF SQL, API stubs, tests, bench) |
| **2 – HTLC atomic swaps** | (same tree / follow-on branch) | **Portable code in-tree** (create / redeem / refund; `to`-only redeem) |
| **3a–3g – Metered contracts (plugin-first)** | (same tree) | **Portable code in-tree** (ops, null engine, host allow-list; optional Wasmtime) |
| **3h – Consensus activation** | — | **Still human-gated** — do not start without explicit approval |

## Build (CMake)

Requirements: CMake ≥ 3.16, C++17 compiler (`clang++` / `g++`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Options

| CMake option | Default | Meaning |
|--------------|---------|---------|
| `HIVE_NATIVE_BUILD_BENCH` | `ON` | Build `hive_native_bench` |
| `HIVE_NATIVE_WITH_WASMTIME` | `OFF` | Compile with Wasmtime C API (`WASMTIME_ROOT` for includes/libs) |

Example with Wasmtime (optional; not required for CI or portable tests):

```bash
cmake -S . -B build -DHIVE_NATIVE_WITH_WASMTIME=ON -DWASMTIME_ROOT=/path/to/wasmtime
cmake --build build -j
```

Without Wasmtime, the library still builds a **null engine** for protocol work and host-deny tests.

### Install (headers + static lib)

```bash
cmake --install build --prefix /usr/local   # or any prefix
```

## Test / bench commands

```bash
# Unit / edge suite (portable in-memory DB)
./build/hive_native_tests
# or via CTest
ctest --test-dir build --output-on-failure

# Microbenchmark skeleton (JSON on stdout)
./build/hive_native_bench
```

Expected smoke:

- Tests: `passed=N failed=0` (exit 0). Recent run: **63 / 0**.
- Bench: JSON with `nft_transfer_ratio` and `nft_within_hard_fail` (hard-fail budget ratio 5.0).

## Architecture decisions (ADRs)

| ADR | Title | Status |
|-----|-------|--------|
| [ADR-0001](./docs/decisions/ADR-0001-phase-0-human-gates.md) | Phase 0 human-gate decisions (feature order, public repo, HTLC `to`-only redeem, NFT approval-for-all, PR #1 merge) | Accepted |
| [ADR-0002](./docs/decisions/ADR-0002-wasmtime.md) | WASM runtime: **Wasmtime** (plugin-first; 3h still gated) | Accepted |

## Repository layout

```
include/hive_native/     Public headers (portable API)
  protocol/              NFT / HTLC / contract operation structs + validate
  chain/                 Database sketch + evaluators
  contracts/             Contract engine interface (null / optional Wasmtime)
  rc/                    RC cost placeholders
  api/                   database_api method stubs
  util/                  Shared types + crypto helpers
src/                     Matching implementations (static lib hive_native)
tests/                   Portable test runner (no external deps)
benchmarks/              Microbench vs synthetic transfer baseline
haf/sql/                 Proposed HAF / indexer tables (NFT, HTLC, contracts)
sdk/typescript/          Client SDK stubs (placeholder)
plugins/hive_contracts/  Non-consensus plugin skeleton (placeholder)
docs/                    Design pack + ADRs + swarm notes
  decisions/             ADR-0001, ADR-0002
  swarm/                 Agent work logs
scripts/                 Utility scripts (reserved)
CMakeLists.txt           Library, tests, optional bench
```

| Path | Role |
|------|------|
| `include/` | Headers for protocol, chain, RC, contracts, API stubs |
| `src/` | Implementation of the portable static library |
| `tests/` | Deterministic unit/edge suite (`hive_native_tests`) |
| `haf/` | SQL proposals for Hive Account History / indexer tables |
| `sdk/` | Downstream client stubs (e.g. TypeScript) |
| `plugins/` | Plugin-first contract host (non-consensus until 3h) |

## Documentation map

| Doc | Purpose |
|-----|---------|
| [WORKFLOW.md](./WORKFLOW.md) | Agent-swarm playbook, roles, universal work loop, verification checklists |
| [PROGRESS.md](./PROGRESS.md) | Live phase status, task board, verification log |
| [docs/00-architecture-overview.md](./docs/00-architecture-overview.md) | Hybrid phased approach, constraints, goals |
| [docs/01-nft-design.md](./docs/01-nft-design.md) | NFT object model, ops, RC, pruning, light-node |
| [docs/02-htlc-design.md](./docs/02-htlc-design.md) | HTLC create / redeem / refund, timeouts |
| [docs/03-contracts-design.md](./docs/03-contracts-design.md) | WASM options, metering, isolation, plugin-first |
| [docs/04-performance-budgets.md](./docs/04-performance-budgets.md) | Latency / RAM / RC budgets and gates |
| [docs/05-verification-and-testing-strategy.md](./docs/05-verification-and-testing-strategy.md) | Test pyramid, CI, fuzzing, regression policy |
| [docs/decisions/](./docs/decisions/) | Architecture Decision Records |

## Working with this repo

1. Read **WORKFLOW.md** end-to-end before changing anything.
2. Architect assigns a **TASK-ID**; specialists implement on a feature branch.
3. **Reviewer** + **Test & Verification** must pass before **GitOps** commits.
4. Commit messages: `phase-X: imperative summary` plus verification notes.
5. Never merge to `main` without explicit human approval.
6. **Phase 3h** (consensus contracts) remains blocked until an explicit human gate.

## Target upstream

Design and code should remain portable to:

- Hive consensus / protocol / chain: `gitlab.syncad.com/hive/hive` (and mirrors such as `openhive-network/hive`)
- HAF / sql_serializer / `database_api` plugins
- Client SDKs (dhive, hive-tx, etc.) as separate stubs

## License

TBD — confirm with repository owner before release. Until then, all rights reserved by the repository owner.

## Contact / ownership

- GitHub: [@nathansenn](https://github.com/nathansenn)
- Project start: 24 July 2026
