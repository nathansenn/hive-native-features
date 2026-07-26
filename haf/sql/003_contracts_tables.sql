-- HAF tables for metered contracts (plugin + future consensus)
-- Task-ID: phase-3 / HAF
-- See: haf/README.md, docs/03-contracts-design.md §13
--
-- Retention:
--   * hive.contracts: current-state registry (id, owner, code_hash). Code blobs
--     are NOT stored here — live in plugin/RocksDB; only sha256 on-chain/index.
--   * hive.contract_calls: call audit stream; HAF irreversible retention.
--   * hive.contract_storage_updates: event-sourced storage diffs; retention policy
--     may drop old diffs if a current-state store exists elsewhere.
--
-- Light-mode: hive.contracts_light (no code blobs — already hash-only here).

CREATE SCHEMA IF NOT EXISTS hive;

-- ---------------------------------------------------------------------------
-- Current-state contract registry
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.contracts (
    id              BIGINT PRIMARY KEY,
    owner           VARCHAR(16) NOT NULL,
    code_hash       BYTEA NOT NULL,
    created_at      TIMESTAMPTZ NOT NULL,
    -- Optional archival pointer for full-code hosts (NULL on light/plugin-skip)
    code_available  BOOLEAN NOT NULL DEFAULT FALSE,
    CONSTRAINT contracts_code_hash_len CHECK (octet_length(code_hash) = 32)
);

CREATE INDEX IF NOT EXISTS contracts_by_owner
    ON hive.contracts (owner, id);

-- ---------------------------------------------------------------------------
-- Call audit / metering stream
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.contract_calls (
    id              BIGSERIAL PRIMARY KEY,
    block_num       BIGINT NOT NULL,
    trx_in_block    INT NOT NULL DEFAULT 0,
    op_in_trx       INT NOT NULL DEFAULT 0,
    timestamp       TIMESTAMPTZ NOT NULL,
    contract_id     BIGINT NOT NULL REFERENCES hive.contracts (id),
    caller          VARCHAR(16) NOT NULL,
    export_name     VARCHAR(64) NOT NULL DEFAULT 'call',
    fuel_limit      BIGINT NOT NULL DEFAULT 0,
    fuel_used       BIGINT NOT NULL CHECK (fuel_used >= 0),
    success         BOOLEAN NOT NULL,
    error_code      VARCHAR(64),            -- NULL on success
    args_hash       BYTEA                   -- optional sha256 of args; omit raw args
);

CREATE INDEX IF NOT EXISTS contract_calls_by_contract
    ON hive.contract_calls (contract_id, id);
CREATE INDEX IF NOT EXISTS contract_calls_by_caller
    ON hive.contract_calls (caller, id);
CREATE INDEX IF NOT EXISTS contract_calls_by_block
    ON hive.contract_calls (block_num, id);

-- ---------------------------------------------------------------------------
-- Storage update events (event-sourced; not a full k/v dump)
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS hive.contract_storage_updates (
    id              BIGSERIAL PRIMARY KEY,
    block_num       BIGINT NOT NULL,
    timestamp       TIMESTAMPTZ NOT NULL,
    contract_id     BIGINT NOT NULL REFERENCES hive.contracts (id),
    call_id         BIGINT REFERENCES hive.contract_calls (id),
    storage_key     TEXT NOT NULL,
    -- NULL value means delete; otherwise raw bytes (plugin may hash-only for light)
    value           BYTEA,
    value_hash      BYTEA,                  -- sha256 of value when value omitted
    CONSTRAINT contract_storage_key_len CHECK (char_length(storage_key) <= 256)
);

CREATE INDEX IF NOT EXISTS contract_storage_updates_by_contract_key
    ON hive.contract_storage_updates (contract_id, storage_key, id);
CREATE INDEX IF NOT EXISTS contract_storage_updates_by_block
    ON hive.contract_storage_updates (block_num, id);

-- ---------------------------------------------------------------------------
-- Light views
-- ---------------------------------------------------------------------------

-- Registry is already hash-only; light view is explicit for API symmetry.
CREATE OR REPLACE VIEW hive.contracts_light AS
SELECT id, owner, code_hash, created_at, code_available
FROM hive.contracts;

-- Call history without args payloads (args only as hash if present)
CREATE OR REPLACE VIEW hive.contract_calls_light AS
SELECT id, block_num, trx_in_block, op_in_trx, timestamp,
       contract_id, caller, export_name, fuel_limit, fuel_used,
       success, error_code, args_hash
FROM hive.contract_calls;

COMMENT ON TABLE hive.contracts IS
    'Contract registry; code blobs live off-table (plugin store); hash always present.';
COMMENT ON TABLE hive.contract_calls IS
    'Call audit stream for metering/analytics; retention per HAF policy.';
COMMENT ON TABLE hive.contract_storage_updates IS
    'Event-sourced storage diffs; light hosts may keep value_hash only.';
COMMENT ON VIEW hive.contracts_light IS
    'Light API projection for contracts (hash-only; no WASM code).';
