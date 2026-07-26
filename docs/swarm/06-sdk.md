# 06 – TypeScript SDK stubs

**Task-ID:** sdk / typescript stubs  
**Status:** stubs landed  
**Last Updated:** 2026-07-25  
**Package:** `@nathansenn/hive-native-features` (`private: true`)  
**Path:** `sdk/typescript/`

---

## 1. Purpose

Provide client-side **TypeScript type stubs and builders** for hive-native-features operations so apps and tools can assemble:

1. **Protocol field objects** matching C++ headers (post-HF native ops).
2. **Hive `custom_json` op tuples** for the plugin-first / experimental path (Phase 3 contracts today; NFT/HTLC pre-HF experiments).

This is **not** a full signing/broadcast SDK. Pair with dhive, hive-tx, or similar for keys and RPC.

---

## 2. Layout

```
sdk/typescript/
  package.json          # name: @nathansenn/hive-native-features, private: true
  README.md             # usage examples
  src/
    types.ts            # field types + caps + custom_json shapes
    ops.ts              # builders (nft_*, htlc_*, contract_*)
    index.ts            # re-exports
```

Companion doc: this file (`docs/swarm/06-sdk.md`).

---

## 3. Alignment with C++ headers

| TS area | C++ source |
|---------|------------|
| Shared primitives, caps, `Asset`, `HashDigest` | `include/hive_native/util/types.hpp` |
| NFT ops + virtual ops | `include/hive_native/protocol/nft_operations.hpp` |
| HTLC ops + virtual ops | `include/hive_native/protocol/htlc_operations.hpp` |
| Contract ops + virtual ops | `include/hive_native/protocol/contract_operations.hpp` |

### Field-name rules

- Use **snake_case** identical to C++ struct members (`nft_id`, `operator_account`, `preimage_hash`, `export_name`, `fuel_limit`, …).
- Do **not** camelCase protocol fields in wire JSON; keep 1:1 with headers for HAF and evaluators.
- Defaults mirror C++ defaults where applicable (`max_supply = 0`, `transferable = true`, `collection = 0` for approval-for-all, empty `uri` / `memo`, etc.).

### Operation inventory (builders)

| Action | Required authorities (fee_payer) | custom_json `id` |
|--------|----------------------------------|------------------|
| `nft_create_collection` | `creator` active | `hive_nft` |
| `nft_mint` | `creator` active | `hive_nft` |
| `nft_transfer` | `from` active (apply may accept approved/operator) | `hive_nft` |
| `nft_approve` | `owner` active | `hive_nft` |
| `nft_set_approval_for_all` | `owner` active | `hive_nft` |
| `nft_burn` | `owner` active | `hive_nft` |
| `htlc_create` | `from` active | `hive_htlc` |
| `htlc_redeem` | `to` active (ADR-0001) | `hive_htlc` |
| `htlc_refund` | `from` active | `hive_htlc` |
| `contract_deploy` | `owner` active | `hive_contracts` |
| `contract_call` | `caller` active | `hive_contracts` |

Virtual ops (`nft_minted`, `htlc_created`, `contract_called`, …) are typed for indexers/HAF consumers; they are not built as broadcast ops.

---

## 4. Wire formats

### 4.1 custom_json (current stub path)

```json
[
  "custom_json",
  {
    "required_auths": ["alice"],
    "required_posting_auths": [],
    "id": "hive_nft",
    "json": "{\"action\":\"nft_transfer\",\"from\":\"alice\",\"to\":\"bob\",\"nft_id\":1,\"memo\":\"\"}"
  }
]
```

- `json` is a **string** containing an object with `action` plus protocol fields.
- `required_auths` defaults to the C++ `fee_payer()` account unless overridden.

### 4.2 Native op tuple (future / tests)

```ts
["nft_transfer", { from, to, nft_id, memo }]
```

Produced by `toNativeOp(...)`. Binary serialization remains out of scope until upstream static_variant registration and HF numbers are fixed.

---

## 5. Usage (minimal)

```ts
import {
  buildNftTransferOp,
  buildHtlcCreateOp,
  hashDigest,
} from "@nathansenn/hive-native-features";

const op = buildNftTransferOp({
  from: "alice",
  to: "bob",
  nft_id: 1,
  memo: "",
});
// push into tx.operations with your preferred Hive client
```

Full examples: `sdk/typescript/README.md`.

---

## 6. Install / typecheck notes

```bash
cd sdk/typescript
# optional — may fail offline; package has no runtime deps
npm install
npx tsc --noEmit   # requires local typescript + tsconfig if added later
```

**Policy:** Valid TypeScript source is required even when `npm install` cannot run (network blocked). No runtime dependency on npm packages beyond optional `typescript` for local typecheck.

---

## 7. Out of scope (stubs)

- Transaction signing, key management, RC estimation
- Binary Hive protocol pack/unpack
- Node RPC clients / database_api wrappers
- WASM compile or fuel metering client-side
- CamelCase JSON adapters for non-Hive frontends

---

## 8. Follow-ups

1. Add thin `database_api` query types once API stubs stabilize.
2. Optional `tsconfig.json` + CI typecheck job when Node is available in CI.
3. Keep field parity when C++ headers change (reviewer checklist: compare `*.hpp` ↔ `types.ts`).
4. Document HF activation and migrate examples from `custom_json` → native op names in wallets.
