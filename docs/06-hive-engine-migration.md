# 06 – Hive-Engine → Native NFT Migration

**Task-ID:** phase-1 / 1.11  
**Status:** design  
**Last Updated:** 2026-07-26  
**Depends on:** `docs/01-nft-design.md` §13 (Migration from Hive-Engine), `docs/00-architecture-overview.md` §9  
**Audience:** Project teams, marketplace operators, Architect, HAF/API implementers  

---

## 1. Purpose

Expand the migration sketch in [`01-nft-design.md` §13](./01-nft-design.md) into actionable guidance for projects that today hold or trade Hive-Engine (or `custom_json`-based) NFTs and may want **native** protocol NFTs after the NFT hardfork.

This document does **not** change consensus rules. It describes product and operational paths only.

---

## 2. Principles (non-negotiable)

| Principle | Meaning |
|-----------|---------|
| **Opt-in only** | No account, collection, or token is moved without explicit project + holder process. |
| **No automatic consensus import** | Hive consensus **never** reads Hive-Engine state, side-chain DBs, or historical `custom_json` NFT ledgers to mint native objects. |
| **Additive ops** | Native NFT ops and Engine/`custom_json` continue to coexist; native does not replace Engine in one hardfork. |
| **Social legitimacy > chain magic** | Ownership continuity is a **project commitment** (announce, burn proofs, remint policy), not a free protocol guarantee. |
| **RC and state still apply** | Every native mint pays RC and creates chainbase state; mass “import” is a DoS surface if subsidized carelessly. |

These match architecture stance (`00-architecture-overview.md` §9) and NFT non-goals (no automatic Hive-Engine import in Phase 1).

---

## 3. Path summary (from design §13)

| Path | Name | Description | Trust model | Risk (summary) |
|------|------|-------------|-------------|----------------|
| **A** | Opt-in remint (fresh mint) | Project creates a native collection and mints new tokens to holders by social process | Project authority + holder cooperation | Simple; social process; double-spend if Engine tokens not retired |
| **B** | Attested bridge | Multisig / oracle / committee mints native after proof of Engine burn or lock | Bridge attestors | Trust assumption on attestors and proof pipeline |
| **C** | Parallel coexistence | Engine (or `custom_json`) remains live; native is optional / new drops only | Market and UX choice | Liquidity and identity fragmentation |

**Recommendation (from `01-nft-design.md` §13):** Document and support **A + B** operationally; **do not hardcode Engine into consensus**. Path **C** is the default until a project deliberately migrates.

---

## 4. Path A — Opt-in remint (fresh mint)

### 4.1 What it is

The project treats native NFTs as a **new issuance** under protocol rules:

1. Create a native collection (`nft_create_collection_operation`) with a clear symbol/name policy.
2. Publish a public migration schedule (snapshot height, claim window, burn-or-lock rules if any).
3. Mint native tokens (`nft_mint_operation`) to eligible accounts according to that schedule.
4. Optionally retire Engine inventory (burn, lock, or freeze trading) so dual liquidity does not confuse markets.

No bridge contract is required on Hive consensus. The “bridge” is the project’s off-chain process and signed ops.

### 4.2 When to choose A

- Collection size is manageable for creator-controlled minting and RC budget.
- Holders are mostly active Hive accounts that can receive native mints.
- The team wants maximum simplicity and minimal new infrastructure.
- Legal/product stance is “new series with documented continuity,” not “same chain object.”

### 4.3 Process sketch

```
[Announce policy] → [Optional Engine freeze/burn window]
        → [Snapshot eligibility off-chain]
        → [Create native collection]
        → [Mint to holders (batched)]
        → [Publish mint map: engine_id → native nft_id]
        → [Marketplaces index native virtual ops]
```

### 4.4 Eligibility patterns (project-defined)

| Pattern | Notes |
|---------|-------|
| Snapshot at Engine block / tx id | Export owner list; remint 1:1 |
| Claim window | Holder posts a signed message or pays a nominal fee; project mints on claim |
| Burn-then-mint | Holder burns Engine NFT first; project verifies then mints native |
| Pro-rata / tiered | Only some series migrate; document exclusions |

All patterns are **off consensus**. Witnesses do not validate Engine burns.

### 4.5 Mapping and provenance

Recommend publishing an immutable artifact (IPFS / git tag / Hive custom_json log) containing:

- Engine `symbol` / `nft_id` / series id  
- Native `collection` id + `token_id` / `nft_id`  
- Recipient account  
- Policy version and snapshot reference  

Native `metadata_hash` / `uri` should point at content that either matches Engine metadata or clearly supersedes it (version field in JSON).

### 4.6 Authority and ops

- Collection creator (active) performs mints in v1 (see `01-nft-design.md` §5.2).
- Large collections: plan RC top-ups, batching across blocks, and rate limits to avoid self-DoS.
- Soulbound / non-transferable Engine items: set `soulbound` or collection `transferable=false` consistently at mint.

### 4.7 Path A risks (detail)

| Risk | Mitigation |
|------|------------|
| Double spend (Engine + native both trade) | Burn/lock Engine tokens; marketplace delist; clear “canonical” announcement |
| Wrong recipient / wrong snapshot | Dry-run list; dispute window; hold back a reserve for corrections |
| Symbol squat on native namespace | Register symbol early after HF; document intended symbol |
| RC exhaustion mid-migration | Pre-fund creator; batch; monitor supply vs max_supply |
| Holder inactivity | Claim window + escrow account policy; document unclaimed fate |

---

## 5. Path B — Attested bridge

### 5.1 What it is

A **committee, multisig, or oracle set** attests that a specific Engine (or side) NFT was burned, locked, or otherwise retired, then triggers a native mint to the proven owner (or a declared recipient).

Consensus still only sees ordinary native ops (`nft_mint_operation` from authorized minter keys). The attestation layer is **application-level trust**, not a new consensus precompile.

### 5.2 When to choose B

- High-value or large collections where manual remint is error-prone.
- Need continuous or long-running migration (not a single snapshot day).
- Multiple issuers or a marketplace wants a shared bridge brand.
- Projects accept a published trust root (multisig members, threshold, rotation).

### 5.3 Reference architecture (non-consensus)

```
Engine burn / lock event
        │
        ▼
Indexer / watcher (Engine API, custom_json stream, or export)
        │
        ▼
Attestation service (threshold signatures or Hive multisig accounts)
        │
        ▼
Minter account(s) with active authority
        │
        ▼
nft_mint_operation → native nft_object
        │
        ▼
Public audit log (engine_id, proof refs, native nft_id, tx id)
```

Optional: require holder-signed “mint to me” intent so burned tokens cannot be redirected by a compromised relayer alone (still depends on minter honesty for issuance).

### 5.4 Proof types (examples)

| Proof | Strength | Notes |
|-------|----------|-------|
| Engine burn tx id + receipt | Medium | Depends on Engine finality and API honesty |
| Multi-indexer agreement | Higher | Independent watchers must concur |
| Time-locked challenge period | Higher | Delay mint; allow fraud proofs socially |
| Holder co-signature | Redirect protection | Does not stop malicious minter over-mint |

**None of these are verified inside Hive apply()** in Phase 1.

### 5.5 Minter authority design

- Prefer a **dedicated minter account** (or small set) with keys held by the committee—not the day-to-day project hot wallet.
- Publish threshold policy (e.g. 3-of-5) and rotation procedure.
- Cap mints per block/day at the application layer; use collection `max_supply` as a hard protocol ceiling.
- Log every mint with correlation ids for HAF/indexers.

### 5.6 Failure and dispute

| Event | Expected response |
|-------|-------------------|
| False attestation (mint without burn) | Social slash of bridge reputation; freeze minter keys; marketplace warnings; no consensus rollback of unrelated chain state |
| Missed burn (no mint) | Retry queue; manual path A fallback |
| Key compromise | Rotate authorities; pause bridge; audit supply vs Engine burns |
| Ambiguous Engine reorg / API lie | Multi-source proofs; delay mint past Engine confirmation depth |

### 5.7 Path B risks (detail)

| Risk | Mitigation |
|------|------------|
| Trusted committee is a honeypot | Threshold keys; geographic/org diversity; time delays |
| Oracle lies or lags | Multi-indexer; public watchers; challenge period |
| Unlimited mint if max_supply=0 | Set max_supply to migrated count (+ documented reserve) |
| Users confuse bridge with “trustless L1” | Docs and UI must say **attested**, not trustless |
| Regulatory / custody ambiguity | Project legal review; clear ToS for bridge operators |

---

## 6. Path C — Parallel coexistence

### 6.1 What it is

