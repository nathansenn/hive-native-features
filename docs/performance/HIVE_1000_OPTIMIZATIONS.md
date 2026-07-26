# 1000 Ways to Speed Up Hive (hived + ecosystem)

**As of:** 2026-07-26  
**Source:** Grok + Harper + Benjamin + Lucas catalogue, expanded to full 1000 concrete items.  
**Tracking:** `HIVE_1000_OPTIMIZATIONS.json` / `.csv`  

## Priority legend

| Priority | Meaning |
|----------|---------|
| **P0** | Highest leverage on RAM / apply / mobile / build — start now |
| **P1** | Strong impact; schedule in next 1–2 quarters |
| **P2** | Valuable; parallelize with contributors |

**Counts:** P0=44 · P1=344 · P2=612

## Category index

- **storage** — 150 items (1–150)
- **apply** — 150 items (151–300)
- **p2p** — 120 items (301–420)
- **concurrency** — 120 items (421–540)
- **plugins_haf** — 150 items (541–690)
- **mobile** — 100 items (691–790)
- **build_ci** — 100 items (791–890)
- **rc_deploy** — 110 items (891–1000)

---

## storage (1–150)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 1 | P0 | todo | Explicit template instantiation of top chainbase::generic_index<> specializations in dedicated .cpp files |
| 2 | P0 | todo | Split types.hpp into types_fwd.hpp / types_basic.hpp / types_operations.hpp to cut header tax |
| 3 | P0 | todo | Extern-template declarations for all fc::static_variant operation serializations |
| 4 | P0 | todo | Replace Boost.MultiIndex secondary indexes with absl::flat_hash_map or open-addressing for hot lookups |
| 5 | P1 | todo | Object-level bit-packing for account_object and comment_object |
| 6 | P0 | todo | Hybrid mode: consensus objects in SHM chainbase; non-consensus indexes in RocksDB + LRU |
| 7 | P1 | todo | Configurable pruning of comments older than N days on light nodes |
| 8 | P1 | partial-portable | Drop secondary indexes entirely on pruned / mobile nodes |
| 9 | P1 | partial-portable | Virtual-op retention limited to last M blocks in light mode |
| 10 | P1 | todo | Account-history pruning by activity score rather than pure age |
| 11 | P1 | todo | Adaptive shared-file-size with transparent huge-page advice |
| 12 | P1 | todo | madvise(MADV_HUGEPAGE/WILLNEED/DONTNEED) on chainbase regions |
| 13 | P0 | todo | Differential / incremental state snapshots (only changed objects) |
| 14 | P1 | todo | Parallel snapshot generation across independent indexes |
| 15 | P1 | todo | Snapshot format using trained zstd dictionary on mainnet state |
| 16 | P1 | todo | Merkle-of-indexes verification for snapshots |
| 17 | P1 | todo | Multi-threaded index rebuild on snapshot load |
| 18 | P1 | todo | P2P snapshot object-exchange protocol for missing pieces |
| 19 | P1 | todo | Snapshot CDN / IPFS distribution optimized for mobile first-run |
| 20 | P1 | todo | Auto-snapshot on hardfork boundaries or every N blocks |
| 21 | P1 | todo | Omit recoverable data from snapshots to shrink size |
| 22 | P1 | todo | Resume from partial / corrupted snapshot |
| 23 | P0 | todo | RocksDB block cache sized to 20–30% of free RAM |
| 24 | P1 | todo | Bloom filters (10 bits/key) on account and comment column families |
| 25 | P1 | todo | write_buffer_size 64–128 MB during replay / bulk load |
| 26 | P1 | todo | ZSTD for cold data, LZ4 for hot data in RocksDB |
| 27 | P1 | todo | Universal compaction for history-like column families |
| 28 | P1 | todo | Level-0 compaction trigger tuned for lower write amp on SSD |
| 29 | P1 | todo | Separate RocksDB instances/CFs per major plugin |
| 30 | P1 | todo | RocksDB statistics exposed via Prometheus |
| 31 | P1 | todo | Background compaction that yields immediately to block-apply |
| 32 | P0 | portable-prototype | Fast concurrent integrity (xxHash) without blocking readers |
| 33 | P1 | todo | Pure-RAM mode + periodic checkpoint for high-end witnesses |
| 34 | P1 | todo | Compressed block_log eras with per-era dictionaries |
| 35 | P1 | todo | On-the-fly block_log compression on low-priority thread |
| 36 | P1 | todo | O(1) random-access index into block_log by height |
| 37 | P1 | todo | Separate irreversible vs forkable block_log segments |
| 38 | P1 | todo | Erase old block_log after HAF has ingested it |
| 39 | P1 | todo | Memory-map recent block_log for fast catch-up |
| 40 | P1 | todo | xxHash checksums on block_log segments |
| 41 | P1 | todo | Parallel block_log write / sharding |
| 42 | P1 | todo | S3-compatible external object storage for cold segments |
| 43 | P0 | todo | Selective undo: track only changed fields not whole objects |
| 44 | P1 | todo | Compress undo-session data with zstd for deep forks |
| 45 | P1 | todo | Custom concurrent free-list allocator for undo sessions |
| 46 | P1 | todo | Larger chunk sizes in pool_allocator_t for account objects |
| 47 | P1 | todo | Reduce boost::intrusive::pointer_rebind instantiations via type erasure |
| 48 | P1 | todo | Flat containers for small stable indexes (witness schedule) |
| 49 | P1 | todo | State-size estimation API for operator prune level choice |
| 50 | P1 | todo | Auto-prune when free disk falls below threshold |
| 51 | P1 | todo | Differential prune retaining only consensus-critical keys |
| 52 | P1 | todo | Synchronized HAF-side pruning policies |
| 53 | P1 | todo | Snapshot of already-pruned state for instant light-node start |
| 54 | P1 | todo | Benchmark suite quantifying prune vs full trade-offs |
| 55 | P1 | todo | Column-family-per-object-type layout in RocksDB |
| 56 | P1 | todo | Age-based partitioning of RocksDB data |
| 57 | P1 | todo | Concurrent readers via RCU or snapshot isolation on state DB |
| 58 | P1 | todo | mmap only hot objects; cold stay on disk |
| 59 | P1 | todo | Object lifetime analysis to move more data to stack/static |
| 60 | P1 | todo | In-place mutation for comment cashout instead of copy |
| 61 | P1 | todo | Batch multiple balance adjustments into one undo entry |
| 62 | P1 | todo | Avoid temporary string allocations in name lookups |
| 63 | P0 | todo | Primary-key caching for get_account / find_account |
| 64 | P1 | todo | Compress cold objects in-place in SHM with on-access decompress |
| 65 | P1 | todo | Memory-pressure feedback that shrinks caches or triggers prune |
| 66 | P1 | todo | std::pmr polymorphic allocators for protocol objects |
| 67 | P1 | todo | Lightweight allocation sampler in production builds |
| 68 | P1 | todo | Explicit huge-page allocation for shared-memory file |
| 69 | P1 | todo | madvise(MADV_DONTNEED) on cold regions after prune |
| 70 | P1 | todo | Operator scripts that enable transparent huge pages |
| 71 | P1 | todo | Resizable memory-mapped regions without full restart |
| 72 | P0 | todo | Per-thread arena allocators for temporary full_transaction objects |
| 73 | P1 | todo | Lock-free free lists for shared-memory object pools |
| 74 | P1 | todo | Reduce shared_ptr traffic in full_block / full_transaction via views |
| 75 | P1 | todo | Custom deleters that return objects to pools |
| 76 | P1 | todo | Pre-allocate pending-transaction structures |
| 77 | P1 | todo | Small-string optimization awareness for protocol string fields |
| 78 | P1 | todo | Vectorized processing of batches of similar objects |
| 79 | P1 | todo | State DB checksum concurrent with apply |
| 80 | P1 | todo | Optional encrypted-at-rest for state on untrusted hosts |
| 81 | P1 | todo | Adaptive shared-file growth prediction from account creation rate |
| 82 | P1 | todo | Separate hot/cold partitions inside chainbase |
| 83 | P1 | todo | Background compaction threads with strict latency budgets |
| 84 | P1 | portable-prototype | Fast integrity hashes (xxHash) instead of slower CRCs where possible |
| 85 | P1 | todo | Snapshot distribution with progressive download + verification |
| 86 | P1 | todo | Tooling to convert between full and pruned snapshot formats |
| 87 | P1 | todo | Support for multiple concurrent snapshot formats (legacy + new) |
| 88 | P1 | todo | Automatic snapshot integrity self-test on load |
| 89 | P1 | partial-portable | Prune virtual ops by type (keep only high-value) |
| 90 | P1 | todo | Configurable retention windows per object type |
| 91 | P1 | todo | State growth forecasting metrics |
| 92 | P1 | todo | Disk I/O scheduler hints for sequential block_log access |
| 93 | P1 | todo | Wear-leveling-aware write patterns for flash |
| 94 | P1 | todo | Zoned storage / SMR drives for archival block_log |
| 95 | P1 | todo | Expand external storage provider interface |
| 96 | P1 | todo | RocksDB WAL tuning for lower latency |
| 97 | P1 | todo | Disable RocksDB WAL for pure-replay scenarios |
| 98 | P1 | todo | Memory-budgeting coordinating chainbase + RocksDB + plugins |
| 99 | P1 | todo | Automatic fallback from SHM to process memory on low-RAM devices |
| 100 | P1 | todo | Compressed undo history for rare deep-reorg case |
| 101 | P2 | todo | Per-index bit-packing strategy for comment_index |
| 102 | P2 | todo | Per-index bit-packing for account_authority_object |
| 103 | P2 | todo | RocksDB preset: NVMe witness (high cache, LZ4 hot) |
| 104 | P2 | todo | RocksDB preset: eMMC mobile (small cache, ZSTD, lower L0) |
| 105 | P2 | todo | RocksDB preset: HDD archive (large write buffer, universal compact) |
| 106 | P2 | todo | Snapshot differential encoding: XOR of packed objects |
| 107 | P2 | todo | Snapshot differential encoding: content-defined chunking |
| 108 | P2 | todo | Snapshot differential encoding: index-level CRC + delta pages |
| 109 | P2 | todo | Prune policy: social data (comments/votes) aggressive |
| 110 | P2 | todo | Prune policy: financial data (balances/transfers) conservative |
| 111 | P2 | todo | Prune policy: NFT objects retain owners-only on light nodes |
| 112 | P2 | todo | Prune policy: HTLC open-only on pruned nodes |
| 113 | P2 | todo | Allocator pool auto-tune from object size histograms |
| 114 | P2 | todo | Allocator pool size auto-tune from allocation rate |
| 115 | P2 | todo | SHM region split: accounts hot / comments warm / history cold |
| 116 | P2 | todo | Object packing: pack bools into bitfields in account_object |
| 117 | P2 | todo | Object packing: fixed-width asset fields |
| 118 | P2 | todo | Object packing: interned account_name_type ids |
| 119 | P2 | todo | Object packing: 32-bit relative block heights where safe |
| 120 | P2 | todo | Index rebuild skip flags for non-consensus secondary keys |
| 121 | P2 | todo | Light snapshot: accounts + global props only |
| 122 | P2 | todo | Light snapshot: + open HTLCs + NFT owners |
| 123 | P2 | todo | Archive snapshot: full history with external blob refs |
| 124 | P2 | todo | Chainbase page-fault tracing for cold/hot classification |
| 125 | P2 | todo | Prefetch next N account objects on transfer batch |
| 126 | P2 | todo | Prefetch comment tree on cashout window |
| 127 | P2 | todo | Prefetch witness schedule each block |
| 128 | P2 | todo | SHM fragmentation defrag offline tool |
| 129 | P2 | todo | SHM fragmentation online compact on prune |
| 130 | P2 | todo | RocksDB secondary instance for read-only API nodes |
| 131 | P2 | todo | RocksDB CF: account_history with TTL |
| 132 | P2 | todo | RocksDB CF: market_history with TTL |
| 133 | P2 | todo | RocksDB CF: block_log index |
| 134 | P2 | todo | RocksDB CF: contract storage (plugin) |
| 135 | P2 | todo | xxHash64 for object integrity in tests |
| 136 | P2 | todo | xxHash3 for snapshot piece verification |
| 137 | P2 | todo | CRC32C hardware for block_log if available else xxHash |
| 138 | P2 | todo | Snapshot piece size tuned to 4MB mobile MTU path |
| 139 | P2 | todo | Snapshot piece size tuned to 64MB server CDN |
| 140 | P2 | todo | P2P snapshot piece rarity-first download |
| 141 | P2 | todo | Prune dry-run mode reporting reclaimable bytes |
| 142 | P2 | todo | Prune by irreversible height watermark |
| 143 | P2 | todo | Prune coordinated with last HAF irreversible |
| 144 | P2 | todo | State size estimator by multi_index sizeof * count |
| 145 | P2 | todo | State size estimator sampling average object length |
| 146 | P2 | todo | Cold object compress threshold auto-tune |
| 147 | P2 | todo | madvise SEQUENTIAL on block_log replay |
| 148 | P2 | todo | madvise RANDOM on random account lookups mmap |
| 149 | P2 | todo | Transparent huge pages only on SHM file not process heap |
| 150 | P2 | todo | Fallback 2MB explicit huge pages if THP unavailable |

