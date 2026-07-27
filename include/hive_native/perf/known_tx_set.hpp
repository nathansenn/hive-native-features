#pragma once
/** #204 — mempool known-transaction set: bloom first filter + exact set for confirms.
 *
 * Fast path: bloom `maybe_seen` rejects most novel txids without a hash-table probe.
 * Confirm path: `definitely_seen` answers via unordered_set ground truth (no FPs).
 * `add` inserts into both layers.
 */
#include "hive_native/perf/bloom.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>

namespace hive_native {
namespace perf {

class known_tx_set {
public:
   explicit known_tx_set(size_t bloom_bits = 1 << 20, size_t bloom_hashes = 4)
      : bloom_(bloom_bits, bloom_hashes) {}

   /** Insert a transaction id into bloom + exact set. Returns true if newly added. */
   bool add(std::string_view txid) {
      auto [it, inserted] = exact_.emplace(txid);
      if(inserted)
         bloom_.add(*it);
      return inserted;
   }

   /**
    * Bloom first filter. True ⇒ txid *may* have been seen (false positives possible).
    * False ⇒ txid was *never* added (no false negatives).
    */
   bool maybe_seen(std::string_view txid) const {
      return bloom_.maybe_contains(txid);
   }

   /** Exact-set ground truth. True iff txid was added; never false-positive. */
   bool definitely_seen(std::string_view txid) const {
      return exact_.find(std::string(txid)) != exact_.end();
   }

   void clear() {
      bloom_.clear();
      exact_.clear();
   }

   size_t size() const { return exact_.size(); }
   size_t bloom_bit_size() const { return bloom_.bit_size(); }

private:
   bloom_filter bloom_;
   std::unordered_set<std::string> exact_;
};

} // namespace perf
} // namespace hive_native
