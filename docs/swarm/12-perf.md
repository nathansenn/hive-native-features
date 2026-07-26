# 12 – Performance calibration (portable)

**Task-ID:** swarm / perf  
**Status:** PASS (hard-fail gates)  
**Date:** 2026-07-26  
**Workdir:** `/tmp/hive-native-features`  
**Binary:** `./build/hive_native_bench` (existed; not rebuilt)  
**Commit:** `bfb97f7`  
**Host:** Apple M3 Ultra (arm64)  
**Budgets:** [docs/04-performance-budgets.md](../04-performance-budgets.md) §3 + §12  

---

## Command

```bash
./build/hive_native_bench
```

Exit code: **0** (hard fail only if `nft_transfer_ratio` > 5.0).

---

## Bench JSON (actual)

```json
{
  "synthetic_transfer_us": 0.220729,
  "nft_transfer_us": 0.250479,
  "nft_transfer_ratio": 1.13478,
  "htlc_create_us": 1.47135,
  "budget_nft_p50_ratio_max": 1.5,
  "budget_nft_hard_fail_ratio": 5.0,
  "nft_within_hard_fail": true,
  "note": "portable in-memory; ratio vs synthetic transfer-class op"
}
```

---

## Ratios (primary gates)

Absolute µs are host-local noise; **ratios** vs synthetic transfer are the portable signal.

| Metric | Value | Budget (unchanged) | Result |
|--------|------:|--------------------|--------|
| `nft_transfer_ratio` | **1.13478** | p50 target ≤ 1.5×; hard fail > 5× | **PASS** hard fail; under p50 target |
| `htlc_create` / `synthetic_transfer` | **6.666×** (1.47135 / 0.220729) | p50 target ≤ 2×; hard fail > 8× | under hard fail; **above** p50 soft target |
| `budget_nft_p50_ratio_max` | 1.5 | soft | reference only |
| `budget_nft_hard_fail_ratio` | 5.0 | hard | **not weakened** |
| `nft_within_hard_fail` | true | must be true for CI | **PASS** |

### Ratio summary

| Operation | µs | Ratio vs transfer |
|-----------|---:|------------------:|
| synthetic_transfer | 0.220729 | 1.000× |
| nft_transfer | 0.250479 | **1.135×** |
| htlc_create | 1.47135 | **6.666×** |

---

## Hard-fail policy (unchanged)

From `docs/04` §3 — **do not relax**:

| Operation | Hard fail (p99) |
|-----------|-----------------|
| nft_transfer | > 5× transfer |
| nft_mint | > 8× transfer |
| htlc_create | > 8× transfer |
| htlc_redeem (sha256) | > 10× transfer |
| htlc_refund | > 5× transfer |

Harness enforces NFT hard fail only today (`return ratio > 5.0 ? 2 : 0`).

---

## Cross-run note

Earlier same-session sample (for variance awareness only):

| Field | Prior sample | Documented sample |
|-------|-------------:|------------------:|
| synthetic_transfer_us | 0.320312 | 0.220729 |
| nft_transfer_us | 0.3685 | 0.250479 |
| nft_transfer_ratio | 1.15044 | 1.13478 |
| htlc_create_us | 2.22812 | 1.47135 |
| htlc ratio (derived) | ~6.96× | ~6.67× |

Ratios stay ~1.13–1.15 (NFT) and ~6.7–7.0 (HTLC create); absolute µs swing with scheduling.

---

## Files updated

- `docs/04-performance-budgets.md` — added §12 Portable calibration (2026-07-26)
- `docs/swarm/12-perf.md` — this file
