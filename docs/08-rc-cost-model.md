# 08 – RC Cost Model

**Task-ID:** phase-1 / 1.4, phase-2 RC, phase-3d fuel→RC  
**Status:** implementation sketch (relative micro-units)  
**Last Updated:** 2026-07-25  
**Sources:** `include/hive_native/rc/costs.hpp`, `src/rc/costs.cpp`  
**Related:** `docs/04-performance-budgets.md` §5, feature RC sections in `01`/`02`/`03`

---

## 1. Purpose

Document the **placeholder resource-credit (RC) cost model** used by hive-native-features for NFT, HTLC, and contract operations.

Absolute Hive RC units are chain-internal and depend on the live RC plugin / resource user maps. This repo exposes **relative micro-units** for tests and microbenchmarks so:

- costs stay ordered consistently (mint > transfer, create-state > pure transfer, etc.),
- fuel→RC mapping is explicit and HF-tunable later,
- every formula is reviewable before upstream calibration.

**Policy (architecture):** placeholder costs are allowed only with `TODO – measure` and a tracking TASK-ID. RC cost *decreases* that enable spam require human review.

---

## 2. Units and constants

| Constant | Value | Meaning |
|----------|------:|---------|
| `TRANSFER_BASE` | `1000` | Relative cost of a Hive-class transfer; unit of account for all formulas |
| `FUEL_TO_RC_NUM` | `1` | Numerator of linear fuel→RC slope |
| `FUEL_TO_RC_DEN` | `10` | Denominator; **10 fuel units → 1 RC micro** |

Defined in `include/hive_native/rc/costs.hpp`:

```cpp
inline constexpr uint64_t TRANSFER_BASE = 1000;
inline constexpr uint64_t FUEL_TO_RC_NUM = 1;
inline constexpr uint64_t FUEL_TO_RC_DEN = 10; // 10 fuel units → 1 RC micro
```

### 2.1 Byte component

```text
bytes_cost(n) = n × 2
```

Implemented as a private helper in `src/rc/costs.cpp`. Every variable-length field (memo, URI, preimage, code, args, symbol/name) uses this linear byte tax.

---

## 3. General formula shape

Modeled after existing Hive RC: **base + size/bytes + state creation + (optional) compute/fuel**.

```text
RC ≈ k_base × TRANSFER_BASE
   + bytes_cost(|variable fields|)
   + state_flat          // fixed surcharge when creating / expanding state
   + fuel_component      // contracts only
```

Integer arithmetic only; fuel mapping uses floor division:

```text
fuel_component = (fuel × FUEL_TO_RC_NUM) / FUEL_TO_RC_DEN
```

---

## 4. NFT operations

| Function | Formula | Relative intent |
|----------|---------|-----------------|
| `cost_nft_create_collection` | `TRANSFER_BASE × 3 + bytes_cost(symbol + name) + 500` | High base + state surcharge |
| `cost_nft_mint` | `TRANSFER_BASE × 2 + bytes_cost(uri) + 800` | State creation (mint is DoS-sensitive) |
| `cost_nft_transfer` | `TRANSFER_BASE + bytes_cost(memo)` | Align near transfer |
| `cost_nft_approve` | `TRANSFER_BASE / 2` | Low / approval only |
| `cost_nft_set_approval_for_all` | `TRANSFER_BASE / 2 + 100` | Slightly above single approve |
| `cost_nft_burn` | `TRANSFER_BASE` | Base; free-state credit TBD upstream |

### 4.1 Expanded

```text
RC_nft_create_collection = 3·B + 2·(|symbol| + |name|) + 500
RC_nft_mint              = 2·B + 2·|uri| + 800
RC_nft_transfer          = B + 2·|memo|
RC_nft_approve           = B / 2
RC_nft_set_approval_all  = B / 2 + 100
RC_nft_burn              = B

where B = TRANSFER_BASE = 1000
```

**DoS note:** mint is the expensive path; RC must keep mass minting costly. Prefer RC-only first; per-block soft limits only if RC proves insufficient.

---

## 5. HTLC operations

| Function | Formula | Relative intent |
|----------|---------|-----------------|
| `cost_htlc_create` | `TRANSFER_BASE × 2 + bytes_cost(memo) + 600` | State + open-lock deterrence |
| `cost_htlc_redeem` | `TRANSFER_BASE + bytes_cost(preimage) + 200` | Bytes + hash compute flat |
| `cost_htlc_refund` | `TRANSFER_BASE` | Close path, base only |

### 5.1 Expanded

```text
RC_htlc_create = 2·B + 2·|memo| + 600
RC_htlc_redeem = B + 2·|preimage| + 200
RC_htlc_refund = B
```

Create must stay expensive enough to deter millions of open dust HTLCs (see `02-htlc-design.md` §10).

---

## 6. Contract operations and fuel → RC

| Function | Formula |
|----------|---------|
| `cost_contract_deploy` | `TRANSFER_BASE × 10 + bytes_cost(code) + (fuel_limit × NUM) / DEN` |
| `cost_contract_call` | `TRANSFER_BASE × 2 + bytes_cost(args) + (fuel × NUM) / DEN` |

For `cost_contract_call`, fuel selection:

```text
fuel = fuel_used if fuel_used ≠ 0 else op.fuel_limit
```

Evaluators pass `res.fuel_used` after execution (`evaluators_contracts.cpp`). Deploy charges against **declared** `op.fuel_limit` (prepaid budget at deploy time).

### 6.1 Fuel mapping

```text
RC_fuel = (fuel × FUEL_TO_RC_NUM) / FUEL_TO_RC_DEN
        = fuel / 10   (with current constants; integer floor)
```

So **10 fuel units → 1 RC micro**.

### 6.2 Expanded

