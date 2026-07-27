# P0 Roadmap — Hive Performance
Ordered for parallel workstreams. Status reflects `hive-native-features` portable progress.

| ID | Category | Status | Title |
|----|----------|--------|-------|
| 1 | storage | todo | Explicit template instantiation of top chainbase::generic_index<> specializations in dedicated .cpp files |
| 2 | storage | todo | Split types.hpp into types_fwd.hpp / types_basic.hpp / types_operations.hpp to cut header tax |
| 3 | storage | todo | Extern-template declarations for all fc::static_variant operation serializations |
| 4 | storage | portable-prototype | Replace Boost.MultiIndex secondary indexes with absl::flat_hash_map or open-addressing for hot lookups |
| 6 | storage | todo | Hybrid mode: consensus objects in SHM chainbase; non-consensus indexes in RocksDB + LRU |
| 13 | storage | todo | Differential / incremental state snapshots (only changed objects) |
| 23 | storage | portable-prototype | RocksDB block cache sized to 20–30% of free RAM |
| 32 | storage | portable-prototype | Fast concurrent integrity (xxHash) without blocking readers |
| 43 | storage | todo | Selective undo: track only changed fields not whole objects |
| 63 | storage | portable-prototype | Primary-key caching for get_account / find_account |
| 72 | storage | portable-prototype | Per-thread arena allocators for temporary full_transaction objects |
| 151 | apply | portable-prototype | Expand blockchain_worker_thread_pool to independent op evaluation via dependency analysis |
| 152 | apply | portable-prototype | Add apply priority queue to existing high/medium/low lock-free queues |
| 153 | apply | portable-prototype | Dynamic thread-pool sizing from hardware_concurrency and load |
| 173 | apply | todo | Fast path for empty or low-operation-count blocks |
| 176 | apply | todo | Cache get_dynamic_global_properties results inside a single block |
| 178 | apply | todo | Inline the hottest evaluators (vote, transfer, custom_json) |
| 179 | apply | todo | Pre-compute authorities for repeated signers inside a block |
| 184 | apply | todo | Reduce shared_ptr copies on the apply path |
| 185 | apply | todo | Aggressive move semantics for full_transaction |
| 199 | apply | todo | Parallel pre-validation of mempool transactions |
| 204 | apply | portable-prototype | Bloom / compact-set for is_known_transaction |
| 205 | apply | todo | Separate validation thread pool from apply |
| 209 | apply | todo | SIMD / NEON acceleration for vote-weight and common arithmetic |
| 210 | apply | todo | Faster Merkle-root and block-ID computation |
| 216 | apply | partial-portable | Batch RC deduction / refund |
| 219 | apply | portable-prototype | Arena / pool allocation for all temporary apply data |
| 301 | p2p | todo | Compact-block relay using short transaction IDs + fill requests |
| 303 | p2p | todo | Optional QUIC transport (msquic / quiche) with TCP fallback |
| 323 | p2p | todo | Headers-first then body-on-demand for light peers |
| 330 | p2p | todo | Unix-domain sockets for co-located HAF + hived |
| 421 | concurrency | todo | Fine-grained per-index RW locks |
| 424 | concurrency | portable-prototype | Lock-free hash for hottest account lookups |
| 547 | plugins_haf | todo | Zero-copy JSON (simdjson / RapidJSON) |
| 691 | mobile | design | `HIVE_LIGHT_NODE` CMake option that strips non-essential plugins and indexes at compile time |
| 701 | mobile | todo | Personal-node mode (own + followed accounts only) |
| 728 | mobile | todo | < 4 GB RAM benchmark suite |
| 791 | build_ci | todo | Default PGO + LTO + mold/lld in release builds |
| 794 | build_ci | todo | Continuous micro-benchmarks + regression gates in CI |
| 805 | build_ci | todo | Modular CMake so light builds compile only needed plugins |
| 891 | rc_deploy | portable-prototype | Dynamically calibrate RC costs to measured wall-time / CPU / I/O |
| 894 | rc_deploy | portable-prototype | Formal cost model that accounts for actual measured resource use |
| 896 | rc_deploy | todo | Faster signature verification batching / SIMD |
| 911 | rc_deploy | in-progress | WASM sandbox for future smart-contract / plugin evaluation (roadmap-aligned) |
