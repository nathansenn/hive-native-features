# Hive Performance Program (1000 optimizations)

As of **2026-07-26**. Catalogue grounded in hived realities (chainbase, worker pool, RocksDB, build analysis, HF28, HAF/mobile roadmap).

## Files

| [HIVED_DEVELOPMENT_REVIEW_2026-07.md](./HIVED_DEVELOPMENT_REVIEW_2026-07.md) | Live clone review + top 100 grounded in openhive-network/hive |

| File | Purpose |
|------|---------|
| [HIVE_1000_OPTIMIZATIONS.md](./HIVE_1000_OPTIMIZATIONS.md) | Full 1000-item catalogue by category |
| [HIVE_1000_OPTIMIZATIONS.json](./HIVE_1000_OPTIMIZATIONS.json) | Machine-readable tracking |
| [HIVE_1000_OPTIMIZATIONS.csv](./HIVE_1000_OPTIMIZATIONS.csv) | Spreadsheet import |
| [P0_ROADMAP.md](./P0_ROADMAP.md) | Highest-leverage subset |

## Portable prototypes in this repo

Implemented under `include/hive_native/perf/` and tested by `hive_native_perf_tests`:

| Item IDs | Module | What |
|----------|--------|------|
| #32 #84 | `xxhash64.hpp` | Fast integrity hash |
| #4 #63 #424 | `flat_hash_map.hpp` | Open-addressing hot lookups |
| #72 #219 | `arena.hpp` | Bump-pointer apply arena |
| #204 | `bloom.hpp` | Known-tx style bloom |
| #151 | `op_dependency.hpp` | Parallel apply schedule |
| #152 #153 | `worker_pool.hpp` | Priority queues + pool |
| #891 #894 | `rc_calibrator.hpp` | Measure → RC mapping |
| #23–#28 (#103–#105) | `rocksdb_presets.hpp` | Witness / mobile / archive RocksDB presets |

See also [ROCKSDB_PRESETS.md](./ROCKSDB_PRESETS.md).

## Build / test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/hive_native_perf_tests
```

## Upstream mapping

See [docs/07-upstream-integration.md](../07-upstream-integration.md). P0 patches targeting `gitlab.syncad.com/hive/hive` should reference catalogue IDs in commit messages: `perf(#151): dependency-aware apply sketch`.

## Constraints never violated

- ~3 s block time headroom  
- Light / pruned / mobile skip paths  
- RC metering honesty  
- No consensus change without HF + human gate

## Development review (live clone)

See **[HIVED_DEVELOPMENT_REVIEW_2026-07.md](./HIVED_DEVELOPMENT_REVIEW_2026-07.md)** for the shallow-clone review of `openhive-network/hive` @ 1.28.7, build-analysis findings, and the prioritized top-100 list mapped to catalogue IDs.