Hive-Engine (or existing `custom_json` NFT schemes) **keep running**. Native NFTs are used for:

- New collections launched natively, and/or  
- Optional dual listing without retiring Engine inventory.

No forced migration. Marketplaces and wallets may support **both** asset classes.

### 6.2 When to choose C

- Engine liquidity or tooling is still primary for the project.
- Native HF is new and indexers/wallets are incomplete.
- Legal or community process for retirement is not ready.
- Experimentation: native for a side series, Engine for the main series.

### 6.3 Operational guidance

- Use **distinct names/symbols** and metadata so users never assume 1:1 fungibility across rails.
- Marketplaces should show rail badges: `native` vs `engine`.
- Avoid auto-routing “the same NFT” across rails without an explicit A/B process.
- Plan a future cutover (A or B) with a published date rather than silent dual minting.

### 6.4 Path C risks (detail)

| Risk | Mitigation |
|------|------------|
| Fragmented liquidity | Pick a primary rail; dual-list only with clear UX |
| Confused royalties / provenance | Separate metadata; link docs both ways |
| Support burden (two stacks) | Time-box dual support; sunset policy |
| Fake “wrapped” listings | Verified collection registries off-chain |

---

## 7. No automatic consensus import

### 7.1 Explicit non-goal

Hive block producers **must not**:

- Pull Hive-Engine levelDB / side state into chainbase at hardfork time  
- Interpret historical `custom_json` NFT payloads as native `nft_object`s without a user-submitted op  
- Embed Engine RPC dependencies inside `evaluator` apply paths  
- Treat any single bridge multisig as a protocol-enshrined system account  

### 7.2 Why

| Reason | Explanation |
|--------|-------------|
| Determinism | Engine state is not part of Hive consensus history; nodes would disagree without a shared import snapshot. |
| Validation cost | Full import would bloat state and block time; RC would not gate genesis dump fairly. |
| Sovereignty | Projects and holders choose whether and how to move; protocol neutrality. |
| Attack surface | A bogus import snapshot or Engine fork would poison native ownership. |
| Upstream fitness | Clean HF patches stay reviewable; no hidden side-channel I/O in apply. |

### 7.3 What *is* allowed

- Off-chain indexers reading Engine + native HAF tables  
- Application bridges (path B) that submit ordinary signed ops  
- Documentation, SDKs, and marketplace features that assist A/B/C  
- Future **optional** plugin tools (non-consensus) that help operators build mint batches  

---

## 8. Checklist for projects migrating NFTs

Use this as a go-live gate. Check items that apply to the chosen path.

### 8.1 Strategy

- [ ] Choose path **A**, **B**, **C**, or a sequenced combo (e.g. C → A).
- [ ] Publish a migration brief: motivation, timeline, what “canonical” means after cutover.
- [ ] Confirm native HF is active on target chain/environment before minting.
- [ ] Confirm wallets/marketplaces the community uses can show native NFTs (or provide interim explorers).

### 8.2 Collection design (native)

- [ ] Symbol available and reserved (`nft_create_collection`); matches brand policy.
- [ ] `max_supply` set appropriately (prefer hard cap = migrated supply + documented reserve).
- [ ] Transferable vs soulbound policy matches Engine behavior (or documented differences).
- [ ] Metadata scheme: `metadata_hash` algorithm (SHA-256), URI length ≤ design cap, media still off-chain.
- [ ] Creator / minter keys: cold storage, multisig, or bridge committee as required.

### 8.3 Holder and market process

- [ ] Snapshot or continuous eligibility rules published (height, API source, hash of export).
- [ ] Dispute / correction window defined.
- [ ] Engine retirement plan (burn, lock, delist) if avoiding dual trading—**or** explicit dual-rail UX for path C.
- [ ] Marketplace and explorer partners notified with symbol / collection id / sample txs.
- [ ] Royalty / license text updated if the legal object is a new series.

### 8.4 Path A extras

- [ ] Mint batching plan (accounts per block, RC budget, resume-from-failure).
- [ ] Public mint map (Engine id → native id) released and archived.
- [ ] Unclaimed inventory policy (burn, treasury, extended claim).

### 8.5 Path B extras

- [ ] Attestor set, threshold, and key ceremony documented.
- [ ] Proof format and confirmation depth documented.
- [ ] Challenge period and pause switch tested.
- [ ] Audit log schema and public endpoint live before first mint.
- [ ] Incident response: rotate keys, freeze mints, communicate overages.

