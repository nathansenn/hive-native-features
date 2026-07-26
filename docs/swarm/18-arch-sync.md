# Swarm note 18 — Architecture decision log sync

**Task-ID:** phase-0 / arch-sync  
**Date:** 2026-07-25  
**Branch:** `phase-1-nft`  
**Workdir:** `/tmp/hive-native-features`  
**Canonical sources:** `docs/decisions/ADR-0001-phase-0-human-gates.md`, `docs/decisions/ADR-0002-wasmtime.md`

## Purpose

Bring `docs/00-architecture-overview.md` §13 Decision log and §14 Acceptance in line with accepted Phase 0 human gates so specialist agents stop treating closed gates as open.

## Decisions locked (synced into §13)

| Decision | Status | Source |
|----------|--------|--------|
| Public repository | **Accepted** | ADR-0001 #2 |
| Feature order NFT → HTLC → contracts | **Approved** | ADR-0001 #1 |
| WASM runtime | **Wasmtime selected** | ADR-0001 #3 / ADR-0002 |
| HTLC redeem authority | **`to`-only redeem** | ADR-0001 #4 |
| NFT operator model | **Approval-for-all included** | ADR-0001 #5 |
| Phase 0 design pack (PR #1) | **Merged** | ADR-0001 #6 |

Still open (unchanged):

- HF numbers (**TBD** upstream)
- NFT-in-HTLC Phase 2 MVP (optional extension; fungible-only default)
- Consensus contracts activation (not before **3h**)

## What changed

### `docs/00-architecture-overview.md`

- Header: **Status** → `accepted (Phase 0 merged)`; **Last Updated** → `2026-07-25`.
- §13 Decision log rows updated from “Proposed / Options only / Awaiting review” to accepted statuses above.
- Added explicit rows for HTLC `to`-only redeem, approval-for-all, Wasmtime, and Phase 0 merge.
- Linked ADR-0001 / ADR-0002 from the decision log.
- §14 Acceptance: marked **Reviewer agent approval** and **Second agent (or human) review** complete.

### This file

- `docs/swarm/18-arch-sync.md` — handoff / change log only.

## Agent implications

| Area | Implication |
|------|-------------|
| Phase 1 NFT | Ship `nft_set_approval_for_all_operation` + `nft_operator_object` |
| Phase 2 HTLC | Redeem requires **`to` active authority** only |
| Phase 3a | Prefer **Wasmtime**; null engine without link; no consensus until 3h |
| GitOps | Public repo; Phase 0 merged; do not re-open Phase 0 gates |
| Standing | Do not block on closed gates; only 3h / true consensus ambiguity pauses |

## Out of scope

- No source/code changes.
- No PROGRESS.md / README rewrite (see `docs/swarm/17-readme-progress.md`).
- No Phase 3h design.
- Did not commit or open PRs (docs task only).

## Verification

Docs-only edit. Cross-checked decision table against ADR-0001 and ADR-0002; feature docs (`01`/`02`/`03`) already reflect the same locks.
