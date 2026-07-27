# Swarm perf-19 — GitOps report

**Agent:** gitops  
**Branch:** `swarm-perf-p0-impl`  
**Base:** `main`  
**Date:** 2026-07-27  
**Remote:** `origin` (`github.com:nathansenn/hive-native-features.git`)  
**Tip (implementation commit):** `76b6e374d368b4405bc4e78497d34aadd90461ac`  
**Workdir:** `/tmp/hive-native-features`

## Actions performed

1. Waited 180s for concurrent swarm writers to finish.
2. Confirmed `build/` is listed in `.gitignore` (not staged).
3. Staged useful sources and docs only; excluded `build/`.
4. Created implementation commit on `swarm-perf-p0-impl`.
5. Pushed with `git push -u origin swarm-perf-p0-impl` (no force-push).
6. Opened PR against `main` (draft=false).
7. Follow-up commit for this gitops report.

## Commit SHAs

| Full SHA | Short | Subject |
|----------|-------|---------|
| `76b6e374d368b4405bc4e78497d34aadd90461ac` | `76b6e37` | perf: swarm P0 portable implementations with tests |

(Implementation tip at push time; gitops report commit follows.)

## PR

- **URL:** https://github.com/nathansenn/hive-native-features/pull/9
- **Title:** perf: swarm P0 portable implementations with tests
- **Base ← head:** `main` ← `swarm-perf-p0-impl`
- **Draft:** false

## Catalogue items shipped

| Area | Artifacts |
|------|-----------|
| Apply scheduler | `include/hive_native/perf/apply_scheduler.hpp`, `tests/test_apply_scheduler.cpp` |
| Account cache | `include/hive_native/perf/account_cache.hpp`, `tests/test_account_cache.cpp` |
| SIMD math | `include/hive_native/perf/simd_math.hpp`, `tests/test_simd_math.cpp` |
| Known-tx set | `include/hive_native/perf/known_tx_set.hpp`, `tests/test_known_tx.cpp` |
| Light profiles | `include/hive_native/chain/node_profiles.hpp`, `tests/test_node_profiles.cpp` |
| RocksDB presets | `include/hive_native/perf/rocksdb_presets.hpp`, `docs/performance/ROCKSDB_PRESETS.md`, `tests/test_rocksdb_presets.cpp` |
| Selective undo | `include/hive_native/perf/selective_undo.hpp`, `tests/test_selective_undo.cpp` |
| Compact block | `include/hive_native/perf/compact_block.hpp`, `tests/test_compact_block.cpp` |
| Fuzz | `tests/fuzz_htlc_preimage.cpp` |
| Dep stress / CI | `tests/test_op_dependency_stress.cpp`, `.github/workflows/ci.yml` |
| Swarm notes | `docs/swarm/perf-01` … `perf-13` |

## Verification (at push time)

| Check | Result |
|-------|--------|
| Message | `perf: swarm P0 portable implementations with tests` |
| `build/` in index | No |
| Force-push | No |
| Remote tracking | `origin/swarm-perf-p0-impl` |
| Open PR | https://github.com/nathansenn/hive-native-features/pull/9 |

## Related swarm reports

- [perf-01-apply-scheduler.md](./perf-01-apply-scheduler.md)
- [perf-02-account-cache.md](./perf-02-account-cache.md)
- [perf-03-simd.md](./perf-03-simd.md)
- [perf-04-known-tx.md](./perf-04-known-tx.md)
- [perf-05-light-profiles.md](./perf-05-light-profiles.md)
- [perf-06-rc-cal.md](./perf-06-rc-cal.md)
- [perf-08-rocksdb.md](./perf-08-rocksdb.md)
- [perf-09-undo.md](./perf-09-undo.md)
- [perf-10-compact-block.md](./perf-10-compact-block.md)
- [perf-11-dep-stress.md](./perf-11-dep-stress.md)
- [perf-12-ci.md](./perf-12-ci.md)
- [perf-13-fuzz.md](./perf-13-fuzz.md)
