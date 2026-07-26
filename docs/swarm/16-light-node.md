# 16 – Light-node (swarm note)

**Status:** documented from code  
**Date:** 2026-07-25  
**Workdir:** `/tmp/hive-native-features`  
**Canonical matrix:** [`docs/09-light-node-matrix.md`](../09-light-node-matrix.md)

---

## Purpose

Every Hive native feature (NFT, HTLC, contracts) must remain **light / pruned / mobile safe**. This note is the swarm operational summary of **what the portable chain stand-in actually does**, so agents do not invent skip paths that contradict `node_config` or `require_full_*`.

---

## Config surface

```cpp
// include/hive_native/chain/database.hpp
struct node_config {
   bool nft_skip_state = false;   // light/mobile non-consensus
   bool htlc_skip_state = false;
   bool contracts_skip = false;
   bool is_consensus_node = true; // witnesses must be true
   uint32_t hardfork = HIVE_HARDFORK_CONTRACTS;
};
```

**Rules of thumb**

1. **Witnesses / consensus:** `is_consensus_node = true`, all skip flags `false`.
2. **Light / mobile non-consensus:** set the feature skip flag(s); expect **no local apply** of those ops and **empty local API**.
3. Never set NFT/HTLC skip on a consensus node — `require_full_*` hard-fails.

---

## Apply gates (implementers)

| Feature | Gate | File(s) |
|---------|------|---------|
| NFT (all 6 ops) | `db.require_full_nft_state()` | `evaluators_nft.cpp` |
| HTLC (create/redeem/refund) | `db.require_full_htlc_state()` | `evaluators_htlc.cpp` |
| Contracts (deploy/call) | `if (contracts_skip && !is_consensus_node) throw` | `evaluators_contracts.cpp` |

### `require_full_nft_state` / `require_full_htlc_state` (`database.cpp`)

```
if consensus && skip → "consensus node cannot skip …"
if skip             → "… state skipped on this node"
```

### Contracts asymmetry

- Non-consensus + `contracts_skip` → throw `"contracts skipped"`.
- Consensus + `contracts_skip` → **still executes** (flag does not disable consensus apply).
- There is **no** `require_full_contracts_state()`.

---

## API stubs under skip

| Method | Skip flag | Behavior |
|--------|-----------|----------|
| `get_nft` / NFT lists | `nft_skip_state` | empty / `nullopt` |
| NFT lists + `list_args.light` | (full state) | clears `uri`; limit clamped to 100 |
| `get_htlc` | `htlc_skip_state` | `nullopt` |
| `get_contract` | `contracts_skip` | `nullopt`; otherwise returns object with **code stripped** |

Source: `src/api/database_api_stubs.cpp`.

---

## Behavior matrix (short)

| Role | NFT | HTLC | Contracts |
|------|-----|------|-----------|
| Consensus full | Apply + store | Apply + store | Apply + engine |
| Consensus + skip misconfig | **Error** (NFT/HTLC) | **Error** | Apply (ignore skip) |
| Light non-consensus + skip | Apply **rejects**; API empty | Apply **rejects**; API empty | Apply **rejects**; API empty |
| Pruned (design) | Current owners | Open only | Hash / optional blobs |

Full table and design-vs-code gaps: **`docs/09-light-node-matrix.md`**.

---

## Agent checklist

When touching evaluators or config:

- [ ] Call / preserve `require_full_nft_state` / `require_full_htlc_state` on every NFT/HTLC apply path.
- [ ] Do not allow consensus nodes to “soft skip” NFT or HTLC state.
- [ ] For contracts, keep consensus execution mandatory; skip is non-consensus only.
- [ ] API methods must not invent local objects when skip is set (return empty).
- [ ] Add/extend tests when changing skip semantics (`test_nft_light_skip` is the NFT pattern).
- [ ] Do not claim prune-tier retention is fully implemented unless you add real prune hooks (today: comments + design).

---

## Related docs

| Doc | Content |
|-----|---------|
| `docs/00-architecture-overview.md` §12 | Design node-class summary |
| `docs/01-nft-design.md` §9 | NFT light / pruned |
| `docs/02-htlc-design.md` §12 | HTLC light behavior |
| `docs/03-contracts-design.md` §10 | Contracts verify-only / skip |
| `docs/04-performance-budgets.md` §11 | Light-node perf budgets |
| `docs/05-verification-and-testing-strategy.md` §6 | Skip-path tests |
| `docs/09-light-node-matrix.md` | Full implemented matrix |

---

## Verification pointer

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hive_native_tests   # includes test_nft_light_skip
```
