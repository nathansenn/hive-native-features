# 04 – Contracts Engine Review

**Reviewer scope:** portable contracts engine (null + policy), deploy/call evaluators, design docs / ADR  
**Workspace:** `/tmp/hive-native-features`  
**Date:** 2026-07-25  
**Sources reviewed:**

| Path | Role |
|------|------|
| `include/hive_native/contracts/engine.hpp` | Engine API, host allow-list, fuel weights |
| `src/contracts/engine.cpp` | `null_engine` meta-protocol |
| `src/chain/evaluators_contracts.cpp` | Deploy/call apply path; storage commit |
| `docs/03-contracts-design.md` | Design + acceptance |
| `docs/decisions/ADR-0002-wasmtime.md` | Runtime selection ADR |
| `CMakeLists.txt` | Wasmtime flag default OFF |
| `tests/test_runner.cpp` | Portable contract + host tests |

**Related (spot-checked):** `include/hive_native/protocol/contract_operations.hpp`, `src/protocol/validate.cpp`, `src/rc/costs.cpp`, `include/hive_native/rc/costs.hpp`

**Test run:** `hive_native_tests` → `passed=115 failed=0`

---

## 1. Verification checklist

| Requirement | Result | Evidence |
|-------------|--------|----------|
| Wasmtime selected in docs | **PASS** | ADR-0002 **Status: Accepted**; design §3 **DECIDED: Wasmtime**; ops header cites ADR-0002 |
| Null engine default for CI | **PASS** | `default_engine()` → static `null_engine`; `HIVE_NATIVE_WITH_WASMTIME` defaults **OFF**; no Wasmtime link required for tests |
| `transfer` / `net` / `fs` / `random` denied | **PASS** | `host_allowed()` returns `false` for those (+ `wall_clock`); DENY path + unit checks for transfer/net |
| Fuel cannot be bypassed (BURN path) | **PASS** | All metering goes through `burn()`; `BURN:N` and host-weighted paths cap at `lim.fuel`; OOF fails closed |
| Storage commit only on success | **PASS** | Evaluator applies `res.storage_writes` only after `res.success` |
| Partial failure does not write storage | **PASS** | Failure returns before commit; null WRITE only stages after successful burn; throw path leaves DB storage unchanged |

---

## 2. Wasmtime selection (docs)

**Verdict: PASS**

- `docs/decisions/ADR-0002-wasmtime.md` accepts Wasmtime as the default runtime (sandboxing, fuel, determinism path, license).
- Alternatives (Wasmer, WAMR, custom interpreter) are explicitly rejected for the *default*.
- `docs/03-contracts-design.md` records the decision, ties it to human gate language, and maps consequences: optional `HIVE_NATIVE_WITH_WASMTIME`, CI without Wasmtime via null engine.
- Protocol header documents “Runtime: Wasmtime (ADR-0002)” for future consensus shape.

**Gap (expected for phase 3a):** no `WasmtimeEngine` implementation is present yet. The flag only adds a compile definition and optional link paths; runtime behavior remains null-engine-only. This matches “plugin skeleton / preferred engine locked” rather than “Wasmtime executes today.”

---

## 3. Null engine as CI default

**Verdict: PASS**

```115:117:src/contracts/engine.cpp
engine& default_engine() {
   return g_null;
}
```

```8:8:CMakeLists.txt
option(HIVE_NATIVE_WITH_WASMTIME "Link Wasmtime C API if available" OFF)
```

- Headers state: “NullEngine always available for CI”; ADR consequences match.
- Evaluators always use `contracts::default_engine()` — no hard dependency on Wasmtime for deploy/call.
- Portable tests exercise deploy, WRITE, DENY, BURN without external deps (`passed=115 failed=0`).

---

## 4. Host deny policy: transfer / net / fs / random

**Verdict: PASS** (policy + tests; simulation coverage uneven)

### 4.1 Allow-list

