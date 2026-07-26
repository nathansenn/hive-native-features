# 11 – Plugin layer (`hive_contracts`)

**Task-ID:** phase-3 / 3a  
**Audience:** Architect, Protocol Coder, Reviewer, GitOps  
**Status:** skeleton in tree; non-consensus  

---

## 1. Purpose

Document how the agent swarm treats the **plugin-first** contracts path so no
specialist accidentally promotes WASM execution into consensus before the
**Phase 3h human gate**.

Primary tree:

```
plugins/hive_contracts/
├── README.md
├── include/hive_contracts_plugin.hpp
└── src/hive_contracts_plugin.cpp
```

Core engine (shared library):

- `include/hive_native/contracts/engine.hpp`
- `src/contracts/engine.cpp` → `hive_native::contracts::default_engine()`

---

## 2. Plugin-first rules (3a–3g)

1. **Non-consensus by default** — plugin deploy/call must not be required for
   block validation or shared chain state until 3h.
2. **Wasmtime is the selected runtime** — [ADR-0002](../decisions/ADR-0002-wasmtime.md).
3. **Build flag** — `HIVE_NATIVE_WITH_WASMTIME` (CMake option, default **OFF**).
   - OFF → `null_engine` for CI and protocol work.
   - ON + `WASMTIME_ROOT` → link Wasmtime C API when available.
4. **Thin façade** — `hive_contracts_plugin` only wraps `default_engine()`
   (`init`, `shutdown`, `deploy`, `call`). Heavy logic stays in `hive_native::contracts`.
5. **Host allow-list** — transfer / net / fs / random / wall clock denied in v1
   (see engine host policy + design doc §8).
6. **Storage** — apply writes only on successful calls; no partial commit.

---

## 3. Consensus activation is Phase 3h — human-gated

| Rule | Detail |
|------|--------|
| Gate | **Phase 3h only**, after explicit **human approval** |
| Not automatic | Completing 3a–3g does **not** flip HF or require WASM on witnesses |
| HF placeholder | `HIVE_HARDFORK_CONTRACTS` (e.g. `9003`) remains TBD upstream |
| Swarm duty | Never open “enable consensus contracts” PRs without Architect + human |

### What agents may do before 3h

- Extend plugin stubs, storage provider, fuel→RC experiments, fuzz harness.
- Document light-node verify-only / skip paths.
- Keep evaluators HF-gated and off for mainnet sketches.

### What agents must not do before 3h

- Merge WASM into the mandatory consensus apply path.
- Set mainnet HF numbers or “activate by default” flags.
- Claim Phase 3 complete without the human 3h decision recorded.

See [docs/03-contracts-design.md](../03-contracts-design.md) §14–16 and
[ADR-0001](../decisions/ADR-0001-phase-0-human-gates.md).

---

## 4. Sub-phase ownership (swarm)

| Sub-phase | Owner roles | Deliverable |
|-----------|-------------|-------------|
| 3a | Architect + Protocol Coder | Runtime ADR + plugin skeleton |
| 3b | Protocol Coder | Deploy/call interface (`custom_json` and/or gated ops) |
| 3c | Protocol Coder | Isolated storage provider |
| 3d | Performance + Protocol | Fuel → RC mapping experiments |
| 3e | Reviewer + Protocol | Host allow-list audit |
| 3f | Test & Verification | Fuzzing harness |
| 3g | Architect + Light-path | Light-node verify design |
| **3h** | **Human + Architect** | **Consensus activation design** |

---

## 5. Verification checklist (plugin work)

- [ ] Plugin still optional (node without it can follow consensus pre-3h)
- [ ] `HIVE_NATIVE_WITH_WASMTIME` documented; default build needs no Wasmtime
- [ ] Stubs call `hive_native::contracts::default_engine()` only
- [ ] No secrets; no unbounded host surface added
- [ ] Docs state **3h human gate** for consensus
- [ ] Reviewer + tests green before GitOps commit

---

## 6. References

| Doc / path | Why |
|------------|-----|
| [plugins/hive_contracts/README.md](../../plugins/hive_contracts/README.md) | Build flags, lifecycle |
| [ADR-0002 – Wasmtime](../decisions/ADR-0002-wasmtime.md) | Runtime decision |
| [03 – Contracts design](../03-contracts-design.md) | Metering, host list, phases |
| [00 – Architecture](../00-architecture-overview.md) | Hybrid phase model |
| `include/hive_native/contracts/engine.hpp` | Engine API |
