# ADR-0001 – Phase 0 human-gate decisions

**Date:** 2026-07-26  
**Status:** Accepted  

## Context

Phase 0 design pack required human gates before Phase 1+.

## Decisions

| # | Question | Decision |
|---|----------|----------|
| 1 | Feature order NFT → HTLC → contracts | **Approved** |
| 2 | Repository visibility | **Public** |
| 3 | WASM runtime | **Wasmtime** (see ADR-0002) — selected as best fit without further human Q&A |
| 4 | HTLC redeem authority | **`to` account active authority only** (recommended) |
| 5 | NFT operator model | **Include approval-for-all** in MVP |
| 6 | Merge Phase 0 PR #1 | **Merged** |

## Standing orders

- Do not block on further human questions unless a hard security/consensus activation gate (Phase 3h) or true ambiguity that would destroy work.
- Goal: maximize Hive capability — advanced features, speed, usefulness — while preserving 3s blocks, RC, light-node safety.

## Consequences

- Phase 1 ships `nft_set_approval_for_all_operation` + `nft_operator_object`.
- Phase 2 redeem requires `to` active auth.
- Phase 3a embeds Wasmtime; plugin-first until 3h.
