-- HAF tables for HTLC
-- Task-ID: phase-2 / HAF
-- See: haf/README.md, docs/02-htlc-design.md §13
--
-- Retention:
--   * hive.htlcs holds open projection on full/API nodes (status=0 preferred).
--     Closed rows may be deleted after irreversible finality; history lives in
--     hive.htlc_history (virtual ops stream).
--   * Pruned nodes: open HTLCs only.
--   * History: HAF irreversible retention.
--
-- Light-mode: hive.htlcs_open (status=0); never expose preimage in history tables
-- (preimage is only in the redeem op body, not mirrored into closed state).

CREATE SCHEMA IF NOT EXISTS hive;

-- ---------------------------------------------------------------------------
-- Current-state / open projection
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.htlcs (
    id              BIGINT PRIMARY KEY,
    from_account    VARCHAR(16) NOT NULL,
    to_account      VARCHAR(16) NOT NULL,
    amount          BIGINT NOT NULL CHECK (amount > 0),
    symbol          SMALLINT NOT NULL CHECK (symbol IN (0, 1)),  -- 0 HIVE, 1 HBD
    preimage_hash   BYTEA NOT NULL,
    hash_algo       SMALLINT NOT NULL CHECK (hash_algo IN (0, 1)), -- 0 sha256, 1 ripemd160
    preimage_size   INT NOT NULL CHECK (preimage_size > 0 AND preimage_size <= 1024),
    expiration      TIMESTAMPTZ NOT NULL,
    created_at      TIMESTAMPTZ NOT NULL,
    memo            TEXT,
    status          SMALLINT NOT NULL DEFAULT 0
                        CHECK (status IN (0, 1, 2)), -- 0 open, 1 redeemed, 2 refunded
    CONSTRAINT htlcs_hash_len CHECK (
        (hash_algo = 0 AND octet_length(preimage_hash) = 32) OR
        (hash_algo = 1 AND octet_length(preimage_hash) = 20)
    ),
    CONSTRAINT htlcs_memo_len CHECK (memo IS NULL OR char_length(memo) <= 2048),
    CONSTRAINT htlcs_expiration_after_created CHECK (expiration >= created_at)
);

CREATE INDEX IF NOT EXISTS htlcs_by_from
    ON hive.htlcs (from_account, id);
CREATE INDEX IF NOT EXISTS htlcs_by_to
    ON hive.htlcs (to_account, id);
CREATE INDEX IF NOT EXISTS htlcs_by_expiration
    ON hive.htlcs (expiration, id);
CREATE INDEX IF NOT EXISTS htlcs_open_by_from
    ON hive.htlcs (from_account, id)
    WHERE status = 0;

-- ---------------------------------------------------------------------------
-- History (from virtual ops: htlc_created / redeemed / refunded)
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.htlc_history (
    id              BIGSERIAL PRIMARY KEY,
    block_num       BIGINT NOT NULL,
    trx_in_block    INT NOT NULL DEFAULT 0,
    op_in_trx       INT NOT NULL DEFAULT 0,
    timestamp       TIMESTAMPTZ NOT NULL,
    op_type         VARCHAR(16) NOT NULL,   -- created|redeemed|refunded
    htlc_id         BIGINT NOT NULL,
    from_account    VARCHAR(16) NOT NULL,
    to_account       VARCHAR(16),
    amount          BIGINT NOT NULL,
    symbol          SMALLINT NOT NULL CHECK (symbol IN (0, 1)),
    -- Hashes only — never store preimage here (privacy / light-mode policy)
    preimage_hash   BYTEA,
    hash_algo       SMALLINT,
    expiration      TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS htlc_history_by_htlc
    ON hive.htlc_history (htlc_id, id);
CREATE INDEX IF NOT EXISTS htlc_history_by_block
    ON hive.htlc_history (block_num, id);
CREATE INDEX IF NOT EXISTS htlc_history_by_from
    ON hive.htlc_history (from_account, id);
CREATE INDEX IF NOT EXISTS htlc_history_by_to
    ON hive.htlc_history (to_account, id)
    WHERE to_account IS NOT NULL;

-- ---------------------------------------------------------------------------
-- Light views
-- ---------------------------------------------------------------------------

-- Open HTLCs only (pruned / wallet default listing)
CREATE OR REPLACE VIEW hive.htlcs_open AS
SELECT id, from_account, to_account, amount, symbol,
       preimage_hash, hash_algo, preimage_size,
       expiration, created_at, memo, status
FROM hive.htlcs
WHERE status = 0;

-- Light listing: omit memo (bounded but can be large relative to other fields)
CREATE OR REPLACE VIEW hive.htlcs_light AS
SELECT id, from_account, to_account, amount, symbol,
       preimage_hash, hash_algo, preimage_size,
       expiration, created_at, status
FROM hive.htlcs;

COMMENT ON TABLE hive.htlcs IS
    'HTLC projection; prefer open-only after irreversible close (status 1/2 delete).';
COMMENT ON TABLE hive.htlc_history IS
    'Virtual-op history; preimage never stored; retention per HAF policy.';
COMMENT ON VIEW hive.htlcs_open IS
    'Open HTLCs only (status=0).';
COMMENT ON VIEW hive.htlcs_light IS
    'Light API projection: omits memo.';
