# perf-08 – RocksDB presets (#23–#28)

**Task-ID:** swarm / perf-08-rocksdb  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Scope:** Catalogue #23–#28 document + portable config structs (named packages #103–#105)

---

## Deliverables

| Artifact | Path |
|----------|------|
| Header (presets) | `include/hive_native/perf/rocksdb_presets.hpp` |
| Sanity tests | `tests/test_rocksdb_presets.cpp` |
| Design / operator doc | `docs/performance/ROCKSDB_PRESETS.md` |
| This report | `docs/swarm/perf-08-rocksdb.md` |
| CMake target | `hive_native_rocksdb_preset_tests` |

---

## Catalogue mapping

| ID | Item | Implemented as |
|----|------|----------------|
| #23 | Block cache 20–30% free RAM | `block_cache_mb` per preset + `block_cache_mb_from_free_ram()` |
| #24 | Bloom 10 bits/key | `bloom_bits_per_key = 10` on all presets |
| #25 | write_buffer 64–128 MB bulk | `write_buffer_mb` (witness 128, archive 256, mobile 16) |
| #26 | ZSTD cold / LZ4 hot | `compression_hot` / `compression_cold` enums |
| #27 | Universal compaction history CFs | `archive_hdd.compaction_style = universal` |
| #28 | L0 trigger for SSD write amp | `level0_file_num_compaction_trigger` (NVMe 4, eMMC 2, HDD 8) |

### Named presets

| Name | Intent |
|------|--------|
| `witness_nvme` | High cache, LZ4 hot, level compaction, SSD L0=4 |
| `mobile_emmc` | Small cache/buffer, ZSTD both, L0=2 |
| `archive_hdd` | Large write buffer, universal compact, L0=8 |

---

## Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target hive_native_rocksdb_preset_tests
./build/hive_native_rocksdb_preset_tests
ctest --test-dir build -R rocksdb_preset --output-on-failure
```

**Actual (2026-07-27):** exit 0; `rocksdb_presets_passed=48 failed=0`.  
`ctest -R rocksdb_preset` → 100% passed.

### Checks covered

- Struct fields present on all three presets  
- Relative memory ordering (mobile < archive < witness cache)  
- Bloom bits = 10  
- Compression policy per #26 (mobile ZSTD/ZSTD exception for density)  
- Universal compaction only on archive  
- L0 triggers ordered for media class  
- `find_rocksdb_preset` / `preset_sane` / free-RAM helper clamps  

---

## Constraints

- Header-only; **no RocksDB dependency** in this repo  
- Does not alter consensus, chainbase, or plugin runtime  
- Upstream apply path remains a separate integration task  

---

## Follow-ups (out of scope)

- Wire presets into real `account_history_rocksdb` Options  
- Prometheus RocksDB statistics (#30)  
- Per-CF TTL / hybrid SHM+RocksDB (#6, #131–#134)  
- Replay WAL-off operator flag (#97)
