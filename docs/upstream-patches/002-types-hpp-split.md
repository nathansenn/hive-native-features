# Upstream sketch #2 — Split `types.hpp` to cut header tax

**Catalogue ID:** #2 (P0 / storage)  
**Status:** sketch only — **not applied** to hive tree  
**Clone root:** `/Users/commander/hive-sources/hive`  
**Evidence:** `HIVED_BUILD_ANALYSIS.md` — `hive/protocol/types.hpp` **86.4 s** total, **67** includes, **~1290 ms** avg per include (**CRITICAL**)

---

## Problem

Almost every protocol/chain/plugin header eventually pulls:

```
types.hpp (~1290 ms)
  → transaction.hpp (~853 ms)
  → block.hpp (~905 ms)
```

`types.hpp` is only **187 lines** but **include-heavy**: crypto, multiprecision, flat containers, raw I/O, reflection, static_variant.

### Canonical path

| File | Absolute path | Lines (clone) |
|------|---------------|---------------|
| Main | `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types.hpp` | 187 |
| Existing forward | `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types_fwd.hpp` | 82 |
| Impl | `/Users/commander/hive-sources/hive/libraries/protocol/types.cpp` | public_key / extended key bodies |
| Sibling types | `/Users/commander/hive-sources/hive/libraries/chain/include/hive/chain/external_storage/types.hpp` | **unrelated** RocksDB comment-archive types — do not confuse |

---

## What `types.hpp` currently pulls

From the header body:

| Include | Why it’s expensive | Needed for |
|---------|--------------------|------------|
| `types_fwd.hpp` | Light; already split | `fixed_string_impl`, asset_symbol fwd, flat_set_ex |
| `asset_symbol.hpp` | Medium (258 LOC) | asset symbols |
| `fixed_string.hpp` | Medium–high (285 LOC, templates) | `account_name_type` |
| `fc/crypto/sha224.hpp`, `ripemd160.hpp`, `elliptic.hpp` | **Heavy** | digests, keys |
| `fc/container/flat.hpp` (+ flat_fwd) | Heavy templates | `flat_map` / `flat_set` usings |
| `fc/io/raw.hpp` | Very heavy (serialization) | often not needed by pure type users |
| `fc/reflect/reflect.hpp`, `variant.hpp` | Heavy | FC_REFLECT at bottom |
| `fc/static_variant.hpp` | Medium–heavy | `recurrent_transfer_extension` + re-export |
| `boost/multiprecision/cpp_int.hpp` | **Heavy** | `u256` / `u512` only |
| `fc/safe.hpp`, `optional.hpp`, `uint128.hpp`, … | Medium | share_type etc. |

**Type content buckets inside `types.hpp`:**

1. **Namespace re-exports** — `std::` / `fc::` usings (`vector`, `optional`, `flat_set`, `static_variant`, …)
2. **Scalar typedefs** — `account_name_type`, `block_id_type`, `share_type`, `weight_type`, …
3. **Crypto key types** — `public_key_type`, `extended_public_key_type`, `extended_private_key_type` + FC_REFLECT + `to_variant` decls
4. **Ops-adjacent** — `recurrent_transfer_pair_id`, `recurrent_transfer_extension` (`static_variant`)
5. **Multiprecision** — `u256`, `u512`

---

## Who includes `types.hpp` (headers, clone sample)

**Protocol (direct):**

- `asset.hpp`, `authority.hpp`, `base.hpp`, `config.hpp`, `comment_types.hpp`
- `operations.hpp` (via chain of op headers + types)
- `sign_state.hpp`, `transaction.hpp`, `key_utils.hpp`
- `schema_types/account_name_type.hpp`

**Chain:**

- `hive_object_types.hpp`, `compound.hpp`, `database_impl.hpp` (private)
- `rc/rc_curve.hpp`, `rc/rc_export_objects.hpp`, `rc/resource_user.hpp`
- `util/data_filter.hpp`, `util/reward.hpp`, `util/uint256.hpp`
- `custom_operation_interpreter.hpp`, `generic_custom_operation_interpreter.hpp`
- `block_log_artifacts.hpp`, `irreversible_block_data.hpp`, `shared_db_merkle.hpp`
- `external_storage/types.hpp` (includes protocol types)

