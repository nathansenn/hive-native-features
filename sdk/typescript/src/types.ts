/**
 * Protocol field types for hive-native-features.
 * Field names match include/hive_native/protocol/*.hpp and util/types.hpp.
 *
 * Stubs only — no network, signing, or consensus serialization.
 */

// ---------------------------------------------------------------------------
// Primitive aliases (mirror hive_native::util/types.hpp)
// ---------------------------------------------------------------------------

/** Hive account name (≤ 16 chars; a-z, 0-9, '.', '-'). */
export type AccountName = string;

/** Seconds since Unix epoch (uint32). */
export type TimePointSec = number;

/** Signed share amount (int64). */
export type ShareType = number;

/** Collection primary key (uint64). */
export type CollectionId = number | string;

/** NFT primary key (uint64). */
export type NftId = number | string;

/** HTLC primary key (uint64). */
export type HtlcId = number | string;

/** Contract primary key (uint64). */
export type ContractId = number | string;

/** 32-byte SHA-256 digest as hex (64 chars) or byte array. */
export type Sha256 = string | Uint8Array;

/** 20-byte RIPEMD-160 digest as hex (40 chars) or byte array. */
export type Ripemd160 = string | Uint8Array;

/** Binary blob: base64 string, hex string, or raw bytes. */
export type Bytes = string | Uint8Array | number[];

// ---------------------------------------------------------------------------
// Size / duration caps (from types.hpp)
// ---------------------------------------------------------------------------

export const MAX_ACCOUNT_NAME_LEN = 16;
export const MAX_NFT_SYMBOL_LEN = 16;
export const MAX_NFT_NAME_LEN = 64;
export const MAX_NFT_URI_LEN = 256;
export const MAX_MEMO_LEN = 2048;
export const MAX_HTLC_PREIMAGE_LEN = 1024;
export const MAX_CONTRACT_EXPORT = 64;
export const MAX_CONTRACT_ARGS = 64 * 1024;
export const MAX_CODE_BYTES = 512 * 1024;

export const HTLC_MIN_DURATION_SEC = 60;
export const HTLC_MAX_DURATION_SEC = 30 * 24 * 3600;
export const HTLC_MAX_OPEN_PER_ACCOUNT = 256;

/** Hardfork placeholders (human-set before mainnet). */
export const HIVE_HARDFORK_NFT = 9001;
export const HIVE_HARDFORK_HTLC = 9002;
export const HIVE_HARDFORK_CONTRACTS = 9003;

// ---------------------------------------------------------------------------
// Asset / hash (types.hpp)
// ---------------------------------------------------------------------------

/** Matches hive_native::asset_symbol */
export type AssetSymbol = "HIVE" | "HBD" | 0 | 1;

export interface Asset {
  /** share_type amount */
  amount: ShareType;
  /** asset_symbol: HIVE = 0, HBD = 1 */
  symbol: AssetSymbol;
}

/** Matches hive_native::hash_algo */
export type HashAlgo = "sha256" | "ripemd160" | 0 | 1;

/** Matches hive_native::hash_digest */
export interface HashDigest {
  algo: HashAlgo;
  /** 32 bytes (sha256) or 20 bytes (ripemd160); hex or raw */
  bytes: Bytes;
}

// ---------------------------------------------------------------------------
// NFT ops — include/hive_native/protocol/nft_operations.hpp
// ---------------------------------------------------------------------------

/** nft_create_collection_operation */
export interface NftCreateCollectionOperation {
  creator: AccountName;
  /** unique, ≤ MAX_NFT_SYMBOL_LEN */
  symbol: string;
  /** ≤ MAX_NFT_NAME_LEN */
  name: string;
  /** 0 = unlimited */
  max_supply: number;
  transferable: boolean;
}

/** nft_mint_operation */
export interface NftMintOperation {
  /** must be collection creator (v1) */
  creator: AccountName;
  collection: CollectionId;
  to: AccountName;
  metadata_hash: Sha256;
  /** optional, ≤ MAX_NFT_URI_LEN */
  uri: string;
  soulbound: boolean;
}

/** nft_transfer_operation */
export interface NftTransferOperation {
  from: AccountName;
  to: AccountName;
  nft_id: NftId;
  memo: string;
}

/** nft_approve_operation */
export interface NftApproveOperation {
  owner: AccountName;
  nft_id: NftId;
  /** empty string clears approval */
  approved: AccountName;
}

/** nft_set_approval_for_all_operation (ADR-0001) */
export interface NftSetApprovalForAllOperation {
  owner: AccountName;
  operator_account: AccountName;
  /** 0 = all collections owned by owner */
  collection: CollectionId;
  approved: boolean;
}

/** nft_burn_operation */
export interface NftBurnOperation {
  owner: AccountName;
  nft_id: NftId;
}

// Virtual ops (HAF / indexers)

export interface NftCollectionCreatedOperation {
  collection: CollectionId;
  creator: AccountName;
  symbol: string;
}

export interface NftMintedOperation {
  nft_id: NftId;
  collection: CollectionId;
  to: AccountName;
  metadata_hash: Sha256;
}

export interface NftTransferredOperation {
  nft_id: NftId;
  from: AccountName;
  to: AccountName;
}

export interface NftApprovedOperation {
  nft_id: NftId;
  owner: AccountName;
  approved: AccountName;
}

export interface NftApprovalForAllOperation {
  owner: AccountName;
  operator_account: AccountName;
  collection: CollectionId;
  approved: boolean;
}

export interface NftBurnedOperation {
  nft_id: NftId;
  owner: AccountName;
  collection: CollectionId;
}

