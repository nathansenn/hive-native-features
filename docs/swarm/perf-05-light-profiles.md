# perf-05 — Light node profile presets (#8 #691)

**Task-ID:** catalogue #8 #691 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Canonical matrix:** [`docs/09-light-node-matrix.md`](../09-light-node-matrix.md)

---

## Goal

Named **node profile presets** that map operational roles onto portable
`node_config` skip flags — so agents and node operators do not hand-set illegal
witness combinations.

| Catalogue | Title |
|-----------|--------|
| **#8** | Drop secondary indexes entirely on pruned / mobile nodes |
| **#691** | `HIVE_LIGHT_NODE` CMake option that strips non-essential plugins and indexes at compile time |

---

## Implementation

| Path | Role |
|------|------|
| [`include/hive_native/chain/node_profiles.hpp`](../../include/hive_native/chain/node_profiles.hpp) | `node_profile` enum, `apply_profile`, `make_node_config`, `default_profile`, consensus sanitizers |
| [`include/hive_native/chain/database.hpp`](../../include/hive_native/chain/database.hpp) | Optional wire comment: call `apply_profile` / `apply_default_profile` at startup |
| [`tests/test_node_profiles.cpp`](../../tests/test_node_profiles.cpp) | Flag matrix + apply-path gates for each profile |
| [`CMakeLists.txt`](../../CMakeLists.txt) | `HIVE_LIGHT_NODE` option + target `hive_native_node_profiles_tests` |

### Profiles → flags

| Profile | `is_consensus_node` | `nft_skip_state` | `htlc_skip_state` | `contracts_skip` |
|---------|---------------------|------------------|-------------------|------------------|
| `full` | true | false | false | false |
| `api_pruned` | false | false | false | true |
| `mobile_light` | false | true | true | true |

### Consensus invariant

Witnesses / consensus nodes **cannot** enable `nft_skip_state` or `htlc_skip_state`:

- `apply_profile(..., full)` always clears those skips.
- `config_ok_for_role` / `require_config_ok_for_role` reject consensus + NFT/HTLC skip.
- `sanitize_consensus_skips` clears illegal skips in place.
- Existing `database::require_full_nft_state` / `require_full_htlc_state` remain the apply-time hard gate.

(`contracts_skip` on consensus is allowed; contract evaluators ignore it for deploy/call.)

### API sketch

```cpp
#include "hive_native/chain/node_profiles.hpp"

using hive_native::chain::node_profile;
using hive_native::chain::apply_profile;

database db;
apply_profile(db, node_profile::mobile_light); // or api_pruned / full

// Compile-time default (#691):
// cmake -DHIVE_LIGHT_NODE=ON → default_profile() == mobile_light
apply_default_profile(db.config);
```

### CMake

```bash
# Optional light compile default
cmake -S . -B build -DHIVE_LIGHT_NODE=ON
```

Defines `HIVE_LIGHT_NODE=1` on the `hive_native` target (PUBLIC), so
`default_profile()` returns `mobile_light` for dependents.

---

## Build / verify

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_node_profiles_tests
./build/hive_native_node_profiles_tests
ctest --test-dir build -R node_profiles --output-on-failure
```

### Evidence (this run)

```text
$ ./build/hive_native_node_profiles_tests
node_profiles_passed=41 failed=0

$ ctest --test-dir build -R node_profiles --output-on-failure
100% tests passed out of 1

# regression
$ ./build/hive_native_tests && ./build/hive_native_perf_tests
passed=165 failed=0
perf_passed=27 failed=0
```

---

## Apply-path behavior (sanity)

| Profile | NFT create | Contract deploy |
|---------|------------|-----------------|
| `full` | applies | applies (when HF active) |
| `api_pruned` | applies (indexes kept for API) | **rejects** (`contracts skipped`) |
| `mobile_light` | **rejects** (`NFT state skipped`) | **rejects** |

Full matrix: [`docs/09-light-node-matrix.md`](../09-light-node-matrix.md), swarm note [`16-light-node.md`](./16-light-node.md).

---

## Upstream mapping

When porting to hived:

1. Map profiles to plugin/index enablement (not only runtime bools) under `HIVE_LIGHT_NODE`.
2. Keep witness config validation: refuse start if consensus + skipped NFT/HTLC indexes.
3. `api_pruned` ≈ API node with current-state indexes, contracts plugin optional/off.
4. `mobile_light` ≈ personal/mobile: no secondary feature indexes; headers / selective body.

---

## Related

| Doc / ID | Note |
|----------|------|
| #8 | Drop secondary indexes on pruned/mobile |
| #691 | `HIVE_LIGHT_NODE` CMake |
| #873 | CMake preset `mobile-light` (follow-on) |
| `docs/09-light-node-matrix.md` | Implemented skip matrix |
| `docs/swarm/16-light-node.md` | Operational agent checklist |
