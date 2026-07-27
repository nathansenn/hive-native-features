# Upstream sketch #1 — Explicit instantiation of top `chainbase::generic_index<>` types

**Catalogue ID:** #1 (P0 / storage)  
**Status:** sketch only — **not applied** to hive tree  
**Clone root:** `/Users/commander/hive-sources/hive`  
**Evidence:** `HIVED_BUILD_ANALYSIS.md` (2026-01-06) — `chainbase::generic_index<>` **181.0 s** across **123** instances (~1471 ms avg)

---

## Problem

`chainbase::generic_index<MultiIndexType>` wraps every Boost.MultiIndex container used for chain state. Build analysis attributes **~181 s** of compile time to this template alone. Each TU that creates/modifies objects re-instantiates MultiIndex insert/erase/modify + undo stack machinery.

**Partial work already in tree:** every core/plugin index already has **explicit instantiation of** `database::get_index` / `get_mutable_index` only (thin wrappers in `chainbase.inl`). That is **not** full `generic_index` / MultiIndex ETI.

Example of current pattern:

```22:27:/Users/commander/hive-sources/hive/libraries/chain/database_account.cpp
// Explicit template instantiations for chainbase::database methods
template const chainbase::generic_index<hive::chain::dynamic_global_property_index>& chainbase::database::get_index<hive::chain::dynamic_global_property_index>() const;
template chainbase::generic_index<hive::chain::dynamic_global_property_index>& chainbase::database::get_mutable_index<hive::chain::dynamic_global_property_index>();

template const chainbase::generic_index<hive::chain::account_index>& chainbase::database::get_index<hive::chain::account_index>() const;
template chainbase::generic_index<hive::chain::account_index>& chainbase::database::get_mutable_index<hive::chain::account_index>();
```

Implementation lives in:

- `/Users/commander/hive-sources/hive/libraries/chainbase/include/chainbase/chainbase.inl`
- Class body: `/Users/commander/hive-sources/hive/libraries/chainbase/include/chainbase/chainbase.hpp` (`generic_index` ~L329+)

---

## Index registration surface (clone)

Orchestrator (only `index*.cpp` under chain):

| Path | Role |
|------|------|
| `/Users/commander/hive-sources/hive/libraries/chain/index.cpp` | `initialize_core_indexes()` → `_01`…`_13` (no `_10`) |
| `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/index.hpp` | `add_core_index` / `HIVE_ADD_CORE_INDEX` |
| `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/multi_index_types.hpp` | Boost.MultiIndex using-aliases |

Per-domain init + **current** `get_index` ETI (formerly measured as `index-0N.cpp` in build analysis; **renamed** in this clone):

| Init fn | File | Core indexes registered |
|---------|------|-------------------------|
| `_01` | `libraries/chain/database_account.cpp` | `dynamic_global_property_index`, `account_index` |
| `_02` | `libraries/chain/database_authority.cpp` | `account_authority_index`, `transaction_index` |
| `_03` | `libraries/chain/database_comment.cpp` | `block_summary_index`, `comment_index`, `comment_vote_index`, `comment_cashout_index`, `comment_cashout_ex_index` |
| `_04` | `libraries/chain/database_witness.cpp` | `witness_index`, `witness_vote_index`, `witness_schedule_index` |
| `_05` | `libraries/chain/database_conversion.cpp` | `feed_history_index`, `convert_request_index`, `collateralized_convert_request_index` |
| `_06` | `libraries/chain/database_vesting.cpp` | `liquidity_reward_balance_index`, `hardfork_property_index`, `withdraw_vesting_route_index` |
| `_07` | `libraries/chain/database_recovery.cpp` | `owner_authority_history_index`, `account_recovery_request_index`, `change_recovery_account_request_index` |
| `_08` | `libraries/chain/database_market.cpp` | `escrow_index`, `savings_withdraw_index`, `decline_voting_rights_request_index`, `limit_order_index` |
| `_09` | `libraries/chain/database_delegation.cpp` | `reward_fund_index`, `vesting_delegation_index`, `vesting_delegation_expiration_index` |
| `_11` | `libraries/chain/database_dhf.cpp` | `proposal_index`, `proposal_vote_index`, `recurrent_transfer_index` |
| `_12` | `libraries/chain/database_rc_core.cpp` | `rc_resource_param_index`, `rc_pool_index`, `rc_stats_index` |
| `_13` | `libraries/chain/database_rc_delegation.cpp` | `rc_expired_delegation_index`, `rc_direct_delegation_index`, `rc_usage_bucket_index` |

