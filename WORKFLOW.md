# Complete Detailed Workflow Instructions for an Agent Swarm

**Project:** Implement hybrid native NFTs + HTLC Atomic Swaps + Metered Smart Contracts on Hive  
**Target GitHub:** `nathansenn` (authenticated user)  
**Primary Repo:** `nathansenn/hive-native-features`  
**Date:** 24 July 2026

This document is a self-contained, executable playbook. Any agent (or human + agent team) following it must treat it as the single source of truth. Every action that changes code or documentation must pass verification before being committed.

---

## 1. Core Principles (Non-Negotiable)

1. **Small, verifiable units of work** – Never commit more than one logical feature or document at a time.
2. **Verify before commit** – The Test & Verification agent must explicitly pass a checklist. No exceptions.
3. **Performance first** – Every change must preserve (or improve) 3-second block times, low RAM for mobile/pruned nodes, and RC metering.
4. **Light-node safe** – Every new feature must have an explicit skip / verify-only path for light, pruned, and mobile nodes.
5. **Incremental & reviewable** – Prefer many small commits and draft PRs over large dumps.
6. **Upstream-friendly** – Code should be written so it can later be submitted as clean patches or MRs to `gitlab.syncad.com/hive/hive` or `openhive-network/hive`.
7. **Human gates** – Pause and ask the user on architecture decisions, HF numbers, WASM runtime choice, repo visibility, and before any merge to `main`.
8. **No secrets** – Never commit keys, tokens, or credentials.
9. **Traceability** – Every commit message and PR must reference the phase and the verification that was performed.

---

## 2. Swarm Roles & Responsibilities

| Role | Core Duties | Handoff Rules | Tools Focus |
|------|-------------|---------------|-------------|
| **Architect / Planner** | Maintains overall plan, assigns next task, updates PROGRESS.md, decides phase transitions | Assigns tasks in structured messages; receives completion reports | Docs, PROGRESS.md |
| **Protocol Coder** | New operations, types, serialization, hardfork guards, authority rules | Hands completed protocol files to Reviewer + Test agent | `libraries/protocol/` equivalents |
| **State / Storage Coder** | Objects, multi-indexes, RocksDB providers, pruning logic, snapshots | Hands state code to Evaluator Coder and Test agent | Chain objects, storage |
| **Evaluator / Apply Coder** | Evaluators, RC integration, worker-pool expansion, parallel apply paths | Hands evaluators to Test + Reviewer | Evaluators, `blockchain_worker_thread_pool` |
| **HAF / API / SDK Coder** | sql_serializer tables, database_api methods, client library stubs | Hands API changes to Test agent | HAF, plugins, SDKs |
| **Test & Verification Agent** | Runs / writes tests, executes checklists, produces pass/fail report | Blocks commit if any gate fails; reports structured result | tests/, benchmarks |
| **Reviewer / Security Agent** | Diff review, authority checks, DoS analysis, WASM safety | Must approve before GitOps may commit | Code review |
| **Performance Benchmarker** | Microbenchmarks, latency/RAM/RC measurements, regression gates | Flags any change that risks > agreed threshold | Benchmark scripts |
| **GitOps / Committer Agent** | Only agent allowed to call GitHub write tools after gates pass | Receives “READY_TO_COMMIT” package; performs push + PR | All GitHub tools |

**Communication format between agents (mandatory):**

```
FROM: <Role>
TO: <Role(s)>
TASK-ID: <phase>-<number>
STATUS: assigned | in-progress | blocked | ready-for-review | verified | failed
SUMMARY: <1-3 sentences>
ARTIFACTS: <list of files or paths>
VERIFICATION: <pass/fail + notes>
NEXT: <what the receiving agent should do>
```

---

## 3. Repository Bootstrap (First Actions – Exact Sequence)

**Step 0.1 – Confirm identity** via `github___get_me` → expect `"login": "nathansenn"`.

