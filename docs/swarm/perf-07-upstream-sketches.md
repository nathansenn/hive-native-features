# Swarm — perf-07: Upstream patch sketches (#1 #2 #3)

**Task-ID:** catalogue / perf-07  
**Status:** COMPLETE (sketches only; hive tree **not** mutated)  
**Date:** 2026-07-27  
**Read root:** `/Users/commander/hive-sources/hive`  
**Write root:** `/tmp/hive-native-features`  
**Catalogue source:** `docs/performance/HIVE_1000_OPTIMIZATIONS.*` IDs 1–3 (P0 storage)  
**Build evidence:** `/Users/commander/hive-sources/hive/HIVED_BUILD_ANALYSIS.md` (2026-01-06)

---

## Deliverables

| Sketch | Catalogue | Doc |
|--------|-----------|-----|
| #1 generic_index ETI | Explicit template instantiation of top `chainbase::generic_index<>` | [docs/upstream-patches/001-generic-index-explicit-instantiation.md](../upstream-patches/001-generic-index-explicit-instantiation.md) |
| #2 types.hpp split | Split `types.hpp` → fwd/basic/ops to cut header tax | [docs/upstream-patches/002-types-hpp-split.md](../upstream-patches/002-types-hpp-split.md) |
| #3 static_variant extern | Extern-template for `operation` static_variant serialization | [docs/upstream-patches/003-static-variant-extern.md](../upstream-patches/003-static-variant-extern.md) |

---

## Scan results (clone)

### `index*.cpp`

| Path | Notes |
|------|-------|
| `/Users/commander/hive-sources/hive/libraries/chain/index.cpp` | **Only** chain `index*.cpp`; orchestrates `initialize_core_indexes_01`…`_13` |
| *(no `index-0N.cpp`)* | Historical names from build analysis; **renamed** to `database_{account,comment,witness,…}.cpp` |

### `types.hpp`

| Path | Role |
|------|------|
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types.hpp` | Protocol types umbrella (**86.4 s** header tax) |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types_fwd.hpp` | Existing forward header (under-used) |
| `/Users/commander/hive-sources/hive/libraries/protocol/types.cpp` | Key type implementations |
| `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/external_storage/types.hpp` | Unrelated external-storage types |

### Operation `static_variant`

| Path | Role |
|------|------|
| `.../protocol/include/hive/protocol/operations.hpp` | `typedef fc::static_variant<…> operation` (~93 alts w/ vops) |
| `.../protocol/operations.cpp` | `HIVE_DEFINE_OPERATION_TYPE(operation)` |
| `.../protocol/include/hive/protocol/operation_util.hpp` | Auth + extended serialization functors |
| Nested variants | `hive_operations.hpp`, `base.hpp`, `types.hpp`, DHF, condenser legacy |

### Multi_index / generic_index (top types)

Already have **`get_index` / `get_mutable_index` ETI** in matching `database_*.cpp` (and plugins). **Missing:** deep `extern template class generic_index<…>` for MultiIndex method codegen.

Hot indexes (see sketch 001): `account_index`, `comment_index`, `comment_vote_index`, `comment_cashout_index`, `witness_index`, `witness_vote_index`, plus market/delegation/DHF/RC set registered via `HIVE_ADD_CORE_INDEX`.

---

## Expected leverage (from build analysis — not remeasured here)

| ID | Bottleneck | Ballpark savings if done well |
|----|------------|-------------------------------|
| #1 | `generic_index<>` 181 s / 123 inst | 50–70 s CPU |
| #2 | `types.hpp` 86.4 s / 67 includes | 40–50 s CPU |
| #3 | static_variant ser ~750 ms × ops × TUs | 100+ s CPU |

Runtime/consensus impact: **none** if patches stay compile-only (no variant reorder, no object layout change).

---

## Explicit non-actions

- No patches applied under `/Users/commander/hive-sources/hive`
- No CMake / source edits in hive
- No new `*.cpp` under hive — sketches only under `/tmp/hive-native-features/docs/`

---

## File list (this task)

```
/tmp/hive-native-features/docs/upstream-patches/001-generic-index-explicit-instantiation.md
/tmp/hive-native-features/docs/upstream-patches/002-types-hpp-split.md
/tmp/hive-native-features/docs/upstream-patches/003-static-variant-extern.md
/tmp/hive-native-features/docs/swarm/perf-07-upstream-sketches.md
```

---

## Suggested next steps (out of scope)

1. Upstream MR for #2 Phase 1 (umbrella-preserving types split) — lowest behavioral risk.
2. Parallel measure pass: `-ftime-trace` on current tree (index rename may shift TU names vs 2026-01 analysis).
3. #3 requires confirming `fc` pack/unpack are extern-template capable (submodule may need companion MR).
