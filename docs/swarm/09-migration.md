# Swarm note 09 – Hive-Engine migration

**Task-ID:** phase-1 / 1.11  
**Role focus:** Architect, HAF/API, Reviewer (docs), project-facing guidance  

---

## Canonical document

Full migration design lives here:

**→ [`docs/06-hive-engine-migration.md`](../06-hive-engine-migration.md)**

Upstream sketch (do not duplicate long-form here):

**→ [`docs/01-nft-design.md` §13 – Migration from Hive-Engine](../01-nft-design.md)**  
**→ [`docs/00-architecture-overview.md` §9 – Migration stance](../00-architecture-overview.md)**

---

## One-screen summary for agents

| Path | Name | Agent takeaway |
|------|------|----------------|
| **A** | Opt-in remint | Ordinary `nft_create_collection` + `nft_mint`; social process; no Engine in evaluators |
| **B** | Attested bridge | Multisig/oracle mints after off-chain burn proof; trust is application-level |
| **C** | Parallel coexistence | Engine/`custom_json` stay live; native optional; label dual rails in UX |
| **—** | No automatic import | **Never** load Engine state in consensus apply or HF genesis dump |

When implementing protocol/state/evaluators: migration is **documentation and app ops only**. Do not add Engine RPC, side-DB reads, or auto-mint at hardfork.

When writing marketplace/SDK copy: use checklist and risk tables in `06-hive-engine-migration.md`.

---

## Handoff

```
FROM: Architect
TO: Protocol / Evaluator / HAF agents
TASK-ID: phase-1-1.11
STATUS: documented
SUMMARY: Migration paths A/B/C expanded; consensus remains Engine-agnostic.
ARTIFACTS: docs/06-hive-engine-migration.md, docs/swarm/09-migration.md
VERIFICATION: docs-only; links resolve to 01 §13 and 00 §9
NEXT: No chain code required for 1.11; continue 1.x implementation tasks without Engine import hooks
```