**Step 0.2 – Check for existing repo** via search: `user:nathansenn hive-native-features OR hive-nft OR hive-contracts`.

**Step 0.3 – Create repository** `hive-native-features` (public unless human requests private).

**Step 0.4 – Create branch** `phase-0-design` from `main`.

**Step 0.5 – Initial commit** README + WORKFLOW + PROGRESS + architecture overview + `.gitignore`.

If GitHub MCP write tools return 403, GitOps falls back to local `gh` + git with verified `nathansenn` identity and no secrets.

---

## 4. PROGRESS.md Template

Keep `PROGRESS.md` current with: Current Phase, Last Updated, Active Branch, Open PRs, Completed, In Progress, Blocked, Next 3 Tasks, Verification Log.

---

## 5. The Universal Work Loop (Mandatory for every change)

1. Architect assigns TASK-ID and description to specialist agent(s).
2. Specialist(s) implement the change in a feature branch.
3. Specialist declares “ready-for-review” and lists artifacts.
4. Reviewer agent examines the diff and either APPROVES or REQUESTS_CHANGES.
5. Test & Verification agent runs the full checklist for that task type.
6. Performance Benchmarker runs if the change touches apply path, state size, or RC.
7. If all gates pass → GitOps receives a “READY_TO_COMMIT” package.
8. GitOps executes push (and opens/updates draft PR if appropriate).
9. Architect updates PROGRESS.md in the same or follow-up commit.
10. If any gate fails → return to step 2 (max 3 iterations). On 3rd failure → escalate to human.

**Decision tree for GitOps:**

- All checklists green? → commit
- Any red? → refuse and return to specialist
- Ambiguous performance impact? → require Performance Benchmarker sign-off
- Security-sensitive (authority, WASM host functions, time-locks)? → require explicit Reviewer security approval

---

## 6. Master Verification Checklists

### 6.1 Universal Checklist (every commit)

- [ ] Files are syntactically valid
- [ ] New public APIs / operations have documentation comments
- [ ] RC cost is defined or explicitly marked “TODO – measure”
- [ ] Light / pruned / mobile skip or verify-only path exists (or justified why not needed)
- [ ] No unbounded main-chainbase growth without a documented prune path
- [ ] Tests (or clear test stubs with TASK-ID) exist
- [ ] Reviewer has approved
- [ ] Commit message follows convention: `phase-X: imperative summary`
- [ ] PROGRESS.md will be updated

### 6.2 Protocol / Operation Checklist

- [ ] Operation added to the correct variant / list
- [ ] Serialization / reflection handled
- [ ] Authority validation rules present
- [ ] Input validation (sizes, ranges, null checks) present
- [ ] Virtual operation(s) defined for important state changes
- [ ] Hardfork guard ready (or noted for later HF)

### 6.3 State / Storage Checklist

- [ ] Object size estimate documented
- [ ] Indexes defined and justified
- [ ] Pruning policy defined
- [ ] Snapshot support considered
- [ ] RocksDB / external storage used for non-critical or large data where possible
- [ ] Undo session compatibility verified

### 6.4 Evaluator / Apply Checklist

- [ ] Evaluator follows existing patterns (validate → pay RC → mutate → push virtual ops)
- [ ] Parallelism safety annotated
- [ ] Worker-pool integration point identified if CPU-heavy
- [ ] Error codes / exceptions are cheap and informative
- [ ] No new global locks that harm concurrency

### 6.5 HAF / API Checklist

- [ ] Table schema proposed
- [ ] Pruning / retention policy for the table
- [ ] database_api method signatures proposed
- [ ] Light-mode behavior defined

### 6.6 Performance Gate (when applicable)

- [ ] Expected apply-latency impact stated
- [ ] Expected RAM delta stated
- [ ] Microbenchmark stub or actual number provided
- [ ] Regression threshold not exceeded (or waived by Architect + human)

### 6.7 Security Gate (authority, HTLC, WASM)

