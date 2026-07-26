# 09 – Light-node matrix (as implemented)

**Task-ID:** light-node / docs  
**Status:** implementation snapshot  
**Sources:** `include/hive_native/chain/database.hpp` (`node_config`, `require_full_*`),  
`src/chain/database.cpp`, evaluators (`evaluators_nft.cpp`, `evaluators_htlc.cpp`, `evaluators_contracts.cpp`),  
`src/api/database_api_stubs.cpp`, design summaries in `docs/00`–`03`.

---

## 1. Configuration model

Implemented knobs live on `hive_native::chain::node_config`:

| Field | Default | Role |
|-------|---------|------|
| `nft_skip_state` | `false` | Skip local NFT object indexes (light/mobile non-consensus) |
| `htlc_skip_state` | `false` | Skip local HTLC object indexes |
| `contracts_skip` | `false` | Skip contract apply/API on non-consensus nodes |
| `is_consensus_node` | `true` | Witness / block-producer role; must not skip NFT/HTLC |
| `hardfork` | contracts HF | Enables HF-gated ops in tests |

There is **no** separate enum for “full / pruned / light” in code. Node **class** below is the operational mapping of those flags plus design retention policy (pruning is design/comment level only in the portable stand-in).

---

## 2. Gate functions (apply path)

### NFT — `database::require_full_nft_state()`

Called at the start of every NFT evaluator (`create_collection`, `mint`, `transfer`, `approve`, `set_approval_for_all`, `burn`).

| Condition | Result |
|-----------|--------|
| `is_consensus_node && nft_skip_state` | `protocol_error("consensus node cannot skip NFT state")` |
| `nft_skip_state` (any role) | `protocol_error("NFT state skipped on this node")` |
| otherwise | apply proceeds (full state required) |

### HTLC — `database::require_full_htlc_state()`

Called at the start of every HTLC evaluator (`create`, `redeem`, `refund`).

| Condition | Result |
|-----------|--------|
| `is_consensus_node && htlc_skip_state` | `protocol_error("consensus node cannot skip HTLC state")` |
| `htlc_skip_state` (any role) | `protocol_error("HTLC state skipped on this node")` |
| otherwise | apply proceeds |

### Contracts — inline check (no `require_full_contracts_state`)

In `contract_deploy` / `contract_call`:

| Condition | Result |
|-----------|--------|
| `contracts_skip && !is_consensus_node` | `protocol_error("contracts skipped")` |
| `contracts_skip && is_consensus_node` | **apply still runs** (skip flag ignored for consensus) |
| `!contracts_skip` | apply runs (deploy/call + engine) |

**Asymmetry:** NFT/HTLC reject apply whenever skip is set. Contracts reject only non-consensus skippers; consensus nodes always execute deploy/call when HF is active.

---

## 3. Node class × feature matrix

Legend for **Apply**:

- **Full** — evaluators run; local state updated.
- **Reject** — evaluator throws on feature ops (no local state mutation).
- **Design-only** — intended retention/behavior from design docs; not fully modeled as separate code paths in the in-memory DB.

Legend for **API** (`database_api` stubs):

- **Full** — returns objects from local maps.
- **Empty** — skip flag → `nullopt` / empty lists.
- **Code stripped** — `get_contract` always clears `code` before return.

| Node class | Config (typical) | NFT apply | NFT API | HTLC apply | HTLC API | Contracts apply | Contracts API |
|------------|------------------|-----------|---------|------------|----------|-----------------|---------------|
| **Consensus / witness / full producer** | `is_consensus_node=true`, all skips `false` | Full | Full | Full | Full | Full (engine) | Present; **code always stripped** in stub |
| **Consensus misconfigured** | `is_consensus_node=true` + `nft_skip` / `htlc_skip` | **Reject** (cannot skip) | Empty if skip | **Reject** (cannot skip) | Empty if skip | Full even if `contracts_skip` | Empty if `contracts_skip` |
| **Full archival (design)** | same as full; retention outside skip flags | Full | Full | Full; closed history via virtual ops / HAF | Full open (+ history off-chain) | Full or plugin | Full or hash+events |
| **Standard API (design)** | skips false; indexes current state | Full | Full current; `list_args.light` drops URI | Full | Open + recent resolved (HAF) | Configurable | Configurable |
| **Pruned (design)** | skips false; drop burned/closed | Full current owners/collections; burned GC | Full current | Open HTLCs only (design); closed drop after irreversible | Open only | Code hash + storage root; blobs optional | Hash / no blob |
| **Light / mobile (non-consensus)** | `is_consensus_node=false`, feature skip(s) `true` | **Reject** NFT ops on this node | Empty | **Reject** HTLC ops | Empty | **Reject** deploy/call | Empty |
| **Light API client of full node** | peer has full state; client uses `list_args.light=true` | N/A (remote) | URI cleared on list; limit ≤ 100 | N/A | hashes only (design for history) | N/A | code stripped |