**Plugins / APIs:** many `*_api.hpp` / `database_api_objects.hpp` / RocksDB objects — **~46 headers** total direct include.

**Already using `types_fwd.hpp` only:**

- `types.hpp` itself
- `fixed_string.hpp`, `asset_symbol.hpp`
- dump/schema utilities

→ **`types_fwd.hpp` is under-used** by mid-layer headers; they jump to full `types.hpp`.

---

## Proposed split layout

Target names from catalogue / build analysis: `types_fwd.hpp` / `types_basic.hpp` / `types_operations.hpp` — refined for actual content:

```
hive/protocol/types_fwd.hpp          # KEEP & grow (forwards only)
hive/protocol/types_basic.hpp        # NEW — scalars + light usings, no crypto/raw
hive/protocol/types_crypto.hpp       # NEW — public_key* / extended_* + reflect
hive/protocol/types_ops_ext.hpp      # NEW — recurrent_transfer_extension (or fold into ops headers)
hive/protocol/types.hpp              # COMPAT umbrella: includes the above in order
```

Optional rename of catalogue `types_operations.hpp` → **`types_ops_ext.hpp`** to avoid confusion with `operations.hpp`.

### `types_fwd.hpp` (expand)

Already has:

- `fixed_string_impl` forward
- `asset_symbol_type`, `legacy_hive_asset*` forwards
- `flat_set_ex` + raw pack/unpack **declarations**
- variant helpers for symbols

**Add:**

```cpp
namespace hive { namespace protocol {
  struct public_key_type;           // if made incomplete-friendly, or keep opaque key_data fwd
  class asset;                      // if needed
  struct authority;
  // chain_id_type etc. as using to incomplete? prefer types_basic for aliases
}}
namespace fc {
  class variant;
  // keep raw pack decls
}
```

Goal: headers that only need a name or pointer/reference compile without elliptic/raw.

### `types_basic.hpp` (new)

```cpp
#pragma once
#include <hive/protocol/types_fwd.hpp>
#include <hive/protocol/fixed_string.hpp>   // account_name_type storage
// NO elliptic, NO raw.hpp, NO multiprecision, NO static_variant if avoidable

#include <fc/uint128.hpp>
#include <fc/safe.hpp>
#include <fc/optional.hpp>
#include <fc/time.hpp>
#include <fc/crypto/ripemd160.hpp>  // block_id_type / checksum — consider sha types only
#include <fc/crypto/sha256.hpp>     // chain_id_type, digest_type

#include <cstdint>
#include <string>
#include <vector>
// … minimal std usings

namespace hive {
  // light std/fc usings only
namespace protocol {
  using account_name_type = fixed_string<16>;
  using chain_id_type     = fc::sha256;
  using block_id_type     = fc::ripemd160;
  using checksum_type     = fc::ripemd160;
  using transaction_id_type = fc::ripemd160;
  using digest_type       = fc::sha256;
  using share_type        = fc::safe<int64_t>;
  using ushare_type       = fc::safe<uint64_t>;
  using weight_type       = uint16_t;
  // …
}}
```

### `types_crypto.hpp` (new)

Move `public_key_type` / `extended_*` structs, operators, `FC_REFLECT`, and `fc::to_variant` / `from_variant` **declarations**.  
Implementations stay in `types.cpp`.

Includes: `fc/crypto/elliptic.hpp`, base58-related only as needed by **.cpp**.

### `types_ops_ext.hpp` (new) — catalogue “types_operations”

```cpp
#pragma once
#include <hive/protocol/types_basic.hpp>
#include <fc/static_variant.hpp>
#include <fc/container/flat.hpp>

namespace hive { namespace protocol {
  struct recurrent_transfer_pair_id { uint8_t pair_id = 0; };
  using recurrent_transfer_extension =
    fc::static_variant<void_t, recurrent_transfer_pair_id>;
  using recurrent_transfer_extensions_type =
    fc::flat_set<recurrent_transfer_extension>;
}}
FC_REFLECT(...);
```

**Include from** `hive_operations.hpp` / recurrent transfer op only — not from every consumer of `account_name_type`.

### `types.hpp` (umbrella, keep path stable)

