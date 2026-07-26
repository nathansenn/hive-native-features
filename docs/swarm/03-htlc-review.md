# 03 – HTLC edge-case review

**Task-ID:** phase-2 / swarm review  
**Scope:** `/tmp/hive-native-features` only  
**Date:** 2026-07-25  
**Sources:** `include/hive_native/protocol/htlc_operations.hpp`, `src/chain/evaluators_htlc.cpp`, `src/protocol/validate.cpp`, `include/hive_native/util/types.hpp`, `src/chain/database.cpp`, `docs/02-htlc-design.md`, `docs/decisions/ADR-0001-phase-0-human-gates.md`, `tests/test_runner.cpp`  
**Test run:** `./build/hive_native_tests` → `passed=115 failed=0` (EXIT 0)  
**Critical bugs found:** none (no code changes)

---

## Verdict matrix

| Edge case | Design rule | Result | Evidence |
|-----------|-------------|--------|----------|
| Redeem requires `to` only | ADR-0001 / design §5.2, §8 | **PASS** | Protocol auth + evaluator match |
| Redeem fails at/after expiration | `head_time >= expiration` → no redeem | **PASS** | Evaluator + unit test |
| Refund only after expiration | open + `head_time >= expiration` | **PASS** | Evaluator + unit test |
| Wrong preimage | hash mismatch fail | **PASS** | constant-time compare + test |
| Wrong preimage size | exact equality to `preimage_size` | **PASS** | apply gate (logic; no dedicated test) |
| SHA-256 path | digest 32 B, redeem via sha256 | **PASS** | crypto + create/redeem tests |
| RIPEMD-160 path | digest 20 B, redeem via ripemd160 | **PASS** | create length check + test |
| Open HTLC cap | per-`from` open limit | **PASS** | cap enforced (logic; no dedicated test) |
| Balance debit/credit | debit `from` on create; credit `to`/`from` on close | **PASS** | adjust_balance + balance asserts |

**Overall: PASS** — portable HTLC evaluators match design edge rules and ADR-0001. No critical bug fix required.

---

## 1. Redeem requires `to` only — **PASS**

**Design / ADR-0001**

- Redeem authority = **`to` active authority only** (MVP). Open-redeem deferred.

**Protocol** (`htlc_operations.hpp`)

```cpp
// htlc_redeem_operation
account_name_type to; // must match HTLC.to and sign (ADR-0001)
std::vector<account_name_type> get_required_active_authorities() const { return {to}; }
```

**Evaluator** (`evaluators_htlc.cpp`)

```cpp
if(h.to != op.to) throw protocol_error("redeem requires to authority"); // ADR-0001
```

**Test** (`test_htlc_edge_cases`)

- Redeem with `r.to = "carol"` (not HTLC `to`) → `CHECK_THROW` — **PASS** at runtime (suite green).

**Notes**

- Funds always credit stored `h.to` / `h.amount`, never an attacker-chosen destination.
- Wrong-`to` fails before preimage check, so third parties cannot complete redeem even with the secret.

---

## 2. Redeem fails at/after expiration — **PASS**

**Design §5.2 / §8**

- Redeem only when `head_time < expiration`.
- At exact expiry: **fail redeem**; refund allowed.

**Evaluator**

```cpp
if(db.head_time >= h.expiration) throw protocol_error("htlc expired");
```

Boundary:

| Condition | Redeem |
|-----------|--------|
| `head_time < expiration` | allowed (if other checks pass) |
| `head_time == expiration` | **fail** |
| `head_time > expiration` | **fail** |

**Test**

```cpp
db.head_time = c.expiration;  // exact expiry
r.preimage = preimage;
CHECK_THROW(apply(db, r));    // redeem fails
```

**PASS** — strict inequality matches design table.

---

## 3. Refund only after expiration — **PASS**

**Design §5.3 / §8**

- Refund validates: open; `head_time ≥ expiration`.
- Portable choice (design §18): **`from` only** (stricter than optional anyone-can-refund).

**Evaluator**

```cpp
if(h.from != op.from) throw protocol_error("refund requires from authority");
if(db.head_time < h.expiration) throw protocol_error("htlc not yet expired");
```

**Protocol auth**

```cpp
get_required_active_authorities() const { return {from}; }
```

**Tests**

1. Refund before expiry → throw — **PASS**.
2. At `head_time = expiration` → refund succeeds, status `refunded` — **PASS**.

Boundary consistent with redeem:

| Condition | Redeem | Refund |
|-----------|--------|--------|
| `head_time < expiration` | ok | **fail** |
| `head_time >= expiration` | **fail** | ok |

No overlap window where both succeed.

---

## 4. Wrong preimage / wrong size — **PASS**

### Wrong preimage (correct length)

**Evaluator**

```cpp
auto dig = digest_of(h.preimage_hash.algo, op.preimage);
if(!constant_time_equal(dig.bytes, h.preimage_hash.bytes))
   throw protocol_error("preimage hash mismatch");
```

- Uses object’s stored algo (sha256 or ripemd160).
- Digest compare is constant-time (`types.hpp`) per design §7.

**Test**

```cpp
r.preimage = {9,9,9,9};  // same length, wrong secret
CHECK_THROW(apply(db, r));
```

**PASS**.

### Wrong size (exact equality policy)

**Design §5.2:** fixed size equality for determinism (`preimage.size == preimage_size`).

**Create validate** (`validate.cpp`)

```cpp
if(preimage_size == 0 || preimage_size > MAX_HTLC_PREIMAGE_LEN)
   throw protocol_error("invalid preimage_size");
```

**Redeem validate**

```cpp
if(preimage.empty() || preimage.size() > MAX_HTLC_PREIMAGE_LEN)
   throw protocol_error("invalid preimage");
```

