# HAF SQL proposals — native NFTs, HTLC, contracts

**Task-IDs:** phase-1 / 1.8 · phase-2 / HAF · phase-3 / HAF  
**Audience:** HAF / API / SDK coders, indexers, upstream reviewers  

This directory holds **SQL schema proposals** for Hive Application Framework (HAF) /
`sql_serializer` tables that mirror native feature state and virtual ops. They are
portable sketches — not an automatic migration for production HAF installs.

## Layout

| File | Feature | Primary tables |
|------|---------|----------------|
| [`sql/001_nft_tables.sql`](./sql/001_nft_tables.sql) | Native NFTs | `nft_collections`, `nfts`, `nft_operators`, `nft_ops` |
| [`sql/002_htlc_tables.sql`](./sql/002_htlc_tables.sql) | HTLC | `htlcs`, `htlc_history` |
| [`sql/003_contracts_tables.sql`](./sql/003_contracts_tables.sql) | Contracts | `contracts`, `contract_calls`, `contract_storage_updates` |

Apply in numeric order on a PostgreSQL instance that already hosts HAF (schema
`hive` is created `IF NOT EXISTS` for standalone dry-runs).

API method ↔ SQL mapping: [`docs/swarm/07-haf-api.md`](../docs/swarm/07-haf-api.md).  
Portable C++ stubs: `include/hive_native/api/database_api_stubs.hpp`.

---

## Design principles

1. **Current-state vs history**  
   - **Current-state** tables project chainbase objects for wallet/API queries.  
   - **Ops / history** tables are fed from virtual operations for indexers and
     explorers; they follow HAF irreversible retention.

2. **No multi-MB blobs in HAF current-state**  
   - NFT: `metadata_hash` on-chain; `uri` optional and bounded (≤ 256).  
   - Contracts: `code_hash` only in `hive.contracts`; WASM code lives in plugin /
     RocksDB when available (`code_available` flag).  
   - HTLC: never store preimage in history tables (hash only).

3. **Light-mode first**  
   Views omit heavy fields. `database_api` list methods clamp `limit` ≤ 100 and
   clear uri / memo / code when `light=true` (lists of contracts always strip code).

4. **Skip-state nodes**  
   Light/mobile nodes with `nft_skip_state` / `htlc_skip_state` / `contracts_skip`
   do not require these tables locally; they query a full API/HAF node.

---

## Tables

### NFT (`001`)

| Table | Kind | Description |
|-------|------|-------------|
| `hive.nft_collections` | current | Collection registry (`symbol` unique) |
| `hive.nfts` | current | Token ownership, approval, hash, optional uri |
| `hive.nft_operators` | current | Approval-for-all; `collection_id = 0` ⇒ all collections |
| `hive.nft_ops` | history | Virtual-op stream (mint/transfer/burn/approve/…) |

**Indexes:** `(owner, id)`, `(collection_id, token_serial)`, partial `(approved, id)`,
ops by block / nft / recipient.

### HTLC (`002`)

| Table | Kind | Description |
|-------|------|-------------|
| `hive.htlcs` | current / open | Open projection preferred; status 0/1/2 |
| `hive.htlc_history` | history | created / redeemed / refunded events |

**Indexes:** by from, to, expiration; partial open-by-from; history by htlc/block.

**Status enum:** `0 open`, `1 redeemed`, `2 refunded`.

**Symbol enum:** `0 HIVE`, `1 HBD`.  
**Hash algo:** `0 sha256` (32-byte digest), `1 ripemd160` (20-byte digest).

### Contracts (`003`)

| Table | Kind | Description |
|-------|------|-------------|
| `hive.contracts` | current | id, owner, code_hash, created_at |
| `hive.contract_calls` | history | fuel used, success, export, args_hash |
| `hive.contract_storage_updates` | history | event-sourced key writes/deletes |

Storage is **not** a full k/v dump of live state in HAF by default; live k/v is
the plugin storage provider. HAF keeps update events for audit/replay.

---

## Light views

| View | Source | Omits / filters |
|------|--------|-----------------|
| `hive.nfts_light` | `nfts` | `uri` (keeps `metadata_hash`) |
| `hive.nft_collections_light` | `nft_collections` | (full row; already compact) |
| `hive.htlcs_open` | `htlcs` | only `status = 0` |
| `hive.htlcs_light` | `htlcs` | `memo` |
| `hive.contracts_light` | `contracts` | already hash-only; API symmetry |
| `hive.contract_calls_light` | `contract_calls` | same as table (raw args never stored) |

Upstream `database_api` should prefer light views (or equivalent field masks)
for wallet and mobile clients.

---

## Retention policy

| Data | Policy |
|------|--------|
| NFT collections | Always retain (symbol reservation even at supply 0) |
| Live NFTs | Always retain while unburned |
| Burned NFTs | Remove from `hive.nfts` after irreversible; keep `nft_ops` |
| NFT operators | Retain while `approved = true`; delete row on revoke |
| Open HTLCs | Always retain on full/API nodes |
| Closed HTLCs | Delete from `hive.htlcs` after irreversible; keep `htlc_history` |
| Contract registry | Retain while contract exists |
| Contract calls / storage updates | HAF irreversible retention (configurable prune of old blocks) |
| Preimages | **Never** in HAF history tables |

History tables (`*_ops`, `htlc_history`, `contract_calls`, `contract_storage_updates`)
inherit the same irreversible-block window as other HAF app-data tables unless an
archival profile keeps them longer.

---

## Consistency with chainbase (portable model)

Field names track `include/hive_native/chain/database.hpp`:

| C++ object | SQL table |
|------------|-----------|
| `nft_collection_object` | `hive.nft_collections` |
| `nft_object` | `hive.nfts` |
| `nft_operator_object` | `hive.nft_operators` |
| `htlc_object` | `hive.htlcs` |
| `contract_object` | `hive.contracts` |
| `contract_storage` map | plugin store + `contract_storage_updates` events |

Timestamps in SQL are `TIMESTAMPTZ`; the portable C++ model uses `time_point_sec`
(unix seconds). HAF serializers convert at ingest.

---

## Checklist (WORKFLOW §6.5)

- [x] Table schema proposed  
- [x] Pruning / retention policy documented  
- [x] `database_api` method signatures in C++ stubs  
- [x] Light-mode behavior defined (views + stubs)  

---

## Out of scope

- Automatic Hive-Engine NFT import  
- Storing WASM code or NFT media in PostgreSQL  
- Consensus-forcing SQL (HAF is indexer/API layer only)
