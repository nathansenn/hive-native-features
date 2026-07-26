# 07 – Upstream Integration Map

**Task-ID:** phase-1 / upstream-portability  
**Status:** design + portable code ready; no live Hive MR yet  
**Last Updated:** 2026-07-25  
**Audience:** Protocol / State / Evaluator / HAF coders, Reviewer, GitOps  
**Upstream targets:** `gitlab.syncad.com/hive/hive`, mirrors such as `openhive-network/hive`, HAF / `sql_serializer` stack  

---

## 1. Purpose

This repository ships **portable** C++ under `hive_native::` that mirrors Hive naming and layering **without** depending on FC, chainbase, appbase, or a full node build.

This document maps every portable unit to the **real upstream path** it must become, and lists the concrete conversion steps from `hive_native` types to FC / chainbase objects.

**Repo strategy (unchanged from architecture):** prepare small, reviewable patches — not a full fork dump.

---

## 2. Layer map (portable → upstream)

| Layer | Portable location (`hive-native-features`) | Upstream path (Hive / HAF) | What moves |
|-------|--------------------------------------------|----------------------------|------------|
| Types & caps | `include/hive_native/util/types.hpp` | `libraries/protocol/include/hive/protocol/` (`types.hpp`, `config.hpp`, asset helpers) | `account_name_type`, `asset`, digests, size caps, HF placeholders |
| Operations | `include/hive_native/protocol/*_operations.hpp` | `libraries/protocol/include/hive/protocol/` (+ `operations.hpp` variant lists) | Op structs, virtual ops, `validate()`, authority helpers |
| Validation | `src/protocol/validate.cpp` | `libraries/protocol/*.cpp` (or op `validate()` impls next to ops) | Bounds, symbol rules, HTLC duration |
| Crypto | `src/util/crypto.cpp` | Prefer **existing** `fc::sha256` / RIPEMD in protocol; only add if missing | HTLC preimage hashes must match node crypto |
| State objects | `include/hive_native/chain/database.hpp` (structs) | `libraries/chain/include/hive/chain/` object headers | `*_object`, indexes, object ids |
| Database / apply host | `src/chain/database.cpp` | `libraries/chain/database.cpp` (+ undo, indices registration) | Id allocation, balance adjust, HF gate helpers |
| Evaluators | `src/chain/evaluators_*.cpp` | `libraries/chain/*_evaluator.cpp` + evaluator registration | validate → RC → mutate → virtual ops |
| RC costs | `include/hive_native/rc/costs.hpp`, `src/rc/costs.cpp` | RC plugin resource user / cost tables (see §5) | Per-op cost curves, fuel→RC |
| database_api | `include/hive_native/api/database_api_stubs.hpp` | `libraries/plugins/database_api/` (or `plugins/database_api/`) | Query methods, light-mode field omit |
| HAF SQL | `haf/sql/001_*.sql` … `003_*.sql` | HAF migrations + `sql_serializer` op/table wiring | Tables, views, retention |
| Contracts engine | `include/hive_native/contracts/`, `plugins/hive_contracts/` | Optional plugin first: `libraries/plugins/hive_contracts/` | Wasmtime host; consensus only after HF 3h |
| Hardfork guards | `HIVE_HARDFORK_*` in `types.hpp` + `require_hardfork` | `libraries/protocol/.../hardfork*.hpp` + `database::has_hardfork` | Real HF numbers (human gate) |

Conceptual architecture layer table: `docs/00-architecture-overview.md` §4.

---

## 3. `libraries/protocol`

### 3.1 Portable sources

| File | Contents |
|------|----------|
| `include/hive_native/util/types.hpp` | Caps, `asset`, hash digests, account name checks, **placeholder HF numbers** (`9001`/`9002`/`9003`) |
| `include/hive_native/protocol/nft_operations.hpp` | NFT ops + virtual ops |
| `include/hive_native/protocol/htlc_operations.hpp` | HTLC create/redeem/refund + virtual |
| `include/hive_native/protocol/contract_operations.hpp` | deploy/call + virtual (Phase 3h consensus) |
| `src/protocol/validate.cpp` | `validate()` bodies |

### 3.2 Upstream destinations (typical Hive layout)

```
libraries/protocol/include/hive/protocol/
  types.hpp / config.hpp          ← caps, ids, HF macros
  nft_operations.hpp              ← NEW (or hive_native_operations.hpp split)
  htlc_operations.hpp             ← NEW
  contract_operations.hpp         ← NEW (later HF)
  operations.hpp                  ← add to operation / virtual_operation static_variant
  hardfork.hpp / hardfork_ops     ← HF enum / schedule entries
libraries/protocol/
  nft_operations.cpp              ← validate + any helpers
  ...
```