```83:94:include/hive_native/contracts/engine.hpp
inline bool host_allowed(host_fn f) {
   switch(f) {
      case host_fn::transfer:
      case host_fn::net:
      case host_fn::fs:
      case host_fn::random:
      case host_fn::wall_clock:
         return false;
      default:
         return true;
   }
}
```

Denied host weight is intentionally punitive (`default: return 1000000`) so a mis-wired path still burns fuel rather than “free” work.

### 4.2 Null-engine simulation

- Meta-command `DENY:transfer` records `denied_hosts`, sets error `host function denied: transfer`, `success = false`.
- Design + open questions: transfer host **disallowed in v1**.

### 4.3 Tests

- `test_contracts`: `DENY:transfer` → `CHECK_THROW(apply(...))`.
- `test_host_allow_list`: `read_storage` allowed; `transfer` and `net` denied.

### 4.4 Gaps (non-blocking)

| Item | Note |
|------|------|
| `fs` / `random` / `wall_clock` | Denied in `host_allowed`, not asserted in `test_host_allow_list` |
| Null DENY meta-protocol | Only models `transfer`, not `net`/`fs`/`random` attempt paths |
| Real Wasm host table | Not implemented; deny must be re-enforced when Wasmtime host imports are wired |

---

## 5. Fuel cannot be bypassed (BURN path)

**Verdict: PASS**

### 5.1 Single gate: `burn()`

```17:26:src/contracts/engine.cpp
static bool burn(call_result& r, host_limits& lim, uint64_t amount) {
   if(r.fuel_used + amount > lim.fuel) {
      r.success = false;
      r.error = "out of fuel";
      r.fuel_used = lim.fuel;
      return false;
   }
   r.fuel_used += amount;
   return true;
}
```

Properties:

1. **Hard cap:** never exceeds `lim.fuel`; on OOF, `fuel_used` saturates at the limit.
2. **Fail closed:** `success = false`, caller returns without staging further effects.
3. **No free paths:** deploy base, call base (50), `BURN:N`, WRITE/READ host weights, and default proportional cost all call `burn()`.
4. **BURN meta-command** cannot request unlimited work:

```64:68:src/contracts/engine.cpp
   if(cmd.rfind("BURN:", 0) == 0) {
      uint64_t n = std::stoull(cmd.substr(5));
      if(!burn(r, limits, n)) return r;
      r.success = true;
      return r;
   }
```

### 5.2 Host weights vs bypass

- Design rule: “User cannot bypass fuel via host functions.”
- Allowed hosts have static weights ≥ nominal work; denied hosts map to huge weight if ever reached.
- DENY path still pays base call fuel (50) *before* denial — denial is not a free probe that skips metering.

### 5.3 RC coupling

- `cost_contract_call(op, res.fuel_used)` uses **actual** `fuel_used` when non-zero (else prepaid `fuel_limit`).
- On call failure, evaluator still sets `last_rc_charged` from `fuel_used` before throwing — fuel/RC consumption on failure matches design §6.2 (“consume fuel/RC; no partial storage commit”).

### 5.4 Tests

- `BURN:100000` with `fuel_limit = 10` → throw (`out of fuel` path).

### 5.5 Residual risks (for Wasmtime phase)

- Real instruction fuel must be engine-enforced (Wasmtime fuel/epoch), not only host-weight simulation.
- `std::stoull` on `BURN:` can throw on malformed input (test harness issue, not a consensus fuel hole).
- `max_memory_bytes` / `max_storage_total` are defined on `host_limits` but not fully enforced by null_engine (key/value size checks exist; total quota does not).

---

## 6. Storage commit only on success / no partial writes

**Verdict: PASS**

### 6.1 Evaluator commit fence

```54:67:src/chain/evaluators_contracts.cpp
   auto res = eng.call(it->second.code, op.export_name, op.args, storage, lim, op.caller);

   db.last_rc_charged = rc::cost_contract_call(op, res.fuel_used);

   if(!res.success) {
      std::ostringstream p;
      p << "{\"contract_id\":" << op.contract_id << ",\"success\":false,\"fuel\":" << res.fuel_used << "}";
      db.push_virtual("contract_called", p.str());
      throw protocol_error(res.error.empty() ? "call failed" : res.error);
   }

   // Commit storage writes only on success (no partial mutation)
   for(const auto& [k, v] : res.storage_writes)
      storage[k] = v;
```

