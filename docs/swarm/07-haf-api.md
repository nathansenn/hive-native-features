# 07 – HAF / database_api mapping

**Task-ID:** phase-1 / 1.8–1.9 · phase-2 HAF · phase-3 HAF  
**Status:** proposal + portable stubs  
**Sources:** `haf/sql/*.sql`, `include/hive_native/api/database_api_stubs.hpp`,  
`docs/01-nft-design.md` §11, `docs/02-htlc-design.md` §13, `docs/03-contracts-design.md` §13  

This document maps **upstream-facing `database_api` methods** to **HAF SQL**
tables/views and the portable C++ stubs used for verification in this repo.

---

## Conventions

| Rule | Value |
|------|--------|
| Pagination | `start` = exclusive lower bound on primary id; `limit` clamped to **[1, 100]** |
| Light mode | `light=true` (default for contract **lists**) omits heavy fields |
| Skip state | If node config skip flag set, methods return null / empty without scan |
| Schema | All tables under PostgreSQL schema `hive` |
| IDs | `BIGINT` / `uint64_t` dense ids |

Portable helpers: `hive_native::api::list_args`, `clamp_limit()`.

---

## NFT methods

| API method | C++ stub | SQL (primary) | Notes |
|------------|----------|---------------|-------|
| `database_api.get_nft(nft_id, light?)` | `get_nft(db, id, light)` | `hive.nfts` or `hive.nfts_light` | light ⇒ omit `uri` |
| `database_api.get_nft_collection(id)` | `get_nft_collection(db, id)` | `hive.nft_collections` | |
| `database_api.get_nft_collection_by_symbol(symbol)` | `get_nft_collection_by_symbol` | `hive.nft_collections` WHERE `symbol` | unique index |
| `database_api.list_nfts_by_owner(owner, start, limit, light?)` | `list_nfts_by_owner` | `hive.nfts` / `nfts_light` WHERE `owner`, `id >= start` ORDER BY `id` | index `nfts_by_owner` |
| `database_api.list_nfts_by_collection(collection, start, limit, light?)` | `list_nfts_by_collection` | `hive.nfts` WHERE `collection_id` | index `nfts_by_collection` |

**History / indexers (not wallet primary path):**

| Consumer query | SQL |
|----------------|-----|
| NFT activity feed | `hive.nft_ops` by `nft_id` / `block_num` |
| Collection events | `hive.nft_ops` WHERE `collection_id` |
| Operator approvals | `hive.nft_operators` PK `(owner, operator_account, collection_id)` |

**Example SQL — list by owner (light):**

```sql
SELECT id, collection_id, token_serial, owner, approved,
       metadata_hash, soulbound, minted_at
FROM hive.nfts_light
WHERE owner = $1 AND id >= $2
ORDER BY id
LIMIT $3;  -- clamp to 100
```

---

## HTLC methods

| API method | C++ stub | SQL (primary) | Notes |
|------------|----------|---------------|-------|
| `database_api.get_htlc(id, light?)` | `get_htlc(db, id, light)` | `hive.htlcs` / `htlcs_light` | light ⇒ omit `memo` |
| `database_api.list_htlcs_by_from(from, start, limit, light?)` | `list_htlcs_by_from` | `hive.htlcs` WHERE `from_account` | |
| `database_api.list_htlcs_by_to(to, start, limit, light?)` | `list_htlcs_by_to` | `hive.htlcs` WHERE `to_account` | |
| `database_api.list_htlcs_by_expiration(min_exp, start, limit, light?)` | `list_htlcs_by_expiration` | `hive.htlcs_open` WHERE `expiration >= min_exp` | open only |

**History / indexers:**

| Consumer query | SQL |
|----------------|-----|
| Closed HTLC audit | `hive.htlc_history` by `htlc_id` |
| Open wallet locks | `hive.htlcs_open` by `from_account` |

**Light-mode policy:** history tables store **hashes only** — never preimage bytes.
Preimage appears only in the signed `htlc_redeem_operation` body on the chain op
stream, not in `htlc_history`.

**Example SQL — open by to:**

```sql
SELECT id, from_account, to_account, amount, symbol,
       preimage_hash, hash_algo, preimage_size,
       expiration, created_at, status
FROM hive.htlcs_open
WHERE to_account = $1 AND id >= $2
ORDER BY id
LIMIT $3;
```