### Implemented vs design note

| Design intent | Implementation today |
|---------------|----------------------|
| Light nodes skip **storage** but may still validate op shape/signatures without claiming local object truth | Skip path **throws** in evaluators; no partial “validate-only” apply path yet |
| Pruned keeps open HTLCs / current NFT owners only | HTLC closed objects kept with status for tests; comment notes prune after irreversible in production |
| Contracts light: verify-only, skip execution | Non-consensus + `contracts_skip` → throw (no verify-only execution path) |
| Witnesses must not skip NFT/HTLC | Enforced: consensus + skip → hard error |

---

## 4. Per-feature detail

### 4.1 NFT

| Op | Gate | On full node | On `nft_skip_state` |
|----|------|--------------|---------------------|
| `nft_create_collection` | `require_full_nft_state` | Write collection + symbol index | Throw |
| `nft_mint` | same | Write NFT, bump supply | Throw |
| `nft_transfer` | same | Owner change, clear approve | Throw |
| `nft_approve` | same | Per-token approve | Throw |
| `nft_set_approval_for_all` | same | Operator map | Throw |
| `nft_burn` | same | Erase NFT, decrement supply | Throw |

API:

- `get_nft` / lists → empty when `nft_skip_state`
- `list_args.light` → clear `uri` on returned copies (full-state node only)
- `clamp_limit`: 1…100

### 4.2 HTLC

| Op | Gate | On full node | On `htlc_skip_state` |
|----|------|--------------|----------------------|
| `htlc_create` | `require_full_htlc_state` | Debit `from`, open object | Throw |
| `htlc_redeem` | same | Credit `to` if preimage OK, pre-expiry | Throw |
| `htlc_refund` | same | Credit `from` post-expiry | Throw |

API:

- `get_htlc` → `nullopt` when `htlc_skip_state`

### 4.3 Contracts

| Op | Gate | Non-consensus + skip | Consensus (even if skip) |
|----|------|----------------------|---------------------------|
| `contract_deploy` | HF + skip check | Throw | Deploy + store code |
| `contract_call` | HF + skip check | Throw | Call engine; commit storage on success only |

API:

- `get_contract` → `nullopt` when `contracts_skip`; otherwise returns object with **`code` cleared**

---

## 5. Design-class summary (architecture §12)

For comparison with product docs (not separate code enums):

| Node class | NFT | HTLC | Contracts |
|------------|-----|------|-----------|
| Full archival | Full | Full history optional | Full or plugin |
| Standard API | Full current | Open + recent resolved | Configurable |
| Pruned | Current owners only | Open only | Code hash only |
| Light / mobile | Skip indices; verify via API | Skip; verify proofs if provided | Skip |

---

## 6. Tests covering skip policy

`tests/test_runner.cpp` → `test_nft_light_skip`:

1. `is_consensus_node=false`, `nft_skip_state=true` → create collection **throws**.
2. `is_consensus_node=true`, `nft_skip_state=true` → create collection **throws** (consensus cannot skip).

No dedicated HTLC/contracts light-skip unit tests in the runner at this snapshot; gates follow the same pattern as documented above.

---

## 7. Performance expectations (from budgets)

When skip flags are true on light/mobile:

| Requirement | Budget intent |
|-------------|----------------|
| Header sync with nft-skip | No regression vs current light sync |
| Memory | No NFT/HTLC index allocations when skip |
| CPU on skipped feature ops | Signature / shape verify only (design); apply currently **rejects** rather than soft-skip |

---

## 8. Acceptance mapping

| Rule | Status in portable code |
|------|-------------------------|
| Explicit skip flags for NFT / HTLC / contracts | Yes (`node_config`) |
| Witnesses cannot skip NFT/HTLC | Yes (`require_full_*`) |
| Light nodes do not claim local NFT/HTLC object truth | Yes (apply reject + empty API) |
| Contract consensus always executes | Yes (skip ignored when `is_consensus_node`) |
| Validate-only light apply without local store | Not implemented (throws) |
| Production prune of closed HTLC / burned NFT from hot state | Design + comments; test DB keeps status/objects as needed |