## apply (151–300)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 151 | P0 | portable-prototype | Expand blockchain_worker_thread_pool to independent op evaluation via dependency analysis |
| 152 | P0 | todo | Add apply priority queue to existing high/medium/low lock-free queues |
| 153 | P0 | todo | Dynamic thread-pool sizing from hardware_concurrency and load |
| 154 | P1 | todo | Work-stealing queues in addition to lock-free design |
| 155 | P1 | todo | Separate pools for crypto, I/O, apply, and indexing |
| 156 | P1 | todo | CPU affinity for worker threads |
| 157 | P1 | todo | Expand pool coverage to account_history_rocksdb writes |
| 158 | P1 | todo | Expand pool to background state-snapshot generation |
| 159 | P1 | todo | Expand pool to sql_serializer batching |
| 160 | P1 | todo | Task batching of small operations into larger work items |
| 161 | P1 | todo | Cancellation of low-priority tasks when high-priority work arrives |
| 162 | P1 | todo | Queue-depth / wait-time / task-latency metrics per priority |
| 163 | P1 | todo | Inline fallback when the pool is saturated |
| 164 | P1 | todo | NUMA-local work queues |
| 165 | P1 | todo | Priority inheritance for critical-path tasks |
| 166 | P1 | todo | Hybrid io_uring + thread-pool completion |
| 167 | P1 | todo | C++20 coroutine tasks on top of the pool |
| 168 | P1 | todo | Speculative parallel validation of multiple candidate blocks |
| 169 | P1 | todo | Parallel independent multi-index updates after apply |
| 170 | P1 | todo | Parallel plugin notify handlers with isolation |
| 171 | P1 | todo | Configurable max concurrency for internal apply phases |
| 172 | P1 | todo | Specialize _apply_block for skip-flag combinations at compile time |
| 173 | P0 | todo | Fast path for empty or low-operation-count blocks |
| 174 | P1 | todo | Batch process_comment_cashout across comments |
| 175 | P1 | todo | Parallelize independent process_funds / process_conversions |
| 176 | P0 | todo | Cache get_dynamic_global_properties results inside a single block |
| 177 | P1 | todo | Filter inactive plugins before notify_pre_apply / post_apply |
| 178 | P0 | todo | Inline the hottest evaluators (vote, transfer, custom_json) |
| 179 | P0 | todo | Pre-compute authorities for repeated signers inside a block |
| 180 | P1 | todo | Priority-queue data structures for clear_expired_* methods |
| 181 | P1 | todo | Lazy retally_witness_votes only when required |
| 182 | P1 | todo | Incremental hardfork application |
| 183 | P1 | todo | Skip header validation for locally produced blocks |
| 184 | P0 | todo | Reduce shared_ptr copies on the apply path |
| 185 | P0 | todo | Aggressive move semantics for full_transaction |
| 186 | P1 | todo | Specialized fast-confirm vs normal apply paths |
| 187 | P1 | todo | Batch adjust_balance calls inside one transaction |
| 188 | P1 | todo | Calendar indexing for process_recurrent_transfers |
| 189 | P1 | todo | Incremental median structures for update_median_feed |
| 190 | P1 | todo | Lower-cost pop_block via more efficient undo |
| 191 | P1 | todo | Parallel clear_pending where safe |
| 192 | P1 | todo | Profile-driven specialization of the top 10 operation evaluators |
| 193 | P1 | todo | Generated specialized code paths for each of the 66 operations |
| 194 | P1 | todo | Filtered / lazy virtual-op emission under load |
| 195 | P1 | todo | More granular skip flags for replay |
| 196 | P1 | todo | More frequent irreversible checkpoints |
| 197 | P1 | todo | Faster initialize_indexes on startup |
| 198 | P1 | todo | Vectorized processing of batches of identical op types |
| 199 | P0 | todo | Parallel pre-validation of mempool transactions |
| 200 | P1 | todo | Adaptive mempool size limits based on RAM |
| 201 | P1 | todo | Evict lowest-RC transactions first under pressure |
| 202 | P1 | todo | Pre-apply cost estimation for large transactions |
| 203 | P1 | todo | Cache validation results for identical transactions |
| 204 | P0 | portable-prototype | Bloom / compact-set for is_known_transaction |
| 205 | P0 | todo | Separate validation thread pool from apply |
| 206 | P1 | todo | Speculative inclusion of high-confidence transactions |
| 207 | P1 | todo | Optimized parsing for large custom_json |
| 208 | P1 | todo | Specialized post-HF28 paths for claim/create account |
| 209 | P0 | todo | SIMD / NEON acceleration for vote-weight and common arithmetic |
| 210 | P0 | todo | Faster Merkle-root and block-ID computation |
| 211 | P1 | todo | Cache deserialised full_transaction across P2P and apply |
| 212 | P1 | todo | Skip redundant authority checks after successful mempool validation |
| 213 | P1 | todo | Profile-guided inlining and de-virtualization of evaluators |
| 214 | P1 | todo | Configurable apply-parallelism exposed to operators |
| 215 | P1 | todo | Micro-optimizations for common multi-sig patterns |
| 216 | P0 | partial-portable | Batch RC deduction / refund |
| 217 | P1 | todo | Pre-validate transactions more aggressively before inclusion |
| 218 | P1 | todo | Reduce virtual-op generation for low-value ops under load |
| 219 | P0 | portable-prototype | Arena / pool allocation for all temporary apply data |
| 220 | P1 | todo | Speculative execution of the predicted next block |
| 221 | P2 | todo | Specialize evaluator: vote_operation hot path |
| 222 | P2 | todo | Specialize evaluator: transfer_operation hot path |
| 223 | P2 | todo | Specialize evaluator: custom_json_operation hot path |
| 224 | P2 | todo | Specialize evaluator: comment_operation hot path |
| 225 | P2 | todo | Specialize evaluator: limit_order hot path |
| 226 | P2 | todo | Specialize evaluator: claim_account / create_claimed_account post-HF28 |
| 227 | P2 | todo | Specialize evaluator: recurrent_transfer |
| 228 | P2 | todo | Specialize evaluator: escrow ops |
| 229 | P2 | todo | Specialize evaluator: witness_update |
| 230 | P2 | todo | Specialize evaluator: account_update |
| 231 | P2 | todo | Skip flag combo: replay-no-plugins |
| 232 | P2 | todo | Skip flag combo: replay-no-virtual-ops |
| 233 | P2 | todo | Skip flag combo: replay-authority-cached |
| 234 | P2 | todo | Skip flag combo: light-node-no-comments |
| 235 | P2 | todo | Skip flag combo: mobile-headers-only-validate |
| 236 | P2 | todo | Mempool eviction: lowest RC first |
| 237 | P2 | todo | Mempool eviction: oldest first under age cap |
| 238 | P2 | todo | Mempool eviction: largest custom_json first under RAM pressure |
| 239 | P2 | todo | Mempool eviction: non-witness txs first on producer nodes |
| 240 | P2 | todo | Mempool eviction: score = RC_cost / size priority |
| 241 | P2 | todo | SIMD target: x86_64 AVX2 vote weight |
| 242 | P2 | todo | SIMD target: aarch64 NEON vote weight |
| 243 | P2 | todo | SIMD target: batch ED25519/secp if adopted paths |
| 244 | P2 | todo | SIMD target: parallel SHA-256 of tx digests |
| 245 | P2 | todo | SIMD target: parallel RIPEMD160 for address-like hashes |
| 246 | P2 | todo | Apply phase: parallel sig verify only |
| 247 | P2 | todo | Apply phase: parallel JSON parse only |
| 248 | P2 | todo | Apply phase: serial state mutation with dependency graph |
| 249 | P2 | todo | Apply phase: post-apply parallel index maintenance |
| 250 | P2 | todo | Apply phase: post-apply parallel virtual op emit |
| 251 | P2 | todo | Dependency graph: account-level serialization |
| 252 | P2 | todo | Dependency graph: object-id serialization |
| 253 | P2 | todo | Dependency graph: global props single-writer |
| 254 | P2 | todo | Dependency graph: witness schedule single-writer |
| 255 | P2 | todo | Dependency graph: allow parallel distinct NFT transfers |
| 256 | P2 | todo | Dependency graph: allow parallel distinct HTLC redeems |
| 257 | P2 | todo | Inline path: empty block finalize only |
| 258 | P2 | todo | Inline path: single-tx block |
| 259 | P2 | todo | Inline path: all-transfer block vectorized |
| 260 | P2 | todo | Inline path: all-vote block vectorized |
| 261 | P2 | todo | Cache: recent block merkle intermediate nodes |
| 262 | P2 | todo | Cache: witness key set for 21 schedule |
| 263 | P2 | todo | Cache: RC curves per resource type |
| 264 | P2 | todo | Cache: hardfork feature flags bitset |
| 265 | P2 | todo | Arena: per-block bump allocator |
| 266 | P2 | todo | Arena: per-tx bump allocator |
| 267 | P2 | todo | Arena: per-plugin notify allocator |
| 268 | P2 | todo | PGO profile: mainnet apply 1h workload |
| 269 | P2 | todo | PGO profile: social-heavy synthetic |
| 270 | P2 | todo | PGO profile: defi-heavy synthetic |
| 271 | P2 | todo | Bench gate: apply p99 vs transfer baseline |
| 272 | P2 | todo | Bench gate: block full-apply wall time budget |
| 273 | P2 | todo | Bench gate: mempool validate TPS |
| 274 | P2 | todo | Speculative next-block cancel on fork |
| 275 | P2 | todo | Speculative next-block commit only if matches |
| 276 | P2 | todo | Worker pool: apply-priority over snapshot |
| 277 | P2 | todo | Worker pool: crypto-priority over history write |
| 278 | P2 | todo | Worker pool: saturation metrics to Prometheus |
| 279 | P2 | todo | Worker pool: auto-shrink on thermal/mobile |
| 280 | P2 | todo | Worker pool: pin apply threads to big cores |
| 281 | P2 | todo | Worker pool: pin history I/O to little cores |
| 282 | P2 | todo | Virtual op: suppress low-value under load flag |
| 283 | P2 | todo | Virtual op: always emit financial ops |
| 284 | P2 | todo | Virtual op: sample social ops 1:N under load |
| 285 | P2 | todo | Hardfork apply: batch state migrations |
| 286 | P2 | todo | Hardfork apply: background index rebuild |
| 287 | P2 | todo | pop_block: field-level undo when available |
| 288 | P2 | todo | pop_block: parallel free of session nodes |
| 289 | P2 | todo | clear_expired: calendar wheel by second |
| 290 | P2 | todo | clear_expired: calendar wheel by block |
| 291 | P2 | todo | recurrent_transfer: next-due min-heap |
| 292 | P2 | todo | median_feed: sliding window deque |
| 293 | P2 | todo | authority: prehash pubkey ids in block |
| 294 | P2 | todo | authority: cache account auth for block lifetime |
| 295 | P2 | todo | custom_json: size-classed parsers |
| 296 | P2 | todo | custom_json: schema-known id fast path |
| 297 | P2 | todo | Merkle: incremental root with tx stream |
| 298 | P2 | todo | Merkle: BLAKE3 experiment non-consensus research |
| 299 | P2 | todo | Known-tx: cuckoo filter |
| 300 | P2 | todo | Known-tx: blocked-bloom dual filter |

