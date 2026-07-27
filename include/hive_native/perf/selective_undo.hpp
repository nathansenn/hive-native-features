#pragma once
/**
 * #43 — Selective undo: track only changed fields, not whole objects.
 *
 * Classic Graphene/chainbase undo sessions clone entire multi_index objects.
 * For hot account_balance rows the payload is tiny (hive + hbd); recording only
 * those field values (plus account key) is enough to restore after failed apply
 * or pop_block, with far less memory traffic on the reversible window.
 */
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hive_native {
namespace perf {

/** Minimal portable stand-in for account liquid balances. */
struct account_balance {
   int64_t hive = 0;
   int64_t hbd  = 0;
};

using balance_map = std::unordered_map<std::string, account_balance>;

/**
 * One selective undo record: account key + prior field values only.
 * Does not store the full object, indexes, or unrelated fields.
 */
struct balance_change {
   std::string account;
   int64_t     old_hive = 0;
   int64_t     old_hbd  = 0;
};

/**
 * undo_session bound to a balance_map. Callers push pre-mutation field values
 * then mutate; rollback() restores LIFO. commit() discards the stack.
 */
class undo_session {
public:
   explicit undo_session(balance_map& balances) : balances_(balances) {}

   undo_session(const undo_session&) = delete;
   undo_session& operator=(const undo_session&) = delete;

   /** Record pre-change hive/hbd for `account` (field-level, not whole object). */
   void push_balance_change(const std::string& account, int64_t old_hive, int64_t old_hbd) {
      stack_.push_back(balance_change{account, old_hive, old_hbd});
   }

   /** Restore every recorded balance change in reverse order; clear the stack. */
   void rollback() {
      for(auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
         auto& b = balances_[it->account];
         b.hive = it->old_hive;
         b.hbd  = it->old_hbd;
      }
      stack_.clear();
   }

   /** Accept mutations; drop undo records without restoring. */
   void commit() { stack_.clear(); }

   size_t size() const { return stack_.size(); }
   bool   empty() const { return stack_.empty(); }

private:
   balance_map&               balances_;
   std::vector<balance_change> stack_;
};

/**
 * Adjust balances under an open undo session: push prior fields, then apply delta.
 * `hive_delta` / `hbd_delta` may be negative (spend) or positive (credit).
 */
inline void adjust_balance(undo_session& session, balance_map& balances,
                           const std::string& account,
                           int64_t hive_delta, int64_t hbd_delta) {
   auto& b = balances[account];
   session.push_balance_change(account, b.hive, b.hbd);
   b.hive += hive_delta;
   b.hbd  += hbd_delta;
}

} // namespace perf
} // namespace hive_native
