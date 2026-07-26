# 14 – HTLC Negative / Edge Tests

**Task-ID:** phase-2 / HTLC test matrix  
**Scope:** Additional portable unit tests in `tests/test_runner.cpp` covering failure paths for Hash Time Locked Contracts.  
**Workspace:** `/tmp/hive-native-features`

## 1. Purpose

Extend the Phase 2 HTLC suite beyond happy-path redeem, basic edge cases (wrong redeemer, bad preimage, refund-before-expiry), and RIPEMD-160 interop. The new cases assert that:

1. Closed HTLCs cannot be spent twice (redeem or refund).
2. Create duration bounds (`HTLC_MIN_DURATION_SEC` / `HTLC_MAX_DURATION_SEC`) are enforced at validation time.
3. Create fails cleanly on insufficient liquid balance (no partial HTLC object).
4. Redeem enforces the exact `preimage_size` locked at create (size mismatch before hash check).

## 2. Implementation map

| Test function | Behavior under test | Expected outcome |
|---------------|---------------------|------------------|
| `test_htlc_double_redeem_fails` | Redeem open HTLC, then redeem again | Second `apply` throws; status stays `redeemed`; `to` not double-credited |
| `test_htlc_double_refund_fails` | Expire + refund, then refund again | Second `apply` throws; status stays `refunded`; `from` not double-credited |
| `test_htlc_expiration_too_soon_fails` | `expiration < head_time + HTLC_MIN_DURATION_SEC` | Create throws; no HTLC; balances unchanged |
| `test_htlc_expiration_too_far_fails` | `expiration > head_time + HTLC_MAX_DURATION_SEC` | Create throws; no HTLC; balances unchanged |
| `test_htlc_insufficient_balance_fails` | Create amount = balance + 1 | Create throws (`insufficient balance`); no HTLC |
| `test_htlc_wrong_preimage_size_fails` | Redeem with shorter/longer preimage than create size | Size mismatches throw; wrong-bytes same size throws on hash; correct redeem succeeds |

Constants (from `include/hive_native/util/types.hpp`):

- `HTLC_MIN_DURATION_SEC` = 60
- `HTLC_MAX_DURATION_SEC` = 30 days
- `MAX_HTLC_PREIMAGE_LEN` = 1024

Evaluator / validation sources:

- `src/protocol/validate.cpp` — duration and create field checks
- `src/chain/evaluators_htlc.cpp` — open-status, size match, hash match, authority
- `src/chain/database.cpp` — `adjust_balance` → `insufficient balance`

## 3. How to run

```bash
cd /tmp/hive-native-features/build
cmake --build . --target hive_native_tests -j
./hive_native_tests
# or: ctest --output-on-failure
```

Success line shape:

```
passed=<N> failed=0
```

## 4. Coverage notes

| Area | Covered here | Covered elsewhere |
|------|--------------|-------------------|
| Happy redeem (SHA-256) | — | `test_htlc_redeem` |
| Wrong redeemer / bad hash / early refund / post-expiry redeem | — | `test_htlc_edge_cases` |
| RIPEMD-160 redeem | — | `test_htlc_ripemd` |
| Double redeem / double refund | ✓ | — |
| Duration min / max | ✓ | — |
| Insufficient funds | ✓ | — |
| preimage_size mismatch | ✓ | — |
| Max open HTLCs per account | not in this pass | evaluator + bench path |
| Skip-state / hardfork gates | not in this pass | config paths / light-node design |

## 5. Design references

- [docs/02-htlc-design.md](../02-htlc-design.md) — object model, create/redeem/refund rules, edge matrix
- [docs/05-verification-and-testing-strategy.md](../05-verification-and-testing-strategy.md) — security + edge testing gates
- ADR-0001 — redeem authority is `to` only

## 6. Result (this task)

```
$ cmake --build build --target hive_native_tests && ./build/hive_native_tests
passed=165 failed=0
```

**Pass count: 165** (0 failures).

