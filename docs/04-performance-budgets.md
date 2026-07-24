# 04 – Performance Budgets

**Task-ID:** phase-0 / 0.6  
**Status:** design  
**Last Updated:** 2026-07-24  

---

## 1. Purpose

Numeric and qualitative budgets so agents can **fail closed** when a change risks Hive’s operational envelope: ~3s blocks, mobile/pruned RAM, and RC fairness.

All thresholds below are **initial engineering budgets**. Exceeding them requires Architect + Performance Benchmarker sign-off, and for consensus paths **human waiver**.

---

## 2. Global system budgets

| Metric | Budget | Measurement |
|--------|--------|-------------|
| Target block interval | ~3.0 s | Network config / production reference |
| Apply headroom | Leave ≥ 40% of interval free under “normal” load model | Bench harness |
| Peak apply for single op (p99) | See per-feature tables | Microbench |
| Witness RAM (additional feature total, full node) | Phase 1+2 combined ≤ **512 MB** steady-state for projected object counts* | RSS delta |
| Pruned node RAM delta | ≤ **128 MB** for same projection | RSS delta |
| Mobile / light | **0** mandatory new indexes | Config skip |

\*Projection assumptions documented in §6; revise when real counts known.

---

## 3. Per-operation apply latency budgets (CPU, single-thread µs)

Order-of-magnitude relative to existing Hive `transfer`-class ops. Absolute numbers to be calibrated on reference hardware (document CPU model in bench README).

| Operation | Target p50 | Target p99 | Hard fail (p99) |
|-----------|------------|------------|-----------------|
| nft_transfer | ≤ 1.5× transfer | ≤ 2× transfer | > 5× transfer |
| nft_mint | ≤ 2× transfer | ≤ 3× transfer | > 8× transfer |
| nft_approve / burn | ≤ 1× transfer | ≤ 2× transfer | > 5× transfer |
| htlc_create | ≤ 2× transfer | ≤ 3× transfer | > 8× transfer |
| htlc_redeem (sha256) | ≤ 2× transfer + hash | ≤ 4× transfer | > 10× transfer |
| htlc_refund | ≤ 1.5× transfer | ≤ 2× transfer | > 5× transfer |
| contract_call (empty, metered) | fuel-bound | fuel-bound | exceeds block fuel share |

**Rule:** If absolute calibration unavailable, use **relative** benchmarks vs `transfer_operation` on the same binary.

---

## 4. State size budgets

| Object | Max steady size (bytes, estimate) | Notes |
|--------|-----------------------------------|-------|
| nft_collection_object | ≤ 256 + string caps | Enforce caps in protocol |
| nft_object | ≤ 320 without long URI; URI ≤ 256 | Prefer hash-only |
| htlc_object | ≤ 350 + memo cap | Memo ≤ 2048 |
| contract_object (header) | ≤ 256 | Code off hot path |
| contract storage total / contract | Quota TBD | Hard limit required before 3h |

**Growth rules:**

- No O(n) evaluator scans of all NFTs/HTLCs.  
- Indexes only as justified in feature docs.  
- Closed HTLCs do not accumulate in chainbase.

---

## 5. RC budgets (policy)

| Rule | Policy |
|------|--------|
| New ops | Must define cost curve or `TODO – measure` + TASK-ID |
| State-creating ops | Cost ≥ k × state_bytes (k from Hive RC model) |
| Hash ops | Include compute component |
| Contract fuel | Prepaid; cannot execute over limit |
| Regression | RC cost decrease that enables spam requires human review |

---

## 6. Load projections (planning only)

| Asset | 1-year planning count | Full-node RAM estimate |
|-------|----------------------|-------------------------|
| NFT objects | 10M × 300 B ≈ 3 GB raw | Mitigate via pruning of burned + metadata off-chain; **revisit** if 10M on hot chainbase is unacceptable → archive tier |
| Open HTLCs | 100k × 300 B ≈ 30 MB | Acceptable |
| Contracts | 10k headers negligible; storage separate | Quota |

**Important:** If NFT hot-state projection exceeds witness norms, Architect must propose **cold storage / RocksDB secondary** for inactive NFTs before Phase 1 implementation freezes layout.

**Action item:** Phase 1.2 must re-validate object size with packed layout and decide chainbase vs RocksDB for inactive tokens.

---

## 7. Block-level fuel (contracts)

| Param | Initial proposal |
|-------|------------------|
| Max fuel per call | Fixed constant F_call |
| Max fuel per block | F_block = N × F_call |
| Max contract calls per block | Explicit cap |

If cumulative fuel would exceed F_block, later ops fail cheaply (or are not packed by wallets — consensus still defines failure).

---

## 8. Parallel apply / worker pool

| Feature | Parallelism note |
|---------|------------------|
| NFT transfer distinct ids | Mostly independent; account balance/auth coupling remains |
| NFT mint same collection | Serialize on collection supply |
| HTLC | Independent by id; account balances couple |
| Contract calls | Serialize per contract_id; optional worker for CPU with merge barrier |

**Budget:** No new global mutex. Any lock hierarchy change needs Reviewer + Performance sign-off.

---

## 9. Microbenchmark harness requirements

Location (planned): `benchmarks/` or `tests/benchmarks/`

Minimum cases:

1. Baseline `transfer`  
2. `nft_transfer` / mint  
3. `htlc_create` + redeem (both hashes)  
4. Contract empty call / storage write (Phase 3)  

Output: JSON with p50/p99, RSS sample, build commit SHA.

CI: run microbenches in relative mode; fail if ratio > hard fail column without `PERF_WAIVER` file signed in PR description by human.

---

## 10. Regression gate process

```
Performance Benchmarker runs suite
  ├─ green → cite numbers in PR
  ├─ yellow (within 20% of hard fail) → Architect review
  └─ red (exceeds hard fail) → block commit
```

---

## 11. Light-node performance

| Requirement | Budget |
|-------------|--------|
| Sync headers with nft-skip | No regression vs current light sync |
| Memory | No NFT/HTLC index allocations when skip=true |
| CPU | Signature verify only for skipped feature ops |

---

## 12. Acceptance

- [x] Global and per-op budgets drafted  
- [x] State and RC policy  
- [x] Regression process  
- [ ] Calibrate absolute µs on reference hardware (implementation phase)  
- [ ] Human review of NFT 10M projection / storage tiering  