Invariants:

1. **Commit after success gate** — durable `db.contract_storage` mutates only if `res.success`.
2. **Failure path** emits `contract_called` with `success:false`, throws, **does not** apply `storage_writes`.
3. **Engine stages writes** in `call_result::storage_writes` (header comment: “applied on success only”); null WRITE only inserts after successful fuel burn and size checks.
4. **Happy path test:** `WRITE:k:v` persists `contract_storage[1]["k"] == {'v'}`.
5. **Failure tests:** DENY and BURN throw; storage is not updated by those args (prior successful WRITE remains; no new keys from failed calls).

### 6.2 Partial multi-write semantics (design vs null)

- Null engine processes **one** meta-command per call (single WRITE). Multi-key atomicity is therefore trivial today.
- Defense-in-depth: even if a future engine filled `storage_writes` partially and set `success = false`, the evaluator still would not commit.
- When multi-write WASM hosts land, keep **all-or-nothing** staging in the engine result; never write through to DB mid-call.

### 6.3 Deploy path

- Deploy does not apply storage deltas from init in null_engine (`deploy_and_init` only fuel-checks code).
- On deploy success, evaluator creates empty `contract_storage[id]` and contract object only after `res.success` — consistent fail-closed pattern.

---

## 7. Cross-cutting notes

### Strengths

- Clear separation: engine stages effects; chain evaluator owns durable commit.
- Host deny list is centralized (`host_allowed`) rather than scattered conditionals.
- Plugin-first / HF-gated evaluators (`HIVE_HARDFORK_CONTRACTS`, `contracts_skip` for non-consensus).
- Docs, ADR, code, and tests agree on the six review requirements.

### Follow-ups (priority order)

1. **Implement Wasmtime engine** behind `HIVE_NATIVE_WITH_WASMTIME` with the same `engine` interface; keep null as default/CI fallback.
2. **Wire host imports** exclusively through allow-list; never register transfer/net/fs/random/wall_clock.
3. **Extend tests:** assert `fs`/`random`/`wall_clock` denied; optional partial-failure multi-write unit once multi-key staging exists; post-fail storage equality snapshot.
4. **Enforce** `max_storage_total` and memory limits in null + Wasmtime paths.
5. **Populate** `plugins/hive_contracts` (currently empty directory) with plugin glue when hived integration starts.

### Security gate mapping (`03-contracts-design.md` §11)

| Gate item | Portable status |
|-----------|-----------------|
| Host allow-list reviewed | Reviewed here — transfer/net/fs/random/wall_clock denied |
| Fuel cannot be bypassed | Satisfied for null engine + BURN simulation |
| No reentrancy in v1 | No contract→contract host; policy stated |
| Code / arg size limits | Validated in protocol (`MAX_CODE_BYTES`, `MAX_CONTRACT_ARGS`) |
| Fuzzing harness (3f) | Not yet |
| Pin runtime version | N/A until Wasmtime linked |

---

## 8. Conclusion

All six verification requirements **pass** against the current portable implementation and documentation:

1. **Wasmtime** is the accepted documented runtime (ADR-0002).  
2. **null_engine** is the default/`default_engine()` for CI builds (Wasmtime flag OFF).  
3. **transfer / net / fs / random** (and wall_clock) are denied by host policy.  
4. **Fuel** is gated by `burn()`; BURN and host paths cannot exceed `fuel_limit`.  
5. **Storage** is committed only after successful call.  
6. **Partial failure** does not mutate durable contract storage.

Residual work is forward-looking (real Wasmtime host table, fuller quota enforcement, fuzzing, plugin skeleton), not regressions against this review’s acceptance criteria.

**Review status:** APPROVED for phase-3 portable contracts surface (3a–3e policy + null engine).