## p2p (301–420)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 301 | P0 | todo | Compact-block relay using short transaction IDs + fill requests |
| 302 | P1 | todo | BIP152-style compact blocks with prefilled high-priority transactions |
| 303 | P0 | todo | Optional QUIC transport (msquic / quiche) with TCP fallback |
| 304 | P1 | todo | UDP unreliable transport for block announcements only |
| 305 | P1 | todo | Adaptive compression (zstd blocks, snappy txs, none for small inv) |
| 306 | P1 | todo | Peer bandwidth estimation and dynamic allocation |
| 307 | P1 | todo | Dedicated high-priority witness-to-witness channels |
| 308 | P1 | todo | Full IPv6 + Happy Eyeballs |
| 309 | P1 | todo | STUN/TURN/ICE for NAT traversal (mobile/residential) |
| 310 | P1 | todo | Kademlia-style DHT for peer discovery |
| 311 | P1 | todo | DNS SRV seeds |
| 312 | P1 | todo | Peer scoring by propagation latency + uptime + version |
| 313 | P1 | todo | Soft-ban / hard-ban for invalid blocks |
| 314 | P1 | todo | Rate-limit inv while allowing unlimited blocks from trusted peers |
| 315 | P1 | todo | Bloom / Golomb-coded set reconciliation for mempool sync |
| 316 | P1 | todo | Batched `get_transactions` |
| 317 | P1 | todo | RC-aware or size-aware transaction relay prioritization |
| 318 | P1 | todo | Connection multiplexing / multiple streams over one transport |
| 319 | P1 | todo | HTTP/3 for API and WebSocket paths |
| 320 | P1 | todo | Mobile-aware keep-alives |
| 321 | P1 | todo | Peer rotation that preserves good peers while avoiding eclipse |
| 322 | P1 | todo | Geographic / latency-based peer preference |
| 323 | P0 | todo | Headers-first then body-on-demand for light peers |
| 324 | P1 | todo | DoS-resistant connection limits per IP/subnet |
| 325 | P1 | todo | Granular message-size limits by type |
| 326 | P1 | todo | Fast reconnect after flaps |
| 327 | P1 | todo | Prometheus metrics for peer latency histograms, bytes, bans |
| 328 | P1 | todo | Configurable max connections scaled by RAM |
| 329 | P1 | todo | Zero-copy fast path for localhost / same-machine P2P |
| 330 | P0 | todo | Unix-domain sockets for co-located HAF + hived |
| 331 | P1 | todo | “I am a mobile node” capability flag |
| 332 | P1 | todo | Reduced heartbeat for long-lived good peers |
| 333 | P1 | todo | Parallel header download for initial sync |
| 334 | P1 | todo | Optional trusted-checkpoint sync mode |
| 335 | P1 | todo | RTT-aware retransmission |
| 336 | P1 | todo | Multipath QUIC / MPTCP support |
| 337 | P1 | todo | Persistent peer reputation across restarts |
| 338 | P1 | todo | External bootstrap / discovery service integration |
| 339 | P1 | todo | Aggressive size limits on relayed `custom_json` |
| 340 | P1 | todo | Separate relay policy for high-volume social ops |
| 341 | P1 | todo | Compressed peer-address lists in handshakes |
| 342 | P1 | todo | Preferred-peers list for operators |
| 343 | P1 | todo | Soft version enforcement / upgrade prompts |
| 344 | P1 | todo | Block-propagation simulation tools |
| 345 | P1 | todo | Built-in chaos testing (drop, delay, partition) |
| 346 | P1 | todo | End-to-end production-to-irreversible latency measurement |
| 347 | P1 | todo | Prefer SSD-backed peers for history |
| 348 | P1 | todo | Content-addressable hints for large payloads |
| 349 | P1 | todo | Zero-RTT QUIC resumption |
| 350 | P1 | todo | “Slow peer” mode that throttles instead of bans |
| 351 | P1 | todo | Dual-stack listening and advertising |
| 352 | P1 | todo | Rate-controlled peer exchange (PEX) |
| 353 | P1 | todo | Optional authenticated channels via Hive keys |
| 354 | P1 | todo | Geographic sharding of connections |
| 355 | P1 | todo | Priority queues for outgoing messages |
| 356 | P1 | todo | Amplification-attack mitigation |
| 357 | P1 | todo | Configurable max blocks-in-flight per peer |
| 358 | P1 | todo | Dynamic push vs pull policy |
| 359 | P1 | todo | Dynamic seed-list updates via API |
| 360 | P1 | todo | Low-power P2P mode (fewer connections, longer intervals) |
| 361 | P1 | todo | Inventory bloom-filter exchange |
| 362 | P1 | todo | Block reconstruction from compact + local mempool |
| 363 | P1 | todo | Latency-based peer selection (<50 ms preferred) |
| 364 | P1 | todo | Bandwidth throttling by peer class |
| 365 | P1 | todo | Automatic pruning of unreachable peers |
| 366 | P1 | todo | WebRTC data channels for browser light nodes |
| 367 | P1 | todo | Message prioritization queues (blocks > txs > history) |
| 368 | P1 | todo | Adaptive outstanding-request windows |
| 369 | P1 | todo | Sybil behavioral detection |
| 370 | P1 | todo | Prefer same-or-newer version peers |
| 371 | P1 | todo | Parallel body download (rarest-first) |
| 372 | P1 | todo | Seed health monitoring and auto-update |
| 373 | P1 | todo | Feature-bit protocol negotiation |
| 374 | P1 | todo | Back-pressure when local apply lags |
| 375 | P1 | todo | Separate ports/interfaces for P2P vs API |
| 376 | P1 | todo | `SO_REUSEPORT` for multi-threaded accept |
| 377 | P1 | todo | `TCP_NODELAY` + tuned buffers for witnesses |
| 378 | P1 | todo | Explicit congestion control (BBR) |
| 379 | P1 | todo | Anomaly-only P2P logging in production |
| 380 | P1 | todo | FEC for lossy links |
| 381 | P1 | todo | Peer capability advertisement |
| 382 | P1 | todo | Automatic best-transport selection per peer |
| 383 | P1 | todo | Rate limits derived from measured peer bandwidth |
| 384 | P1 | todo | Witness-only preferential connectivity mode |
| 385 | P1 | todo | Pre-verification of block signatures in P2P thread |
| 386 | P1 | todo | Efficient varint inventory serialization |
| 387 | P1 | todo | Delta encoding of repeated addresses |
| 388 | P1 | todo | Local-network multicast / discovery |
| 389 | P1 | todo | systemd socket activation |
| 390 | P1 | todo | DSCP / TOS marking for prioritization |
| 391 | P1 | todo | High-bandwidth (10 G+) window tuning |
| 392 | P1 | todo | Middlebox interference detection + fallback |
| 393 | P1 | todo | Proof-of-possession requests for light peers |
| 394 | P1 | todo | Compressed headers-only initial sync |
| 395 | P1 | todo | Parallel multi-source header validation |
| 396 | P1 | todo | Gossip of bad-peer ban lists among trusted nodes |
| 397 | P1 | todo | Private-network authenticated channels |
| 398 | P1 | todo | Orphan / duplicate / bandwidth-efficiency metrics |
| 399 | P1 | todo | Adaptive `max_inv_size` |
| 400 | P2 | todo | Prefer peers with higher last-irreversible |
| 401 | P2 | todo | Archive-node capability flag + preferential history serving |
| 402 | P2 | todo | Load-balanced history requests |
| 403 | P2 | todo | RTT-multiplied timeouts |
| 404 | P2 | todo | Exponential backoff with jitter |
| 405 | P2 | todo | Persistent connection cache |
| 406 | P2 | todo | SOCKS / proxy support |
| 407 | P2 | todo | IPv6-only mode |
| 408 | P2 | todo | Network-simulation test harness |
| 409 | P2 | todo | Chaos-engineering hooks |
| 410 | P2 | todo | Bandwidth documentation per node type |
| 411 | P2 | todo | BDP-based socket-buffer auto-tuning |
| 412 | P2 | todo | Multi-homing / multiple listen addresses |
| 413 | P2 | todo | ASN / regional diversity requirements |
| 414 | P2 | todo | External Prometheus peer exporter |
| 415 | P2 | todo | Compact peer-state structures |
| 416 | P2 | todo | Lazy connection for low-priority peers |
| 417 | P2 | todo | “Push blocks, pull txs” policy |
| 418 | P2 | todo | High-churn mobile peer isolation |
| 419 | P2 | todo | Forward-compatible protocol versioning |
| 420 | P2 | todo | Comprehensive P2P message fuzzing |

