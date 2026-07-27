# perf-13 – HTLC preimage fuzz (deterministic)

**Task-ID:** swarm-perf / fuzz-htlc-preimage  
**Status:** PASS  
**Date:** 2026-07-27  
**Branch:** `swarm-perf-p0-impl`  
**Workdir:** `/tmp/hive-native-features`  
**Catalogue / strategy:** [docs/05-verification-and-testing-strategy.md](../05-verification-and-testing-strategy.md) §8 (HTLC redeem: random preimage lengths → no crash; no unauthorized credit)

---

## 1. Purpose

Portable, **deterministic** fuzz harness for HTLC redeem security properties:

1. Wrong / random preimages **never crash** the evaluator path.
2. Failed redeem **never credits** any account (including third parties).
3. Wrong redeemer (`to` ≠ HTLC.to) is rejected even with the correct preimage (ADR-0001).
4. Correct preimage credits **only** HTLC.to exactly once; double-redeem does not re-credit.

This is a fixed-iteration pseudo-random stress test (CI-friendly), not a coverage-guided fuzzer like libFuzzer/AFL.

---

## 2. Implementation

| Artifact | Role |
|----------|------|
| [`tests/fuzz_htlc_preimage.cpp`](../../tests/fuzz_htlc_preimage.cpp) | xorshift64 PRNG, 5000 iterations, oracle checks |
| CMake target `hive_native_fuzz_htlc` | Always-on when `HIVE_NATIVE_BUILD_FUZZ=ON` (default) |
| ctest name `hive_native_fuzz_htlc` | Short-runtime regression gate |

### Loop (per iteration)

1. Draw lock amount, preimage size (1–64), hash algo (SHA-256 or RIPEMD-160).
2. Build correct preimage bytes from xorshift; `htlc_create` from alice → bob.
3. **8 wrong attempts** mixed modes: same-size bad bytes, size mismatch, wrong redeemer + correct preimage, empty preimage — each must throw/reject with **unchanged balances** and HTLC still `open`.
4. Correct redeem: bob `+amount`, alice/carol unchanged, status `redeemed`.
5. Double-redeem rejected; bob balance stable.

### Determinism

- Fixed seed: `0xC0FFEEF1A5202601`
- Pure xorshift64 (no `rand()`, no time, no threads)
- Same host → same pass/fail (bit-identical control flow for a given library)

---

## 3. How to build & run

```bash
cd /tmp/hive-native-features
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target hive_native_fuzz_htlc -j
./build/hive_native_fuzz_htlc
# or:
ctest --test-dir build -R hive_native_fuzz_htlc --output-on-failure
```

Optional disable:

```bash
cmake -S . -B build -DHIVE_NATIVE_BUILD_FUZZ=OFF
```

Success line shape:

```
fuzz_htlc_preimage seed=0xc0ffeef1a5202601 iterations=5000 ok=5000 wrong_rejected=40000 correct_accepted=5000 failures=0
fuzz_htlc_preimage PASSED
```

---

## 4. Result (this task)

```
$ cmake --build build --target hive_native_fuzz_htlc -j && ./build/hive_native_fuzz_htlc
fuzz_htlc_preimage seed=0xc0ffeef1a5202601 iterations=5000 ok=5000 wrong_rejected=40000 correct_accepted=5000 failures=0
fuzz_htlc_preimage PASSED

$ ctest --test-dir build -R hive_native_fuzz_htlc --output-on-failure
100% tests passed out of 1
```

| Check | Expect | Result |
|-------|--------|--------|
| Exit code | 0 | **0** |
| `failures` | 0 | **0** |
| `ok` | 5000 | **5000** |
| `wrong_rejected` | 40000 (8×5000) | **40000** |
| `correct_accepted` | 5000 | **5000** |
| Runtime | short (CI) | **~0.3 s** wall (ctest) |

---

## 5. Design references

- [docs/02-htlc-design.md](../02-htlc-design.md) — create/redeem rules, edge matrix
- [docs/05-verification-and-testing-strategy.md](../05-verification-and-testing-strategy.md) §8 — fuzz oracles
- ADR-0001 — redeem authority is `to` only
- Unit coverage (non-fuzz): [`docs/swarm/14-htlc-tests.md`](./14-htlc-tests.md)
