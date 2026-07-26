# NFT implementation review (portable Phase 1)

**Scope:** read-only review under `/tmp/hive-native-features`  
**Sources:** ADR-0001, `docs/01-nft-design.md`, protocol/evaluators/database/RC, tests  
**Tests at review time:** `hive_native_tests` → `passed=165 failed=0`  
**Production code changed:** none  

---

## Checklist summary

| # | Item | Verdict |
|---|------|---------|
| 1 | approval-for-all present per ADR-0001 | **PASS** |
| 2 | soulbound fails transfer | **PASS** |
| 3 | approve cleared on transfer | **PASS** |
| 4 | max supply enforced | **PASS** |
| 5 | light/consensus skip rules | **PASS** |
| 6 | authority matrix | **PASS** (gaps noted) |
| 7 | RC charged | **PASS** (portable stub) |

**Overall:** Phase 1 NFT MVP matches ADR-0001 and the design’s security-critical behaviors. No critical consensus bugs found. Non-critical gaps below (docs drift, soft light-path, RC not real mana).

---

## 1. approval-for-all present per ADR-0001 — **PASS**

**ADR-0001:** MVP includes approval-for-all; ship `nft_set_approval_for_all_operation` + `nft_operator_object`.

| Layer | Evidence |
|-------|----------|
| Protocol | `include/hive_native/protocol/nft_operations.hpp:66-76` — `nft_set_approval_for_all_operation` (owner, operator_account, collection, approved); virtual `nft_approval_for_all_operation` at `:113-118` |
| State | `include/hive_native/chain/database.hpp:50-56` — `nft_operator_object`; map at `:107-108` |
| Lookup | `src/chain/database.cpp:30-41` — `is_operator_approved` checks collection-specific key then `collection=0` (all) |
| Apply | `src/chain/evaluators_nft.cpp:122-149` — set/erase operator; transfer uses operator-for-all at `:86-87` |
| Validate | `src/protocol/validate.cpp:40-44` — rejects self-as-operator |
| Tests | `tests/test_runner.cpp:101-128` (all-collections transfer), `:213-266` (collection scope), `:268-299` (revoke) |

---

## 2. soulbound fails transfer — **PASS**

| Layer | Evidence |
|-------|----------|
| State | `nft_object.soulbound` in `database.hpp:46` |
| Mint | `evaluators_nft.cpp:65` — `n.soulbound = op.soulbound \|\| !col.transferable` (collection non-transferable forces soulbound) |
| Transfer | `evaluators_nft.cpp:89` — `if(n.soulbound) throw protocol_error("soulbound");` (after auth check) |
| Tests | `test_runner.cpp:130-147` — owner transfer of soulbound NFT `CHECK_THROW` |

Fail-closed: transfer throws; burn still allowed for owner (`evaluators_nft.cpp:151-168`), consistent with design burn-by-owner.

---

## 3. approve cleared on transfer — **PASS**

| Layer | Evidence |
|-------|----------|
| Apply | `evaluators_nft.cpp:97` — `n.approved.clear();` after ownership change |
| Design | `docs/01-nft-design.md` §5.3 — “clear approved” |
| Tests | `test_runner.cpp:92-98` — transfer by per-token approved operator then `CHECK(db.nfts[1].approved.empty())` |

**Note (not a bug):** operator-for-all entries are **not** cleared on transfer (owner-scoped approvals; new owner does not inherit them). Correct.

---

## 4. max supply enforced — **PASS**

| Layer | Evidence |
|-------|----------|
| Mint gate | `evaluators_nft.cpp:53-54` — `if(col.max_supply != 0 && col.supply >= col.max_supply) throw` |
| Unlimited | `max_supply == 0` skips the check (design: 0 = unlimited) |
| Supply++ | `evaluators_nft.cpp:68` |
| Burn | `evaluators_nft.cpp:163-164` — supply decremented (outstanding supply, remint allowed) |
| Tests | `test_runner.cpp:159-173` — `max_supply=1`, second mint throws |

---

## 5. light / consensus skip rules — **PASS**

| Layer | Evidence |
|-------|----------|
| Config | `database.hpp:86-91` — `nft_skip_state`, `is_consensus_node` |
| Guard | `database.cpp:63-68` — consensus+skip → `"consensus node cannot skip NFT state"`; any skip → `"NFT state skipped on this node"` |
| Evaluators | every NFT `apply` calls `db.require_full_nft_state()` (`evaluators_nft.cpp:19,45,77,106,124,153`) |
| API | `database_api_stubs.cpp:8,17,34` — empty/nullopt when skip |
| Tests | `test_runner.cpp:175-188` — non-consensus skip throws; consensus skip throws |

**Non-critical gap:** design §9 / verification strategy describe light nodes validating op structure without storing state and “block apply still succeeds.” Portable evaluators **fail closed** when skip is set rather than structure-only no-op. Safe for consensus; soft light apply path is not implemented (callers must skip NFT evaluators entirely).

---

