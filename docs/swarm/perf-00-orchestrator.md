# perf-00 — Orchestrator (swarm-perf-p0-impl)

**Task-ID:** swarm-perf-p0-impl / perf-00-orchestrator  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  

Coordinates the P0 portable performance swarm: scan portable code under
`include/hive_native/perf/` (+ related new chain headers), flip catalogue
statuses, record progress, and index all worker reports.

---

## Portable code inventory

### `include/hive_native/perf/`

| Header | Catalogue IDs | Role |
|--------|---------------|------|
| `account_cache.hpp` | #63 | Primary-key account balance cache |
| `apply_scheduler.hpp` | #151 | Op dependency → layered apply schedule |
| `arena.hpp` | #72 #219 | Bump-pointer arena for temps |
| `bloom.hpp` | #204 | Compact bloom filter |
| `compact_block.hpp` | #301 | Short-txid compact block + fill request |
| `flat_hash_map.hpp` | #4 #63 #424 | Open-addressing hot map |
| `known_tx_set.hpp` | #204 | Bloom + exact known-tx set |
| `op_dependency.hpp` | #151 | Greedy dependency scheduler |
| `rc_calibrator.hpp` | #891 #894 | RC wall-time calibration samples |
| `rocksdb_presets.hpp` | #23–#28, #103–#105 | RocksDB tuning presets |
| `selective_undo.hpp` | #43 | Field-level undo for balances |
| `simd_math.hpp` | #209 | Batch mul/add (NEON/SSE2/scalar) |
| `worker_pool.hpp` | #152 #153 #155 | Priority worker pool sketch |
| `xxhash64.hpp` | #32 #84 | Fast non-crypto integrity hash |

### Related new / touched files

| Path | Catalogue IDs |
|------|---------------|
| `include/hive_native/chain/account_index.hpp` | #63 |
| `include/hive_native/chain/node_profiles.hpp` | #8 #691 |
| `benchmarks/bench_rc_calibrate.cpp` | #891 |
| `tests/test_*.cpp`, `tests/fuzz_htlc_preimage.cpp` | various / verification |

---

## Catalogue status flips (`HIVE_1000_OPTIMIZATIONS.json`)

IDs that gained portable code and whose **status** field was upgraded
(2026-07-27 orchestrator pass). JSON remains valid (1000 objects).

| ID | From | To | Title (short) |
|----|------|-----|---------------|
| 23 | todo | portable-prototype | RocksDB block cache 20–30% free RAM |
| 24 | todo | portable-prototype | Bloom 10 bits/key on CFs |
| 25 | todo | portable-prototype | write_buffer_size bulk load |
| 26 | todo | portable-prototype | ZSTD cold / LZ4 hot |
| 27 | todo | portable-prototype | Universal compaction history CFs |
| 28 | todo | portable-prototype | L0 compaction trigger SSD |
| 43 | todo | portable-prototype | Selective undo (changed fields only) |
| 103 | todo | portable-prototype | RocksDB preset: NVMe witness |
| 104 | todo | portable-prototype | RocksDB preset: eMMC mobile |
| 105 | todo | portable-prototype | RocksDB preset: HDD archive |
| 155 | todo | portable-prototype | Separate pools crypto/I/O/apply/index |
| 209 | todo | portable-prototype | SIMD / NEON vote-weight arithmetic |
| 301 | todo | portable-prototype | Compact-block relay short IDs |
| 691 | design | partial-portable | `HIVE_LIGHT_NODE` / light profiles |

**Total status flips: 14**

Also set `portable: true` (no status change) for IDs already
`portable-prototype` but flagged false: **#4, #63, #72, #424**.

Pre-existing portable statuses left as-is where already correct:
#8 partial-portable, #32 #63 #72 #84 #151–#153 #204 #216 #219 #424 #891 #894, etc.

---

## Swarm reports found (`docs/swarm/perf-*.md`)

| Report | Status | Focus | Catalogue |
|--------|--------|-------|-----------|
| [perf-00-orchestrator.md](./perf-00-orchestrator.md) | PASS | This index + catalogue sync | — |
| [perf-01-apply-scheduler.md](./perf-01-apply-scheduler.md) | PASS | Apply scheduler integration | #151 |
| [perf-02-account-cache.md](./perf-02-account-cache.md) | PASS | Account primary-key cache | #63 |
| [perf-03-simd.md](./perf-03-simd.md) | PASS | SIMD batch mul/add | #209 |
| [perf-04-known-tx.md](./perf-04-known-tx.md) | PASS | Known-transaction set | #204 |
| [perf-05-light-profiles.md](./perf-05-light-profiles.md) | PASS | Light / pruned node profiles | #8 #691 |
| [perf-06-rc-cal.md](./perf-06-rc-cal.md) | PASS | RC calibration microbench | #891 |
| [perf-08-rocksdb.md](./perf-08-rocksdb.md) | PASS | RocksDB presets | #23–#28, #103–#105 |
| [perf-09-undo.md](./perf-09-undo.md) | PASS | Selective undo | #43 |
| [perf-10-compact-block.md](./perf-10-compact-block.md) | PASS | Compact-block structures | #301 |
| [perf-11-dep-stress.md](./perf-11-dep-stress.md) | PASS | Op-dependency stress | #151 |
| [perf-12-ci.md](./perf-12-ci.md) | PASS | Full ctest + benches | #794 (gate) |
| [perf-13-fuzz.md](./perf-13-fuzz.md) | PASS | HTLC preimage fuzz | verification §8 |
| [perf-19-gitops.md](./perf-19-gitops.md) | PASS | Commit, push, PR | GitOps |

**Gaps:** `perf-07-*.md` and `perf-14`–`perf-18` were **not** present at orchestrator close.

**Count:** 14 report files matching `docs/swarm/perf-*.md` (including this orchestrator).

---

## Related docs

- Catalogue JSON: [`docs/performance/HIVE_1000_OPTIMIZATIONS.json`](../performance/HIVE_1000_OPTIMIZATIONS.json)
- P0 roadmap: [`docs/performance/P0_ROADMAP.md`](../performance/P0_ROADMAP.md)
- Progress: [`PROGRESS.md`](../../PROGRESS.md) — section **swarm-perf-p0-impl (2026-07-27)**

---

## Orchestrator checklist

- [x] Wait for worker agents to land headers / tests / reports
- [x] Scan `include/hive_native/perf` + new chain files for catalogue IDs
- [x] Flip JSON statuses for newly portable IDs (valid JSON, 1000 entries)
- [x] Update `PROGRESS.md` with swarm-perf-p0-impl section (2026-07-27)
- [x] Index all `docs/swarm/perf-*.md` reports in this file
