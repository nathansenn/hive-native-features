# hive_contracts plugin (skeleton)

**Status:** directory reserved for hived plugin glue (Phase 3a+).  
**Runtime decision:** [Wasmtime](https://wasmtime.dev/) — see `docs/decisions/ADR-0002-wasmtime.md`.  
**Portable engine:** `hive_native::contracts` in the main library (null engine for CI).

This plugin is **non-consensus** until Phase 3h (human gate). Consensus nodes without the plugin must remain unaffected pre-activation.

---

## Build / engine selection

| Mode | Flag | Engine |
|------|------|--------|
| CI / default | `HIVE_NATIVE_WITH_WASMTIME=OFF` (default) | `null_engine` via `default_engine()` |
| Optional Wasmtime | `HIVE_NATIVE_WITH_WASMTIME=ON` + `WASMTIME_ROOT` | Intended real runtime (not fully wired yet) |

Portable sources:

- Interface / host policy: `include/hive_native/contracts/engine.hpp`
- Null implementation: `src/contracts/engine.cpp`
- Apply path: `src/chain/evaluators_contracts.cpp`

---

## Host function table

Static policy in `host_allowed()` / `host_fuel_weight()`. **Denied hosts must never be registered** as Wasm imports when Wasmtime is linked.

### Allowed (v1)

| Host | Enum | Purpose | Fuel weight (portable) |
|------|------|---------|------------------------|
| `read_storage` | `host_fn::read_storage` | Load contract K/V | `50 + bytes` |
| `write_storage` | `host_fn::write_storage` | Store K/V | `200 + bytes*2` |
| `remove_storage` | `host_fn::remove_storage` | Delete key | `80` |
| `get_caller` | `host_fn::get_caller` | Caller account | `10` |
| `get_contract_id` | `host_fn::get_contract_id` | Self id | `10` |
| `get_block_time` | `host_fn::get_block_time` | Head time (block context only) | `10` |
| `get_balance` | `host_fn::get_balance` | Read-only balance | `40` |
| `sha256` | `host_fn::sha256_host` | Hash | `30 + bytes` |
| `log` | `host_fn::log_msg` | Capped debug log | `20 + bytes` |
| `abort` | `host_fn::abort_call` | Trap call | `1` |

### Denied (v1 — must stay off)

| Host | Enum | Reason |
|------|------|--------|
| `transfer` | `host_fn::transfer` | No token movement from contracts in v1 |
| `net` | `host_fn::net` | Non-deterministic / side effects |
| `fs` | `host_fn::fs` | Host filesystem forbidden |
| `random` | `host_fn::random` | Non-deterministic |
| `wall_clock` | `host_fn::wall_clock` | Use block time only |

Denied paths use a large default fuel weight (`1_000_000`) if ever reached by mistake — still fail closed via allow-list / trap, not “free” work.

**Reentrancy:** v1 forbids contract→contract calls (no host for cross-contract call).

---

## Fuel and storage rules (apply path)

1. Caller sets `fuel_limit` on deploy/call ops (must be &gt; 0).
2. Engine meters via fuel; **cannot** exceed limit (`burn` / Wasmtime fuel when linked).
3. Host functions have **static weights** ≥ real work; users cannot bypass fuel by choosing hosts.
4. On success: evaluator commits staged `storage_writes` to `db.contract_storage`.
5. On failure: RC/fuel still accounted; **no** durable storage mutation; virtual `contract_called` with `success:false`.

Limits (`host_limits`): default max memory 4 MiB; key ≤ 256 B; value ≤ 4 KiB; total storage quota field 64 KiB (total quota enforcement incomplete on null engine — see review).

---

## Null engine meta-protocol (tests / CI)

Interprets UTF-8 `args` (not real WASM). Used so CI validates metering and deny policy without Wasmtime.

| Args | Behavior |
|------|----------|
| empty / `OK` | Success after base call fuel |
| `BURN:N` | Burn N fuel; OOF → fail |
| `DENY:transfer` | Simulate denied host; fail |
| `WRITE:key:value` | Stage storage write (fuel + size checks) |
| `READ:key` | Charge read fuel (value not returned in portable stub) |
| other | Generic cost `20 + args.size()` |

See file header comments in `src/contracts/engine.cpp`.

---

## Planned plugin layout (future)

```
plugins/hive_contracts/
  README.md          ← this file
  hive_contracts_plugin.*   (register with hived)
  wasmtime_engine.*         (optional HIVE_NATIVE_WITH_WASMTIME)
  rpc / custom_json hooks   (experimental non-consensus interface)
```

Design reference: `docs/03-contracts-design.md`.  
Review: `docs/swarm/04-contracts-review.md`.
