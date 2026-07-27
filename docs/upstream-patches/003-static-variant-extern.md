# Upstream sketch #3 — `extern template` strategy for operation `static_variant` serialization

**Catalogue ID:** #3 (P0 / storage)  
**Status:** sketch only — **not applied** to hive tree  
**Clone root:** `/Users/commander/hive-sources/hive`  
**Evidence:** `HIVED_BUILD_ANALYSIS.md` — `fc::static_variant` storage/serialization ~**750 ms/op**, **~66** regular ops historically; this clone’s `operation` lists **~93** alternatives (50 regular + 43 virtual when `HIVE_PROTOCOL_SKIP_VOPS` unset). `extended_serialization_functor<static_variant<…>>` **54.0 s** over 47 instances.

---

## Problem

`hive::protocol::operation` is a single large:

```15:125:/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operations.hpp
  typedef fc::static_variant<
        vote_operation, // 0
        comment_operation, // 1
        ...
        recurrent_transfer_operation // 49
#ifndef HIVE_PROTOCOL_SKIP_VOPS
        ,
        /// virtual operations below this point
        fill_convert_request_operation, // last_regular + 1
        ...
        declined_voting_rights_operation //last_regular + 43
#endif // HIVE_PROTOCOL_SKIP_VOPS
      > operation;
```

Every TU that includes `operations.hpp` + FC raw/variant headers re-instantiates:

- `fc::impl::storage_ops<N, Op, …>` for each N
- `pack` / `unpack` / `to_variant` / `from_variant` for the whole variant
- `extended_serialization_functor<operation>` / `variant_creator_functor<operation>` (declared at bottom of `operations.hpp`)

Build analysis: **~750 ms × op × many TUs** → hundreds of CPU-seconds.

**Order is consensus-critical** — any reordering prior to virtual ops is a hardfork (`operations.hpp` note L12–14). Extern-template work must not reorder the variant.

---

## Clone map — operation static_variant machinery

| Path | Role |
|------|------|
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operations.hpp` | `typedef fc::static_variant<…> operation`; FC functors; `HIVE_DECLARE_OPERATION_TYPE` |
| `/Users/commander/hive-sources/hive/libraries/protocol/operations.cpp` | `is_market_operation` / vop helpers; **`HIVE_DEFINE_OPERATION_TYPE(operation)`** |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util.hpp` | auth visitor; `extended_serialization_functor` / `extended_variant_creator_functor` |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util_impl.hpp` | `HIVE_DEFINE_OPERATION_TYPE` → `operation_validate` / `get_required_authorities` |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/hive_operations.hpp` | regular op structs (+ nested static_variants e.g. pow2_work, comment extensions) |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/hive_virtual_operations.hpp` | virtual ops |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/dhf_operations.hpp` | proposal ops + small static_variant |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/base.hpp` | `future_extensions` static_variant |
| `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/types.hpp` | re-exports `fc::static_variant`; `recurrent_transfer_extension` |
| `/Users/commander/hive-sources/hive/libraries/schema/include/hive/schema/schema_types/static_variant.hpp` | schema reflection for variants |
| `/Users/commander/hive-sources/hive/libraries/wallet/include/hive/wallet/reflect_util.hpp` | `create_static_variant_map<operation>()` |
| `/Users/commander/hive-sources/hive/programs/js_operation_serializer/main.cpp` | codegen over static_variant |
| `/Users/commander/hive-sources/hive/programs/util/explain_op.cpp` | debug visitor |

**fc submodule** (`libraries/fc`) holds `fc/static_variant.hpp` + raw pack specializations — empty/unpopulated in this shallow clone; upstream patches must still target **fc + protocol** together when declaring extern templates for pack/unpack.

---

## Existing “define in .cpp” pattern (partial)

`HIVE_DECLARE_OPERATION_TYPE` / `HIVE_DEFINE_OPERATION_TYPE` already move **validate + authority** out of headers:

```61:72:/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util.hpp
#define HIVE_DECLARE_OPERATION_TYPE( OperationType ) ...
void operation_validate( const OperationType& o );
void operation_get_required_authorities( ... );
```

```23:42:/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util_impl.hpp
#define HIVE_DEFINE_OPERATION_TYPE( OperationType ) ...
// defined in operations.cpp for hive::protocol::operation
```

This does **not** cover FC serialization templates — the expensive part.

---

## Extern-template strategy

### Target symbols (primary)

For `using operation = fc::static_variant<…>`:

1. **Binary serialization (hot path — blocks/tx):**
   - `fc::raw::pack<Stream>(Stream&, const operation&)`
   - `fc::raw::unpack<Stream>(Stream&, operation&, …)`
   - Possibly stream-specific instantiations (`datastream`, `vector` buffer types used by hived)

2. **JSON / API path:**
   - `fc::to_variant(const operation&, fc::variant&)`
   - `fc::from_variant(const fc::variant&, operation&)`
   - `fc::serialization_functor<operation>` / `variant_creator_functor<operation>` (specialized in `operations.hpp`)
   - `extended_serialization_functor<operation>` (legacy mode)

3. **Optional:** `fc::get_typename<operation>` if measured hot.

### Non-goals (first pass)

- Full ETI of every nested static_variant (`pow2_work`, comment extensions, `future_extensions`) — second wave.
- Changing visitor dispatch to a codegen table (catalogue #193) — separate project.
- Reordering / splitting the consensus `operation` variant.

---

## Proposed files (sketch only)

### A. Declaration header

**`libraries/protocol/include/hive/protocol/operation_serialization_eti.hpp`** (new):

```cpp
#pragma once
#include <hive/protocol/operations.hpp>
// Stream types used by hive — pick the real ones from fc:
// #include <fc/io/raw_fwd.hpp> or datastream headers

