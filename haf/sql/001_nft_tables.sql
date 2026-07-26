-- HAF table proposals for native NFTs
-- Task-ID: phase-1 / 1.8
-- See: haf/README.md, docs/01-nft-design.md §11
--
-- Retention:
--   * Current-state tables (collections, nfts, operators): always retained.
--   * Burned NFTs: deleted from hive.nfts on burn (after irreversible); history in nft_ops.
--   * Ops stream: follows HAF irreversible retention policy.
--
-- Light-mode API: query hive.nfts_light / hive.nft_collections_light (omit uri payloads).

CREATE SCHEMA IF NOT EXISTS hive;

-- ---------------------------------------------------------------------------
-- Current-state projections (mirror chainbase)
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.nft_collections (
    id              BIGINT PRIMARY KEY,
    creator         VARCHAR(16) NOT NULL,
    symbol          VARCHAR(16) NOT NULL,
    name            VARCHAR(64) NOT NULL,
    max_supply      BIGINT NOT NULL,          -- 0 = unlimited
    supply          BIGINT NOT NULL CHECK (supply >= 0),
    transferable    BOOLEAN NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL,
    CONSTRAINT nft_collections_symbol_unique UNIQUE (symbol),
    CONSTRAINT nft_collections_supply_le_max
        CHECK (max_supply = 0 OR supply <= max_supply)
);

CREATE INDEX IF NOT EXISTS nft_collections_by_creator
    ON hive.nft_collections (creator, id);

CREATE TABLE IF NOT EXISTS hive.nfts (
    id              BIGINT PRIMARY KEY,
    collection_id   BIGINT NOT NULL REFERENCES hive.nft_collections (id),
    token_serial    BIGINT NOT NULL,
    owner           VARCHAR(16) NOT NULL,
    approved        VARCHAR(16),            -- NULL = no per-token approval
    metadata_hash   BYTEA NOT NULL,          -- sha256 (32 bytes)
    uri             TEXT,                   -- optional; max 256 bytes on-chain
    soulbound       BOOLEAN NOT NULL DEFAULT FALSE,
    minted_at       TIMESTAMPTZ NOT NULL,
    CONSTRAINT nfts_collection_serial_unique UNIQUE (collection_id, token_serial),
    CONSTRAINT nfts_metadata_hash_len CHECK (octet_length(metadata_hash) = 32),
    CONSTRAINT nfts_uri_len CHECK (uri IS NULL OR char_length(uri) <= 256)
);

CREATE INDEX IF NOT EXISTS nfts_by_owner
    ON hive.nfts (owner, id);
CREATE INDEX IF NOT EXISTS nfts_by_collection
    ON hive.nfts (collection_id, token_serial);
CREATE INDEX IF NOT EXISTS nfts_by_approved
    ON hive.nfts (approved, id)
    WHERE approved IS NOT NULL;

-- Approval-for-all (ADR-0001). collection_id = 0 means all collections.
CREATE TABLE IF NOT EXISTS hive.nft_operators (
    owner            VARCHAR(16) NOT NULL,
    operator_account VARCHAR(16) NOT NULL,
    collection_id    BIGINT NOT NULL DEFAULT 0,
    approved         BOOLEAN NOT NULL DEFAULT TRUE,
    PRIMARY KEY (owner, operator_account, collection_id)
);

CREATE INDEX IF NOT EXISTS nft_operators_by_operator
    ON hive.nft_operators (operator_account, owner);

-- ---------------------------------------------------------------------------
-- History / ops stream (from virtual ops; subject to HAF retention)
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.nft_ops (
    id              BIGSERIAL PRIMARY KEY,
    block_num       BIGINT NOT NULL,
    trx_in_block    INT NOT NULL DEFAULT 0,
    op_in_trx       INT NOT NULL DEFAULT 0,
    timestamp       TIMESTAMPTZ NOT NULL,
    op_type         VARCHAR(32) NOT NULL,   -- collection_created|minted|transferred|approved|burned|operator_set
    nft_id          BIGINT,                 -- NULL for collection-only events
    collection_id   BIGINT,
    from_account    VARCHAR(16),
    to_account      VARCHAR(16),
    payload         JSONB                   -- residual fields (symbol, hash hex, etc.)
);

CREATE INDEX IF NOT EXISTS nft_ops_by_block
    ON hive.nft_ops (block_num, id);
CREATE INDEX IF NOT EXISTS nft_ops_by_nft
    ON hive.nft_ops (nft_id, id)
    WHERE nft_id IS NOT NULL;
CREATE INDEX IF NOT EXISTS nft_ops_by_owner_accounts
    ON hive.nft_ops (to_account, id)
    WHERE to_account IS NOT NULL;

-- ---------------------------------------------------------------------------
-- Light views (API / mobile: omit heavy payloads)
-- ---------------------------------------------------------------------------

CREATE OR REPLACE VIEW hive.nfts_light AS
SELECT id, collection_id, token_serial, owner, approved,
       metadata_hash, soulbound, minted_at
FROM hive.nfts;

CREATE OR REPLACE VIEW hive.nft_collections_light AS
SELECT id, creator, symbol, name, max_supply, supply, transferable, created_at
FROM hive.nft_collections;

COMMENT ON TABLE hive.nft_collections IS
    'Current-state NFT collections; always retained.';
COMMENT ON TABLE hive.nfts IS
    'Current-state NFT objects; burned tokens removed after irreversible.';
COMMENT ON TABLE hive.nft_operators IS
    'Approval-for-all rows; collection_id=0 means all collections.';
COMMENT ON TABLE hive.nft_ops IS
    'Virtual-op stream for indexers; retention follows HAF irreversible policy.';
COMMENT ON VIEW hive.nfts_light IS
    'Light API projection: omits uri (metadata_hash retained).';
