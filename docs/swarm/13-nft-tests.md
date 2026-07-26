# 13 – NFT Negative / Edge Tests

**Task-ID:** phase-1 / NFT test matrix  
**Scope:** Additional portable unit tests in `tests/test_runner.cpp` covering burn supply accounting, approval-for-all scope/revoke, mint/symbol auth, and transfer self-validation.  
**Workspace:** `/tmp/hive-native-features`

## 1. Purpose

Extend the Phase 1 NFT suite beyond happy-path mint/approve/transfer, approval-for-all transfer, soulbound/auth rejection, max-supply, and light-node skip. The new cases assert that:

1. Burn deletes the NFT object and decrements collection `supply`.
2. `set_approval_for_all` is collection-scoped when `collection != 0`, and all-collections when `collection == 0`.
3. Revoking approval-for-all (`approved=false`) removes operator authority for transfers.
4. Only the collection creator may mint.
5. Collection symbols are unique (`symbol taken`).
6. Transfer to self fails at `validate()` (`cannot transfer to self`).

Existing tests were **not** removed.

## 2. Implementation map

| Test function | Behavior under test | Expected outcome |
|---------------|---------------------|------------------|
| `test_nft_burn_reduces_supply` | Mint then owner burns | `nfts` empty; `collections[id].supply == 0` |
| `test_nft_approval_collection_scope` | Operator approved for collection 1 only, then `collection=0` | Can transfer col-1 NFT; col-2 transfer throws until all-collections approval |
| `test_nft_revoke_approval_for_all` | Approve-for-all then `approved=false` | `is_operator_approved` false; operator transfer throws; owner unchanged |
| `test_nft_mint_non_creator_fails` | Mint with `creator != collection.creator` | Apply throws; no NFT; supply stays 0 |
| `test_nft_symbol_duplicate_fails` | Second create with same symbol | Apply throws; still one collection; symbol map unchanged |
| `test_nft_transfer_to_self_fails_validate` | `from == to` on transfer | `validate()` throws; `apply` throws; owner unchanged |

Evaluator / validation sources:

- `src/protocol/validate.cpp` — transfer self-check, symbol/name bounds
- `src/chain/evaluators_nft.cpp` — mint creator check, symbol uniqueness, burn supply, operator set/clear
- `src/chain/database.cpp` — `is_operator_approved` (collection-specific then `collection=0`)

## 3. How to run

```bash
cd /tmp/hive-native-features
cmake --build build --target hive_native_tests -j
./build/hive_native_tests
# or: cd build && ctest --output-on-failure
```

Success line shape:

```
passed=<N> failed=0
```

## 4. Coverage notes

| Area | Covered here | Covered elsewhere |
|------|--------------|-------------------|
| Mint → approve → transfer (per-token) | — | `test_nft_happy_path` |
| Approval-for-all happy transfer | — | `test_nft_approval_for_all` |
| Soulbound / unauthorized transfer | — | `test_nft_soulbound_and_auth` |
| Max supply | — | `test_nft_max_supply` |
| Light-node / consensus skip | — | `test_nft_light_skip` |
| Burn reduces supply | ✓ | — |
| Collection-specific vs all (`collection=0`) | ✓ | — |
| Revoke approval-for-all | ✓ | — |
| Non-creator mint | ✓ | — |
| Duplicate symbol | ✓ | — |
| Transfer to self at validate | ✓ | — |
| Non-owner burn | not in this pass | evaluator path |
| Per-token approve clear on transfer | — | `test_nft_happy_path` |

## 5. Design references

- [docs/01-nft-design.md](../01-nft-design.md) — object model, approve / approval-for-all, burn supply definition
- [docs/05-verification-and-testing-strategy.md](../05-verification-and-testing-strategy.md) — edge and security testing gates
- ADR-0001 — MVP includes approval-for-all; `collection=0` means all collections

## 6. Result (this task)

```
$ cmake --build build --target hive_native_tests -j && ./build/hive_native_tests
passed=165 failed=0
```

**Pass count: 165** (0 failures).

### New test functions (added)

1. `test_nft_burn_reduces_supply`
2. `test_nft_approval_collection_scope`
3. `test_nft_revoke_approval_for_all`
4. `test_nft_mint_non_creator_fails`
5. `test_nft_symbol_duplicate_fails`
6. `test_nft_transfer_to_self_fails_validate`

### Files changed

- `tests/test_runner.cpp` — six NFT tests + `main()` wiring (existing tests retained)
- `docs/swarm/13-nft-tests.md` — this file