## concurrency (421–540)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 421 | P0 | todo | Fine-grained per-index RW locks |
| 422 | P2 | todo | RCU for read-mostly indexes (account name, etc.) |
| 423 | P2 | todo | Epoch-based reclamation |
| 424 | P0 | todo | Lock-free hash for hottest account lookups |
| 425 | P2 | todo | Seqlocks for simple global properties |
| 426 | P2 | todo | Prepare data outside locks to shrink critical sections |
| 427 | P2 | todo | Lock-free queues for plugin handler dispatch |
| 428 | P2 | todo | Strict lock hierarchy documentation + enforcement |
| 429 | P2 | todo | TSan-clean CI builds |
| 430 | P2 | todo | Optional single-threaded mode for debug / low-core |
| 431 | P2 | todo | Concurrent multi-peer block validation with fork-DB isolation |
| 432 | P2 | todo | Async `flush_to_all_storages` with completion |
| 433 | P2 | todo | Concurrent snapshot readers |
| 434 | P2 | todo | Lock-free pending-transaction index where feasible |
| 435 | P2 | todo | Atomic counters for head-block / simple stats |
| 436 | P2 | todo | Hazard pointers for safe concurrent free |
| 437 | P2 | todo | Hardware transactional memory experiments |
| 438 | P2 | todo | Reduced atomic ref-count traffic |
| 439 | P2 | todo | Document concurrent-access rules for plugin authors |
| 440 | P2 | todo | RCU for account-by-name index reads |
| 441 | P2 | todo | RCU for witness schedule snapshot |
| 442 | P2 | todo | Seqlock for dynamic_global_property_object |
| 443 | P2 | todo | Per-index shared_mutex replacing global chain lock slices |
| 444 | P2 | todo | False-sharing elimination on atomic counters |
| 445 | P2 | todo | Cache-line align worker queue nodes |
| 446 | P2 | todo | NUMA topology discovery at startup |
| 447 | P2 | todo | Core-pinning policy JSON config |
| 448 | P2 | todo | Hazard pointer free of index nodes |
| 449 | P2 | todo | Epoch reclamation for multi-index erase |
| 450 | P2 | todo | Lock-free SPSC for plugin virtual ops |
| 451 | P2 | todo | Lock-free MPSC for P2P inbound |
| 452 | P2 | todo | Arena per-thread for temporary strings |
| 453 | P2 | todo | Oversubscription control when tasks > cores |
| 454 | P2 | todo | TSan suppressions audit for false positives |
| 455 | P2 | todo | Document lock order: p2p < mempool < chain < plugins |
| 456 | P2 | todo | Single-thread debug mode CLI flag |
| 457 | P2 | todo | Concurrent fork-DB candidate validation slots |
| 458 | P2 | todo | Async flush completion future API |
| 459 | P2 | todo | Atomic head_block_num with acquire/release |
| 460 | P2 | todo | Reduce shared_ptr atomic ops via intrusive_ptr experiment |
| 461 | P2 | todo | Prepare undo nodes outside write lock |
| 462 | P2 | todo | Batch index insert under single lock acquisition |
| 463 | P2 | todo | Reader-writer for account_history rocksdb |
| 464 | P2 | todo | Copy-on-write page for rare global props updates |
| 465 | P2 | todo | HTM (hardware transactional memory) research gate |
| 466 | P2 | todo | Parallel snapshot readers with refcounted roots |
| 467 | P2 | todo | Pending tx index sharded by txid prefix |
| 468 | P2 | todo | Shard account locks by name hash 256 ways |
| 469 | P2 | todo | Shard comment locks by author/permlink hash |
| 470 | P2 | todo | Work-stealing deques per NUMA node |
| 471 | P2 | todo | Backpressure when apply queue > threshold |
| 472 | P2 | todo | Priority ceiling for irreversible finalize |
| 473 | P2 | todo | Avoid lock in get_dynamic_global_properties if seqlock |
| 474 | P2 | todo | Double-buffer plugin state for concurrent notify |
| 475 | P2 | todo | Memory order audit of atomics |
| 476 | P2 | todo | Pause-less free list for small objects |
| 477 | P2 | todo | Thread-local cache of account_object* for hot names |
| 478 | P2 | todo | Striped locks for RC mana updates |
| 479 | P2 | todo | Striped locks for balances |
| 480 | P2 | todo | Apply thread parking with futex |
| 481 | P2 | todo | io_uring for RocksDB flush completions |
| 482 | P2 | todo | Coroutine transform for async API |
| 483 | P2 | todo | Fiber experiment for plugin handlers |
| 484 | P2 | todo | Bounded channel for sql_serializer batches |
| 485 | P2 | todo | Deadlock detector sampling in debug |
| 486 | P2 | todo | Contention profiler on chainbase mutex |
| 487 | P2 | todo | Lock-free stats counters export |
| 488 | P2 | todo | Read mostly config atomics |
| 489 | P2 | todo | Immutable config snapshot per block |
| 490 | P2 | todo | Plugin isolation process option (research) |
| 491 | P2 | todo | seccomp-filtered plugin threads |
| 492 | P2 | todo | cgroup freeze noncritical plugins under load |
| 493 | P2 | todo | Fair queuing for multi-producer apply |
| 494 | P2 | todo | Eliminate recursive mutex use |
| 495 | P2 | todo | std::shared_mutex timed try for API |
| 496 | P2 | todo | API reads from irreversible snapshot clone |
| 497 | P2 | todo | MVCC research for chainbase (long-term) |
| 498 | P2 | todo | Object version counters for optimistic concurrency |
| 499 | P2 | todo | ABA-safe free list tags |
| 500 | P2 | todo | Pointer compression for 64-bit indexes on <16TB |
| 501 | P2 | todo | Custom offset_ptr packing for SHM |
| 502 | P2 | todo | Avoid false sharing in worker_thread_pool queues |
| 503 | P2 | todo | Queue node pool preallocation |
| 504 | P2 | todo | Spin-then-park policy for short critical sections |
| 505 | P2 | todo | Adaptive spin counts from CPU model |
| 506 | P2 | todo | Yield on mobile when thermal high |
| 507 | P2 | todo | Parallel index iteration for read-only APIs |
| 508 | P2 | todo | Snapshot isolation for database_api get_* |
| 509 | P2 | todo | Copy elision of multi_index iterators |
| 510 | P2 | todo | const-correct read paths |
| 511 | P2 | todo | Thread sanitizer in CI nightly |
| 512 | P2 | todo | Helgrind optional job |
| 513 | P2 | todo | Race detection on fork switch |
| 514 | P2 | todo | Document happens-before for irreversible |
| 515 | P2 | todo | Formal model of lock hierarchy |
| 516 | P2 | todo | Auto-generated lock order tests |
| 517 | P2 | todo | Benchmark: parallel vs serial apply synthetic |
| 518 | P2 | todo | Benchmark: lock contention under 32 threads |
| 519 | P2 | todo | Benchmark: RCU vs mutex account lookup |
| 520 | P2 | todo | Fallback serial apply if parallel detect conflict |
| 521 | P2 | todo | Conflict rate metrics |
| 522 | P2 | todo | Dependency analysis cache across similar blocks |
| 523 | P2 | todo | Static op-class independence table |
| 524 | P2 | todo | Runtime conflict log for developers |
| 525 | P2 | todo | Shard mempool by account |
| 526 | P2 | todo | Parallel RC pre-charge |
| 527 | P2 | todo | Parallel signature check with batch verify |
| 528 | P2 | todo | Wait-free broadcast of new head to APIs |
| 529 | P2 | todo | Eventfd notify for local consumers |
| 530 | P2 | todo | Shared memory ring for HAF handoff |
| 531 | P2 | todo | Zero-copy block view into SHM |
| 532 | P2 | todo | Immutable full_block after seal |
| 533 | P2 | todo | Const transaction_span type |
| 534 | P2 | todo | Reduce atomic refcounts on block seal |
| 535 | P2 | todo | Intrusive list for pending undo |
| 536 | P2 | todo | Epoch for plugin handler generation |
| 537 | P2 | todo | Generational arena reset per block |
| 538 | P2 | todo | Bounded memory for parallel speculative validates |
| 539 | P2 | todo | Cancel speculative work on better head |
| 540 | P2 | todo | Priority: user-RPC over background index |

