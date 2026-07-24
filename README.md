# hive-native-features

Phased implementation of **native NFTs**, **HTLC atomic swaps**, and **metered smart contracts** for the [Hive](https://hive.blog) blockchain.

This repository is a **design-first, verification-gated** workspace. Code and documentation land only after checklist gates pass. The long-term goal is clean, upstream-friendly patches for Hive core (`gitlab.syncad.com/hive/hive` / `openhive-network/hive`) and related HAF / API layers.

## Goals

| Feature | Intent |
|--------|--------|
| **Native NFTs** | First-class protocol objects (mint / transfer / burn / approve), RC-metered, prune-aware, light-node safe |
| **HTLC Atomic Swaps** | Hash-time-locked contracts for trust-minimized asset exchange (HIVE/HBD and NFT extensions) |
| **Metered Smart Contracts** | Plugin-first WASM contracts with fuel → RC mapping; consensus activation only after human approval |

## Non-negotiable constraints

1. **Preserve ~3-second block times** — no apply-path regressions without Architect + human waiver.
2. **Light / pruned / mobile safe** — every feature has an explicit skip or verify-only path.
3. **RC metering** — every new operation defines (or TODOs) resource-credit cost.
4. **Small, reviewable commits** — one logical unit per commit; draft PRs preferred.
5. **No secrets** in git history.
6. **Human gates** on architecture, HF numbers, WASM runtime, repo policy, and merges to `main`.

## Phases

| Phase | Branch | Status |
|-------|--------|--------|
| **0 – Design** | `phase-0-design` | In progress |
| **1 – Native NFT primitives** | `phase-1-nft` | Planned |
| **2 – HTLC atomic swaps** | `phase-2-htlc` | Planned |
| **3 – Metered smart contracts** | `phase-3-contracts` | Planned (plugin-first; consensus only after human approval) |

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

## Working with this repo

1. Read **WORKFLOW.md** end-to-end before changing anything.
2. Architect assigns a **TASK-ID**; specialists implement on a feature branch.
3. **Reviewer** + **Test & Verification** must pass before **GitOps** commits.
4. Commit messages: `phase-X: imperative summary` plus verification notes.
5. Never merge to `main` without explicit human approval.

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
