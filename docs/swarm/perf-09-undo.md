# perf-09 — Selective undo prototype (catalogue #43)

**Task-ID:** catalogue #43 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Priority:** P0 · category: storage  

---

## Goal

Prototype **field-level undo** for a simple `account_balance` so the reversible window records only changed fields (`hive`, `hbd` + account key), not whole chainbase objects.

Catalogue title: *Selective undo: track only changed fields not whole objects*.

## Motivation (hived / chainbase)

Upstream Graphene-style undo sessions typically snapshot entire multi_index objects on first `modify`. For large objects that is necessary; for hot liquid balances the useful delta is two `int64` fields. Selective field tracking:

- Shrinks undo-session memory on deep reversible windows  
- Cuts memcpy traffic on apply / `pop_block`  
- Aligns with follow-ons: #61 (batch balance undo), #287 (field-level pop_block), #44 (compress undo)

## Portable API

Header: [`include/hive_native/perf/selective_undo.hpp`](../../include/hive_native/perf/selective_undo.hpp)

| Type / API | Role |
|------------|------|
| `account_balance` | `{ hive, hbd }` stand-in |
| `balance_map` | `unordered_map<string, account_balance>` |
| `balance_change` | selective record: account + old_hive + old_hbd |
| `undo_session::push_balance_change(account, old_hive, old_hbd)` | push pre-mutation fields |
| `undo_session::rollback()` | LIFO restore into bound map |
| `undo_session::commit()` | drop stack, keep mutations |
| `adjust_balance(session, map, account, hive_delta, hbd_delta)` | push then mutate helper |

**Not** whole-object clone: stack entries never store indexes, RC, vesting, or other account fields.

## Tests

File: [`tests/test_selective_undo.cpp`](../../tests/test_selective_undo.cpp)  
Target: `hive_native_selective_undo_tests`

| Case | Asserts |
|------|---------|
| Transfer under session + `rollback()` | both accounts restore hive/hbd |
| HBD-only mutation | hive unchanged; hbd restored |
| Multiple pushes LIFO | sequential field writes reverse correctly |
| `commit()` | mutations retained; stack empty |
| Explicit `push_balance_change` | API-only path restores |

## Build & verify

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_selective_undo_tests
./build/hive_native_selective_undo_tests
ctest --test-dir build -R selective_undo --output-on-failure
```

Actual:

```
selective_undo_passed=23 failed=0
```

`ctest -R selective_undo`: **Passed**. Exit **0**.

## Upstream sketch

1. On first touch of `account_object` balance fields in an undo session, record `{id, old_balance, old_hbd_balance}` (or bitset of dirty fields).  
2. Prefer field records for hot paths (transfer, fill_order, claim); keep full-object undo for rare/large mutations.  
3. Measure reversible-window RSS and `pop_block` latency vs baseline.  
4. Gate behind a compile/runtime flag until HF/integration review.

## Files

| Path | Action |
|------|--------|
| `include/hive_native/perf/selective_undo.hpp` | added |
| `tests/test_selective_undo.cpp` | added |
| `CMakeLists.txt` | `hive_native_selective_undo_tests` + ctest |
| `docs/swarm/perf-09-undo.md` | this report |

## Result

**PASS** — portable selective undo for `account_balance` with adjust → rollback restore covered by dedicated tests.