## plugins_haf (541–690)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 541 | P2 | todo | Deterministic, configurable plugin load order |
| 542 | P2 | todo | Lazy plugin initialization |
| 543 | P2 | todo | Dependency-graph loading of only required plugins |
| 544 | P2 | todo | Hot-reload of non-consensus API plugins |
| 545 | P2 | todo | Validated plugin configuration schema |
| 546 | P2 | todo | Per-plugin latency budgets |
| 547 | P0 | todo | Zero-copy JSON (simdjson / RapidJSON) |
| 548 | P2 | todo | Binary protocol option (Cap’n Proto / FlatBuffers) |
| 549 | P2 | todo | Generated gRPC services from existing APIs |
| 550 | P2 | todo | GraphQL layer over `database_api` |
| 551 | P2 | todo | Connection pooling + keep-alive for JSON-RPC |
| 552 | P2 | todo | Native request batching |
| 553 | P2 | todo | ETag / If-None-Match caching |
| 554 | P2 | todo | RC-aware rate limiting |
| 555 | P2 | todo | Async API handlers (coroutines or worker pool) |
| 556 | P2 | todo | Isolated API thread pool |
| 557 | P2 | todo | gzip/brotli for large responses |
| 558 | P2 | todo | Optimized default pagination |
| 559 | P2 | todo | Gradual `condenser_api` deprecation with migration guide |
| 560 | P2 | todo | Per-method latency metrics |
| 561 | P2 | todo | PostgreSQL `COPY` for bulk virtual ops in `sql_serializer` |
| 562 | P2 | todo | Prepared statements everywhere in serializer |
| 563 | P2 | todo | Parallel workers for different HAF tables |
| 564 | P2 | todo | Configurable batch size / flush interval |
| 565 | P2 | todo | Skip unneeded tables |
| 566 | P2 | todo | Automatic block-range partitioning of HAF tables |
| 567 | P2 | todo | Columnar storage option for analytics |
| 568 | P2 | todo | Incrementally refreshed materialized views |
| 569 | P2 | todo | TimescaleDB (or similar) support |
| 570 | P2 | todo | Declarative pruning policy engine |
| 571 | P2 | todo | Lightweight HAF that omits full history |
| 572 | P2 | todo | Embedded HAF (SQLite / DuckDB) for mobile / single-user |
| 573 | P2 | todo | Cython / Numba acceleration of WorkerBee hot loops |
| 574 | P2 | todo | Zero-copy `memoryview` access in WorkerBee |
| 575 | P2 | todo | Optimized Wax TS/JS + WASM builds |
| 576 | P2 | todo | Clive TUI latency improvements |
| 577 | P2 | todo | Denser incremental UI updates |
| 578 | P2 | todo | Official OpenAPI / Swagger with examples |
| 579 | P2 | todo | More language SDKs with built-in pooling |
| 580 | P2 | todo | API gateway with load balancing across nodes |
| 581 | P2 | todo | Read-replica HAF nodes |
| 582 | P2 | todo | CDC from HAF to Kafka / NATS |
| 583 | P2 | todo | Webhook plugin for new blocks / ops |
| 584 | P2 | todo | Elasticsearch / OpenSearch export plugin |
| 585 | P2 | todo | seccomp / process sandbox for plugins |
| 586 | P2 | todo | cgroup resource limits per plugin |
| 587 | P2 | todo | Automatic disable on plugin health failure |
| 588 | P2 | todo | Benchmarked example-plugin template |
| 589 | P2 | todo | Community plugin registry |
| 590 | P2 | todo | Versioned plugin API |
| 591 | P2 | todo | Reduced registration overhead |
| 592 | P2 | todo | Compile-time plugin selection for light builds |
| 593 | P2 | todo | Shared-library vs static plugin choice |
| 594 | P2 | todo | Simple KV cache plugin for frequent gets |
| 595 | P2 | todo | Node performance-stats API |
| 596 | P2 | todo | Server-Sent Events for block streams |
| 597 | P2 | todo | WebSocket compression + binary frames |
| 598 | P2 | todo | HiveAuth / JWT for APIs without slowing public reads |
| 599 | P2 | todo | Quota system tied to RC or stake |
| 600 | P2 | todo | HAF query-optimizer hints for social patterns |
| 601 | P2 | todo | Pre-aggregated trending / rewards tables |
| 602 | P2 | todo | Full-text search indexes in HAF |
| 603 | P2 | todo | Incremental maintenance of follows / communities |
| 604 | P2 | todo | Multi-backend HAF (Postgres, MySQL, Cockroach) |
| 605 | P2 | todo | Zero-downtime schema migrations |
| 606 | P2 | todo | Realistic social-workload HAF test suite |
| 607 | P2 | todo | Query best-practice documentation |
| 608 | P2 | todo | Client-side caching libraries |
| 609 | P2 | todo | GraphQL subscriptions |
| 610 | P2 | todo | Pre-computed fields to reduce JSON cost |
| 611 | P2 | todo | Response size limits with clear errors |
| 612 | P2 | todo | Parallel independent methods inside a batch |
| 613 | P2 | todo | Session affinity |
| 614 | P2 | todo | Cache-hit-rate metrics |
| 615 | P2 | todo | Auto-scaling guidance for HAF |
| 616 | P2 | todo | Prometheus exporter plugin |
| 617 | P2 | todo | OpenTelemetry tracing across API → apply |
| 618 | P2 | todo | Personal light HAF (only followed accounts) |
| 619 | P2 | todo | Parquet export for offline analytics |
| 620 | P2 | todo | Optimized batch account-history queries |
| 621 | P2 | todo | Virtual-op filtering at serializer |
| 622 | P2 | todo | Dependency injection for testability |
| 623 | P2 | todo | Compile-time plugin registration |
| 624 | P2 | todo | Reduced virtual calls in hooks |
| 625 | P2 | todo | Inline critical callbacks |
| 626 | P2 | todo | Per-plugin thread pools |
| 627 | P2 | todo | Async notification after irreversible |
| 628 | P2 | todo | Testnet RC-cost override plugin |
| 629 | P2 | todo | Comprehensive plugin test framework |
| 630 | P2 | todo | Per-plugin performance regression tests |
| 631 | P2 | todo | Bounty for fastest alternative history plugin |
| 632 | P2 | todo | Official HAF vs old `account_history` benchmarks |
| 633 | P2 | todo | Migration tools from legacy plugins |
| 634 | P2 | todo | HAF sharding across multiple Postgres instances |
| 635 | P2 | todo | Read-your-writes consistency options |
| 636 | P2 | todo | Pre-broadcast resource-cost estimation API |
| 637 | P2 | todo | Optimized legacy `get_content` / `get_discussions` |
| 638 | P2 | todo | Dedicated cache for witness schedule + global props |
| 639 | P2 | todo | Reduce string copies in response building |
| 640 | P2 | todo | API cleanup: deprecate unused condenser methods |
| 641 | P2 | todo | API cleanup: unify pagination tokens |
| 642 | P2 | todo | API cleanup: binary error codes |
| 643 | P2 | todo | HAF engine: DuckDB analytical backend option |
| 644 | P2 | todo | HAF engine: ClickHouse export path |
| 645 | P2 | todo | HAF engine: SQLite personal mode |
| 646 | P2 | todo | SDK: Python asyncio client |
| 647 | P2 | todo | SDK: Rust high-perf client |
| 648 | P2 | todo | SDK: Go client with pooling |
| 649 | P2 | todo | SDK: Kotlin multiplatform |
| 650 | P2 | todo | Realtime: NATS jetstream bridge |
| 651 | P2 | todo | Realtime: Redis streams bridge |
| 652 | P2 | todo | Realtime: WebPush for follows |
| 653 | P2 | todo | GraphQL: dataloader batching |
| 654 | P2 | todo | GraphQL: persisted queries |
| 655 | P2 | todo | OpenAPI: generated clients |
| 656 | P2 | todo | Rate limit: per-IP token bucket |
| 657 | P2 | todo | Rate limit: per-key RC-linked |
| 658 | P2 | todo | Cache: redis for get_accounts |
| 659 | P2 | todo | Cache: CDN for static chain props |
| 660 | P2 | todo | HAF: BRIN indexes on block_num |
| 661 | P2 | todo | HAF: partial indexes for open orders |
| 662 | P2 | todo | HAF: vacuum autotune |
| 663 | P2 | todo | HAF: logical replication recipes |
| 664 | P2 | todo | Webhook: signed payloads |
| 665 | P2 | todo | Webhook: retry with backoff |
| 666 | P2 | todo | Elastic: op index template |
| 667 | P2 | todo | Plugin: healthcheck HTTP |
| 668 | P2 | todo | Plugin: graceful drain |
| 669 | P2 | todo | Plugin: version negotiation |
| 670 | P2 | todo | Plugin: feature flags |
| 671 | P2 | todo | sql_serializer: COPY FROM STDIN binary |
| 672 | P2 | todo | sql_serializer: multi-connection pipeline |
| 673 | P2 | todo | sql_serializer: backpressure to hived |
| 674 | P2 | todo | WorkerBee: vectorized decode |
| 675 | P2 | todo | WorkerBee: arrow export |
| 676 | P2 | todo | Wax: smaller wasm binary |
| 677 | P2 | todo | Clive: virtualized history list |
| 678 | P2 | todo | condenser_api: compatibility shim metrics |
| 679 | P2 | todo | database_api: field masks |
| 680 | P2 | todo | database_api: projection pushdown |
| 681 | P2 | todo | account_history: rocksdb TTL compaction |
| 682 | P2 | todo | market_history: downsample old buckets |
| 683 | P2 | todo | rc_api: cost estimate endpoint |
| 684 | P2 | todo | debug_node: faster skip |
| 685 | P2 | todo | test plugin performance harness |
| 686 | P2 | todo | plugin registry: signed packages |
| 687 | P2 | todo | plugin sandbox: seccomp profile default |
| 688 | P2 | todo | HAF prune: partition drop by month |
| 689 | P2 | todo | HAF: cold storage tier S3 |
| 690 | P2 | todo | Query planner hints doc pack |