---

## Contract methods

| API method | C++ stub | SQL (primary) | Notes |
|------------|----------|---------------|-------|
| `database_api.get_contract(id, light?)` | `get_contract(db, id, light=true)` | `hive.contracts` / `contracts_light` | light clears in-memory `code`; SQL has hash only |
| `database_api.list_contracts_by_owner(owner, start, limit, light?)` | `list_contracts_by_owner` | `hive.contracts` WHERE `owner` | **always** strip code in list |
| `database_api.get_storage_key(contract_id, key)` | `get_storage_key` | plugin store; optional latest from `contract_storage_updates` | not a full HAF k/v dump |

**History / metering:**

| Consumer query | SQL |
|----------------|-----|
| Fuel / success analytics | `hive.contract_calls` / `contract_calls_light` |
| Storage audit trail | `hive.contract_storage_updates` by `(contract_id, storage_key)` |

**Example SQL — get contract (light):**

```sql
SELECT id, owner, code_hash, created_at, code_available
FROM hive.contracts_light
WHERE id = $1;
```

**Example — latest storage value (archival hosts only):**

```sql
SELECT value, value_hash, block_num
FROM hive.contract_storage_updates
WHERE contract_id = $1 AND storage_key = $2
ORDER BY id DESC
LIMIT 1;
```

Live reads should prefer the plugin storage provider; HAF updates are event-sourced.

---

## Skip-state behavior (API nodes)

| Config flag | Affected methods | Behavior |
|-------------|------------------|----------|
| `nft_skip_state` | all NFT get/list | `nullopt` / empty vector |
| `htlc_skip_state` | all HTLC get/list | `nullopt` / empty vector |
| `contracts_skip` | contract get/list/storage | `nullopt` / empty vector |

Witnesses and consensus nodes **must not** enable skip flags for features they
apply (see architecture light-node rules).

---

## Field masks (light vs full)

| Object | Full fields | Light omits |
|--------|-------------|-------------|
| NFT | id, collection, serial, owner, approved, metadata_hash, **uri**, soulbound, minted | `uri` |
| HTLC | id, from, to, amount, hash, algo, size, expiration, created, **memo**, status | `memo` |
| Contract | id, owner, code_hash, created, **code[]** | `code` (lists always) |

Matches views: `nfts_light`, `htlcs_light`, `contracts_light`.

---

## Virtual ops → history tables

| Virtual op (conceptual) | History table | `op_type` |
|-------------------------|---------------|-----------|
| `nft_collection_created` | `nft_ops` | `collection_created` |
| `nft_minted` | `nft_ops` | `minted` |
| `nft_transferred` | `nft_ops` | `transferred` |
| `nft_approved` / operator set | `nft_ops` | `approved` / `operator_set` |
| `nft_burned` | `nft_ops` | `burned` |
| `htlc_created` | `htlc_history` | `created` |
| `htlc_redeemed` | `htlc_history` | `redeemed` |
| `htlc_refunded` | `htlc_history` | `refunded` |
| `contract_deployed` | (updates `contracts`) | — |
| `contract_called` | `contract_calls` | success/fuel columns |

Ingest is the responsibility of HAF app / sql_serializer bindings (out of scope
for portable C++ stubs).

---

## Files

| Path | Role |
|------|------|
| `haf/sql/001_nft_tables.sql` | NFT DDL + light views |
| `haf/sql/002_htlc_tables.sql` | HTLC DDL + open/light views |
| `haf/sql/003_contracts_tables.sql` | Contracts DDL + light views |
| `haf/README.md` | Tables, light views, retention narrative |
| `include/hive_native/api/database_api_stubs.hpp` | Method signatures |
| `src/api/database_api_stubs.cpp` | Portable in-memory implementations |
| `CMakeLists.txt` | Builds `database_api_stubs.cpp` into `hive_native` |

---

## Checklist

- [x] Schema proposed per feature  
- [x] Retention documented (`haf/README.md`)  
- [x] API signatures proposed and implemented as stubs  
- [x] Light-mode field omission defined  
- [x] Method ↔ SQL mapping (this file)  