Exact filenames may follow Hive style at MR time (e.g. one header per feature vs. aggregated). Prefer **one feature per reviewable MR**.

### 3.3 Protocol integration checklist

1. Replace `hive_native::account_name_type` (`std::string`) with Hive’s `account_name_type` (fixed buffer / FC type).
2. Replace `hive_native::asset` / `asset_symbol` with `hive::protocol::asset` and NAI / symbol type used on mainnet.
3. Replace `sha256_t` / `ripemd160_t` / `hash_digest` with `fc::sha256`, `fc::ripemd160` (or Hive wrappers).
4. Add each **user** op to the main `operation` `fc::static_variant` (order is consensus-critical once shipped).
5. Add each **virtual** op to `virtual_operation` variant.
6. Add `FC_REFLECT(...)` for every struct (field order = serialization order).
7. Implement `validate()` using Hive exception macros (`FC_ASSERT` / protocol exceptions), not `protocol_error` alone.
8. Wire `get_required_active_authorities` into Hive’s authority visitor patterns (portable already returns vectors).
9. Ensure ops are **not** accepted in binary protocol packs before HF (pack version / hardfork-gated deserialization as Hive does for other HFs).
10. Update client bindings (dhive / hive-tx / wax) **after** op tags stabilize — out of band from core MR.

### 3.4 Operation → variant mapping (planned)

| Portable op | Variant kind | Authority (summary) |
|-------------|--------------|---------------------|
| `nft_create_collection_operation` | user | creator active |
| `nft_mint_operation` | user | creator active |
| `nft_transfer_operation` | user | from active (apply also allows approved/operator) |
| `nft_approve_operation` | user | owner active |
| `nft_set_approval_for_all_operation` | user | owner active |
| `nft_burn_operation` | user | owner active |
| `htlc_create_operation` | user | from active |
| `htlc_redeem_operation` | user | to active only (ADR-0001) |
| `htlc_refund_operation` | user | from active |
| `contract_deploy_operation` | user (plugin then HF) | owner active |
| `contract_call_operation` | user (plugin then HF) | caller active |
| `nft_*` / `htlc_*` / `contract_*` virtuals | virtual | N/A (pushed by evaluators) |

---

## 4. `libraries/chain`

### 4.1 Portable sources

| File | Contents |
|------|----------|
| `include/hive_native/chain/database.hpp` | In-memory objects + `database` stand-in |
| `src/chain/database.cpp` | balances, HF gate, light-state flags |
| `include/hive_native/chain/evaluators.hpp` | `apply(db, op)` declarations |
| `src/chain/evaluators_nft.cpp` | NFT apply path |
| `src/chain/evaluators_htlc.cpp` | HTLC apply path |
| `src/chain/evaluators_contracts.cpp` | Contract apply path (plugin/HF) |

### 4.2 Upstream destinations

```
libraries/chain/include/hive/chain/
  nft_objects.hpp           ← nft_collection_object, nft_object, nft_operator_object
  htlc_objects.hpp          ← htlc_object
  contract_objects.hpp      ← contract_object (+ storage provider interface)
  *_evaluator.hpp
libraries/chain/
  nft_evaluator.cpp
  htlc_evaluator.cpp
  contract_evaluator.cpp    ← later / plugin
  database.cpp              ← index registration, apply dispatch hooks
  # index types via chainbase multi_index
```

### 4.3 Object conversion (portable → chainbase)

See §8 for the full type-translation recipe. Summary of indexes required by design docs:

| Object | Indexes (upstream) | Portable stand-in |
|--------|--------------------|-------------------|
| `nft_collection_object` | `by_id`, `by_symbol` (unique) | `collections`, `collection_by_symbol` maps |
| `nft_object` | `by_id`, `by_owner`, `by_collection` | `nfts` map |
| `nft_operator_object` | composite `(owner, operator, collection)` | `operators` tuple map |
| `htlc_object` | `by_id`, `by_from`, `by_to`, `by_expiration` | `htlcs` map |
| `contract_object` | `by_id`, `by_owner` | `contracts` map |
| contract k/v storage | isolated provider / RocksDB | `contract_storage` nested map |

### 4.4 Evaluator pattern (upstream)

Portable already follows:

```text
require_hardfork → require_full_*_state → op.validate()
  → charge RC → mutate objects → push_virtual
```