## mobile (691–790)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 691 | P0 | design | `HIVE_LIGHT_NODE` CMake option that strips non-essential plugins and indexes at compile time |
| 692 | P1 | todo | Runtime plugin whitelist |
| 693 | P1 | todo | Aggressive “last N days” comment/vote pruning |
| 694 | P1 | todo | Compressed in-memory account representation |
| 695 | P1 | todo | Full ARM NEON acceleration of remaining crypto and hashing |
| 696 | P1 | todo | SVE support for newer ARM cores |
| 697 | P1 | todo | Binary size target < 50 MB via LTO + section GC + `-Os` |
| 698 | P1 | todo | Symbol stripping for mobile builds |
| 699 | P1 | todo | Snapshot format optimized for sequential flash writes |
| 700 | P1 | todo | Header-only + selective body download |
| 701 | P0 | todo | Personal-node mode (own + followed accounts only) |
| 702 | P1 | todo | SQLite-backed HAF for mobile |
| 703 | P1 | todo | Sync only while charging / on Wi-Fi |
| 704 | P1 | todo | Thermal throttling of worker threads |
| 705 | P1 | todo | Battery-level pause of non-critical work |
| 706 | P1 | todo | Android NDK / iOS cross-compile support |
| 707 | P1 | todo | `madvise` sequential / dontneed on mobile block_log |
| 708 | P1 | todo | Smaller undo window (with checkpoints) |
| 709 | P1 | todo | Pure process memory (no shared-file) + periodic serialize |
| 710 | P1 | todo | Simplified RC approximation tables for mobile |
| 711 | P1 | todo | ARM-specific assembly for hot hashes |
| 712 | P1 | todo | Power-efficient sleep between 3 s blocks |
| 713 | P1 | todo | User notification when node falls behind |
| 714 | P1 | todo | Simple mobile management daemon / UI |
| 715 | P1 | todo | Automatic trusted-mirror snapshot on first run |
| 716 | P1 | todo | Efficient delta sync after snapshot |
| 717 | P1 | todo | External SD-card support for block_log |
| 718 | P1 | todo | Local state encryption |
| 719 | P1 | todo | Minimal plugin set for mobile |
| 720 | P1 | todo | Compile-out of all testnet / mirrornet code |
| 721 | P1 | todo | musl or smaller runtime |
| 722 | P1 | todo | ARM-specific PGO |
| 723 | P1 | todo | Compile-time Boost reduction for light builds |
| 724 | P1 | todo | Small-heap-tuned custom allocator |
| 725 | P1 | todo | Periodic explicit free of cold caches |
| 726 | P1 | todo | 32-bit ARM support if still required |
| 727 | P1 | todo | Device-specific documentation (Pi, phones, SBCs) |
| 728 | P0 | todo | < 4 GB RAM benchmark suite |
| 729 | P1 | todo | Auto-detect RAM → choose prune level |
| 730 | P2 | todo | Progressive-web or lightweight control frontend |
| 731 | P2 | todo | Mobile OS background-task integration |
| 732 | P2 | todo | Reduced flash wear from logging |
| 733 | P2 | todo | Wear-leveling-aware block_log writes |
| 734 | P2 | todo | Optional cloud state backup |
| 735 | P2 | todo | Prefer other mobile / low-bandwidth peers |
| 736 | P2 | todo | Mandatory compressed P2P on mobile |
| 737 | P2 | todo | Lower max connections (8–16) |
| 738 | P2 | todo | Duty-cycled P2P |
| 739 | P2 | todo | Fast resume from sleep |
| 740 | P2 | todo | Data-saver mode that skips non-essentials |
| 741 | P2 | todo | big.LITTLE awareness (pin critical threads to big cores) |
| 742 | P2 | todo | Static linking of dependencies |
| 743 | P2 | todo | Regular ARM CI builds and tests |
| 744 | P2 | todo | Size / RAM regression tests in CI |
| 745 | P2 | todo | User-facing sync / RAM / battery metrics |
| 746 | P2 | todo | One-click images for common ARM boards |
| 747 | P2 | todo | Community PostmarketOS / mobile-Linux images |
| 748 | P2 | todo | Pruned HAF retention policies (30/90/365 days) |
| 749 | P2 | todo | On-device indexing only for followed accounts |
| 750 | P2 | todo | Approximate indexes for search |
| 751 | P2 | todo | Lazy account-history load |
| 752 | P2 | todo | View-only mode (no private keys) |
| 753 | P2 | todo | TEE / secure-enclave key storage |
| 754 | P2 | todo | Reduced virtual-op processing |
| 755 | P2 | todo | Skip sig checks below last irreversible when using trusted snapshot |
| 756 | P2 | todo | Configurable trust level for faster sync |
| 757 | P2 | todo | Mobile-friendly seed list |
| 758 | P2 | todo | Adaptive apply rate based on device load |
| 759 | P2 | todo | Pause when user is interacting with other apps |
| 760 | P2 | todo | zram / compressed-swap advice |
| 761 | P2 | todo | `mlock` only the critical working set |
| 762 | P2 | todo | Avoid large stack allocations |
| 763 | P2 | todo | Thread stack-size tuning |
| 764 | P2 | todo | Page-size / huge-page choice for ARM MMU |
| 765 | P2 | todo | Device-specific performance expectations documentation |
| 766 | P2 | todo | Community device config templates |
| 767 | P2 | todo | Long-term light-client with fraud / optimistic proofs |
| 768 | P1 | todo | Battery: coalesce wakeups to block interval |
| 769 | P1 | todo | Battery: batch disk flushes |
| 770 | P1 | todo | Thermal: stepwise reduce thread count |
| 771 | P1 | todo | Thermal: reduce RocksDB compaction |
| 772 | P1 | todo | Flash: fsync policy relaxed on mobile |
| 773 | P1 | todo | Flash: append-only state log option |
| 774 | P1 | todo | Cross-compile: Android ABI arm64-v8a |
| 775 | P1 | todo | Cross-compile: iOS bitcode-free arm64 |
| 776 | P1 | todo | Cross-compile: RISC-V experimental |
| 777 | P1 | todo | Light client: optimistic fraud proof research |
| 778 | P1 | todo | Light client: UTXO-like account proof research |
| 779 | P1 | todo | Light client: witness checkpoint signatures |
| 780 | P1 | todo | Personal node: follow-graph limited state |
| 781 | P1 | todo | Personal node: mute list local only |
| 782 | P1 | todo | Data saver: skip images metadata ops |
| 783 | P1 | todo | Data saver: headers + own txs only |
| 784 | P1 | todo | Wi-Fi only sync toggle |
| 785 | P1 | todo | Background sync OS job (Android WorkManager) |
| 786 | P1 | todo | Background sync OS job (iOS BGTask) |
| 787 | P1 | todo | First-run: streaming snapshot with UI progress |
| 788 | P1 | todo | Resume: crash-safe snapshot cursor |
| 789 | P1 | todo | Key storage: Android Keystore |
| 790 | P1 | todo | Key storage: iOS Secure Enclave |

