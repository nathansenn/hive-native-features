# 02 – HTLC Atomic Swap Design

**Task-ID:** phase-0 / 0.4 → phase-2 portable code in-tree  
**Status:** accepted (ADR-0001 redeem = `to` only)  
**Last Updated:** 2026-07-26  
**Depends on:** `00-architecture-overview.md`, `docs/decisions/ADR-0001-phase-0-human-gates.md`

---

## 1. Summary

Hash Time Locked Contracts (HTLCs) enable **atomic swaps** and conditional payments on Hive:

1. **Lock** assets with a hash digest and a refund timeout.  
2. **Redeem** by revealing the preimage before timeout.  
3. **Refund** to the locker after timeout if unredeemed.

MVP focuses on **HIVE and HBD**. Optional extension: lock native NFTs once Phase 1 is stable.

---

## 2. Goals and non-goals

### Goals

- Trust-minimized conditional transfer without custodial escrow accounts (state machine on-chain).
- Clear time-lock and hash-lock edge-case rules.
- Prune completed HTLCs after irreversible finality window.
- RC-metered create to prevent lock spam.

### Non-goals (Phase 2 MVP)

- Cross-chain light-client bridges (use HTLC as building block only)
- Partial fills / multi-hop Lightning-style networks
- Complex scripting beyond hash + time

---

## 3. Object model: `htlc_object`

| Field | Type | Notes |
|-------|------|-------|
| `id` | htlc_id_type | |
| `from` | account_name_type | Funder / locker |
| `to` | account_name_type | Intended redeemer |
| `amount` | asset | HIVE or HBD in MVP |
| `preimage_hash` | hash_type | See hash algos |
| `hash_type` | enum | e.g. ripemd160, sha256 |
| `preimage_size` | uint16 | Max allowed preimage length for redeem |
| `expiration` | time_point_sec | Refund available at/after this head time |
| `created` | time_point_sec | |
| `memo` | string (bounded) | Optional |
| `status` | enum | open / redeemed / refunded (or delete on close) |

**Size estimate:** ~200–350 bytes + memo.

**Funds:** Amount is **held in the HTLC object** (not in `to` balance) until redeem/refund — similar to escrow/savings patterns; exact balance-book design must match Hive’s balance model (reduce liquid balance on create).

---

## 4. Indexes

| Index | Key | Use |
|-------|-----|-----|
| `by_id` | id | Primary |
| `by_from` | (from, id) | User open locks |
| `by_to` | (to, id) | Redeemer lookup |
| `by_expiration` | (expiration, id) | Timeout processing / explorers |

---

## 5. Operations

### 5.1 `htlc_create_operation`

- **Auth:** active of `from`
- **Fields:** to, amount, preimage_hash, hash_type, preimage_size, expiration, memo
- **Validates:**
  - amount > 0; symbol HIVE or HBD (MVP)
  - `to` exists; `to` ≠ `from`? (allow self for testing? **recommend allow** for tooling)
  - expiration ∈ [now + MIN_LOCK, now + MAX_LOCK]
  - preimage_size ∈ [1, MAX_PREIMAGE]
  - hash digest length matches hash_type
- **Effects:** debit `from`; create open HTLC; virtual `htlc_created`
- **RC:** base + state + amount not in RC (amount is economic)

### 5.2 `htlc_redeem_operation`

- **Auth:** active of `to` (MVP) — *alternative designs allow anyone with preimage; **security note below***
- **Fields:** htlc_id, preimage (bytes)
- **Validates:**
  - HTLC open
  - head_time < expiration (strict: redeem only before expiration)
  - preimage.size == preimage_size **or** ≤ preimage_size with fixed policy — **choose fixed size equality for determinism**
  - hash(preimage) == preimage_hash
- **Effects:** credit `to` with amount; remove/close HTLC; virtual `htlc_redeemed` (include preimage for chain analytics? **include hash only on-chain event, preimage in op body**)
- **RC:** base + preimage bytes

### 5.3 `htlc_refund_operation`

- **Auth:** active of `from` (or anyone after timeout — prefer `from` only to reduce spam ops, **or** allow anyone to refund to `from` for liveness)
- **Validates:** open; head_time ≥ expiration
- **Effects:** credit `from`; close HTLC; virtual `htlc_refunded`
- **Recommendation:** allow **any account** to trigger refund paying RC, funds always return to `from` (improves liveness).

### 5.4 Optional: maintenance

If Hive prefers automatic processing: block finalizer walks `by_expiration` for due HTLCs — **risky for apply time**. Prefer **user-triggered refund** only (explicit).

---

## 6. Time-lock parameters (proposed defaults)

| Param | Proposed | Notes |
|-------|----------|-------|
| `HTLC_MIN_DURATION` | 60 s | Avoid instant grief edge cases |
| `HTLC_MAX_DURATION` | 30 days | Cap state lifetime |
| `HTLC_MAX_PREIMAGE` | 1024 bytes | RC + DoS bound |
| `HTLC_MAX_OPEN_PER_ACCOUNT` | 100? | Optional; prefer RC-only first |

