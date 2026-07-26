# 00 – Architecture Overview

**Task-ID:** phase-0 / 0.2  
**Status:** accepted (Phase 0 merged)  
**Last Updated:** 2026-07-25  
**Audience:** Architect, all specialist agents, human reviewers  

---

## 1. Purpose

Deliver three related Hive capabilities without breaking the properties that make Hive viable for social and financial use:

1. **Native NFTs** — first-class protocol assets (not only `custom_json` / side engines).
2. **HTLC atomic swaps** — trust-minimized time/hash-locked exchange.
3. **Metered smart contracts** — optional programmable logic with hard resource bounds.

This document defines the **hybrid phased approach**, system constraints, layering, and the rules every later design must obey.

---

## 2. Design goals

| Goal | Success signal |
|------|----------------|
| Consensus safety | Deterministic apply; HF-gated activation; no silent state divergence |
| Performance | Block interval stays ~3s under representative load; apply latency budgets in `04-performance-budgets.md` |
| Resource credits | Every new op has measurable RC cost; no free DoS surface |
| Light-node safety | Pruned / mobile / light modes can skip heavy state and still verify headers / proofs as designed |
| Upstream friendliness | Changes map cleanly onto Hive protocol, chain, plugins, HAF |
| Incremental ship | NFT → HTLC → contracts; each phase independently useful |

### Non-goals (Phase 0–2)

- Full EVM compatibility
- Unbounded Turing-complete on-chain compute without fuel
- Replacing Hive-Engine in one hardfork
- Consensus-forcing WASM in Phase 3a–3g (plugin-only until human gate 3h)

---

## 3. Hybrid phased approach

```
Phase 0  Design docs + budgets + verification strategy
    │
Phase 1  Native NFT primitives (protocol + state + eval + API stubs)
    │
Phase 2  HTLC (fungible + optional NFT lock extensions)
    │
Phase 3a–3g  Contracts as non-consensus plugin + metering design
    │
Phase 3h  Human-gated consensus activation design (not automatic)
```

**Why hybrid?**

- Hive’s social chain prioritizes fast blocks and low node cost.
- Side engines (`custom_json` + external interpreters) already show demand for NFTs/contracts but lack native finality integration and uniform light-client rules.
- Native primitives for **scarce, high-value state** (NFT ownership, HTLC locks) give strong guarantees.
- **General compute** is riskier; ship it plugin-first, meter ruthlessly, only then consider consensus activation.

---

## 4. Hive layering (where work lives)

| Layer | Path (conceptual upstream) | This project touches |
|-------|----------------------------|----------------------|
| Protocol types & ops | `libraries/protocol/` | New ops, types, serialization, authority fields |
| Chain state | `libraries/chain/` objects, indices | Objects, multi_index, pruning hooks |
| Evaluators | `libraries/chain/include/.../evaluator` | apply logic, RC hooks |
| Resource credits | RC plugin / resource user maps | Cost curves for new ops |
| Worker pool | `blockchain_worker_thread_pool` | CPU-heavy eval offload points (contracts later) |
| Plugins | `plugins/` | Optional contract runtime, APIs |
| HAF / sql_serializer | HAF stack | Tables for NFT, HTLC, contract events |
| database_api | `plugins/database_api` | Query methods; light-mode field omission |
| Clients | dhive / hive-tx / apps | Stubs and examples (later phases) |

**Repo strategy:** This repository holds design, portable C++ sketches, tests, and HAF SQL proposals. Upstream patches are prepared as clean, reviewable units — not a full Hive fork dump.

---

## 5. Cross-cutting constraints

### 5.1 Block time and apply path

- Assume **~3 second** target block time and existing Hive parallel-apply patterns.
- New evaluators must document: independent vs dependent; lock granularity; worst-case CPU.
- Prefer O(1) or O(log n) index ops; forbid unbounded loops over chainbase in evaluators.

### 5.2 RC metering

- Model after existing Hive RC: operation base cost + size/bytes + state creation.
- Placeholder costs allowed only with `TODO – measure` and a tracking TASK-ID.
- Contracts: **fuel** is strictly finite; fuel exhaustion aborts call without partial consensus mutation (or with well-defined rollback rules).

### 5.3 Light / pruned / mobile nodes

Every feature must pick one:

| Mode | Behavior |
|------|----------|
| **Skip** | Feature state not stored; node still processes blocks if op is present in block (validate signatures/structure only) |
| **Verify-only** | Recompute roots / verify proofs without full object history |
| **Full** | Store and index all objects (witness / archival / API nodes) |

**Default recommendation:**

- Witnesses with full state: full for NFT + HTLC; contracts storage per config.
- Pruned API: recent objects + irreversible HTLC; NFT metadata offloaded where possible.
- Light/mobile: skip heavy indices; verify ownership proofs via API or merkle/inclusion designs (Phase 1 details).

### 5.4 Pruning and state growth

- No unbounded main-chainbase growth without a prune path.
- Prefer: irreversible-only retention for completed HTLCs; NFT burn garbage-collection; contract storage rent or deposit (Phase 3 design).
- Large blobs (media, WASM code) → external/RocksDB/IPFS-hash-on-chain patterns, not multi-MB chainbase rows.

### 5.5 Hardfork discipline

- All consensus-changing ops gated by hardfork number **TBD (human gate)**.
- Serialization changes must remain backward compatible for pre-HF blocks.
- Virtual ops introduced for indexers (HAF) even when state is compact.

### 5.6 Authority and security