### 8.6 Path C extras

- [ ] UI labels for native vs Engine everywhere user-facing.
- [ ] No shared token id namespace assumptions in app code.
- [ ] Sunset or revisit date for dual support.

### 8.7 Technical verification

- [ ] Testnet dry-run of full mint set (or statistically significant sample).
- [ ] Auth matrix: only intended minter can mint; holders can transfer/burn per policy.
- [ ] Indexer/HAF projections match chain (`nft_minted` virtual ops).
- [ ] Light/API clients paginate lists; no reliance on full metadata in chain state.
- [ ] RC costs measured for planned mint volume (`04-performance-budgets.md`).

### 8.8 Communication

- [ ] Announcement on project channels + Hive posts with dates in UTC.
- [ ] FAQ: fees, soulbound, marketplace links, scam warnings (fake bridges).
- [ ] Single canonical URL for migration status.

---

## 9. Risks (consolidated)

| ID | Risk | Paths | Severity | Primary mitigation |
|----|------|-------|----------|--------------------|
| R1 | Dual trading / double representation | A, B, C | High | Burn/lock Engine or explicit dual-rail UX |
| R2 | Bridge or project key compromise → unauthorized mint | A, B | Critical | Multisig, max_supply, pause, monitoring |
| R3 | Incorrect eligibility snapshot | A, B | High | Published export hash, dispute window |
| R4 | Users believe import is trustless L1 | B | Medium | Honest “attested” labeling |
| R5 | State/RC DoS via mass mint | A, B | High | RC costs, batching, max_supply |
| R6 | Liquidity and brand fragmentation | C | Medium | Primary rail choice, clear labels |
| R7 | Marketplace desync (Engine vs native) | All | Medium | Virtual ops + HAF; partner checklists |
| R8 | Metadata / legal mismatch after remint | A, B | Medium | Versioned metadata; license update |
| R9 | Unclaimed holders after deadline | A | Medium | Extended claim; published residual policy |
| R10 | Consensus feature creep (“just import Engine”) | All | Critical | **Refuse** automatic import; this doc §7 |

Architecture-level related risks (state bloat, light-node) remain as in `00-architecture-overview.md` §10 and NFT design pruning/light sections.

---

## 10. Roles and responsibilities

| Actor | Responsibility |
|-------|----------------|
| Project team | Path choice, policy, keys, holder comms, mint map |
| Holders | Follow claim/burn instructions; verify recipient accounts; beware fake bridges |
| Bridge attestors (B) | Honest proofs, uptime, key hygiene, public logs |
| Marketplaces / wallets | Dual-rail labeling, collection verification, delist retired Engine series when asked |
| Witnesses / protocol | Ship native ops only; **no** Engine import in apply |
| Indexers / HAF | Project native virtual ops; optional Engine side indexes stay separate |

---

## 11. Relationship to other docs

| Doc | Relationship |
|-----|--------------|
| [`01-nft-design.md` §13](./01-nft-design.md) | Source sketch (paths A/B/C); this file is the expansion (task 1.11) |
| [`00-architecture-overview.md` §9](./00-architecture-overview.md) | Migration stance: opt-in, no automatic migrate |
| [`04-performance-budgets.md`](./04-performance-budgets.md) | RC and apply budgets for mint volume planning |
| [`05-verification-and-testing-strategy.md`](./05-verification-and-testing-strategy.md) | Test expectations for evaluators (not Engine import tests) |
| [`docs/swarm/09-migration.md`](./swarm/09-migration.md) | Short swarm pointer to this document |

---

## 12. Out of scope (Phase 1)

- Enshrined Engine adapter in `libraries/chain`  
- Automatic airdrop at hardfork height  
- Cross-chain bridges beyond Hive-Engine-style side rails (see also NFT non-goals)  
- Full royalty enforcement as migration glue  
- Legal templates for securities / IP (project counsel)

---

## 13. Acceptance (documentation)

- [x] Paths A, B, C specified with process and risks  
- [x] No automatic consensus import stated as hard rule  
- [x] Project migration checklist included  
- [x] Consolidated risk table  
- [x] References `01-nft-design.md` migration section  

---

## 14. Revision history

| Date | Change |
|------|--------|
| 2026-07-26 | Initial expansion of `01-nft-design.md` §13 (task 1.11) |