namespace fc {

  // Variant JSON
  extern template void to_variant( const hive::protocol::operation&, variant& );
  extern template void from_variant( const variant&, hive::protocol::operation& );

  namespace raw {
    // Repeat for each Stream type measured in -ftime-trace, e.g.:
    // extern template void pack( datastream<...>&, const hive::protocol::operation& );
    // extern template void unpack( datastream<...>&, hive::protocol::operation&, uint32_t, bool );
  }

} // fc

// If these are class templates with non-inline methods:
// extern template struct serialization_functor<hive::protocol::operation>;
// extern template struct extended_serialization_functor<hive::protocol::operation>;
```

**Include rule:** any TU that currently serializes `operation` includes this header **after** `operations.hpp` (or fold into `operations.hpp` behind a macro `HIVE_OPERATION_ETI_DECLARE=1`).

### B. Explicit instantiation TU

**`libraries/protocol/operation_serialization_eti.cpp`** (new):

```cpp
#include <hive/protocol/operation_serialization_eti.hpp>
// Include full raw / static_variant implementation headers from fc

namespace fc {

  template void to_variant( const hive::protocol::operation&, variant& );
  template void from_variant( const variant&, hive::protocol::operation& );

  namespace raw {
    // template void pack<...>(...);
    // template void unpack<...>(...);
  }

} // fc
```

Add to `libraries/protocol/CMakeLists.txt` next to `operations.cpp`.

### C. fc-side enablement (may need fc MR)

`fc::static_variant` pack/unpack are often **header-only templates**. For `extern template` to work:

1. Ensure pack/unpack are **declared** in a visible header and **defined** in a header that ETI TU includes (standard C++).
2. Add matching `extern template` in fc’s `static_variant` consumers **or** only in hive protocol (hive can instantiate if definitions are visible).
3. If definitions are inline in class body, may need a small **fc** change: move pack/unpack bodies to `static_variant_impl.hpp` included only from ETI TUs + selected .cpps — larger patch; coordinate with `libraries/fc` submodule.

### D. Macro packaging (optional polish)

```cpp
// operation_util_impl.hpp style
#define HIVE_DEFINE_OPERATION_SERIALIZATION( OperationType ) \
  namespace fc { \
    template void to_variant( const OperationType&, variant& ); \
    template void from_variant( const variant&, OperationType& ); \
    /* pack/unpack ... */ \
  }
