# 05 – Verification and Testing Strategy

**Task-ID:** phase-0 / 0.7  
**Status:** design  
**Last Updated:** 2026-07-24  

---

## 1. Purpose

Make “verify before commit” operational. Every TASK-ID maps to checklists in `WORKFLOW.md` §6 and to concrete tests described here.

---

## 2. Test pyramid

```
        /\
       /  \      Fuzz / adversarial (HTLC, WASM)
      /----\
     / Integ \   Multi-op scenarios, undo, HF guards
    /--------\
   /  Unit    \  Serialization, auth matrix, pure logic
  /____________\
  Microbenchmarks & RC measurement (parallel track)
```

---

## 3. Gate mapping

| Change type | Checklists | Required tests |
|-------------|------------|----------------|
| Docs only | Universal (docs subset) | Markdown links OK; no secrets |
| Protocol / ops | Universal + Protocol | Ser/deser, validation unit tests |
| State / indexes | Universal + State | Object size, index uniqueness, undo |
| Evaluators | Universal + Evaluator | Auth matrix, failure paths, virtual ops |
| RC | Universal + Performance | Cost smoke; TODO allowed with TASK-ID |
| HAF / API | Universal + HAF | Schema lint; stub method signatures |
| HTLC | + Security | Full edge matrix in `02-htlc-design.md` §16 |
| WASM / contracts | + Security + Performance | Fuel trap, host deny, fuzz corpus |

---

## 4. Unit testing standards

- Framework: match upstream Hive (Boost.Test / existing chain tests).  
- Location: mirror upstream layout under portable `tests/` in this repo until patched upstream.  
- Naming: `test_<feature>_<behavior>`.  
- Each bugfix: regression test with TASK-ID in comment.

**Minimum auth matrix (NFT example):**

| Actor | Op | Expect |
|-------|-----|--------|
| owner | transfer | pass |
| approved | transfer | pass |
| stranger | transfer | fail |
| owner posting key only | transfer | fail |
| owner | burn soulbound | pass |
| owner | transfer soulbound | fail |

---

## 5. Integration testing

Scenarios:

1. Mint → approve → transfer → burn  
2. HTLC create → redeem; create → expire → refund  
3. Mixed block: transfers + NFT + HTLC ordering  
4. Undo: apply block, pop block, balances/objects restore  
5. HF off: ops rejected; HF on: ops accepted  

Use fixture chains with funded accounts.

---

## 6. Light-node / skip path tests

- Config flag enables skip.  
- Assert: no NFT tables/indexes allocated (or empty providers).  
- Assert: block apply still succeeds for non-consensus light roles.  
- Assert: witness config refuses skip if that is policy (recommended: witnesses cannot set skip).

---

## 7. Performance testing

See `04-performance-budgets.md`.

- Microbench on every PR touching apply/state.  
- Record results in PR body.  
- Red builds block GitOps commit.

---

## 8. Fuzzing (Phase 2+ / 3f)

| Target | Input | Oracle |
|--------|-------|--------|
| HTLC redeem | Random preimage lengths | No crash; no unauthorized credit |
| NFT ser/deser | Random bytes | No crash |
| WASM module | wasm-smith / corpus | Fuel trap; no host escape |
| Host args | Random keys/values | Size rejects |

Fuzz jobs: time-boxed in CI (e.g. 60s PR / longer nightly).

---

## 9. Security review triggers

Mandatory Security Reviewer approval when diff touches:

- Authority evaluation  
- HTLC time/hash logic  
- WASM host functions  
- Balance debits/credits  
- Pruning that might resurrect or drop value  

---

## 10. Verification report format

Test agent output (required before READY_TO_COMMIT):

```
VERIFICATION REPORT
Task-ID: phase-1-1.3
Checklists: Universal=PASS Protocol=PASS Evaluator=PASS Security=N/A Perf=N/A
Tests run: <list>
Tests result: PASS | FAIL
Failures: <none or details>
Reviewer: APPROVED | REQUESTS_CHANGES
Performance: <numbers or N/A>
Ready: YES | NO
```

---

## 11. CI plan (this repo)

Phased introduction:

| Stage | CI jobs |
|-------|---------|
| Phase 0 | Doc lint (markdown), secret scan |
| Phase 1 | Build portable protocol sketches; unit tests |
| Phase 2 | + HTLC edge tests |
| Phase 3 | + plugin build; fuzz smoke |

Secret scan: fail on PEM/key patterns.  
Never store API tokens in repo.

---

## 12. Documentation verification

For design commits:

- [ ] Internal links resolve  
- [ ] Open questions listed  
- [ ] Human gates explicit  
- [ ] Consistent with architecture constraints  

---

## 13. Escalation

After 3 failed verify cycles → human escalation message per WORKFLOW §9 with full report attachment (PR comment).

---

## 14. Acceptance

- [x] Pyramid, gates, auth matrix, fuzz, CI stages  
- [ ] Reviewer approval  
- [ ] Test agent adopts report format on first code task  
