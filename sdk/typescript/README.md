# @nathansenn/hive-native-features

TypeScript **stubs** for [hive-native-features](https://github.com/nathansenn/hive-native-features): native NFT, HTLC, and metered contract operation payloads.

Field names match the C++ protocol headers:

| Feature | Header |
|---------|--------|
| NFT | `include/hive_native/protocol/nft_operations.hpp` |
| HTLC | `include/hive_native/protocol/htlc_operations.hpp` |
| Contracts | `include/hive_native/protocol/contract_operations.hpp` |
| Shared types | `include/hive_native/util/types.hpp` |

This package does **not** sign transactions or talk to a node. It builds:

1. **Protocol field objects** (for future native ops post-hardfork)
2. **Hive `custom_json` op tuples** (plugin-first / experimental path)

## Install

```bash
# from monorepo root (or path to this package)
npm install ./sdk/typescript
# or yarn / pnpm link as needed
```

> Stubs only — `npm install` may be skipped if the network is blocked. Types and builders are plain TypeScript with no runtime dependencies.

## Usage

```ts
import {
  buildNftCreateCollectionOp,
  buildNftMintOp,
  buildNftTransferOp,
  buildHtlcCreateOp,
  buildHtlcRedeemOp,
  buildContractDeployOp,
  buildContractCallOp,
  nftTransfer,
  toNativeOp,
  hashDigest,
  CUSTOM_JSON_ID,
} from "@nathansenn/hive-native-features";

// --- NFT: custom_json (plugin / pre-HF) ---
const createColl = buildNftCreateCollectionOp({
  creator: "alice",
  symbol: "ART",
  name: "Alice Art",
  max_supply: 1000,
  transferable: true,
});
// => ["custom_json", {
//      required_auths: ["alice"],
//      required_posting_auths: [],
//      id: "hive_nft",
//      json: "{\"action\":\"nft_create_collection\",...}"
//    }]

const mint = buildNftMintOp({
  creator: "alice",
  collection: 1,
  to: "bob",
  metadata_hash: "a".repeat(64), // sha256 hex
  uri: "ipfs://Qm...",
  soulbound: false,
});

const transfer = buildNftTransferOp({
  from: "bob",
  to: "carol",
  nft_id: 42,
  memo: "gift",
});

// Protocol fields only (no custom_json wrapper)
const fields = nftTransfer({ from: "bob", to: "carol", nft_id: 42 });
const nativeTuple = toNativeOp({ action: "nft_transfer", ...fields });
// => ["nft_transfer", { from, to, nft_id, memo }]

// --- HTLC ---
const lock = buildHtlcCreateOp({
  from: "alice",
  to: "bob",
  amount: { amount: 1000, symbol: "HIVE" },
  preimage_hash: hashDigest("sha256", "ab".repeat(32)),
  preimage_size: 32,
  expiration: Math.floor(Date.now() / 1000) + 3600,
  memo: "swap",
});

const redeem = buildHtlcRedeemOp({
  to: "bob",
  htlc_id: 7,
  preimage: "00".repeat(32),
});

// --- Contracts (custom_json id: hive_contracts) ---
const deploy = buildContractDeployOp({
  owner: "alice",
  code_hash: "b".repeat(64),
  code: [], // or WASM bytes
  fuel_limit: 1_000_000,
  init_args: [],
});

const call = buildContractCallOp({
  caller: "bob",
  contract_id: 1,
  export_name: "call",
  args: [],
  fuel_limit: 100_000,
});

console.log(CUSTOM_JSON_ID);
// { nft: "hive_nft", htlc: "hive_htlc", contracts: "hive_contracts" }
```

Push the resulting op into a Hive transaction (dhive / hive-tx / etc.):

```ts
// pseudo — depends on your client library
tx.operations.push(transfer);
```

## custom_json layout

| Feature | `id` | `json.action` examples |
|---------|------|------------------------|
| NFT | `hive_nft` | `nft_create_collection`, `nft_mint`, `nft_transfer`, `nft_approve`, `nft_set_approval_for_all`, `nft_burn` |
| HTLC | `hive_htlc` | `htlc_create`, `htlc_redeem`, `htlc_refund` |
| Contracts | `hive_contracts` | `contract_deploy`, `contract_call` |

JSON body shape: `{ "action": "<op_name>", ...fields matching C++ struct }`.

## API surface

| Module | Contents |
|--------|----------|
| `src/types.ts` | Op interfaces, ids, caps, `CustomJsonOpTuple` |
| `src/ops.ts` | Field builders + `build*Op` / `toCustomJsonOp` / `toNativeOp` |
| `src/index.ts` | Re-exports |

## Status

Phase stubs aligned with portable C++ protocol sketches. Not production-ready; serialization to Hive binary protocol is out of scope until hardfork numbers and upstream op lists are fixed.

See also: [docs/swarm/06-sdk.md](../../docs/swarm/06-sdk.md).
