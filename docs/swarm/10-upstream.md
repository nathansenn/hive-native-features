# Swarm note 10 – Upstream integration

**Task-ID:** phase-1 / upstream-portability  
**Canonical doc:** [`docs/07-upstream-integration.md`](../07-upstream-integration.md)  
**Date:** 2026-07-25  
**Workdir:** `/tmp/hive-native-features`

---

## Purpose (agents)

Use this as a **short handoff** when preparing Hive MRs. Full detail lives in `07-upstream-integration.md`. Do not open core MRs until human HF numbers and phase gates allow.

---

## Portable → upstream cheat sheet

| Portable path | Upstream target |
|---------------|-----------------|
| `include/hive_native/util/types.hpp` | `libraries/protocol/...` types, config, HF macros |
| `include/hive_native/protocol/*_operations.hpp` | `libraries/protocol/` + `operations.hpp` variants |
| `src/protocol/validate.cpp` | protocol `validate()` impls |
| `include/hive_native/chain/database.hpp` objects | `libraries/chain/` objects + multi_index |
| `src/chain/evaluators_*.cpp` | `libraries/chain/*_evaluator.cpp` |
| `include/hive_native/rc/costs.hpp` + `src/rc/costs.cpp` | **RC plugin** resource counters / cost maps |
| `include/hive_native/api/database_api_stubs.hpp` | **database_api** plugin methods |
| `haf/sql/001_*.sql` … `003_*.sql` | HAF migrations + **sql_serializer** virtual-op sinks |
| `HIVE_HARDFORK_NFT/HTLC/CONTRACTS` (9001–9003) | Real **hardfork guards** (human-assigned numbers) |

---

## Hardfork guards

| Feature | Portable constant | Evaluator gate |
|---------|-------------------|----------------|
| NFT | `HIVE_HARDFORK_NFT` (9001) | all NFT `apply` |
| HTLC | `HIVE_HARDFORK_HTLC` (9002) | all HTLC `apply` |
| Contracts consensus | `HIVE_HARDFORK_CONTRACTS` (9003) | deploy/call consensus path |

Upstream: `db.has_hardfork(...)` / Hive HF schedule. Placeholders **must not** ship to mainnet.

---

## FC / chainbase conversion (minimum steps)

1. **Namespace** `hive_native::*` → `hive::protocol` / `hive::chain`.
2. **Primitives** → FC: `account_name_type`, `asset`, `fc::sha256`, `time_point_sec`, exceptions.
3. **Ops** → `FC_REFLECT` + append to `operation` / `virtual_operation` `static_variant`.
4. **Objects** → chainbase `object<...>` + multi_index (`by_id`, `by_owner`, …) + undo-safe create/modify/remove.
5. **Evaluators** → Hive `evaluator<T>::do_apply`; push **typed** virtual ops; use real `adjust_balance`.
6. **RC** → register op in RC plugin; recalibrate from portable relative units vs `transfer`.
7. **API** → database_api impl + reflect args; clamp limit ≤ 100; light omit heavy fields.
8. **HAF** → land SQL migrations; wire sql_serializer to virtual op names.
9. **Crypto** → drop portable sha/ripemd in consensus; use FC (keep portable for standalone tests).
10. **Verify** → chain tests + HF off/on + budgets in `docs/04-performance-budgets.md`.

---

## Role ownership (WORKFLOW.md)

| Role | Upstream surface |
|------|------------------|
| Protocol Coder | `libraries/protocol`, HF tags, reflections |
| State / Storage Coder | chain objects, indexes, prune |
| Evaluator / Apply Coder | evaluators, parallel notes, worker pool later |
| HAF / API / SDK Coder | sql_serializer, database_api, client stubs |
| Reviewer | authority, DoS, HF safety |
| GitOps | draft MRs to Syncad/Hive only after gates |

---

## MR order (do not collapse)

1. NFT protocol + HF stub  
2. NFT chain + RC  
3. NFT database_api  
4. NFT HAF / sql_serializer  
5. HTLC (same vertical slice)  
6. Contracts **plugin** (non-consensus)  
7. Contracts consensus — **human gate 3h only**

---

## Explicit non-actions

- No wholesale dump of this repo into Hive.
- No mainnet HF number invention.
- No consensus Wasmtime until Phase 3h approval.
- No secrets in patches.

---

## Related swarm notes

- `docs/swarm/01-build-verify.md` — portable build/test green baseline  
- `docs/swarm/08-crypto-vectors.md` — hash KATs (portable crypto only)

**Status:** mapping written; ready for Architect/Reviewer read. Upstream code changes not started in this task.
