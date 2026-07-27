# perf-10 — Compact-block relay data structures (#301)

**Task-ID:** catalogue #301 / swarm-perf-p0-impl  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Priority:** P0 · category: p2p  

---

## Goal

Portable **compact-block relay** data structures for catalogue **#301** (*Compact-block relay using short transaction IDs + fill requests*):

1. **`short_tx_id`** — first 6 bytes of `sha256(tx_bytes)` (portable stand-in for BIP152 salted SipHash).
2. **`compact_block`** — `{ header_hash, short_ids[], prefilled[] }` BIP152-style announcement.
3. **`reconstruct(local_mempool)`** — full ordered body when the mempool hits; otherwise **missing short IDs** for a fill request.

## Files

| Path | Role |
|------|------|
| [`include/hive_native/perf/compact_block.hpp`](../../include/hive_native/perf/compact_block.hpp) | Header-only API |
| [`tests/test_compact_block.cpp`](../../tests/test_compact_block.cpp) | Unit tests |
| [`CMakeLists.txt`](../../CMakeLists.txt) | Target `hive_native_compact_block_tests` |
| [`docs/swarm/perf-10-compact-block.md`](./perf-10-compact-block.md) | This report |

## API sketch

```cpp
using short_tx_id = std::array<uint8_t, 6>;
using mempool_map = std::unordered_map<short_tx_id, std::string, short_tx_id_hash>;

short_tx_id make_short_tx_id(const sha256_t& digest);
short_tx_id make_short_tx_id(std::string_view tx_bytes);

struct prefilled_tx { uint32_t index; std::string tx; };

struct compact_block {
  sha256_t                  header_hash;
  std::vector<short_tx_id>  short_ids;   // non-prefilled, block order
  std::vector<prefilled_tx> prefilled;  // ascending absolute indexes
  size_t tx_count() const;              // short_ids + prefilled
};

struct reconstruct_result {
  std::vector<std::string> txs;       // full ordered body if ok()
  std::vector<short_tx_id> missing;   // fill-request short IDs
  bool ok() const;
  std::optional<std::vector<std::string>> full_list() const;
};

reconstruct_result reconstruct(const compact_block& cb,
                               const mempool_map& local_mempool);

compact_block make_compact_block(const sha256_t& header_hash,
                                 const std::vector<std::string>& txs,
                                 const std::vector<uint32_t>& prefill_indexes = {});
```

### Semantics

| Rule | Detail |
|------|--------|
| Short ID | `sha256(tx)[0..6)` via portable `hive_native::sha256` |
| Body length | `short_ids.size() + prefilled.size()` |
| Prefill | Absolute index into the full block body; sorted ascending, unique |
| Reconstruct hit | All short IDs found in mempool (or prefilled) → `full_list()` set |
| Reconstruct miss | `missing` lists short IDs to request; `txs` cleared |
| Malformed prefilled | Out-of-range / non-ascending indexes → treat as failure (missing short IDs) |

### Wire savings (sketch)

For *N* transactions of average size *B* bytes, a compact announcement is roughly:

- 32-byte header hash  
- 6×(N − P) short IDs  
- full bodies for *P* prefilled (coinbase / high-priority)  

vs *N×B* for a full block body. With a warm mempool, *P* ≪ *N* and most short IDs hit, so fill requests stay small.

## Tests

| Case | Asserts |
|------|---------|
| `short_tx_id` from sha256 | First 6 bytes of FIPS SHA-256("abc") |
| Full mempool hit | Reconstruct equals original body; `full_list()` set |
| Prefill coinbase + priority | Middle txs from mempool; ends from prefilled |
| Missing IDs | Single absent short ID reported; no full list |
| Partial then fill | Missing set → add to map → second reconstruct ok |
| Empty block | `ok()`, empty txs |
| Malformed prefilled index | Failure path (no silent success) |

## Build / verify

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_compact_block_tests
./build/hive_native_compact_block_tests
ctest --test-dir build -R compact_block --output-on-failure
```

## Evidence

| Check | Result |
|-------|--------|
| Branch | `swarm-perf-p0-impl` |
| Host | Apple arm64 |
| Build | `cmake --build build -j --target hive_native_compact_block_tests` → **success** |
| Unit binary | `./build/hive_native_compact_block_tests` → `compact_block_passed=45 failed=0` |
| Regression | `hive_native_perf_tests` → `perf_passed=27 failed=0`; `hive_native_tests` → `passed=165 failed=0` |

```text
$ ./build/hive_native_compact_block_tests
compact_block_passed=45 failed=0
```

## Upstream mapping

Catalogue: **#301** — *Compact-block relay using short transaction IDs + fill requests*  
Follow-on: **#302** — BIP152-style compact blocks with prefilled high-priority transactions  
(`docs/performance/HIVE_1000_OPTIMIZATIONS.md`, P0 p2p; review item 31)

Suggested hived integration points:

1. New P2P messages: `compact_block` / `getblocktxn` (or Hive-named equivalents).  
2. Peer path: on block announcement, send compact form when peer supports the feature bit.  
3. Receiver: index mempool by short ID (or derive on the fly from full txid); call `reconstruct`; on miss, issue fill request for `missing`.  
4. Prefer salted short IDs (BIP152 SipHash + header/nonce) before mainnet — this portable prototype uses bare sha256 truncation for determinism without extra deps.  
5. Prefill coinbase-equivalent + any tx the sender knows the peer likely lacks (high-priority / just-seen).

Constraints: **non-consensus** portable prototype; no HF; wire format and salted ID scheme remain integration work.
