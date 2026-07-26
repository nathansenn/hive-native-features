# ADR-0002 – WASM runtime: Wasmtime

**Date:** 2026-07-26  
**Status:** Accepted  
**Phase:** 3a  

## Decision

Use **[Wasmtime](https://wasmtime.dev/)** (Bytecode Alliance) as the default runtime for Hive metered contracts.

## Rationale

| Criterion | Wasmtime |
|-----------|----------|
| Sandboxing | Strong, production-proven |
| Fuel / epoch interruption | First-class fuel metering |
| Determinism path | Cranelift; disable non-deterministic proposals |
| Memory limits | Store limits API |
| Maintenance | Bytecode Alliance; active |
| Embed | C API + C++ possible; static link options |
| License | Apache-2.0 WITH LLVM-exception (compatible) |

### Alternatives rejected for default

- **Wasmer:** capable, but multi-backend complexity; higher version churn risk for consensus later.
- **WAMR:** excellent for tiny embeds; weaker long-term consensus/tooling bet for full contract platform.
- **Custom interpreter:** too slow / high maintenance.

## Consequences

- Plugin skeleton depends on Wasmtime C API (optional compile flag `HIVE_NATIVE_WITH_WASMTIME`).
- Without Wasmtime installed, builds still compile a **null engine** for CI and protocol work.
- Consensus activation (3h) remains human-gated; this only locks the *preferred* engine for experiments and future HF design.
