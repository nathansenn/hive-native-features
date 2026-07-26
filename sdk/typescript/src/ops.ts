/**
 * Builders for nft_*, htlc_*, contract_* op payloads and custom_json shapes.
 * Field names match include/hive_native/protocol/*.hpp.
 */

import {
  CUSTOM_JSON_ID,
  type AccountName,
  type ContractCallOperation,
  type ContractDeployOperation,
  type ContractPayload,
  type CustomJsonOpBody,
  type CustomJsonOpTuple,
  type HashDigest,
  type HtlcCreateOperation,
  type HtlcPayload,
  type HtlcRedeemOperation,
  type HtlcRefundOperation,
  type NativeOpTuple,
  type NativePayload,
  type NftApproveOperation,
  type NftBurnOperation,
  type NftCreateCollectionOperation,
  type NftMintOperation,
  type NftPayload,
  type NftSetApprovalForAllOperation,
  type NftTransferOperation,
} from "./types.js";

// ---------------------------------------------------------------------------
// Defaults helpers
// ---------------------------------------------------------------------------

function withDefaults<T extends object>(
  fields: Partial<T>,
  defaults: T
): T {
  return { ...defaults, ...fields };
}

// ---------------------------------------------------------------------------
// NFT payload builders (protocol field shapes)
// ---------------------------------------------------------------------------

export function nftCreateCollection(
  fields: Partial<NftCreateCollectionOperation> &
    Pick<NftCreateCollectionOperation, "creator" | "symbol" | "name">
): NftCreateCollectionOperation {
  return withDefaults(fields, {
    creator: fields.creator,
    symbol: fields.symbol,
    name: fields.name,
    max_supply: 0,
    transferable: true,
  });
}

export function nftMint(
  fields: Partial<NftMintOperation> &
    Pick<NftMintOperation, "creator" | "collection" | "to" | "metadata_hash">
): NftMintOperation {
  return withDefaults(fields, {
    creator: fields.creator,
    collection: fields.collection,
    to: fields.to,
    metadata_hash: fields.metadata_hash,
    uri: "",
    soulbound: false,
  });
}

export function nftTransfer(
  fields: Partial<NftTransferOperation> &
    Pick<NftTransferOperation, "from" | "to" | "nft_id">
): NftTransferOperation {
  return withDefaults(fields, {
    from: fields.from,
    to: fields.to,
    nft_id: fields.nft_id,
    memo: "",
  });
}

export function nftApprove(
  fields: Partial<NftApproveOperation> &
    Pick<NftApproveOperation, "owner" | "nft_id">
): NftApproveOperation {
  return withDefaults(fields, {
    owner: fields.owner,
    nft_id: fields.nft_id,
    approved: "",
  });
}

export function nftSetApprovalForAll(
  fields: Partial<NftSetApprovalForAllOperation> &
    Pick<NftSetApprovalForAllOperation, "owner" | "operator_account">
): NftSetApprovalForAllOperation {
  return withDefaults(fields, {
    owner: fields.owner,
    operator_account: fields.operator_account,
    collection: 0,
    approved: true,
  });
}

export function nftBurn(
  fields: Pick<NftBurnOperation, "owner" | "nft_id">
): NftBurnOperation {
  return { owner: fields.owner, nft_id: fields.nft_id };
}

// ---------------------------------------------------------------------------
// HTLC payload builders
// ---------------------------------------------------------------------------

export function htlcCreate(
  fields: Partial<HtlcCreateOperation> &
    Pick<
      HtlcCreateOperation,
      "from" | "to" | "amount" | "preimage_hash" | "expiration"
    >
): HtlcCreateOperation {
  return withDefaults(fields, {
    from: fields.from,
    to: fields.to,
    amount: fields.amount,
    preimage_hash: fields.preimage_hash,
    preimage_size: 0,
    expiration: fields.expiration,
    memo: "",
  });
}

export function htlcRedeem(
  fields: Pick<HtlcRedeemOperation, "to" | "htlc_id" | "preimage">
): HtlcRedeemOperation {
  return {
    to: fields.to,
    htlc_id: fields.htlc_id,
    preimage: fields.preimage,
  };
}

export function htlcRefund(
  fields: Pick<HtlcRefundOperation, "from" | "htlc_id">
): HtlcRefundOperation {
  return { from: fields.from, htlc_id: fields.htlc_id };
}

/** Convenience: build a HashDigest for sha256 / ripemd160. */
export function hashDigest(
  algo: HashDigest["algo"],
  bytes: HashDigest["bytes"]
): HashDigest {
  return { algo, bytes };
}

// ---------------------------------------------------------------------------
// Contract payload builders
// ---------------------------------------------------------------------------

export function contractDeploy(
  fields: Partial<ContractDeployOperation> &
    Pick<ContractDeployOperation, "owner" | "code_hash">
): ContractDeployOperation {
  return withDefaults(fields, {
    owner: fields.owner,
    code: [],
    code_hash: fields.code_hash,
    fuel_limit: 0,
    init_args: [],
  });
}

export function contractCall(
  fields: Partial<ContractCallOperation> &
    Pick<ContractCallOperation, "caller" | "contract_id" | "export_name">
): ContractCallOperation {
  return withDefaults(fields, {
    caller: fields.caller,
    contract_id: fields.contract_id,
    export_name: fields.export_name,
    args: [],
    fuel_limit: 0,
  });
}

