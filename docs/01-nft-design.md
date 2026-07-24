# 01 – Native NFT Design

**Task-ID:** phase-0 / 0.3  
**Status:** design  
**Last Updated:** 2026-07-24  
**Depends on:** `00-architecture-overview.md`  

---

## 1. Summary

Introduce **native non-fungible tokens** as first-class Hive protocol objects with mint, transfer, burn, and approve (operator) operations. Design prioritizes compact state, RC metering, light-node skip paths, and migration notes from Hive-Engine-style NFTs.

---

## 2. Goals and non-goals

### Goals

- Deterministic ownership of unique assets under account authority.
- ERC-721-like mental model (owner, approve, operator) adapted to Hive authority.
- Efficient indexes for `by_id`, `by_owner`, `by_collection`.
- HAF-friendly virtual ops for indexers and marketplaces.

### Non-goals (Phase 1)

- On-chain media storage
- Full royalty enforcement engine (may be metadata convention only)
- Cross-chain bridging
- Automatic Hive-Engine import

---

## 3. Object model

### 3.1 `nft_collection_object` (optional but recommended)

| Field | Type (conceptual) | Notes |
|-------|-------------------|-------|
| `id` | collection_id_type | Dense id |
| `creator` | account_name_type | Mint authority default |
| `symbol` | string (bounded) | e.g. max 16 chars |
| `name` | string (bounded) | max 32–64 chars |
| `max_supply` | uint64 | 0 = unlimited (discouraged; RC still applies per mint) |
| `supply` | uint64 | Current minted minus burned (define precisely) |
| `transferable` | bool | If false, soulbound-style |
| `created` | time_point_sec | |

**Size estimate:** ~128–256 bytes + string tails (enforce max lengths).

### 3.2 `nft_object`

| Field | Type | Notes |
|-------|------|-------|
| `id` | nft_id_type | Global unique |
| `collection` | collection_id_type | |
| `token_id` | uint64 | Per-collection serial (optional if global id sufficient) |
| `owner` | account_name_type | |
| `approved` | account_name_type | Single approved operator for this token; empty = none |
| `metadata_hash` | ripemd160 / sha256 | Content hash only |
| `uri` | string (bounded) | Optional short URI; prefer off-chain via hash |
| `minted` | time_point_sec | |
| `soulbound` | bool | Overrides collection if set at mint |

**Size estimate:** ~160–320 bytes depending on URI cap. **URI max length:** recommend ≤ 256 bytes or omit URI and use hash-only.

### 3.3 Operator approvals (collection-wide)

Option A (Phase 1 simpler): only per-token `approved`.  
Option B: `nft_operator_object` (owner, operator, collection, approved bool).

**Recommendation:** Phase 1 ships per-token approve + optional `nft_set_approval_for_all_operation` if indexes stay small; otherwise defer for-all to Phase 1.x.

---

## 4. Indexes

| Index | Key | Use |
|-------|-----|-----|
| `by_id` | nft_id | Primary |
| `by_owner` | (owner, nft_id) | Wallet listing |
| `by_collection` | (collection, token_id) | Collection enumeration |
| `by_approved` | (approved, nft_id) | Optional; marketplace |

Collection object: `by_id`, `by_symbol` (unique symbol recommended).

**Justification:** Wallet and marketplace queries must avoid full scans. Pruned nodes may drop `by_approved` if unused.

---

## 5. Operations

### 5.1 `nft_create_collection_operation`

- **Auth:** active of `creator`
- **Validates:** symbol uniqueness, string bounds, max_supply rules
- **Effects:** create collection object; virtual `nft_collection_created`
- **RC:** base + bytes (TODO – measure)

### 5.2 `nft_mint_operation`

- **Auth:** active of collection creator (or designated minter authority — v1: creator only)
- **Fields:** collection, to, metadata_hash, optional uri, optional soulbound
- **Validates:** supply < max_supply (if set); account exists
- **Effects:** create nft_object; increment supply; virtual `nft_minted`
- **RC:** higher than transfer (state creation)

### 5.3 `nft_transfer_operation`

- **Auth:** active of owner **or** active of `approved` operator
- **Fields:** from, to, nft_id, optional memo (bounded)
- **Validates:** from is owner; not soulbound; to exists
- **Effects:** owner = to; clear approved; virtual `nft_transferred`
- **RC:** similar to transfer + memo bytes

### 5.4 `nft_approve_operation`

- **Auth:** active of owner
- **Fields:** nft_id, approved (empty to clear)
- **Effects:** set approved; virtual `nft_approved`

### 5.5 `nft_burn_operation`

- **Auth:** active of owner
- **Effects:** remove object; decrement supply if policy says so; virtual `nft_burned`
- **Prune:** immediate object removal (undo-session safe)

### 5.6 Authority rules summary

| Op | Required authority |
|----|--------------------|
| create_collection | creator active |
| mint | creator active (v1) |
| transfer | owner active **or** approved active |
| approve | owner active |
| burn | owner active |

Posting authority is **not** sufficient (prevents social-key abuse).

