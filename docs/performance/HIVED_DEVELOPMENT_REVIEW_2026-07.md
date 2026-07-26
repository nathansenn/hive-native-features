# Hive (hived) Development Review + Optimization Program

**Date:** 2026-07-26  
**Clone:** shallow `openhive-network/hive` @ `1584099` (`bump mainnet HIVE_BLOCKCHAIN_VERSION to 1.28.7`)  
**Primary upstream:** [gitlab.syncad.com/hive/hive](https://gitlab.syncad.com/hive/hive) (`develop`)  
**GitHub mirror:** [openhive-network/hive](https://github.com/openhive-network/hive)  
**Related work:** this repo’s [1000-item catalogue](./HIVE_1000_OPTIMIZATIONS.md) + [P0 roadmap](./P0_ROADMAP.md)

---

## 1. Clone status

| Item | Result |
|------|--------|
| Method | `git clone --depth 1` of GitHub mirror |
| Path | `/Users/commander/hive-sources/hive` (~89 MB tree) |
| HEAD | `1584099` — mainnet version **1.28.7** |
| Full history / mainnet replay | Not practical here (~600 GB disk, ~12 GB+ SHM) |
| Reviewed in-tree | `HIVED_BUILD_ANALYSIS.md`, worker pool, external RocksDB storage, protocol ops, plugins, Docker/ARM scripts, IPv6 P2P paths |

---

## 2. What development is doing (as of July 2026)

### 2.1 Version & hardforks

- **Blockchain version:** `1.28.7` (config also carries `1.28.6` paths for non-mainnet defines).
- **HF28** present (`libraries/protocol/hardfork.d/1_28.hf`, `HIVE_HARDFORK_1_28_BLOCK`).
- HF28 themes (public roadmap + tree): voting-power overhaul, key hierarchy, RC/vesting/price stabilization work ongoing in plugin/test space.

### 2.2 Active engineering themes (evidence in tree)

| Theme | Evidence in clone |
|-------|-------------------|
| **Build cost / template bloat** | `HIVED_BUILD_ANALYSIS.md` (2026-01-06): `generic_index<>` **181 s**, `types.hpp` header tax, `fc::static_variant` ~750 ms/op × 66 ops; `further-build-optimizations.txt`, `another-build-improvement.txt`, split evaluators (`hive_evaluator_{account,social,transfer}.cpp`) |
| **Worker pool (P2P prework)** | `blockchain_worker_thread_pool` — compress/decompress, deser, sig validation; **0 threads = inline**; **does not yet evaluate ops in parallel** |
| **RocksDB externalization** | `external_storage/` comment archive + snapshot; `account_history_rocksdb` plugin |
| **Block log compression** | `block_log_compression.hpp`, dictionaries, artifacts |
| **ARM / mobile prep** | `scripts/build_arm.sh`, `run_arm.sh`, multi-arch Docker base (`ubuntu24.04`), static-link aarch64 checks |
| **IPv6 dual-stack P2P** | New message types 5018–5022, `peer_supports_ipv6`, dual hello in `libraries/net/node.cpp` |
| **Plugin surface** | account_history_rocksdb, state_snapshot, colony, queen, pacemaker, reputation, webserver, witness, … |
| **Test quality** | `REPORT.md` flaky-test campaign (RC mana tolerances, msgspec warmup, colony thresholds) |
| **Dev velocity tools** | IYWU/Clang analysis docs, CLAUDE workflow notes, renovate |

### 2.3 Architecture (constraints that drive optimisations)

```
P2P / API
   → full_block / full_transaction (+ worker pool precompute)
   → chain apply (evaluators, multi_index chainbase SHM)
   → plugins (history RocksDB, snapshot, HAF sql_serializer out-of-tree)
   → block_log (compressed eras)
```

| Resource | Ballpark | Implication |
|----------|----------|-------------|
| Shared memory state | ~12 GB+ | Mobile / personal node blocker |
| Full disk | ~600 GB | Needs prune + HAF split |
| Block time | ~3 s | Apply must leave headroom |
| Ops | ~66 + many virtual | static_variant + evaluator cost |

### 2.4 Health assessment

**Healthy, quality-first development** — not a reckless TPS rewrite.  
Roadmap force-function: **ARM/mobile full nodes + pruned HAF** → RAM, binary size, prune, and apply latency are first-class.  
Highest leverage: finish what build analysis already measured; extend worker pool into **dependency-aware apply**; deepen hybrid RocksDB; ship light build profiles.

---

## 3. Top 100 recommendations (prioritized, code-grounded)

Full **1000** live in `HIVE_1000_OPTIMIZATIONS.*`. Below is the **actionable top 100** mapped to clone evidence and catalogue IDs.

### A. Storage & state (1–15)

| # | Recommendation | Catalogue | Clone hook |
|---|----------------|-----------|------------|
| 1 | Explicit instantiate top `generic_index<>` in .cpp | #1 | Build analysis 181 s |
| 2 | Split `types.hpp` → fwd/basic/ops | #2 | Header tax ~86 s class |
| 3 | Extern-template all op `static_variant` ser | #3 | ~750 ms × 66 ops |
| 4 | flat_hash / open-address for account-by-name | #4 | MultiIndex hot path |
| 5 | Bit-pack `account_object` / `comment_object` | #5 | SHM size |
| 6 | Hybrid: consensus SHM + RocksDB non-consensus | #6 | `external_storage/` exists |
| 7 | Comment age prune on light | #7 | comment RocksDB archive |
| 8 | Drop secondary indexes on mobile | #8 | index-*.cpp pattern |
| 9 | Virtual-op retention window light mode | #9 | history plugins |
| 10 | Activity-score history prune | #10 | AH RocksDB |
| 11 | Adaptive SHM + huge pages | #11–12 | chainbase mmap |
| 12 | Differential snapshots | #13 | `state_snapshot` plugin |
| 13 | Parallel snapshot gen | #14 | worker pool expand |
| 14 | zstd dictionary snapshots | #15 | block_log dicts exist |
| 15 | RocksDB mobile/NVMe presets | #23–28 | vendor rocksdb |

### B. Apply & transactions (16–30)

| # | Recommendation | Catalogue | Clone hook |
|---|----------------|-----------|------------|
| 16 | **Dependency-graph parallel op eval** | #151 | pool currently P2P-only |
| 17 | Codegen / table dispatch for ops | #193 | static_variant cost |
| 18 | Batch RC mana | #216 | RC plugin |
| 19 | Parallel mempool pre-validate | #199 | pool data_source API |
| 20 | Fast paths vote/custom_json/transfer | #178 #192 | split evaluators already |
| 21 | Lazy virtual ops under load | #194 | virtual_operations |
| 22 | Arena allocators on apply | #219 | pool_allocator_t cost |
| 23 | NEON/SIMD weights + hash | #209 | build_arm.sh |
| 24 | Speculative next block | #220 | fork_database |
| 25 | Faster merkle / block id | #210 | full_block |
| 26 | Cache full_tx across P2P/apply | #211 | full_transaction |
| 27 | Skip re-auth after mempool | #212 | validate paths |
| 28 | PGO de-virtualize evaluators | #213 | build analysis |
| 29 | Operator-visible apply parallelism | #214 | thread_pool_size |
| 30 | Multi-sig micro-opts | #215 | authority |

### C. Networking (31–42)

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 31 | Compact block relay | #301 |
| 32 | Optional QUIC | #303 |
| 33 | Adaptive compression priorities | #305 |
| 34 | Witness-priority links | #307 |
| 35 | Finish IPv6 + ICE/NAT | #308–309 |
| 36 | Batched mempool sync | #315–316 |
| 37 | Better discovery (DHT/DNS-SRV) | #310–311 |
| 38 | Peer scoring + soft ban | #312–313 |
| 39 | Persistent witness channels | #307 |
| 40 | DoS limits preserving throughput | #324–325 |
| 41 | HTTP/3 API | #319 |
| 42 | Mobile keep-alives / duty cycle | #320 #360 |

### D. Parallelism & memory (43–55)

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 43 | Fine-grained / RCU chainbase reads | #421–424 |
| 44 | Pool → snapshot, AH write, HAF serialize | #157–159 |
| 45 | Bump arenas apply-wide | #72 #219 |
| 46 | NUMA + core pin | #156 #164 |
| 47 | Thread-per-core research | #47 |
| 48 | Cut shared_ptr on hot path | #74 #184 |
| 49 | Prefault critical pages | #49 |
| 50 | Broader io_uring | #166 |
| 51 | C++20 coroutines web/P2P | #167 |
| 52 | False-sharing elimination | concurrency pack |
| 53 | Oversubscription control | #53 |
| 54 | Huge-page + mlock SHM | #68 |
| 55 | Leak/fragmentation sampling | #67 |

### E. Plugins / HAF / ecosystem (56–70)

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 56 | Dynamic plugin load/unload | #542 |
| 57 | Binary API / zero-copy | #547–548 |
| 58 | REST/gRPC/GraphQL gateways | #549–550 |
| 59 | Plugin health + metrics | #546 #616 |
| 60 | sql_serializer COPY bulk | #561 |
| 61 | HAF prune + partitions + matviews | #566–570 |
| 62 | Embedded HAF SQLite/DuckDB mobile | #572 |
| 63 | Wax / WorkerBee zero-copy | #573–575 |
| 64 | Light-client / proof SDK | #64 #767 |
| 65 | Redis/read cache plugin | #594 |
| 66 | Deprecate legacy paths | #559 |
| 67 | Plugin sandbox | #585 |
| 68 | CDC / event stream HAF | #582 |
| 69 | Example plugin templates | #588 |
| 70 | L2 inspector hooks | roadmap |

### F. Mobile / ARM (71–82) — roadmap critical

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 71 | `HIVE_LIGHT_NODE` CMake + flags | #691 |
| 72 | LTO + section GC + strip &lt;50 MB | #697–698 |
| 73 | Full NEON/SVE crypto/hash | #695–696 |
| 74 | &lt;2–4 GB RAM modes | #728–729 |
| 75 | Flash-friendly I/O | #733 |
| 76 | Thermal/battery schedule | #704–705 |
| 77 | Headers-first selective sync | #700 #323 |
| 78 | Mobile snapshot bootstrap | #715–716 |
| 79 | Duty-cycled crypto | #712 |
| 80 | Mobile management UI | #714 |
| 81 | aarch64 CI (expand build_arm) | #743 |
| 82 | Resource APIs for OS | #745 |

### G. Build / CI / quality (83–95)

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 83 | Default PGO+LTO+mold/lld | #791 |
| 84 | PCH/modules; cut includes | #792 #2 |
| 85 | Split remaining large TUs | #793 |
| 86 | Microbench + regression CI | #794 #812 |
| 87 | ASan/TSan/UBSan + tidy perf | #795 |
| 88 | Explicit template instantiations | #1 #3 #796 |
| 89 | Reduce Boost → std/absl | #797 #4 |
| 90 | Publish sync/RAM/TPS benches | #798 #834 |
| 91 | Continuous apply/P2P fuzz | #799 |
| 92 | Flamegraph endpoint | #800 |
| 93 | Hardware auto-tuner | #801 |
| 94 | Hot-reload non-consensus cfg | #802 |
| 95 | Perf good-first-issues + examples | #803–804 |

### H. RC / consensus / observability (96–100)

| # | Recommendation | Catalogue |
|---|----------------|-----------|
| 96 | Dynamic RC calibration to wall time | #891 #894 |
| 97 | Faster fork-DB / LIB path | #892 |
| 98 | Prometheus + OTel histograms | #616–617 #98 |
| 99 | Structured async logging | #914 |
| 100 | Formal perf budgets + bounties | #843–844 #915 |

---

## 4. What this repo already prototypes

| Catalogue IDs | Portable code |
|---------------|---------------|
| #4 #63 #424 | `include/hive_native/perf/flat_hash_map.hpp` |
| #32 #84 | `xxhash64.hpp` |
| #72 #219 | `arena.hpp` |
| #151 | `op_dependency.hpp` |
| #152 #153 | `worker_pool.hpp` |
| #204 | `bloom.hpp` |
| #891 #894 | `rc_calibrator.hpp` |
| #911 | contracts null engine + Wasmtime ADR |

Tests: `hive_native_perf_tests` + full NFT/HTLC suite on `main`.

---

## 5. Recommended execution order (next 90 days)

```
Week 1–2   Build: #1 #2 #3 #88 explicit instantiation + types split (dev velocity)
Week 2–4   Measure: #86 #90 microbench gates on apply + account lookup
Week 3–6   Apply: #16 dependency graph on top of blockchain_worker_thread_pool
Week 4–8   State: #6 #7 #8 light profiles + RocksDB presets #15
Week 6–12  Mobile: #71 #72 #74 #81 ARM CI + light CMake
Ongoing    RC #96, P2P compact blocks #31, HAF prune #61
```

### Definition of done (any item)

1. Patch or design linked to catalogue ID  
2. Before/after measurement  
3. No consensus change without HF  
4. Light/pruned/mobile path considered  

---

## 6. Closing

Hive development is **stable, multi-arch aware, and bottleneck-documented**. The highest ROI is not a greenfield rewrite: it is finishing the work the tree already points at—**template instantiation, hybrid storage, worker-pool expansion into apply, and light/ARM profiles**—while the 1000-item catalogue tracks the long tail.

**Sources used:** local shallow clone `openhive-network/hive@1584099`, `HIVED_BUILD_ANALYSIS.md`, headers under `libraries/chain/`, `libraries/net/`, `scripts/build_arm.sh`, plugin layout, and this project’s prior 100/1000 research pass (Grok + Harper + Benjamin + Lucas).