// ---------------------------------------------------------------------------
// HTLC ops — include/hive_native/protocol/htlc_operations.hpp
// ---------------------------------------------------------------------------

/** htlc_create_operation */
export interface HtlcCreateOperation {
  from: AccountName;
  to: AccountName;
  amount: Asset;
  preimage_hash: HashDigest;
  /** exact size required at redeem */
  preimage_size: number;
  expiration: TimePointSec;
  memo: string;
}

/** htlc_redeem_operation — redeem authority: `to` only (ADR-0001) */
export interface HtlcRedeemOperation {
  /** must match HTLC.to and sign */
  to: AccountName;
  htlc_id: HtlcId;
  preimage: Bytes;
}

/** htlc_refund_operation */
export interface HtlcRefundOperation {
  /** locker; funds always return to HTLC.from */
  from: AccountName;
  htlc_id: HtlcId;
}

// Virtual

export interface HtlcCreatedOperation {
  htlc_id: HtlcId;
  from: AccountName;
  to: AccountName;
  amount: Asset;
  expiration: TimePointSec;
}

export interface HtlcRedeemedOperation {
  htlc_id: HtlcId;
  from: AccountName;
  to: AccountName;
  amount: Asset;
}

export interface HtlcRefundedOperation {
  htlc_id: HtlcId;
  from: AccountName;
  amount: Asset;
}

// ---------------------------------------------------------------------------
// Contract ops — include/hive_native/protocol/contract_operations.hpp
// ---------------------------------------------------------------------------

/** contract_deploy_operation */
export interface ContractDeployOperation {
  owner: AccountName;
  /** WASM bytes (or empty if code_hash-only publish) */
  code: Bytes;
  code_hash: Sha256;
  fuel_limit: number;
  init_args: Bytes;
}

/** contract_call_operation */
export interface ContractCallOperation {
  caller: AccountName;
  contract_id: ContractId;
  /** e.g. "call"; ≤ MAX_CONTRACT_EXPORT */
  export_name: string;
  args: Bytes;
  fuel_limit: number;
}

// Virtual

export interface ContractDeployedOperation {
  contract_id: ContractId;
  owner: AccountName;
  code_hash: Sha256;
}

export interface ContractCalledOperation {
  contract_id: ContractId;
  caller: AccountName;
  fuel_used: number;
  success: boolean;
}

// ---------------------------------------------------------------------------
// Op name / id unions
// ---------------------------------------------------------------------------

export type NftOpName =
  | "nft_create_collection"
  | "nft_mint"
  | "nft_transfer"
  | "nft_approve"
  | "nft_set_approval_for_all"
  | "nft_burn";

export type HtlcOpName = "htlc_create" | "htlc_redeem" | "htlc_refund";

export type ContractOpName = "contract_deploy" | "contract_call";

export type NativeOpName = NftOpName | HtlcOpName | ContractOpName;

/** custom_json id strings used by plugin-first / experimental path */
export const CUSTOM_JSON_ID = {
  nft: "hive_nft",
  htlc: "hive_htlc",
  contracts: "hive_contracts",
} as const;

export type CustomJsonId =
  (typeof CUSTOM_JSON_ID)[keyof typeof CUSTOM_JSON_ID];

/**
 * Payload body inside custom_json.json (stringified).
 * action + fields mirror the C++ operation struct of the same name.
 */
export interface CustomJsonPayloadBase {
  action: NativeOpName;
}

export type NftPayload =
  | ({ action: "nft_create_collection" } & NftCreateCollectionOperation)
  | ({ action: "nft_mint" } & NftMintOperation)
  | ({ action: "nft_transfer" } & NftTransferOperation)
  | ({ action: "nft_approve" } & NftApproveOperation)
  | ({ action: "nft_set_approval_for_all" } & NftSetApprovalForAllOperation)
  | ({ action: "nft_burn" } & NftBurnOperation);

export type HtlcPayload =
  | ({ action: "htlc_create" } & HtlcCreateOperation)
  | ({ action: "htlc_redeem" } & HtlcRedeemOperation)
  | ({ action: "htlc_refund" } & HtlcRefundOperation);

export type ContractPayload =
  | ({ action: "contract_deploy" } & ContractDeployOperation)
  | ({ action: "contract_call" } & ContractCallOperation);

export type NativePayload = NftPayload | HtlcPayload | ContractPayload;

/**
 * Hive custom_json operation body (wire shape, not the outer array form).
 * @see https://developers.hive.io/apidefinitions/#broadcast_ops_custom_json
 */
export interface CustomJsonOpBody {
  required_auths: AccountName[];
  required_posting_auths: AccountName[];
  id: CustomJsonId | string;
  json: string;
}

/** Outer Hive op tuple: ["custom_json", body] */
export type CustomJsonOpTuple = ["custom_json", CustomJsonOpBody];

/**
 * Future native protocol op tuple shape (post-HF).
 * Today this package primarily emits custom_json; these are payload stubs
 * for when ops land in the static_variant operation list.
 */
export type NativeOpTuple =
  | ["nft_create_collection", NftCreateCollectionOperation]
  | ["nft_mint", NftMintOperation]
  | ["nft_transfer", NftTransferOperation]
  | ["nft_approve", NftApproveOperation]
  | ["nft_set_approval_for_all", NftSetApprovalForAllOperation]
  | ["nft_burn", NftBurnOperation]
  | ["htlc_create", HtlcCreateOperation]
  | ["htlc_redeem", HtlcRedeemOperation]
  | ["htlc_refund", HtlcRefundOperation]
  | ["contract_deploy", ContractDeployOperation]
  | ["contract_call", ContractCallOperation];