Upstream mapping:

1. `evaluator<T>::do_apply(const T&)` (Hive evaluator base).
2. `db.has_hardfork(HIVE_HARDFORK_…)` instead of `require_hardfork`.
3. `db.create<nft_object>([&](auto& o){ ... })` / `db.modify` / `db.remove` instead of `std::map` assignment.
4. Authority: use `get_required_*_authorities` + apply-time checks for approve/operator (transfer special case).
5. Virtual ops: `push_virtual_operation(...)`.
6. Balances (HTLC): `adjust_balance` / existing liquid transfer helpers — **never** invent a second balance ledger.
7. Parallel apply: keep annotations from evaluator source comments; register dependencies (e.g. mint serializes on collection id).

### 4.5 Light / pruned node flags

Portable `node_config`:

| Flag | Upstream intent |
|------|-----------------|
| `nft_skip_state` | Non-consensus light/API: skip NFT indexes; witnesses **must** full apply |
| `htlc_skip_state` | Same for HTLC |
| `contracts_skip` | Plugin off or light skip |
| `is_consensus_node` | Witness / block producer always full for enabled HF features |

Wire to Hive node config options (CLI / config.ini) with equivalent names, documented in operator notes at MR time.

---

## 5. RC plugin

### 5.1 Portable sources

- `include/hive_native/rc/costs.hpp`
- `src/rc/costs.cpp`

Relative micro-units (`TRANSFER_BASE = 1000`) exist for tests/benchmarks only.

### 5.2 Upstream destinations

Typical Hive RC integration (names may vary slightly by version):

```
libraries/plugins/rc/          # or equivalent rc plugin tree
  include/.../resource_count.hpp / resource_user.hpp
  # count bytes / state / new objects per op
  # map operation → resource usage
```

### 5.3 Integration steps

1. Register each new op in the RC **resource user** / counter (execution, history, market, state bytes — match current Hive categories).
2. Translate portable relative costs into Hive’s absolute RC units via calibration against `transfer_operation` (see `docs/04-performance-budgets.md`).
3. Contracts: map `fuel_used` with `FUEL_TO_RC_NUM/DEN` into RC; charge **caller** / `fee_payer()`.
4. Keep `TODO – measure` until microbench on real node; do not invent final mainnet curves in the portable lib.
5. Ensure failed validation does not permanently consume RC beyond Hive’s existing rules.

---

## 6. `database_api`

### 6.1 Portable sources

- `include/hive_native/api/database_api_stubs.hpp`
- `src/api/database_api_stubs.cpp`

### 6.2 Upstream destinations

```
libraries/plugins/database_api/include/hive/plugins/database_api/
  database_api.hpp / database_api_args.hpp / database_api_objects.hpp
libraries/plugins/database_api/
  database_api.cpp
```

(If the tree uses `plugins/database_api` without `libraries/`, follow the live repo.)

### 6.3 Method map

| Portable | Upstream method (proposed) | Notes |
|----------|----------------------------|-------|
| `get_nft` | `database_api.get_nft` | Optional; missing → empty |
| `list_nfts_by_owner` | `database_api.list_nfts_by_owner` | `start` + `limit` ≤ 100 |
| `list_nfts_by_collection` | `database_api.list_nfts_by_collection` | Same pagination |
| `get_htlc` | `database_api.get_htlc` | Open + recent resolved per node policy |
| `get_contract` | `database_api.get_contract` | Code hash only in light mode |

**Light mode:** omit `uri` / large blobs; use views analogous to `hive.nfts_light` / `hive.contracts_light` in HAF SQL.

API args/results need `FC_REFLECT` and JSON-friendly field names consistent with existing database_api style.

---

## 7. HAF / `sql_serializer`

### 7.1 Portable sources

| SQL file | Feature |
|----------|---------|
| `haf/sql/001_nft_tables.sql` | collections, nfts, operators, light view |
| `haf/sql/002_htlc_tables.sql` | htlcs, open view |
| `haf/sql/003_contracts_tables.sql` | contracts, contract_calls, light view |

### 7.2 Upstream destinations

```
# HAF project / hive_fork_manager style migrations (exact repo may be separate)
haf/ ... migrations ...

# Node-side serializer plugin
libraries/plugins/sql_serializer/   # or HAF-connected sql_serializer
  # op → SQL sinks for virtual ops and state projections
```

### 7.3 Integration steps