```cpp
#pragma once
#include <hive/protocol/types_basic.hpp>
#include <hive/protocol/types_crypto.hpp>
#include <hive/protocol/types_ops_ext.hpp>
// multiprecision u256/u512 — either here or types_math.hpp for util/uint256 only
#include <boost/multiprecision/cpp_int.hpp>
namespace hive {
  typedef boost::multiprecision::uint256_t u256;
  typedef boost::multiprecision::uint512_t u512;
}
```

**Do not** pull `fc/io/raw.hpp` into the umbrella unless a consumer still needs it via a separate `types_raw.hpp`. Today raw is included at the top of `types.hpp` largely for side-effect/template availability — audit and drop.

---

## Migration strategy (include graph)

### Phase 0 — measure

- Clang `-ftime-trace` on `types.hpp` include cost (already in `HIVED_BUILD_ANALYSIS.md`).
- IWYU or manual: list headers that only need `account_name_type` / `share_type`.

### Phase 1 — additive split (ABI-compatible)

1. Create `types_basic.hpp` / `types_crypto.hpp` / `types_ops_ext.hpp` by **moving** code out of `types.hpp`.
2. Leave `types.hpp` as umbrella (no call-site changes).
3. Green build.

### Phase 2 — leaf conversion (high ROI first)

Replace `#include <hive/protocol/types.hpp>` with the narrowest header:

| Consumer class | Prefer |
|----------------|--------|
| Object ids / names only | `types_basic.hpp` |
| Authority / keys | `types_crypto.hpp` (+ basic) |
| Asset amounts | `asset.hpp` (already includes types) — may drop to basic |
| Recurrent transfer ops | `types_ops_ext.hpp` |
| RC util `uint256` | `types.hpp` or tiny `types_math.hpp` with multiprecision only |
| Full protocol / wallet | umbrella `types.hpp` |

**High-ROI chain headers to convert first:**

- `libraries/chain/include/hive/chain/util/uint256.hpp` → multiprecision only
- `libraries/chain/include/hive/chain/rc/resource_user.hpp`
- `libraries/chain/include/hive/chain/compound.hpp`
- API arg headers that only pass account names

### Phase 3 — drop heavy includes from basic path

- Remove `fc/io/raw.hpp` from any header included by `types_basic.hpp`.
- Move multiprecision out of default umbrella if possible (`util/uint256.hpp` direct include).
- Ensure `config.hpp` (includes `types.hpp` today) can take `types_basic.hpp` only if HF constants don’t need keys.

### Phase 4 — `base.hpp` / `authority.hpp`

`base.hpp` currently includes full `types.hpp` + `authority.hpp`.  
If `base_operation` only needs `account_name_type` + `authority` forward, narrow it — **largest fan-out win** after types itself (`operation_util.hpp` → all ops).

---

## Risks

| Risk | Notes |
|------|-------|
| ODR / FC_REFLECT duplication | Reflect macros only in one header; umbrella must not double-reflect |
| Incomplete types break offsetof-style code | Keep complete `public_key_type` in crypto header; don’t half-forward key_data users |
| Testnet / config include order | `config.hpp` ↔ `types.hpp` ↔ `hardfork.hpp` cycle risk — preserve umbrella during Phase 1 |
| Windows / MSVC include order | Hive primarily GCC/Clang; still test clang-18 |

---

## Success metrics

| Metric | Baseline | Target |
|--------|----------|--------|
| `types.hpp` cumulative parse | 86.4 s | ≤ half for “basic-only” TUs |
| Avg include cost | 1290 ms | ≤ 400–600 ms for basic path |
| Expected overall | — | **40–50 s** CPU (per build analysis) |
| Call sites on umbrella | 46+ headers | progressive reduction; umbrella remains valid |

---

## Suggested upstream MR sequence

1. **MR1:** Split files + umbrella (no consumer change).
2. **MR2:** Convert protocol leaves (`comment_types`, schema account name).
3. **MR3:** Convert chain util / RC headers; drop multiprecision from default path.
4. **MR4:** Narrow `base.hpp` / `authority.hpp` includes; remeasure.

---

## Related clone files (absolute)

- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types_fwd.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/types.cpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/base.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/authority.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/asset.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/config.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/fixed_string.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/asset_symbol.hpp`
- `/Users/commander/hive-sources/hive/HIVED_BUILD_ANALYSIS.md`