MultiIndex typedefs live under:

- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/detail/state/*_multiindex.hpp`
- RC: `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/rc/rc_objects.hpp`

---

## Top multi_index types (priority order for ETI)

Ranked by **secondary-key density** (ordered_unique count in multiindex header) × **hot-path use** in apply:

| Priority | Index type | Definition | Secondary keys (approx) | Why |
|----------|------------|------------|-------------------------|-----|
| P0 | `account_index` | `detail/state/account_object_multiindex.hpp` | 6 (`by_id`, `by_name`, `by_proxy`, `by_next_vesting_withdrawal`, `by_delayed_voting`, `by_governance_vote_expiration_ts`) | Hottest object; every transfer/vote/authority path |
| P0 | `comment_index` | `detail/state/comment_object_multiindex.hpp` | 2 (id + `by_permlink`) | Social apply; large cardinality |
| P0 | `comment_vote_index` | same | 3 | Vote evaluator |
| P0 | `comment_cashout_index` | same | 2 | Payout schedule |
| P0 | `witness_index` | `detail/state/witness_objects_multiindex.hpp` | 6 | Schedule / vote ranking |
| P0 | `witness_vote_index` | same | 3 | Proxy/vote churn |
| P1 | `transaction_index` | `detail/state/transaction_object_multiindex.hpp` | 3 | Dupe tx filter every block |
| P1 | `limit_order_index` | `detail/state/limit_order_object_multiindex.hpp` | 4 | Market ops |
| P1 | `vesting_delegation_index` / `_expiration_index` | account multiindex header | 2–3 | Delegation ops |
| P1 | `proposal_index` / `proposal_vote_index` | `detail/state/dhf_objects_multiindex.hpp` | multi | DHF |
| P1 | `recurrent_transfer_index` | `detail/state/recurrent_transfer_object_multiindex.hpp` | 4 | Periodic apply |
| P2 | Remaining singleton-ish indexes | DGP, HF props, feed history, block summary, RC pools, … | 1 | Cheap; lower ETI ROI |

Also already ETI’d for `get_index` only (plugins — keep consistent if deepening):

- `hive::plugins::reputation::reputation_index` — `libraries/plugins/reputation/reputation_plugin.cpp`
- `market_history::{bucket_index,order_history_index}`
- `account_by_key::key_lookup_index`
- `transaction_status::{transaction_status_index,transaction_status_block_index}`
- `block_log_info::*`
- `account_history_rocksdb::volatile_operation_index`
- `hive::chain::volatile_comment_index` (external storage path)

---

## Proposed `.cpp` explicit instantiation pattern

### Goal

Move **one full `generic_index<Index>` specialization** (and its MultiIndex method set) into a dedicated TU, declare `extern template` in a header included by consumers, so other TUs do not re-codegen MultiIndex.

### Step A — Keep / formalize existing thin ETI

Already done per `database_*.cpp`. Do **not** remove; it is the load-bearing link for `get_index` ODR after moving impl to `.inl`.

### Step B — Deep ETI (sketch)

**New files (proposed, not created in hive):**

```
libraries/chain/generic_index_eti/
  eti_account.cpp
  eti_comment.cpp
  eti_witness.cpp
  eti_market.cpp
  ...
libraries/chain/include/hive/chain/generic_index_eti.hpp
```

**Header sketch** (`generic_index_eti.hpp`):

```cpp
#pragma once
#include <chainbase/chainbase.hpp>
#include <hive/chain/detail/state/account_object_multiindex.hpp>
// ... other multiindex headers as needed

namespace chainbase {
  // Suppress implicit instantiation in consumer TUs:
  extern template class generic_index<hive::chain::account_index>;
  extern template class generic_index<hive::chain::comment_index>;
  extern template class generic_index<hive::chain::comment_vote_index>;
  extern template class generic_index<hive::chain::comment_cashout_index>;
  extern template class generic_index<hive::chain::witness_index>;
  extern template class generic_index<hive::chain::witness_vote_index>;
  // ... top set
}
```

**Implementation TU sketch** (`eti_account.cpp`):

```cpp
#include <hive/chain/detail/state/account_object_multiindex.hpp>
#include <chainbase/chainbase.hpp>
// Include any .inl that holds out-of-line generic_index methods if split later.

template class chainbase::generic_index<hive::chain::account_index>;

// Keep existing database accessor ETI co-located or in database_account.cpp:
#include <chainbase/chainbase.inl>
template const chainbase::generic_index<hive::chain::account_index>&
  chainbase::database::get_index<hive::chain::account_index>() const;
template chainbase::generic_index<hive::chain::account_index>&
  chainbase::database::get_mutable_index<hive::chain::account_index>();
```

### Step C — What cannot be ETI’d easily

From `chainbase.hpp` comments:

- `get_index<MultiIndexType, ByIndex>()` stays **inline** — too many tag combinations.
- Variadic `generic_index::emplace(Args&&...)` — either keep as template (still multiplies) or add non-template `emplace` overloads for common object ctors (larger API change).
- `modify` with arbitrary lambdas cannot be ETI’d; only fixed method bodies can.

**Pragmatic subset for first MR:**

1. `extern template class generic_index<…>` for P0 indexes (forces single codegen of dtor/ctors/undo helpers that are non-template members).
2. Optionally split non-template methods of `generic_index` into `generic_index.inl` and include only from ETI TUs.
3. Measure with `-ftime-trace` against `HIVED_BUILD_ANALYSIS.md` baseline (index-* / database_* TUs).

### Step D — CMake

In `libraries/chain/CMakeLists.txt`, add `generic_index_eti/*.cpp` to the chain library sources (same lib as `index.cpp` / `database_*.cpp`).

---

## Risks / consensus

| Risk | Mitigation |
|------|------------|
| Link errors if `extern template` without matching `template class` | CI full link of `hived` + unit tests |
| Hidden ABI / visibility with shared libs | Keep ETI inside static chain/chainbase archives first |
| No runtime/consensus change | Pure build; still run `chain_tests` + replay smoke |
| Over-ETI of cold indexes increases link time | Start with P0 table only |

---

## Success metrics

| Metric | Baseline (build analysis) | Target |
|--------|---------------------------|--------|
| `generic_index<>` total template time | 181 s / 123 instances | ≥40% reduction in instance count × time for P0 types |
| Wall rebuild of chain lib | (local measure) | −50–70 s CPU per analysis expectation |
| Behavior | — | bit-identical apply / no protocol change |

---

## Suggested upstream MR sequence

1. **MR1:** Document + measure only (`-ftime-trace` on current `database_*.cpp`).
2. **MR2:** `extern template class generic_index` for `account_index` + `comment_*` only.
3. **MR3:** Extend to witness / market / remaining P1.
4. **MR4:** Optional: move non-template `generic_index` methods out of header.

---

## Related clone files (absolute)

- `/Users/commander/hive-sources/hive/libraries/chain/index.cpp`
- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/index.hpp`
- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/multi_index_types.hpp`
- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/detail/state/account_object_multiindex.hpp`
- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/detail/state/comment_object_multiindex.hpp`
- `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/detail/state/witness_objects_multiindex.hpp`
- `/Users/commander/hive-sources/hive/libraries/chainbase/include/chainbase/chainbase.hpp`
- `/Users/commander/hive-sources/hive/libraries/chainbase/include/chainbase/chainbase.inl`
- `/Users/commander/hive-sources/hive/HIVED_BUILD_ANALYSIS.md`
