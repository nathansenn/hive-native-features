# Crypto Known-Answer Vectors

**Task:** Enhance `tests/test_runner.cpp` with published hash vectors  
**Binary:** `./build/hive_native_tests`  
**Date:** 2026-07-25

## Vectors under test

| Algorithm   | Input        | Expected digest (hex) |
|-------------|--------------|------------------------|
| SHA-256     | `"abc"`      | `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad` |
| SHA-256     | `""` (empty) | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| RIPEMD-160  | `"abc"`      | `8eb208f7e05d987a9b044a8e98c6b087f15a0bfc` |

Sources:

- SHA-256: FIPS 180-4 (NIST)
- RIPEMD-160: ISO/IEC 10118-3 / common published test vector for `"abc"`

## Implementation notes

- Existing SHA-256(`"abc"`) check retained (byte-by-byte via shared `check_hex` helper).
- Empty-string SHA-256 vector added.
- RIPEMD-160(`"abc"`) vector added using `ripemd160(const uint8_t*, size_t)`.
- No production crypto code changed; portable implementations in `src/util/crypto.cpp`.

## Rebuild and run

```bash
cd /tmp/hive-native-features/build
cmake --build . --target hive_native_tests -j
./hive_native_tests
```

## Results

```
passed=115 failed=0
```

| Metric  | Count |
|---------|------:|
| Passed  |   115 |
| Failed  |     0 |

All crypto known-answer checks and the full existing suite (NFT, HTLC, contracts, RC, host allow-list) passed.
