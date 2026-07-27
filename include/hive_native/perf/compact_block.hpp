#pragma once
/**
 * #301 — compact-block relay data structures (portable).
 *
 * BIP152-inspired: announce a block as header hash + 6-byte short transaction
 * IDs + a small set of prefilled full transactions. Receivers reconstruct from
 * their local mempool and request only the missing short IDs (fill requests).
 *
 * Short ID = first 6 bytes of sha256(tx_bytes). Portable simplification of
 * BIP152's salted SipHash; collision rate is low for prototype / test scale.
 */
#include "hive_native/util/types.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hive_native {
namespace perf {

/** 6-byte truncated transaction identifier. */
using short_tx_id = std::array<uint8_t, 6>;

/** Hash for short_tx_id as unordered_map key (packs 6 bytes into size_t). */
struct short_tx_id_hash {
   size_t operator()(const short_tx_id& id) const noexcept {
      size_t h = 0;
      for(uint8_t b : id) h = (h << 8) | size_t(b);
      return h;
   }
};

/** Local mempool view: short_id → full transaction payload. */
using mempool_map = std::unordered_map<short_tx_id, std::string, short_tx_id_hash>;

/** First 6 bytes of a full sha256 digest. */
inline short_tx_id make_short_tx_id(const sha256_t& digest) {
   short_tx_id id{};
   std::memcpy(id.data(), digest.data(), 6);
   return id;
}

/** short_tx_id of raw transaction bytes via sha256(tx)[0..6). */
inline short_tx_id make_short_tx_id(std::string_view tx_bytes) {
   return make_short_tx_id(sha256(tx_bytes));
}

/** Full transaction supplied inline at absolute block index. */
struct prefilled_tx {
   uint32_t    index = 0; // absolute position in the block body
   std::string tx;        // full portable payload (serialized tx bytes)
};

/**
 * Compact block announcement.
 *
 * - `short_ids` holds one short ID per non-prefilled transaction, in block order.
 * - `prefilled` holds (index, full_tx) sorted by ascending index (BIP152-style).
 * - Total transaction count = short_ids.size() + prefilled.size().
 */
struct compact_block {
   sha256_t                     header_hash{};
   std::vector<short_tx_id>     short_ids;
   std::vector<prefilled_tx>    prefilled;

   size_t tx_count() const { return short_ids.size() + prefilled.size(); }
};

/**
 * Reconstruction outcome.
 * - If `missing` is empty, `txs` is the full ordered block body.
 * - If `missing` is non-empty, those short IDs must be requested (getblocktxn /
 *   fill request); `txs` is cleared.
 */
struct reconstruct_result {
   std::vector<std::string> txs;
   std::vector<short_tx_id> missing;

   bool ok() const { return missing.empty(); }
   /** Full list when complete; nullopt when fill request(s) are still needed. */
   std::optional<std::vector<std::string>> full_list() const {
      if(!missing.empty()) return std::nullopt;
      return txs;
   }
};

/**
 * Reconstruct the ordered full transaction list from a local mempool map.
 * Prefilled transactions win at their indexes; remaining slots resolve via
 * short_ids against `local_mempool`.
 */
inline reconstruct_result reconstruct(const compact_block& cb,
                                      const mempool_map& local_mempool) {
   reconstruct_result r;
   const size_t n = cb.tx_count();
   if(n == 0) return r;

   // Validate prefilled: ascending unique indexes in range [0, n).
   for(size_t p = 0; p < cb.prefilled.size(); ++p) {
      const auto& pf = cb.prefilled[p];
      if(pf.index >= n) {
         // Malformed — treat every short_id as missing so caller can fall back.
         r.missing = cb.short_ids;
         r.txs.clear();
         return r;
      }
      if(p > 0 && pf.index <= cb.prefilled[p - 1].index) {
         r.missing = cb.short_ids;
         r.txs.clear();
         return r;
      }
   }

   r.txs.resize(n);
   size_t short_i = 0;
   size_t pref_i  = 0;

   for(size_t i = 0; i < n; ++i) {
      if(pref_i < cb.prefilled.size() && cb.prefilled[pref_i].index == i) {
         r.txs[i] = cb.prefilled[pref_i].tx;
         ++pref_i;
         continue;
      }
      if(short_i >= cb.short_ids.size()) {
         // More slots than short_ids — malformed compact block.
         r.missing = cb.short_ids;
         r.txs.clear();
         return r;
      }
      const short_tx_id& sid = cb.short_ids[short_i++];
      auto it = local_mempool.find(sid);
      if(it == local_mempool.end()) {
         r.missing.push_back(sid);
      } else {
         r.txs[i] = it->second;
      }
   }

   // Leftover prefilled/short entries imply a malformed announcement.
   if(pref_i != cb.prefilled.size() || short_i != cb.short_ids.size()) {
      r.missing = cb.short_ids;
      r.txs.clear();
      return r;
   }

   if(!r.missing.empty()) r.txs.clear();
   return r;
}

/**
 * Build a compact_block from a full ordered body.
 * `prefill_indexes` (optional) must be unique indices into `txs`; those
 * positions are carried as full prefilled txs, the rest as short IDs only.
 */
inline compact_block make_compact_block(const sha256_t& header_hash,
                                        const std::vector<std::string>& txs,
                                        const std::vector<uint32_t>& prefill_indexes = {}) {
   compact_block cb;
   cb.header_hash = header_hash;

   std::vector<bool> is_prefilled(txs.size(), false);
   for(uint32_t idx : prefill_indexes) {
      if(idx < txs.size()) is_prefilled[idx] = true;
   }

   for(size_t i = 0; i < txs.size(); ++i) {
      if(is_prefilled[i]) {
         cb.prefilled.push_back({uint32_t(i), txs[i]});
      } else {
         cb.short_ids.push_back(make_short_tx_id(txs[i]));
      }
   }
   return cb;
}

} // namespace perf
} // namespace hive_native