1. Land **migration SQL** in HAF’s versioned schema path (Psql, irreversible-aware).
2. In `sql_serializer` (or HAF app that consumes virtual ops): handle each virtual op name (`nft_minted`, `htlc_created`, …).
3. Maintain **current-state** tables (`hive.nfts`, `hive.htlcs`, …) via apply of virtual ops or state dump hooks — match existing HAF patterns for accounts/tokens.
4. Retention: ops stream follows HAF irreversible policy; current-state rows updated in place; closed HTLCs pruned per design.
5. Do not put multi-MB WASM blobs in SQL current-state; store `code_hash` only (see contracts design).

---

## 8. Hardfork guards

### 8.1 Portable placeholders

From `include/hive_native/util/types.hpp`:

| Macro | Placeholder | Feature |
|-------|-------------|---------|
| `HIVE_HARDFORK_NFT` | `9001` | Phase 1 NFT |
| `HIVE_HARDFORK_HTLC` | `9002` | Phase 2 HTLC |
| `HIVE_HARDFORK_CONTRACTS` | `9003` | Phase 3h consensus contracts only |

**Human gate:** real mainnet / testnet numbers are **TBD**. Never ship `900x` to production.

### 8.2 Portable enforcement

- `database::require_hardfork(hf)` in every evaluator (`evaluators_nft.cpp`, `evaluators_htlc.cpp`, `evaluators_contracts.cpp`).
- Tests set `db.config.hardfork = HIVE_HARDFORK_CONTRACTS` to enable all.

### 8.3 Upstream enforcement

1. Assign official HF enum values in Hive hardfork headers / schedule.
2. Gate **deserialization / apply** with `db.has_hardfork(HIVE_HARDFORK_…)`.
3. Pre-HF blocks: ops must not appear; nodes without HF must reject or ignore per Hive’s existing unknown-op policy for that version.
4. Separate activation if NFT and HTLC ship in different hardforks (portable already uses distinct macros).
5. Contracts: plugin-only until 3h; consensus ops stay behind `HIVE_HARDFORK_CONTRACTS`.

```text
// Portable
db.require_hardfork(HIVE_HARDFORK_NFT);

// Upstream equivalent
FC_ASSERT( db.has_hardfork( HIVE_HARDFORK_1_XX_NFT ), "nft operations not enabled" );
```

---

## 9. Turning `hive_native` types into real FC / chainbase objects

This is the **mechanical port recipe**. Apply in order for each feature (NFT first).

### 9.1 Namespaces and includes

| Portable | Upstream |
|----------|----------|
| `namespace hive_native::protocol` | `namespace hive::protocol` |
| `namespace hive_native::chain` | `namespace hive::chain` |
| `#include "hive_native/..."` | `#include <hive/protocol/...>`, `<hive/chain/...>` |

### 9.2 Primitive type substitution

| `hive_native` | Upstream FC / Hive |
|---------------|--------------------|
| `std::string` account | `account_name_type` |
| `uint32_t time_point_sec` | `time_point_sec` (FC) |
| `int64_t share_type` | `share_type` |
| `asset` + `asset_symbol` | `asset` + symbol/NAI |
| `std::array<uint8_t,32>` / `sha256_t` | `fc::sha256` |
| `std::array<uint8_t,20>` / `ripemd160_t` | `fc::ripemd160` |
| `std::vector<uint8_t>` blobs | `std::vector<char>` or FC blob type per Hive convention |
| `protocol_error` | `fc::exception` / `FC_ASSERT` messages |
| `uint64_t` dense ids | `object_id` / `oid<Tag>` or Hive id typedefs |

### 9.3 Operation structs

1. Copy field list **in the same order** (serialization stability).
2. Add `FC_REFLECT(hive::protocol::nft_mint_operation, (creator)(collection)(to)(metadata_hash)(uri)(soulbound))` (example).
3. Move `validate()` to protocol library; replace throws with Hive asserts.
4. Implement authority methods to match `hive::protocol` visitor expectations.
5. Append to `operation` / `virtual_operation` `static_variant` **at the end** of the list for that HF (coordinate with other MRs to avoid tag collisions).

### 9.4 Chain objects (chainbase)

Portable POD structs become:

```cpp
// Conceptual upstream shape — follow live Hive object macros exactly at MR time
class nft_object : public object< nft_object_type, nft_object >
{
   // object id, fields matching portable nft_object
};
// multi_index_container with ordered_unique / ordered_non_unique indexes
// CHAINBASE_SET_INDEX_TYPE / FC_REFLECT for snapshot & undo
```

Steps:

1. Allocate `object_type` enum entry (or equivalent type id).
2. Define object class with `id_type`, fields sized like design docs.
3. Define `multi_index` with indexes from design (`by_id`, `by_owner`, …).
4. Register index in `database` constructor / init.
5. Ensure **undo session** compatibility (all mutations via `create`/`modify`/`remove`).
6. Snapshot / mirror dump: confirm new indexes included.
7. Pruning: implement burn remove; HTLC closed-object GC after N irreversible blocks.

### 9.5 Evaluators

| Portable | Upstream |
|----------|----------|
| free function `apply(database&, const Op&)` | `class nft_transfer_evaluator : public evaluator<nft_transfer_evaluator>` |
| `db.nfts[id] = n` | `db.create<nft_object>(...)` |
| `db.last_rc_charged = rc::cost_...` | RC plugin hooks / `db` resource count |
| `db.push_virtual("name", json)` | `db.push_virtual_operation(nft_minted_operation{...})` typed |
| `db.adjust_balance` | existing chain balance APIs |

Register evaluators in the global evaluator map alongside `transfer_evaluator`, etc.

### 9.6 Crypto

| Portable | Upstream |
|----------|----------|
| `hive_native::sha256` | `fc::sha256::hash` |
| `hive_native::ripemd160` | `fc::ripemd160::hash` |
| `constant_time_equal` | keep constant-time compare for preimages |

Portable crypto is for **unit tests** and standalone benches. Upstream **must** use the same primitives as the rest of consensus.

### 9.7 API / plugin objects

1. Define API result structs with FC reflection.
2. Implement methods on `database_api_impl` reading chain indexes (or rocksd/plugin store for contract code).
3. Register JSON-RPC method names in plugin startup.
4. Enforce `limit` clamp (portable `clamp_limit` ≤ 100).

### 9.8 Build system

| Portable | Upstream |
|----------|----------|
| Root `CMakeLists.txt` → `libhive_native.a` | Hive monorepo CMake: add sources to `hive_protocol`, `hive_chain`, plugins |
| `HIVE_NATIVE_WITH_WASMTIME` | Plugin option; off by default on consensus until 3h |

Do **not** submodule this entire repo into consensus blindly; **copy/adapt** reviewed units.

### 9.9 Verification after port

1. Replay golden vectors from `tests/test_runner.cpp` logic as Hive chain tests (`libraries/chain/tests` or node test harness).
2. Microbench NFT transfer vs transfer under real chainbase (budgets in `docs/04-performance-budgets.md`).
3. HF off → ops rejected; HF on → apply matches portable semantics.
4. sql_serializer + HAF migration on a testnet dump.
5. Reviewer security pass: authority, HTLC time/hash edge cases, RC DoS.

---

## 10. Suggested MR sequence (upstream)

| Order | MR focus | Depends on |
|-------|----------|------------|
| 1 | Protocol NFT ops + reflections + HF stub (testnet) | Human HF number |
| 2 | Chain objects + indexes + evaluators + RC counters | MR 1 |
| 3 | database_api NFT methods | MR 2 |
| 4 | sql_serializer + HAF SQL NFT | MR 1–2 (virtual op names stable) |
| 5 | HTLC protocol + chain + RC + API + HAF | NFT patterns stable |
| 6 | Contracts **plugin** (non-consensus) | ADR-0002 Wasmtime |
| 7 | Contracts consensus HF (3h) | Human gate only |

---

## 11. Out of scope for direct core dump

- Full EVM
- Automatic Hive-Engine migration
- Unmetered WASM
- Replacing `custom_json` side engines in one HF
- Committing Wasmtime into consensus binary before Phase 3h

---

## 12. Related documents

| Doc | Role |
|-----|------|
| `docs/00-architecture-overview.md` | Constraints, layering |
| `docs/01-nft-design.md` … `03-contracts-design.md` | Feature semantics |
| `docs/04-performance-budgets.md` | Apply latency / RC gates |
| `docs/05-verification-and-testing-strategy.md` | Test pyramid |
| `docs/swarm/10-upstream.md` | Swarm handoff checklist for this map |
| `WORKFLOW.md` | Roles: Protocol / State / Evaluator / HAF coders |

---

## 13. Acceptance for this document

- [x] Maps portable code → `libraries/protocol`, `libraries/chain`, RC plugin, `database_api`, HAF `sql_serializer`, hardfork guards
- [x] Lists FC/chainbase conversion steps for `hive_native` types
- [x] MR sequence and light-node flags noted
- [ ] Human confirmation of HF numbers before any mainnet-bound MR
- [ ] Reviewer sign-off when first upstream draft branch is opened