```text
RC_contract_deploy = 10·B + 2·|code| + (fuel_limit × 1) / 10
RC_contract_call   =  2·B + 2·|args| + (fuel × 1) / 10
```

### 6.3 Design rules (from contracts design)

1. Fuel decrements on instruction batches and host calls.  
2. Fuel = 0 → trap; call fails; contract storage rolled back.  
3. Users cannot bypass fuel via host functions.  
4. Host functions have static weights ≥ real work.  
5. Mapping constants (`FUEL_TO_RC_*`, bases) are consensus parameters (HF-tunable).  
6. Contract fuel is **prepaid**; cannot execute over limit.

Target shape for full model (not all terms coded yet):

```text
RC_cost = base_call + f(fuel) + storage_write_bytes + code_bytes
```

Storage write surcharges beyond `bytes_cost` of op fields remain future work when host metering is wired.

---

## 7. Worked examples (B = 1000)

| Scenario | Inputs | RC micro |
|----------|--------|---------:|
| NFT transfer, empty memo | \|memo\|=0 | 1000 |
| NFT transfer, 50-byte memo | \|memo\|=50 | 1000 + 100 = 1100 |
| NFT mint, empty URI | \|uri\|=0 | 2000 + 800 = 2800 |
| NFT create collection, 4+8 name/symbol | 12 bytes | 3000 + 24 + 500 = 3524 |
| HTLC create, empty memo | — | 2000 + 600 = 2600 |
| HTLC redeem, 32-byte preimage | — | 1000 + 64 + 200 = 1264 |
| Contract call, empty args, 1000 fuel used | — | 2000 + 0 + 100 = 2100 |
| Contract deploy, 1 KiB code, fuel_limit 5000 | — | 10000 + 2048 + 500 = 12548 |

---

## 8. Integration points

| Layer | Behavior |
|-------|----------|
| Evaluators (`evaluators_nft/htlc/contracts.cpp`) | Set `db.last_rc_charged = rc::cost_* (...)` |
| Tests (`tests/test_runner.cpp`) | Smoke: transfer ≥ `TRANSFER_BASE`; mint > transfer |
| Benchmarks (`benchmarks/bench_ops.cpp`) | May reference `TRANSFER_BASE` as baseline RC |

This sketch does **not** yet wire Hive’s real RC plugin resource-user map; absolute calibration is an upstream task.

---

## 9. TODO – measure (upstream calibration)

All numeric constants below are **relative placeholders**. Before consensus activation / upstream merge, Performance Benchmarker + Architect must replace them with values derived from:

- microbenchmarks vs live `transfer_operation` on reference hardware,
- Hive RC resource curves (history, market, state bytes, execution time),
- projected object counts in `04-performance-budgets.md` §6.

| Marker | Scope | TASK-ID | Action |
|--------|-------|---------|--------|
| `TODO – measure` | Absolute scale of `TRANSFER_BASE` vs Hive RC units | phase-1 / 1.4, upstream RC | Map micro-units → real RC; do not ship consensus with uncalibrated scale |
| `TODO – measure` | `bytes_cost` slope (`× 2`) | phase-1 / 1.4 | Align with Hive size/bytes resource weight |
| `TODO – measure` | NFT state flats (`+500` collection, `+800` mint) | TASK 1.4; `01-nft-design` §7 | Measure state object size × k from Hive RC model |
| `TODO – measure` | NFT approve / set_approval_for_all / burn | TASK 1.4 | Confirm low-path costs; burn free-state credit if Hive pattern exists |
| `TODO – measure` | HTLC create `+600`, redeem hash `+200` | Phase 2 RC | Create must deter dust open locks; hash cost vs sha256 bench |
| `TODO – measure` | Contract deploy base `× 10` | Phase 3d | Code-size + cold-start cost on witness hardware |
| `TODO – measure` | `FUEL_TO_RC_NUM` / `FUEL_TO_RC_DEN` | Phase 3d | Fuel→RC experiments; HF-tunable consensus params |
| `TODO – measure` | Host-call weights & storage write RC | Phase 3d / 3h | Not yet in `costs.cpp`; required before consensus contracts |
| `TODO – measure` | Block-level max fuel share | `04` §7, Phase 3 | F_call / F_block packing limits |

### 9.1 Calibration checklist

- [ ] Document reference CPU / node profile used for benches  
- [ ] Relative order preserved after absolute mapping (mint > transfer > approve/2)  
- [ ] State-creating ops satisfy cost ≥ k × state_bytes (k from Hive RC model)  
- [ ] Hash ops include compute component (HTLC redeem)  
- [ ] Contract fuel prepaid; over-limit impossible  
- [ ] No RC *decrease* vs prior calibrated values without human review  
- [ ] Resource-user map entries proposed for upstream RC plugin  

---

## 10. File map

| Path | Role |
|------|------|
| `include/hive_native/rc/costs.hpp` | Constants + declarations |
| `src/rc/costs.cpp` | Formulas |
| `docs/08-rc-cost-model.md` | This document |
| `docs/swarm/15-rc.md` | Swarm agent brief |
| `docs/01-nft-design.md` §7 | NFT RC policy |
| `docs/02-htlc-design.md` §10 | HTLC RC policy |
| `docs/03-contracts-design.md` §7 | Fuel → RC design |
| `docs/04-performance-budgets.md` §5 | RC budget policy |

---

## 11. Sign-off

- [x] Formulas match `costs.cpp` / `costs.hpp`  
- [x] `TRANSFER_BASE` and fuel→RC constants documented  
- [x] `TODO – measure` markers listed with TASK-IDs for upstream calibration  
- [ ] Performance Benchmarker absolute calibration (blocked on upstream / Phase 3d)  
- [ ] Human waiver for consensus-path RC parameters (HF gate)