---

## 6. Virtual operations / events

Emit for HAF and marketplaces:

- `nft_collection_created_operation`
- `nft_minted_operation`
- `nft_transferred_operation`
- `nft_approved_operation`
- `nft_burned_operation`

Each includes ids, accounts, and hashes needed for reconstruction without full object dump.

---

## 7. RC cost model (placeholders)

| Operation | Cost components | Placeholder |
|-----------|-----------------|-------------|
| create_collection | base + state | TODO – measure (TASK 1.4) |
| mint | base + state create | TODO – measure |
| transfer | base + memo | Align near `transfer_operation` |
| approve | base | Low |
| burn | base + free state credit? | Hive pattern for release if any |

**DoS notes:** mint is the expensive path; RC must make mass minting costly. Consider per-block mint soft limits only if RC insufficient (prefer RC-only first).

---

## 8. Hardfork guard

```text
if( !has_hardfork( HIVE_HARDFORK_1_XX_NFT ) ) // XX TBD human gate
   fail( "nft operations not enabled" );
```

Serialization: new ops only appear in HF-enabled protocol packs.

---

## 9. Light-node / pruned behavior

| Node | Behavior |
|------|----------|
| Full | All objects + indexes |
| Pruned | Current `nft_object` + collections; drop burned; optional drop uri strings if hash kept |
| Light / mobile | **Skip** chainbase NFT indexes; may validate op structure + signatures only; query ownership via API |
| API light mode | `get_nfts` omits large fields; pagination required |

**Skip path (explicit):** If node config `nft-skip-state=true`, evaluators still enforce protocol validation that does not require local object store **only when** the node is non-consensus (API light). **Witnesses and block producers must run full NFT state.**

Clarify for implementers:

- **Consensus nodes:** always full NFT apply.
- **Non-validating light clients:** skip storage; trust linked API or future proofs.

---

## 10. Pruning policy

- Burned NFTs: remove at burn (subject to undo for reversible blocks).
- Collections with supply 0: retain (symbol reservation) unless explicit `close_collection` (future).
- URIs: optional strip on pruned nodes after irreversible if hash retained.
- No unbounded history in chainbase; history lives in HAF.

---

## 11. HAF / database_api (proposals)

### Tables (sketch)

- `hive.nft_collections` — current projection  
- `hive.nfts` — current projection  
- `hive.nft_ops` — from virtual ops stream  

Retention: current state always; ops per HAF retention policy.

### API methods (stubs)

- `database_api.get_nft(nft_id)`
- `database_api.list_nfts_by_owner(owner, start, limit)`
- `database_api.list_nfts_by_collection(collection, start, limit)`
- `database_api.get_nft_collection(symbol|id)`

Light-mode: max `limit` ≤ 100; no full metadata payloads.

---

## 12. Parallel apply notes

- Transfers of **different** nft_ids: independent if accounts differ carefully — account RC and auth still serialize per account in Hive’s model.
- Annotate: NFT object mutation locks `nft_id`; avoid collection supply races by mint serialization on collection id.
- Mint increments collection supply → **dependent** on collection object; single-writer semantics for that collection.

---

## 13. Migration from Hive-Engine

| Path | Description | Risk |
|------|-------------|------|
| A. Fresh mint | Projects remint natively | Simple; social process |
| B. Attested bridge | Multisig/oracle mints after Engine burn proof | Trust assumption |
| C. Parallel | Engine remains; native optional | Fragmentation |

**Recommendation:** Document A + B; do not hardcode Engine into consensus.

---

## 14. Security considerations

- Approve is powerful; wallets must surface it clearly.
- Soulbound transfers must fail closed.
- Memo field: size-capped; no executable content.
- Symbol squatting: RC + optional creator reputation later; not in v1.

---

## 15. Test plan pointers

See `05-verification-and-testing-strategy.md`. Minimum Phase 1:

- Unit: auth matrix, soulbound, approve clear on transfer, max supply
- Apply: undo/revert across reversible blocks
- RC stubs
- Light-skip config smoke test

---

## 16. Implementation task map (Phase 1)

| Task | Deliverable |
|------|-------------|
| 1.1 | Protocol types + ops |
| 1.2 | Objects + indexes |
| 1.3 | Evaluators |
| 1.4 | RC cost functions |
| 1.5 | Virtual ops |
| 1.6 | Unit tests |
| 1.7 | Light-node skip path |
| 1.8 | HAF table proposals |
| 1.9 | database_api stubs |
| 1.10 | Microbenchmark skeleton |
| 1.11 | Migration notes (this section expanded) |
| 1.12 | Integration test stubs |

---

## 17. Open questions (human)

1. Per-token approve only vs approval-for-all in MVP?
2. Max URI length / hash algorithm (RIPEMD-160 vs SHA-256)?
3. Collection symbol namespace global uniqueness?
4. HF number and activation timing?

---

## 18. Acceptance

- [x] Object model, ops, RC, pruning, light-node, migration notes
- [ ] Reviewer approval
- [ ] Architect sign-off for Phase 1 start  