// ---------------------------------------------------------------------------
// Action-tagged payloads (custom_json.json body)
// ---------------------------------------------------------------------------

export function asNftPayload(
  action: NftPayload["action"],
  fields: Omit<Extract<NftPayload, { action: typeof action }>, "action">
): NftPayload {
  return { action, ...fields } as NftPayload;
}

export function asHtlcPayload(
  action: HtlcPayload["action"],
  fields: Omit<Extract<HtlcPayload, { action: typeof action }>, "action">
): HtlcPayload {
  return { action, ...fields } as HtlcPayload;
}

export function asContractPayload(
  action: ContractPayload["action"],
  fields: Omit<Extract<ContractPayload, { action: typeof action }>, "action">
): ContractPayload {
  return { action, ...fields } as ContractPayload;
}

// ---------------------------------------------------------------------------
// custom_json / native op wire shapes
// ---------------------------------------------------------------------------

export interface CustomJsonOptions {
  /** Active authority signers (default: fee_payer / primary actor). */
  required_auths?: AccountName[];
  /** Posting authority signers (default: []). */
  required_posting_auths?: AccountName[];
  /** Override custom_json id (default from CUSTOM_JSON_ID by feature). */
  id?: string;
}

function feePayerFromPayload(payload: NativePayload): AccountName {
  switch (payload.action) {
    case "nft_create_collection":
    case "nft_mint":
      return payload.creator;
    case "nft_transfer":
      return payload.from;
    case "nft_approve":
    case "nft_set_approval_for_all":
    case "nft_burn":
      return payload.owner;
    case "htlc_create":
    case "htlc_refund":
      return payload.from;
    case "htlc_redeem":
      return payload.to;
    case "contract_deploy":
      return payload.owner;
    case "contract_call":
      return payload.caller;
    default: {
      const _exhaustive: never = payload;
      return _exhaustive;
    }
  }
}

function customJsonIdFor(payload: NativePayload): string {
  if (payload.action.startsWith("nft_")) return CUSTOM_JSON_ID.nft;
  if (payload.action.startsWith("htlc_")) return CUSTOM_JSON_ID.htlc;
  return CUSTOM_JSON_ID.contracts;
}

/**
 * Build a Hive custom_json operation body for plugin-first / experimental use.
 * `json` is a stringified object: { action, ...protocol_fields }.
 */
export function toCustomJsonBody(
  payload: NativePayload,
  options: CustomJsonOptions = {}
): CustomJsonOpBody {
  const feePayer = feePayerFromPayload(payload);
  return {
    required_auths: options.required_auths ?? [feePayer],
    required_posting_auths: options.required_posting_auths ?? [],
    id: options.id ?? customJsonIdFor(payload),
    json: JSON.stringify(payload),
  };
}

/** Outer op form: ["custom_json", body] suitable for tx.operations push. */
export function toCustomJsonOp(
  payload: NativePayload,
  options: CustomJsonOptions = {}
): CustomJsonOpTuple {
  return ["custom_json", toCustomJsonBody(payload, options)];
}

/**
 * Future native protocol op tuple (post hardfork).
 * Does not wrap in custom_json — payload fields only.
 */
export function toNativeOp(payload: NativePayload): NativeOpTuple {
  const { action, ...fields } = payload;
  return [action, fields] as NativeOpTuple;
}

// ---------------------------------------------------------------------------
// High-level: build custom_json in one call
// ---------------------------------------------------------------------------

export function buildNftCreateCollectionOp(
  fields: Parameters<typeof nftCreateCollection>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "nft_create_collection", ...nftCreateCollection(fields) },
    options
  );
}

export function buildNftMintOp(
  fields: Parameters<typeof nftMint>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp({ action: "nft_mint", ...nftMint(fields) }, options);
}

export function buildNftTransferOp(
  fields: Parameters<typeof nftTransfer>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "nft_transfer", ...nftTransfer(fields) },
    options
  );
}

export function buildNftApproveOp(
  fields: Parameters<typeof nftApprove>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "nft_approve", ...nftApprove(fields) },
    options
  );
}

export function buildNftSetApprovalForAllOp(
  fields: Parameters<typeof nftSetApprovalForAll>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    {
      action: "nft_set_approval_for_all",
      ...nftSetApprovalForAll(fields),
    },
    options
  );
}

export function buildNftBurnOp(
  fields: Parameters<typeof nftBurn>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp({ action: "nft_burn", ...nftBurn(fields) }, options);
}

export function buildHtlcCreateOp(
  fields: Parameters<typeof htlcCreate>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "htlc_create", ...htlcCreate(fields) },
    options
  );
}

export function buildHtlcRedeemOp(
  fields: Parameters<typeof htlcRedeem>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "htlc_redeem", ...htlcRedeem(fields) },
    options
  );
}

export function buildHtlcRefundOp(
  fields: Parameters<typeof htlcRefund>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "htlc_refund", ...htlcRefund(fields) },
    options
  );
}

export function buildContractDeployOp(
  fields: Parameters<typeof contractDeploy>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "contract_deploy", ...contractDeploy(fields) },
    options
  );
}

export function buildContractCallOp(
  fields: Parameters<typeof contractCall>[0],
  options?: CustomJsonOptions
): CustomJsonOpTuple {
  return toCustomJsonOp(
    { action: "contract_call", ...contractCall(fields) },
    options
  );
}