## build_ci (791–890)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 791 | P0 | todo | Default PGO + LTO + mold/lld in release builds |
| 792 | P1 | todo | Pre-compiled headers / C++20 modules for the hot header chain |
| 793 | P1 | todo | Split remaining large TUs (`database.cpp`, evaluators, etc.) |
| 794 | P0 | todo | Continuous micro-benchmarks + regression gates in CI |
| 795 | P1 | todo | Mandatory ASan / TSan / UBSan + clang-tidy performance checks |
| 796 | P1 | todo | Explicit instantiations for the most expensive templates |
| 797 | P1 | todo | Reduce Boost surface; prefer std / absl |
| 798 | P1 | todo | Published sync-time / RAM / TPS benchmarks with every release |
| 799 | P1 | todo | Continuous protocol / apply / P2P fuzzing |
| 800 | P1 | todo | Built-in continuous profiler / flame-graph endpoint |
| 801 | P1 | todo | Hardware-aware config auto-tuner |
| 802 | P1 | todo | Hot-reload of non-consensus settings |
| 803 | P1 | todo | Better contributor docs and “perf good-first-issue” tags |
| 804 | P1 | todo | Example optimization patches |
| 805 | P0 | todo | Modular CMake so light builds compile only needed plugins |
| 806 | P1 | todo | Cross-compile ease for aarch64 / Android / iOS |
| 807 | P1 | todo | sccache / ccache defaults |
| 808 | P1 | todo | Parallel index instantiation control |
| 809 | P1 | todo | Header-include hygiene enforcement (IWYU already started) |
| 810 | P1 | todo | Specialised assertion macros expanded (already begun) |
| 811 | P1 | todo | Build-analysis documents kept up to date |
| 812 | P1 | todo | CI that fails on > X % regression in key benchmarks |
| 813 | P1 | todo | Synthetic social-workload generator |
| 814 | P1 | todo | Chaos + performance-under-failure tests |
| 815 | P1 | todo | Load-testing scripts for every node type |
| 816 | P1 | todo | Opt-in profiling-data upload for core-team analysis |
| 817 | P1 | todo | “Why is my node slow” diagnostic tool |
| 818 | P1 | todo | Config-recommendation engine from live metrics |
| 819 | P1 | todo | Historical performance trend storage |
| 820 | P2 | todo | Integration with external APM |
| 821 | P2 | todo | Custom metric plugins |
| 822 | P2 | todo | Real-time TUI monitor (htop-style for hived) |
| 823 | P2 | todo | Mobile monitoring companion |
| 824 | P2 | todo | Alerting on deviation from performance budgets |
| 825 | P2 | todo | Configurable trace sampling |
| 826 | P2 | todo | Correlation IDs across P2P → apply → API |
| 827 | P2 | todo | Deadlock / long-lock detection |
| 828 | P2 | todo | Allocation tracing in debug builds |
| 829 | P2 | todo | Leak-detection reports |
| 830 | P2 | todo | Build-time instrumentation of hot functions |
| 831 | P2 | todo | Runtime feature flags for extra metrics |
| 832 | P2 | todo | Full metric documentation |
| 833 | P2 | todo | Shared Grafana dashboards |
| 834 | P2 | todo | Per-release benchmark publication |
| 835 | P2 | todo | Micro-benchmark suite for every evaluator and index |
| 836 | P2 | todo | Linux `perf` / VTune integration helpers |
| 837 | P2 | todo | Customizable metric retention |
| 838 | P2 | todo | Comparison mode (two configs side-by-side) |
| 839 | P2 | todo | State-size growth time-series |
| 840 | P2 | todo | Disk / RAM exhaustion prediction |
| 841 | P2 | todo | Opt-in public witness performance leaderboard |
| 842 | P2 | todo | Tools to replay specific blocks under profiler |
| 843 | P2 | todo | Formal performance budgets per hardfork |
| 844 | P2 | todo | Community performance-bounty / leaderboard program |
| 845 | P2 | todo | CI: clang-tidy modernize checks |
| 846 | P2 | todo | CI: clang-tidy performance checks gate |
| 847 | P2 | todo | CI: include-what-you-use diff |
| 848 | P2 | todo | CI: codespell on docs |
| 849 | P2 | todo | CI: license header check |
| 850 | P2 | todo | CI: SBOM generation |
| 851 | P2 | todo | CI: dependency vulnerability scan |
| 852 | P2 | todo | CI: reproducible build attestation |
| 853 | P2 | todo | Doc template: evaluator performance note |
| 854 | P2 | todo | Doc template: plugin README skeleton |
| 855 | P2 | todo | Doc template: ADR for consensus changes |
| 856 | P2 | todo | Onboarding: docker-compose one-shot testnet |
| 857 | P2 | todo | Onboarding: 15-minute contributor guide |
| 858 | P2 | todo | Onboarding: architecture diagram generator |
| 859 | P2 | todo | Good-first-issue auto-labeler |
| 860 | P2 | todo | Benchmark publish to gh-pages |
| 861 | P2 | todo | Benchmark compare bot on PRs |
| 862 | P2 | todo | Fuzz: libFuzzer corpus seed pack |
| 863 | P2 | todo | Fuzz: AFL++ weekly |
| 864 | P2 | todo | Static analysis: CodeQL queries for chain |
| 865 | P2 | todo | Static analysis: Semgrep rules for undo |
| 866 | P2 | todo | Coverage: critical path 80% gate |
| 867 | P2 | todo | Coverage: protocol ser/de 95% gate |
| 868 | P2 | todo | Release: changelog automation |
| 869 | P2 | todo | Release: multi-arch docker matrix |
| 870 | P2 | todo | Release: signed binaries |
| 871 | P2 | todo | sccache remote cache option |
| 872 | P2 | todo | distcc optional farm |
| 873 | P2 | todo | CMake: preset mobile-light |
| 874 | P2 | todo | CMake: preset witness-maxperf |
| 875 | P2 | todo | CMake: preset haf-api |
| 876 | P2 | todo | Module map experiment for types.hpp |
| 877 | P2 | todo | Unity build option for slow TUs |
| 878 | P2 | todo | Split database.cpp by domain |
| 879 | P2 | todo | Split evaluators into per-op TUs with unity opt |
| 880 | P2 | todo | Header unit BMI cache |
| 881 | P2 | todo | Precommit: format + lint |
| 882 | P2 | todo | Precommit: quick unit subset |
| 883 | P2 | todo | Devcontainer with deps |
| 884 | P2 | todo | Gitpod/Codespaces config |
| 885 | P2 | todo | Example patch: generic_index explicit instantiation |
| 886 | P2 | todo | Example patch: types.hpp split |
| 887 | P2 | todo | Example patch: worker pool apply queue |
| 888 | P2 | todo | Perf working group RFC template |
| 889 | P2 | todo | Bounty brief template |
| 890 | P2 | todo | Monthly perf report template |