## 6. authority matrix — **PASS** (coverage gaps)

### Apply-time / declared authorities

| Op | Required (design) | Implementation | Evidence |
|----|-------------------|----------------|----------|
| create_collection | creator active | `get_required_active_authorities → {creator}`; symbol unique | `nft_operations.hpp:28`; `evaluators_nft.cpp:22` |
| mint | creator active (v1) | active of creator + `col.creator == op.creator` | `nft_operations.hpp:41`; `evaluators_nft.cpp:52` |
| transfer | owner **or** approved **or** operator-for-all | `is_owner \|\| is_token_approved \|\| is_op_all` | `evaluators_nft.cpp:84-88` |
| approve | owner active | `owner` auth + `nft.owner == op.owner` | `nft_operations.hpp:63`; `evaluators_nft.cpp:110` |
| set_approval_for_all | owner active (ADR) | owner auth; accounts exist | `nft_operations.hpp:75`; `evaluators_nft.cpp:126-127` |
| burn | owner active | owner match | `nft_operations.hpp:84`; `evaluators_nft.cpp:157` |

All NFT ops expose **active** authorities only (no posting list) → posting-only is insufficient once wired into Hive’s authority resolver (`nft_operations.hpp` fee_payer / get_required_active_*).

### Tests vs verification matrix (`docs/05-verification-and-testing-strategy.md` §4)

| Case | Covered? |
|------|----------|
| owner transfer | not as isolated unit; path exists via non-soulbound + owner `from` |
| approved transfer | yes (`test_nft_happy_path`) |
| stranger transfer fail | yes (`test_nft_soulbound_and_auth`) |
| owner posting-only transfer fail | **no** (no signature layer in portable harness) |
| burn soulbound | **no** dedicated test (code allows owner burn) |
| transfer soulbound fail | yes |
| non-creator mint fail | yes (`test_nft_mint_non_creator_fails`) |
| collection-scoped operator | yes |

### Doc / semantic drift (not critical code bug)

- Design §5.3 “Validates: **from is owner**” conflicts with §3.3 and code: `from` is the **authorized actor** (owner, per-token approved, or operator-for-all). Tests set `t.from = "op"`. Needed so `get_required_active_authorities() → {from}` matches the signer.
- Design §5.6 table omits operator-for-all on transfer and omits `set_approval_for_all` row; code matches §3.3 + ADR-0001.

**Recommendation (docs only):** align §5.3/§5.6 with actor-in-`from` + operator-for-all.

---

## 7. RC charged — **PASS** (portable stub)

| Layer | Evidence |
|-------|----------|
| Cost fns | `src/rc/costs.cpp:8-30` — all six NFT ops; mint > transfer base pattern |
| Apply wire-up | every NFT evaluator sets `db.last_rc_charged = rc::cost_nft_*` before/at mutation (`evaluators_nft.cpp:24,56,93,114,131,159`) |
| Tests | `test_runner.cpp` ~`test_rc_positive`: transfer ≥ `TRANSFER_BASE`, mint > transfer |
| Model doc | `docs/08-rc-cost-model.md` — placeholders + `last_rc_charged` |

**Non-critical gap:** no account RC/mana balance is decremented; charge is recorded only on `database::last_rc_charged`. Acceptable for portable Phase 1; upstream must debit fee_payer.

No test asserts `last_rc_charged` after each NFT apply path.

---

## Concrete code bugs

| Severity | Finding | Location | Action |
|----------|---------|----------|--------|
| None critical | No consensus fail-open, soulbound bypass, max-supply skip, or uncleared per-token approve found | — | No production fix required |
| Low | Design §5.3 “from is owner” vs code/tests (actor in `from`) | design doc vs `evaluators_nft.cpp:84-88` | Doc fix only |
| Low | Light skip has no structure-only success path | `database.cpp:63-68` | Intentional fail-closed; document caller contract |
| Low | RC not applied to account mana | evaluators + `database.hpp:115` | Upstream integration |
| Info | Posting-key rejection untested (no crypto auth harness) | tests | Add when signature fixture exists |

---

## File map (quick)

| Concern | Primary files |
|---------|----------------|
| Ops / auth declare | `include/hive_native/protocol/nft_operations.hpp` |
| Validate | `src/protocol/validate.cpp` |
| Apply | `src/chain/evaluators_nft.cpp` |
| State / skip | `include/hive_native/chain/database.hpp`, `src/chain/database.cpp` |
| RC | `include/hive_native/rc/costs.hpp`, `src/rc/costs.cpp` |
| Design / ADR | `docs/01-nft-design.md`, `docs/decisions/ADR-0001-phase-0-human-gates.md` |
| Tests | `tests/test_runner.cpp` |

---

## Verdict

All seven checklist items **PASS**. Implementation implements ADR-0001 approval-for-all, soulbound fail-closed transfer, approve clear-on-transfer, max supply, consensus cannot skip NFT state, active-authority matrix with operator paths, and RC cost hooks. No critical bug; production code left unchanged.