```

Use for `operation` first; later for condenser legacy op variants if still costly.

---

## Consumers to remeasure after ETI

High include fan-in of `operations.hpp` (clone):

- `libraries/protocol/include/hive/protocol/transaction.hpp`
- `libraries/protocol/include/hive/protocol/forward_impacted.hpp`
- `libraries/chain/include/hive/chain/evaluator.hpp`
- `libraries/chain/include/hive/chain/database_virtual_operations.hpp`
- `libraries/plugins/apis/condenser_api/.../condenser_api_legacy_operations.hpp`
- `libraries/plugins/apis/account_history_api/...`
- `programs/util/explain_op.cpp`, `js_operation_serializer`

Also hot codegen TUs from build analysis: **`full_transaction.cpp`**, **`full_block.cpp`** (backend 34.5s / 29.5s).

---

## Secondary static_variants (wave 2)

| Type | Location | Notes |
|------|----------|-------|
| `future_extensions` | `base.hpp` | tiny (void_t only) — low ROI |
| `pow2_work` | `hive_operations.hpp` | small |
| Comment / witness property extensions | `hive_operations.hpp` | medium |
| `recurrent_transfer_extension` | `types.hpp` | pairs with sketch #2 split |
| Condenser `legacy_*` variants | `condenser_api_legacy_*.hpp` | API-only |
| Plugin test variants | `tests/unit/plugin_tests/plugin_ops.cpp` | ignore |

---

## Interaction with `#1` / `#2`

| Sketch | Interaction |
|--------|-------------|
| #2 types split | Reduces cost of including ops headers; **does not** remove static_variant codegen |
| #1 generic_index ETI | Orthogonal; both reduce total CPU — apply independently |
| `HIVE_PROTOCOL_SKIP_VOPS` | Builds with fewer alts — ETI TU should match the same macro flags as the library |

---

## Risks

| Risk | Mitigation |
|------|------------|
| Missing stream specialization → link error | Enumerate Stream types via nm/`-ftime-trace`; CI full `hived` link |
| Silent fallback to different pack overload | Keep signatures exact; no defaulted template args drift |
| fc header-only forces fc submodule MR | Prefer hive-only ETI first; fc move-out as follow-up |
| Hardfork if someone “simplifies” variant while touching file | CI protocol hash / `js_operation_serializer` golden output |
| Virtual-op count growth | ETI TU recompiles when ops added — acceptable single-point cost |

---

## Success metrics

| Metric | Baseline | Target |
|--------|----------|--------|
| `storage_ops<N,…>` re-instantiation across TUs | ~47 instances × ~750 ms | **1** defining TU; consumers use extern |
| `extended_serialization_functor<operation>` | 54 s / 47 instances | near-single instantiation |
| Expected overall (analysis) | — | **100+ s** CPU |
| Runtime | — | identical binary wire format |

---

## Suggested upstream MR sequence

1. **MR1 (hive):** Measure which `pack`/`to_variant` specializations dominate (`-ftime-trace` on `full_transaction.cpp`, `operations.cpp`).
2. **MR2 (hive):** `operation_serialization_eti.cpp` for `to_variant`/`from_variant` + primary raw Stream.
3. **MR3 (fc, if needed):** Non-inline pack/unpack bodies for `static_variant`.
4. **MR4:** Cover condenser legacy variants; document “add op → touch ETI TU” in contributor guide.

---

## Related clone files (absolute)

- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operations.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/operations.cpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/operation_util_impl.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/hive_operations.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/hive_virtual_operations.hpp`
- `/Users/commander/hive-sources/hive/libraries/protocol/include/hive/protocol/transaction.hpp`
- `/Users/commander/hive-sources/hive/libraries/chain/full_transaction.cpp`
- `/Users/commander/hive-sources/hive/libraries/chain/full_block.cpp`
- `/Users/commander/hive-sources/hive/HIVED_BUILD_ANALYSIS.md`