- [ ] Authority model reviewed
- [ ] Time-lock / hash-lock edge cases tested or enumerated
- [ ] WASM host functions limited and documented
- [ ] Fuel / metering cannot be bypassed
- [ ] Reentrancy / state consistency rules considered

---

## 7. Phase-by-Phase Detailed Instructions

### Phase 0 – Design (Branch: `phase-0-design`)

**Acceptance criteria:** All design docs reviewed by ≥ 2 agents; performance budgets written; light-node rules written; PROGRESS.md shows Phase 0 complete; draft PR opened for human review.

**Tasks:** 0.1 bootstrap → 0.2 architecture → 0.3 NFT design → 0.4 HTLC design → 0.5 contracts design → 0.6 performance budgets → 0.7 verification strategy → 0.8 issue/milestone list → 0.9 draft PR.

### Phase 1 – Native NFT Primitives (Branch: `phase-1-nft`)

1.1 Protocol definitions → 1.2 State objects → 1.3 Evaluators → 1.4 RC costs → 1.5 Virtual ops → 1.6 Unit tests → 1.7 Light-node path → 1.8 HAF tables → 1.9 database_api stubs → 1.10 Microbenchmark skeleton → 1.11 Hive-Engine migration notes → 1.12 Integration stubs → 1.13 Final Phase 1 PR (human review before merge).

### Phase 2 – HTLC Atomic Swaps

Mirror Phase 1 with HTLC-specific objects, create/redeem/refund evaluators, timeout processing, and time-lock edge-case tests.

### Phase 3 – Metered Smart Contracts

- 3a. WASM runtime selection + plugin skeleton (non-consensus)
- 3b. Deploy / call operations
- 3c. Isolated storage provider
- 3d. Fuel → RC mapping
- 3e. Host function allow-list + audit
- 3f. Fuzzing harness
- 3g. Light-node verification path
- 3h. Only after human approval: design consensus activation

---

## 8. Commit & Pull Request Templates

**Commit message format:**

```
phase-1: add nft_transfer_operation and evaluator

- Protocol definition and serialization
- Basic authority checks
- Unit test stubs
- RC cost placeholder
- Light-node skip path noted
Verification: Protocol + Evaluator + Universal checklists passed
```

**Pull Request body template:** see README / Phase PRs for the standard Summary / Phase / Changes / Verification / Performance / Migration / Next Steps sections.

---

## 9. Error Handling & Escalation

- **Gate failure:** Return to specialist with exact failing checklist items. Max 3 attempts.
- **Tool failure:** Retry with exponential backoff (3 attempts), then escalate. MCP 403 → local `gh`/git fallback is allowed if identity matches.
- **Ambiguous design choice:** Architect pauses and asks the human.
- **Performance regression:** Performance Benchmarker blocks commit.
- **Security concern:** Reviewer can hard-block any commit.

**Human escalation format:**

```
ESCALATION
Task-ID: ...
Reason: ...
Current state: ...
Options: A / B / C
Recommendation: ...
```

---

## 10. Ongoing Reporting

- After every successful commit, append a Verification Log line in PROGRESS.md.
- At phase end, produce a Phase Completion Report and summary PR/issue for the human.
- After every 5–10 commits, a short status summary is encouraged for long-running swarms.

---

## 11. Starting the Swarm – First 8 Actions

1. GitOps: confirm identity `nathansenn`
2. GitOps: search for existing repo
3. Architect + Human: confirm repo name and visibility
4. GitOps: create repository
5. GitOps: create `phase-0-design` branch
6. Architect: write initial README + WORKFLOW.md + PROGRESS.md + architecture overview
7. Reviewer: review the initial docs
8. GitOps: push initial commit and open the Phase 0 tracking PR

After step 8 the swarm is live and follows the Universal Work Loop for all subsequent work.

---

This playbook is the single source of truth. All commits to `nathansenn/hive-native-features` must obey the verification gates above.