**Clock source:** `db.head_block_time()` — same as other Hive timed features. Document that block time, not wall clock, governs expiry.

---

## 7. Hash algorithms

| Type | Digest size | Notes |
|------|-------------|-------|
| `ripemd160` | 20 | Familiar from Hive addresses |
| `sha256` | 32 | Interop with many chains |
| (future) `sha1` | 20 | **Do not enable** — weak |
| (future) `hash160` | 20 | sha256+ripemd if needed for interop |

Redeem must use constant-time compare for digests (avoid trivial timing issues in shared hosts; consensus still deterministic).

---

## 8. Edge cases (security gate)

| Case | Rule |
|------|------|
| Redeem exactly at expiration | **Fail redeem**; refund allowed (`head_time >= expiration` → no redeem) |
| Wrong preimage length | Fail |
| Correct hash, wrong intended `to` | If auth is `to`-only, third party cannot redeem; if open-redeem, anyone can take funds to `to` only (funds still go to `to`) |
| Double redeem | Object gone / status closed → fail |
| Refund before expiry | Fail |
| Reorg / undo | Standard undo sessions restore balances + object |
| Zero amount | Fail at create |
| Memo too large | Fail; max e.g. 2048 |
| Preimage revealed on-chain | Expected; race to redeem is by `to` (MVP) |
| Hash collision | Cryptographic assumption; use sha256 for high value |

### Authority recommendation (MVP)

- **Redeem:** `to` active authority only (prevents mempool sniping of preimage by third parties redirecting nothing — funds always to `to`, but sniping could grief `to` by front-running if fee markets exist; on Hive, RC and ordering still matter).  
- **Open redeem (anyone):** simpler atomic swap interop with some protocols; funds still credit `to`. **Human choice.**

**DECIDED (ADR-0001):** MVP = **`to` must authorize redeem**. Open-redeem (anyone with preimage) is deferred to a future optional HF.

---

## 9. NFT extension (Phase 2.x, not MVP)

`htlc_create` may accept `amount` **or** `nft_id` (mutex).

- On create: transfer NFT into protocol custody (owner = reserved null account or clear owner flag `locked=true`).
- Redeem: NFT owner → `to`
- Refund: NFT owner → `from`
- Soulbound NFTs: cannot lock

Implement only after NFT burn/transfer paths are solid.

---

## 10. RC cost model

| Op | Components |
|----|------------|
| create | base + state + memo bytes + hash type constant |
| redeem | base + preimage bytes + hash compute |
| refund | base |

Placeholder: TODO – measure (Phase 2 RC task). Create must be expensive enough to deter millions of open dust HTLCs.

---

## 11. Pruning policy

| State | Policy |
|-------|--------|
| Open | Always on full nodes |
| Closed (redeemed/refunded) | Delete object at close; history via virtual ops + HAF |
| Irreversible window | Rely on undo; after irreversible, no revive |

Pruned nodes: keep open HTLCs only; drop closed immediately.

---

## 12. Light-node behavior

| Node | Behavior |
|------|----------|
| Consensus / full | Full apply |
| Pruned | Open HTLCs only |
| Light | **Skip** storage; validate op shape; do not claim local escrow truth without API |

Witnesses must not skip.

---

## 13. HAF / API proposals

### Tables

- `hive.htlcs` — open projection  
- `hive.htlc_history` — from virtual ops  

### API

- `get_htlc(id)`
- `list_htlcs_by_from` / `by_to` / `by_expiration`
- Light mode: no preimage fields in history if stripped; hashes only

---

## 14. Parallel apply

- Independent if different `htlc_id` and non-overlapping balance accounts.
- Create debits `from` → dependent with other balance ops on `from`.
- Redeem credits `to` → dependent on `to` balance ops.

Annotate evaluators accordingly.

---

## 15. Virtual operations

- `htlc_created_operation`
- `htlc_redeemed_operation` (htlc_id, from, to, amount, preimage_hash, preimage optional)
- `htlc_refunded_operation`

---

## 16. Test matrix (mandatory for Phase 2)

1. Happy redeem before expiry  
2. Refund after expiry  
3. Redeem at exact expiry → fail  
4. Refund before expiry → fail  
5. Bad preimage → fail  
6. Wrong length → fail  
7. Double redeem → fail  
8. Undo/reorg restores open HTLC + balances  
9. RC charged on create  
10. Max duration bounds  
11. SHA256 and RIPEMD160 paths  

---

## 17. Implementation task map (Phase 2)

Mirror Phase 1: protocol → state → evaluators → RC → virtual ops → tests → light path → HAF → API → benches → edge-case tests.

---

## 18. Open questions (human)

1. ~~Redeem authority~~ → **`to`-only** (ADR-0001)  
2. Anyone-can-refund for liveness? — portable code: **`from` only** for refund (stricter); may relax later  
3. MIN/MAX duration — code: **60s … 30d**  
4. NFT-lock in same HF as fungible HTLC? — **no**, fungible MVP only  

---

## 19. Acceptance

- [x] Objects, ops, timeouts, edge cases, prune, light-node  
- [x] Security design + edge tests in portable suite  
- [x] Architect sign-off (continue max)
