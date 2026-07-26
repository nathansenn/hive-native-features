# 03 – Metered Smart Contracts Design

**Task-ID:** phase-0 / 0.5  
**Status:** design (plugin-first)  
**Last Updated:** 2026-07-24  
**Depends on:** `00-architecture-overview.md`, `04-performance-budgets.md`  

---

## 1. Summary

Smart contracts on Hive must not endanger **3s blocks**, **RC economics**, or **light nodes**. Strategy:

1. **Phases 3a–3g:** non-consensus **plugin** runtime, explicit metering, isolated storage, host allow-list, fuzzing.  
2. **Phase 3h:** only after **human approval**, design consensus activation (HF).

No consensus WASM ships without an explicit human gate.

---

## 2. Goals and non-goals

### Goals

- Deterministic execution **candidate** environment (same inputs → same outputs).
- Hard **fuel** bound mapped to **RC**.
- Minimal host surface (no FS, no net, no clocks beyond block context).
- Clear upgrade path from plugin → consensus without rewriting app ABI.

### Non-goals (near term)

- EVM opcode compatibility
- Unbounded storage without rent/deposit
- Floating-point consensus
- Automatic activation with Phase 1/2

---

## 3. WASM runtime options

| Runtime | Pros | Cons | Notes |
|---------|------|------|-------|
| **Wasmtime** | Strong sandboxing, Cranelift, active | Embed size; API churn | Strong candidate |
| **Wasmer** | Multiple compilers; popular | Version/plugin complexity | Strong candidate |
| **WAMR** | Tiny, MCU-friendly | Feature set | Good for light experiments |
| **wavm / other** | Perf | Ops burden | Unlikely default |

**Selection criteria (Phase 3a human gate):**

1. Deterministic mode available (no non-deterministic host)  
2. Fuel / gas instrumentation  
3. Memory limits  
4. License compatibility with Hive  
5. Maintenance health  
6. Embed cost on witness hardware  

**Phase 0 decision:** document options only; **do not pick** until Phase 3a + human confirmation.

---

## 4. Plugin-first architecture

```
hived
 ├── consensus (unchanged in 3a–3g)
 └── plugin: hive_contracts
       ├── WASM engine (selected runtime)
       ├── fuel meter
       ├── host functions (allow-list)
       ├── storage provider (RocksDB / files)
       └── RPC / local apply hooks (non-consensus)
```

**Non-consensus means:**

- Plugin may interpret `custom_json` or dedicated **non-HF ops** only if they do not change consensus state — prefer `custom_json` experimental interface **or** side channel.
- For clean future HF: define op shapes early but gate evaluators behind HF flag **off**.

**Recommended experimental interface:**

- `custom_json` id: `hive_contracts` with actions `deploy`, `call`  
- Parallel design of future `contract_deploy_operation` / `contract_call_operation` structs matching the same payload

---

## 5. Object / state model (future consensus shape)

### 5.1 `contract_object`

| Field | Notes |
|-------|-------|
| `id` | contract_id_type |
| `owner` / `code_hash` | Account + hash of WASM |
| `code_ref` | Blob store key (not full code in chainbase) |
| `storage_root` | Commitment to k/v (merkle/sparse) |
| `version` | uint32 |
| `created` | time |

### 5.2 Isolated storage provider

- Key/value, max key/value sizes
- Per-contract quota (bytes) funded by deposit or RC-rent model (TBD human)
- No access to other contracts’ raw storage without explicit host call
- Snapshots: export storage_root for light verify-only later

---

## 6. Operations (design for 3b / future HF)

### 6.1 `contract_deploy_operation`

- Auth: active of deployer  
- Fields: code (or code_hash + publish blob separately), init args, fuel limit  
- Effects: create contract_object; store code off hot chainbase  
- RC: large base + code bytes + fuel prepaid  

### 6.2 `contract_call_operation`

- Auth: active of caller  
- Fields: contract_id, method/export, args, fuel_limit  
- Effects: run engine; apply storage delta if success; virtual `contract_called`  
- Failure: consume fuel/RC; **no** partial storage commit  

### 6.3 Optional admin

- `contract_set_owner`, `contract_purge` (owner, empty storage) — later

---

## 7. Fuel → RC mapping