## rc_deploy (891–1000)

| ID | Pri | Status | Title |
|----|-----|--------|-------|
| 891 | P0 | portable-prototype | Dynamically calibrate RC costs to measured wall-time / CPU / I/O |
| 892 | P1 | todo | Faster fork-DB and irreversible handling |
| 893 | P1 | todo | RC mana telemetry (usage, regeneration, failures) |
| 894 | P0 | todo | Formal cost model that accounts for actual measured resource use |
| 895 | P1 | todo | Security improvements that enable fewer runtime checks (formal verification of critical paths) |
| 896 | P0 | todo | Faster signature verification batching / SIMD |
| 897 | P1 | todo | Reduced authority recursion depth checks where proven safe |
| 898 | P1 | todo | Checkpoint-based trust modes for faster mobile sync |
| 899 | P1 | todo | Hardware security module / TEE integration paths |
| 900 | P1 | todo | Deployment recipes for Kubernetes / Nomad with resource requests |
| 901 | P1 | todo | Auto-tuning shared-file-size and RocksDB cache from detected hardware |
| 902 | P1 | todo | One-command Docker / binary installers for common platforms |
| 903 | P1 | todo | systemd / service-unit best-practice templates |
| 904 | P1 | todo | cgroup and resource-isolation examples |
| 905 | P1 | todo | Multi-node testnet orchestration tools |
| 906 | P1 | todo | Comparative study and selective adoption of ideas from Solana (parallel execution), Aptos/Sui (object model), other Graphene forks, and modern L2s |
| 907 | P1 | todo | AI / ML assisted hot-path detection and automatic patch suggestion |
| 908 | P1 | todo | Continuous learning of optimal config parameters from fleet telemetry (opt-in) |
| 909 | P1 | todo | Formal methods / model checking of consensus and RC invariants |
| 910 | P1 | todo | Reduced legacy Graphene code paths |
| 911 | P0 | in-progress | WASM sandbox for future smart-contract / plugin evaluation (roadmap-aligned) |
| 912 | P1 | todo | eBPF hooks for advanced monitoring without code changes |
| 913 | P1 | todo | Snapshot + state-diff tools for rapid testnet / mirrornet |
| 914 | P1 | todo | Structured async logging with compile-time levels |
| 915 | P1 | todo | Community process changes that accelerate optimization velocity (bounties, RFCs, dedicated perf working group) |
| 916 | P2 | todo | RC: continuous calibration job from apply traces |
| 917 | P2 | todo | RC: per-op p50/p99 cost tables published |
| 918 | P2 | todo | RC: separate CPU vs storage resource dimensions |
| 919 | P2 | todo | RC: market-history write cost refinement |
| 920 | P2 | todo | RC: custom_json size-squared component |
| 921 | P2 | todo | RC: NFT mint state-creation premium |
| 922 | P2 | todo | RC: HTLC create state premium |
| 923 | P2 | todo | RC: contract fuel hard mapping HF param |
| 924 | P2 | todo | RC: refund unused fuel on success path |
| 925 | P2 | todo | RC: mana regen UI telemetry |
| 926 | P2 | todo | Security-perf: batch signature verify |
| 927 | P2 | todo | Security-perf: preparse without full validate |
| 928 | P2 | todo | Security-perf: reduce redundant hash |
| 929 | P2 | todo | Security-perf: constant-time compare audit |
| 930 | P2 | todo | Formal: TLA+ sketch of HTLC |
| 931 | P2 | todo | Formal: model RC non-negativity |
| 932 | P2 | todo | Formal: undo session algebra |
| 933 | P2 | todo | Deploy: k8s StatefulSet for hived |
| 934 | P2 | todo | Deploy: k8s HAF postgres operator |
| 935 | P2 | todo | Deploy: Nomad jobspecs |
| 936 | P2 | todo | Deploy: Ansible role |
| 937 | P2 | todo | Deploy: Terraform modules for witnesses |
| 938 | P2 | todo | Deploy: one-click cloud image |
| 939 | P2 | todo | Deploy: auto-tune from cgroup limits |
| 940 | P2 | todo | Deploy: systemd hardening sandbox |
| 941 | P2 | todo | Deploy: graceful reload docs |
| 942 | P2 | todo | Comparative: Solana parallel scheduler lessons |
| 943 | P2 | todo | Comparative: Aptos Block-STM research note |
| 944 | P2 | todo | Comparative: Sui object ownership model note |
| 945 | P2 | todo | Comparative: Graphene fork optimizations survey |
| 946 | P2 | todo | Comparative: L2 fraud proof patterns for light clients |
| 947 | P2 | todo | AI: offline hot-path classifier on traces |
| 948 | P2 | todo | AI: suggest config from metrics (opt-in) |
| 949 | P2 | todo | AI: PR review checklist generator for perf |
| 950 | P2 | todo | eBPF: block apply latency probe |
| 951 | P2 | todo | eBPF: syscall count per block |
| 952 | P2 | todo | eBPF: network drop watch |
| 953 | P2 | todo | Logging: async spdlog-style sinks |
| 954 | P2 | todo | Logging: compile-time level stripping |
| 955 | P2 | todo | Logging: structured JSON fields |
| 956 | P2 | todo | Process: perf bounty board |
| 957 | P2 | todo | Process: monthly optimization sprint |
| 958 | P2 | todo | Process: RFC for any >5% apply change |
| 959 | P2 | todo | Process: public benchmark day |
| 960 | P2 | todo | Runbook: disk full recovery |
| 961 | P2 | todo | Runbook: SHM resize live |
| 962 | P2 | todo | Runbook: corrupt block_log repair |
| 963 | P2 | todo | Runbook: fork storm response |
| 964 | P2 | todo | Runbook: mobile node first sync |
| 965 | P2 | todo | Mirrornet: state-diff bisect tool |
| 966 | P2 | todo | Testnet: rapid reset from snapshot |
| 967 | P2 | todo | HSM: PKCS11 signing path research |
| 968 | P2 | todo | TEE: SGX/SEV witness research |
| 969 | P2 | todo | Checkpoint: social consensus trusted state |
| 970 | P2 | todo | Checkpoint: multi-sig committee format |
| 971 | P2 | todo | WASM: metering host weight table (aligned hive-native) |
| 972 | P2 | todo | WASM: deterministic mode flags |
| 973 | P2 | todo | WASM: plugin isolation cgroup |
| 974 | P2 | todo | Authority: proven max recursion depth reduction |
| 975 | P2 | todo | Authority: flatten multi-sig graphs cache |
| 976 | P2 | todo | ForkDB: branch-bound memory cap |
| 977 | P2 | todo | ForkDB: faster switch with shared prefix |
| 978 | P2 | todo | Irreversible: earlier LIB under fast confirm research |
| 979 | P2 | todo | Block producer: packing score optimizations |
| 980 | P2 | todo | Block producer: parallel tx selection |
| 981 | P2 | todo | Economics: RC price stability analysis |
| 982 | P2 | todo | Economics: resource market simulation |
| 983 | P2 | todo | Docs: operator cookbook index |
| 984 | P2 | todo | Docs: perf anti-patterns |
| 985 | P2 | todo | Docs: how to measure before/after |
| 986 | P2 | todo | Onboarding: perf mentor program |
| 987 | P2 | todo | Interop: export Prometheus naming standard |
| 988 | P2 | todo | Interop: OpenTelemetry semantic conventions |
| 989 | P2 | todo | Fleet: opt-in anonymized telemetry schema |
| 990 | P2 | todo | Fleet: config learning with differential privacy |
| 991 | P2 | todo | Legacy: remove unused Graphene paths list |
| 992 | P2 | todo | Legacy: gate old APIs behind compile flag |
| 993 | P2 | todo | Tooling: block profiler CLI |
| 994 | P2 | todo | Tooling: index size reporter |
| 995 | P2 | todo | Tooling: RC cost what-if calculator |
| 996 | P2 | todo | Tooling: snapshot size estimator |
| 997 | P2 | todo | Community: translation of runbooks |
| 998 | P2 | todo | Community: regional mirror seeds |
| 999 | P2 | todo | RC: continuous calibration job from apply traces — detail #999 |
| 1000 | P2 | todo | RC: per-op p50/p99 cost tables published — detail #1000 |
