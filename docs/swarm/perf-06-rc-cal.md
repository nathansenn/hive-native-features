# perf-06 – RC calibration from real apply microbench (#891)

**Task-ID:** catalogue #891 / swarm-perf-p0-impl  
**Status:** PASS (informational harness; not a ctest hard-fail gate)  
**Date:** 2026-07-27  
**Workdir:** `/tmp/hive-native-features`  
**Branch:** `swarm-perf-p0-impl`  
**Binary:** `./build/hive_native_bench_rc`  
**Commit:** `d02792c` (pre-commit at doc write)  
**Host:** Apple M3 Ultra (arm64)  
**Code:** `include/hive_native/perf/rc_calibrator.hpp`, `benchmarks/bench_rc_calibrate.cpp`  
**RC model:** [`docs/08-rc-cost-model.md`](../08-rc-cost-model.md) · swarm brief [`15-rc.md`](15-rc.md)

---

## 1. Mission

Feed **real portable apply microbench samples** (synthetic transfer, `nft_transfer`, `htlc_create`) into `rc_calibrator` and report:

```text
calibrated_rc(op) = TRANSFER_BASE × median_us(op) / median_us(transfer)
```

Compare against current **placeholder** costs from `rc::cost_*`.

---

## 2. Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHIVE_NATIVE_BUILD_BENCH=ON
cmake --build build --target hive_native_bench_rc -j
./build/hive_native_bench_rc
```

CMake target: **`hive_native_bench_rc`** (under `HIVE_NATIVE_BUILD_BENCH`).  
**Not** registered with `ctest` — exit is always 0 (no hard-fail gate).

Related ratio bench (budgets): `hive_native_bench` → `docs/swarm/12-perf.md`.

---

## 3. Sample output (actual run)

```json
{
  "catalogue": 891,
  "transfer_base": 1000,
  "rounds": 21,
  "iters_per_round": 400,
  "samples": {
    "transfer": { "median_us": 0.101719, "calibrated_rc": 1000, "placeholder_rc": 1000, "wall_ratio_vs_transfer": 1.0 },
    "nft_transfer": { "median_us": 0.250727, "calibrated_rc": 2465, "placeholder_rc": 1000, "wall_ratio_vs_transfer": 2.46491 },
    "htlc_create": { "median_us": 0.773125, "calibrated_rc": 7601, "placeholder_rc": 2600, "wall_ratio_vs_transfer": 7.60061 }
  },
  "calibrated_rc": {
    "transfer": 1000,
    "nft_transfer": 2465,
    "htlc_create": 7601
  },
  "placeholder_rc": {
    "transfer": 1000,
    "nft_transfer": 1000,
    "htlc_create": 2600
  },
  "formula": "calibrated_rc = TRANSFER_BASE * median(op) / median(transfer)",
  "note": "portable in-memory apply; htlc refunds only recycle open-cap outside create timer"
}
```

### Summary table

| Op | median_us | calibrated_rc | placeholder_rc | wall ratio vs transfer |
|----|----------:|--------------:|---------------:|-----------------------:|
| transfer | 0.101719 | **1000** | 1000 | 1.00× |
| nft_transfer | 0.250727 | **2465** | 1000 | **2.46×** |
| htlc_create | 0.773125 | **7601** | 2600 | **7.60×** |

`TRANSFER_BASE = 1000`. Absolute µs are host-local; **ratios / calibrated_rc** are the portable signal.

---

## 4. Method notes

| Op | How sampled |
|----|-------------|
| `transfer` | Synthetic transfer-class op (name checks + dual balance move + virtual op); round-trip timed, **÷2** → single-transfer sample |
| `nft_transfer` | Real `apply(db, nft_transfer_operation)` ping-pong on one NFT after mint |
| `htlc_create` | Real `apply(db, htlc_create_operation)` only; refunds + map prune **outside** the create timer (open-cap recycle) |

- **21** rounds (odd → stable median), **400** iters/round for transfer & nft; **200** creates/round for HTLC.  
- Fresh HTLC DB each round so closed-object map growth does not dominate.  
- Ratio clamp in calibrator: `[0.1, 100]` × `TRANSFER_BASE`.

---

## 5. Interpretation (do not auto-ship)

| Finding | Implication |
|---------|-------------|
| `nft_transfer` calibrated **~2.5×** placeholder | Portable apply path is heavier than synthetic transfer; placeholder `B + bytes(memo)` under-prices wall time vs synthetic baseline |
| `htlc_create` calibrated **~7.6×** transfer / **~2.9×** placeholder (2600) | Create+state insert is expensive; placeholders still **TODO – measure** before consensus |
| Baseline is **synthetic** transfer, not live Hive `transfer_operation` | Upstream must re-run vs real evaluator + RC resource-user map |

**Policy:** never lower RC without human review; this harness only *reports* calibrated candidates (`docs/08` §9, swarm `15-rc.md`).

---

## 6. File map

| Path | Role |
|------|------|
| `include/hive_native/perf/rc_calibrator.hpp` | #891/#894 median → RC mapping |
| `benchmarks/bench_rc_calibrate.cpp` | Real apply loops → calibrator → JSON |
| `benchmarks/bench_ops.cpp` | Budget ratios (`hive_native_bench`) |
| `CMakeLists.txt` | Target `hive_native_bench_rc` |
| `src/rc/costs.cpp` | Placeholder `cost_*` still authoritative for apply |

---

## 7. Handoff

```
FROM: Performance Benchmarker
TO: Architect | Reviewer
TASK-ID: catalogue #891
STATUS: ready-for-review
SUMMARY: hive_native_bench_rc feeds rc_calibrator from synthetic transfer / nft_transfer / htlc_create apply samples; prints calibrated_rc vs placeholders
ARTIFACTS: benchmarks/bench_rc_calibrate.cpp, docs/swarm/perf-06-rc-cal.md
VERIFICATION: Release build; ./build/hive_native_bench_rc → JSON exit 0
NEXT: human review before any placeholder decrease/increase in costs.cpp; upstream real-transfer baseline
```