```
fuel_limit  →  execution steps / host call weights
RC_cost     =  base_call + f(fuel_limit) + storage_write_bytes + code_bytes
```

Rules:

1. Fuel is decremented on instruction batches and host calls.  
2. Fuel = 0 → trap; transaction fails; state rolled back for contract storage.  
3. User **cannot** bypass fuel via host functions.  
4. Host functions have **static weights** ≥ real work.  
5. Mapping constants are consensus parameters (HF-tunable).  

Placeholder: `TODO – measure` until microbenchmarks (Phase 3d).

---

## 8. Host function allow-list (initial)

| Host | Purpose | Weight |
|------|---------|--------|
| `read_storage(key)` | Load value | per byte |
| `write_storage(key, val)` | Store | per byte high |
| `remove_storage(key)` | Delete | medium |
| `get_caller()` | Account name | low |
| `get_contract_id()` | Self | low |
| `get_block_time()` | Head time | low |
| `get_balance(account, symbol)` | Read-only | medium |
| `transfer(...)` | **Deferred / disallowed in v1** | N/A |
| `log(msg)` | Debug; capped; not consensus-critical | low |
| `sha256(bytes)` | Hash | per byte |
| `abort()` | Trap | low |

**Disallowed:** network, filesystem, threads, random, clock_gettime wall, unbounded alloc, FFI.

**Reentrancy:** v1 forbids contract→contract calls; no reentrancy. Future: explicit call stack depth 1–4 with fuel share.

---

## 9. Isolation and determinism

- Single-threaded execution per call in v1  
- Memory limit (e.g. 4–16 MiB) configurable, consensus-fixed when activated  
- No SIMD non-determinism; disable non-deterministic proposals  
- Trap on OOB memory  
- Integer-only ABI preferred for app authors  

---

## 10. Light-node / pruned behavior

| Node | Behavior |
|------|----------|
| Full + plugin | Execute and store |
| Pruned | Code hash + storage root; code blobs optional |
| Light | **Verify-only:** check signatures and optional storage proofs; **skip** execution |
| Witness without plugin (pre-3h) | Ignore plugin; consensus unaffected |

Post-3h consensus: all consensus nodes must execute deterministically — plugin becomes required code path or merges into chain.

---

## 11. Security gate (mandatory)

- [ ] Host allow-list reviewed  
- [ ] Fuel cannot be bypassed  
- [ ] No reentrancy in v1  
- [ ] Code size limits  
- [ ] Argument size limits  
- [ ] Fuzzing harness (3f) before any mainnet design sign-off  
- [ ] Supply-chain: pin runtime version  

---

## 12. Performance approach

- CPU-heavy calls: candidate for worker pool **only if** determinism and ordering preserved (likely serial per contract_id).  
- Block packing: max total fuel per block (consensus param).  
- Microbenchmarks: deploy 10KB module; call empty export; call storage write N keys.  

Budgets: see `04-performance-budgets.md`.

---

## 13. HAF / API

- Tables: `contracts`, `contract_calls`, `contract_storage_updates` (event sourced)  
- API: `get_contract`, `get_storage_key`, `list_contracts_by_owner`  
- Light: omit code blob  

---

## 14. Sub-phase checklist

| Sub-phase | Deliverable | Consensus? |
|-----------|-------------|------------|
| 3a | Runtime selection + plugin skeleton | No |
| 3b | Deploy/call interface (custom_json or gated ops) | No |
| 3c | Isolated storage provider | No |
| 3d | Fuel → RC mapping experiments | No |
| 3e | Host allow-list + audit | No |
| 3f | Fuzzing harness | No |
| 3g | Light-node verify path design | No |
| 3h | Consensus activation design | **Human gate** |

---

## 15. Open questions (human)

1. Wasmtime vs Wasmer vs WAMR?  
2. `custom_json` experiment vs early op structs with HF=off?  
3. Storage rent vs deposit?  
4. Allow token transfer host in v1? (**recommend no**)  

---

## 16. Acceptance

- [x] Plugin-first strategy  
- [x] Runtime options without premature lock-in  
- [x] Metering, isolation, host allow-list, light path  
- [ ] Security Reviewer approval  
- [ ] Human: defer runtime pick to 3a  