**Apply**

```cpp
if(op.preimage.size() != h.preimage_size)
   throw protocol_error("preimage size mismatch");
```

**PASS by inspection.** Suite does not include a dedicated wrong-size `CHECK_THROW` (gap, not a logic bug). Empty preimage rejected at validate; size ≠ stored size rejected before hash.

---

## 5. SHA-256 and RIPEMD-160 paths — **PASS**

### Digest length at create

```cpp
const size_t expect = (preimage_hash.algo == hash_algo::sha256) ? 32 : 20;
if(preimage_hash.bytes.size() != expect)
   throw protocol_error("preimage_hash length mismatch");
```

### Hash helpers (`types.hpp` / `crypto.cpp`)

- `digest_of(hash_algo::sha256, …)` → 32-byte SHA-256.
- `digest_of(hash_algo::ripemd160, …)` → 20-byte RIPEMD-160.
- Compact portable implementations (no OpenSSL).
- SHA-256 FIPS vector `"abc"` checked in `test_crypto`.

### Tests

| Path | Test | Result |
|------|------|--------|
| SHA-256 happy redeem | `test_htlc_redeem` | **PASS** |
| SHA-256 edges | `test_htlc_edge_cases` | **PASS** |
| RIPEMD-160 redeem | `test_htlc_ripemd` | **PASS** |

Redeem always hashes with `h.preimage_hash.algo`, so create-time algo selection is binding for the whole lifecycle.

---

## 6. Open HTLC cap — **PASS**

**Constants** (`types.hpp`)

```cpp
inline constexpr uint32_t HTLC_MAX_OPEN_PER_ACCOUNT = 256;
```

(Design §6 proposed `100?` optional; code chose **256** — within design intent of a per-account open cap.)

**Count** (`database.cpp`)

```cpp
// counts htlcs where h.from == from && h.status == open
```

**Create gate**

```cpp
if(db.open_htlc_count(op.from) >= HTLC_MAX_OPEN_PER_ACCOUNT)
   throw protocol_error("too many open htlcs");
```

- Cap is on **open** status only (redeemed/refunded do not consume quota).
- Bench (`bench_ops.cpp`) stays under 256 by batch create → expire → refund.

**PASS by inspection.** No unit test that creates 256 then expects throw on 257th (coverage gap, not a functional fail).

**Non-blocking note:** `open_htlc_count` scans the full `htlcs` map (O(n)). Design perf budget prefers indexed counts; acceptable for portable in-memory stand-in, track for upstream chainbase index.

---

## 7. Balance debit/credit correctness — **PASS**

**Model (design §3):** amount held in HTLC object; liquid balance reduced on create.

| Op | Balance effect | Code |
|----|----------------|------|
| create | debit `from` by `op.amount` | `adjust_balance(op.from, asset{-op.amount.amount, op.amount.symbol})` |
| redeem | credit `to` by `h.amount` | `adjust_balance(h.to, h.amount)` |
| refund | credit `from` by `h.amount` | `adjust_balance(h.from, h.amount)` |

**Safety in `adjust_balance`**

- Missing account → throw.
- Debit larger than balance → `insufficient balance`.
- Symbol-correct field (HIVE vs HBD).

**Test evidence**

1. **Redeem path** (`test_htlc_redeem`, HIVE 5000):
   - After create: alice `1_000_000 - 5000`.
   - After redeem: bob `1_000_000 + 5000`.
   - Conservation: funds leave alice on create, arrive bob on redeem; not double-held in liquid balances.
2. **Refund path** (`test_htlc_edge_cases`, HBD 100):
   - After expire+refund: alice HBD back to `1_000_000`.
3. Closed HTLC cannot re-credit: status → `redeemed`/`refunded`; further redeem/refund hits `htlc not open`.

**PASS.** No path credits both parties; no path credits without a matching prior debit into the HTLC object.

---

## 8. Related gates (supporting)

| Gate | Behavior | Status |
|------|----------|--------|
| Double redeem / double refund | `status != open` → fail | **PASS** (logic; double-redeem not explicit in suite) |
| Unknown `htlc_id` | find miss → fail | **PASS** |
| Refund wrong `from` | `h.from != op.from` → fail | **PASS** (logic) |
| Zero / non-positive amount | create validate `is_positive()` | **PASS** |
| Symbol MVP | HIVE/HBD only at create | **PASS** |
| Duration bounds | `[now+60s, now+30d]` | **PASS** (validate; not all suite-covered) |
| Memo bound | `MAX_MEMO_LEN` 2048 | **PASS** |
| Hardfork | `require_hardfork(HIVE_HARDFORK_HTLC)` | **PASS** |
| Light skip | consensus cannot skip HTLC state | **PASS** (mirror of NFT path) |
| RC charged | create/redeem/refund set `last_rc_charged` | **PASS** (present; not HTLC-asserted in suite) |

---

## 9. Code change log

| Item | Action |
|------|--------|
| Critical bugs | **None found** |
| Code patches | **None** |
| Tests re-run after fix | N/A — suite already green |

```
$ cd /tmp/hive-native-features && ./build/hive_native_tests
passed=115 failed=0
```

---

## 10. Residual gaps (non-critical)

These do **not** fail the design edge matrix; recommended follow-ups for Phase 2 hardening:

1. Add explicit tests: wrong preimage length; open-cap at 256; double redeem; duration min/max reject; insufficient balance on create.
2. Upstream: replace O(n) `open_htlc_count` with `by_from` open index / counter.
3. Design optional “anyone-can-refund” remains deferred (`from`-only as documented §18).

---

## Sign-off

HTLC portable implementation **conforms** to `docs/02-htlc-design.md` security edge cases and **ADR-0001 to-only redeem**. All requested edge checks: **PASS**.