- NFT transfer/approve: active authority of owner (or approved operator), mirror token patterns carefully.
- HTLC: creator funds lock; redeem needs preimage; refund after timeout; no authority bypass.
- Contracts: host functions allow-list; no raw filesystem; no unbounded crypto unless metered.

---

## 6. Feature interaction map

```
                    ┌─────────────┐
                    │   Account   │
                    │  authority  │
                    └──────┬──────┘
           ┌───────────────┼───────────────┐
           v               v               v
     ┌──────────┐   ┌──────────┐   ┌──────────────┐
     │ NFT ops  │   │ HTLC ops │   │ Contract ops │
     └────┬─────┘   └────┬─────┘   └──────┬───────┘
          │              │                │
          v              v                v
     nft_object     htlc_object     contract_object
          │              │           + isolated k/v
          └──────┬───────┘                │
                 v                        v
            optional lock NFT ── fuel/RC ── call
                 into HTLC
```

- Phase 1 ships NFT alone.
- Phase 2 HTLC locks HIVE/HBD first; **optional** NFT-as-lock asset is an extension (design-ready, implement after fungible HTLC stable).
- Phase 3 contracts may read NFT/HTLC state only through allow-listed host functions.

---

## 7. Data placement strategy

| Data | Placement | Rationale |
|------|-----------|-----------|
| NFT id, owner, collection, approvals | Chainbase (compact) | Consensus critical |
| NFT metadata URI / hash | On-chain hash + off-chain URI | Avoid bloat |
| Open HTLC | Chainbase until resolved | Consensus critical |
| Resolved HTLC | Prune after N irreversible blocks | Size control |
| Contract code | Hash on-chain; code blob RocksDB/plugin store | Size + optional plugin |
| Contract storage | Isolated provider, metered | DoS control |
| Historical events | HAF / sql_serializer | Query scale |

---

## 8. API and indexer strategy

1. **Virtual operations** for mint, transfer, burn, approve, htlc_*, contract_*.
2. **HAF tables** mirror virtual ops and current-state projections.
3. **database_api** methods for wallets; light responses omit heavy fields.
4. **SDK stubs** after API shapes stabilize (not Phase 0).

---

## 9. Migration stance (Hive-Engine / custom_json)

- Native features **do not automatically migrate** Engine NFTs.
- Provide a documented **opt-in bridge** pattern: prove Engine ownership off-chain / via oracle committee **or** burn-and-remint with community process (human + legal/product decision).
- `custom_json` continues to work; native ops are additive.

---

## 10. Risk register (architecture level)

| Risk | Mitigation |
|------|------------|
| State bloat from NFTs | Collection limits, metadata off-chain, burn GC, RC on mint |
| HTLC griefing (lock spam) | RC + min lock amounts + max open HTLCs per account (TBD) |
| Contract DoS | Fuel, host allow-list, plugin isolation, no consensus until 3h |
| Parallel apply races | Annotate evaluator independence; avoid cross-object locks |
| Light-node forks | Explicit skip paths; never require full NFT index for header sync |
| Upstream rejection | Keep patches modular; match Hive style; RFCs per feature |

---

## 11. Performance budget pointers

See `docs/04-performance-budgets.md` for numeric gates. Architecture-level rules:

- NFT transfer: comparable to `transfer_operation` order of magnitude.
- HTLC create/redeem: small constant factor over transfer + hash verify.
- Contract call: hard fuel cap; must not monopolize block apply (queue/worker design in Phase 3).

---

## 12. Light-node rules (summary)

| Node class | NFT | HTLC | Contracts |
|------------|-----|------|-----------|
| Full archival | Full | Full history optional | Full or plugin |
| Standard API | Full current | Open + recent resolved | Configurable |
| Pruned | Current owners only | Open only | Code hash only |
| Light / mobile | Skip indices; verify via API | Skip; verify proofs if provided | Skip |

Exact APIs for light verification are specified per feature doc.

---

## 13. Decision log (human gates)

| Decision | Status | Notes |
|----------|--------|-------|
| Repo name `hive-native-features` | **Accepted** | **Public** repo (ADR-0001) |
| Feature order NFT → HTLC → contracts | **Approved** | Hybrid phased ship order locked (ADR-0001) |
| HTLC redeem authority | **Accepted** | **`to`-only redeem** (active authority of `to`); open-redeem deferred |
| NFT operator model | **Accepted** | **Approval-for-all included** in MVP (`nft_set_approval_for_all` + `nft_operator_object`) |
| WASM runtime | **Selected** | **Wasmtime** (ADR-0002); plugin-first until 3h |
| Phase 0 design pack (PR #1) | **Merged** | Phase 0 complete; Phase 1+ unblocked |
| HF numbers | **TBD** | Upstream human gate |
| NFT-in-HTLC in Phase 2 MVP | Optional extension | Default fungible-only MVP |
| Consensus contracts | Not before 3h | Human gate (standing) |

See also: `docs/decisions/ADR-0001-phase-0-human-gates.md`, `docs/decisions/ADR-0002-wasmtime.md`.

---

## 14. Acceptance for this document

- [x] Hybrid phased approach described
- [x] Constraints (performance, RC, light-node, pruning) stated
- [x] Layer map for upstream-friendly work
- [x] Feature interaction and risks
- [x] Reviewer agent approval
- [x] Second agent (or human) review

---

## 15. Next documents

1. `01-nft-design.md`  
2. `02-htlc-design.md`  
3. `03-contracts-design.md`  
4. `04-performance-budgets.md`  
5. `05-verification-and-testing-strategy.md`  
