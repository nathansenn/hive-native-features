# RocksDB presets (catalogue #23–#28, packaged as #103–#105)

**Status:** portable-prototype (header-only)  
**Module:** [`include/hive_native/perf/rocksdb_presets.hpp`](../../include/hive_native/perf/rocksdb_presets.hpp)  
**Tests:** `hive_native_rocksdb_preset_tests`  
**As of:** 2026-07-27  

Portable configuration structs that capture RocksDB tuning for three deployment classes. This repo does **not** link RocksDB; values are applied upstream in `account_history_rocksdb` / `external_storage` (or equivalent) via `rocksdb::Options` / column-family options.

---

## Catalogue coverage

| ID | Priority | Title | Field / mechanism |
|----|----------|-------|-------------------|
| **#23** | P0 | Block cache sized to 20–30% of free RAM | `block_cache_mb` + `block_cache_mb_from_free_ram()` |
| **#24** | P1 | Bloom filters (10 bits/key) on account & comment CFs | `bloom_bits_per_key = 10` |
| **#25** | P1 | `write_buffer_size` 64–128 MB during replay / bulk load | `write_buffer_mb` |
| **#26** | P1 | ZSTD cold, LZ4 hot | `compression_hot` / `compression_cold` |
| **#27** | P1 | Universal compaction for history-like CFs | `compaction_style = universal` (archive) |
| **#28** | P1 | Level-0 compaction trigger tuned for SSD write amp | `level0_file_num_compaction_trigger` |

Named packages (also catalogue #103–#105):

| Preset | Catalogue | Role |
|--------|-----------|------|
| `witness_nvme` | #103 | Block producers / full nodes on NVMe |
| `mobile_emmc` | #104 | Light / personal / ARM on eMMC |
| `archive_hdd` | #105 | History / archive on HDD |

---

## Struct

```cpp
struct rocksdb_preset {
   std::string_view name;
   uint32_t block_cache_mb;              // #23
   uint32_t write_buffer_mb;             // #25
   rocksdb_compression compression_hot;  // #26
   rocksdb_compression compression_cold; // #26
   uint32_t bloom_bits_per_key;          // #24
   rocksdb_compaction compaction_style;  // #27
   int level0_file_num_compaction_trigger; // #28
};
```

Helpers: `find_rocksdb_preset(name)`, `preset_sane(p)`, `compression_name` / `compaction_name`, `block_cache_mb_from_free_ram(free_mb)`.

---

## Preset table

| Field | `witness_nvme` | `mobile_emmc` | `archive_hdd` |
|-------|---------------:|--------------:|--------------:|
| `block_cache_mb` | 2048 | 64 | 512 |
| `write_buffer_mb` | 128 | 16 | 256 |
| `compression_hot` | LZ4 | ZSTD | LZ4 |
| `compression_cold` | ZSTD | ZSTD | ZSTD |
| `bloom_bits_per_key` | 10 | 10 | 10 |
| `compaction_style` | level | level | **universal** |
| `level0_file_num_compaction_trigger` | 4 | 2 | 8 |

### Rationale

**witness_nvme** — Latency-first. Large block cache keeps hot account/comment keys in DRAM; 128 MiB write buffer matches bulk-load band (#25 upper); LZ4 on hot levels keeps CPU cheap under apply pressure; L0 trigger 4 lowers write amplification on SSD (#28).

**mobile_emmc** — Memory- and flash-wear constrained. Tiny cache and write buffer; ZSTD on both hot and cold to shrink on-disk footprint; L0 trigger 2 reduces stalled flushes on slow eMMC; still 10-bit bloom so point lookups do not thrash flash with full SST scans (#24).

**archive_hdd** — Throughput and space on spinning media. Large write buffer batches sequential writes; universal compaction for history-shaped write-once CFs (#27); higher L0 trigger tolerates bulk ingest before expensive seeks; ZSTD cold for long-term density.

---

## Dynamic block cache (#23)

Static presets pin a fixed MiB value for CI determinism. Production nodes should size cache from free RAM:

```text
cache_mb = clamp(0.25 * free_ram_mb, min=64, max=8192)
```

`block_cache_mb_from_free_ram(free_ram_mb)` implements the 25% midpoint of the catalogue’s 20–30% band. Witnesses with fixed RAM budgets may still prefer the static `witness_nvme` value.

---

## Upstream application sketch

Map fields to RocksDB (illustrative; not compiled here):

| Portable field | RocksDB option |
|----------------|----------------|
| `block_cache_mb` | `NewLRUCache(mb << 20)` → `BlockBasedTableOptions::block_cache` |
| `write_buffer_mb` | `Options::write_buffer_size = mb << 20` |
| `compression_hot` | `Options::compression` / per-level `[0..n]` |
| `compression_cold` | `Options::bottommost_compression` or lower-level array |
| `bloom_bits_per_key` | `BlockBasedTableOptions::filter_policy = NewBloomFilterPolicy(bits)` |
| `compaction_style` | `Options::compaction_style` (`kCompactionStyleUniversal` etc.) |
| `level0_file_num_compaction_trigger` | `Options::level0_file_num_compaction_trigger` |

Recommended CF policy:

- **Account / comment** (point-lookup heavy): bloom on, level compaction, LZ4 hot.
- **Account history / market history** (append-heavy): universal or FIFO+TTL, ZSTD cold (#27, #131–#132).
- **Replay / bulk load**: temporarily raise `write_buffer_mb` toward 64–128 (or archive 256) and consider disabling WAL (#97) under operator control.

---

## Build / test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_rocksdb_preset_tests
./build/hive_native_rocksdb_preset_tests
ctest --test-dir build -R rocksdb_preset --output-on-failure
```

---

## Non-goals (this portable pack)

- Linking or vendoring RocksDB  
- Shipping production `hived` config files  
- Changing consensus or SHM chainbase layout (#6 hybrid mode is separate)  
- Prometheus stats exposure (#30)

---

## Related

- Catalogue: [HIVE_1000_OPTIMIZATIONS.md](./HIVE_1000_OPTIMIZATIONS.md) rows 23–28, 103–105  
- P0 roadmap: [P0_ROADMAP.md](./P0_ROADMAP.md) (#23)  
- Swarm report: [docs/swarm/perf-08-rocksdb.md](../swarm/perf-08-rocksdb.md)  
- Live clone review: [HIVED_DEVELOPMENT_REVIEW_2026-07.md](./HIVED_DEVELOPMENT_REVIEW_2026-07.md) (RocksDB externalization / plugins)
