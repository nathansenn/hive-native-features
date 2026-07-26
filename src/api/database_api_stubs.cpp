#include "hive_native/api/database_api_stubs.hpp"

namespace hive_native {
namespace api {

namespace {

chain::nft_object maybe_light_nft(chain::nft_object n, bool light) {
   if(light) n.uri.clear();
   return n;
}

chain::htlc_object maybe_light_htlc(chain::htlc_object h, bool light) {
   if(light) h.memo.clear();
   return h;
}

chain::contract_object maybe_light_contract(chain::contract_object c, bool light) {
   if(light) c.code.clear();
   return c;
}

} // namespace

// ---- NFT ----

std::optional<chain::nft_object>
get_nft(const chain::database& db, protocol::nft_id_type id, bool light) {
   if(db.config.nft_skip_state) return std::nullopt;
   auto it = db.nfts.find(id);
   if(it == db.nfts.end()) return std::nullopt;
   return maybe_light_nft(it->second, light);
}

std::optional<chain::nft_collection_object>
get_nft_collection(const chain::database& db, protocol::collection_id_type id) {
   if(db.config.nft_skip_state) return std::nullopt;
   auto it = db.collections.find(id);
   if(it == db.collections.end()) return std::nullopt;
   return it->second;
}

std::optional<chain::nft_collection_object>
get_nft_collection_by_symbol(const chain::database& db, const std::string& symbol) {
   if(db.config.nft_skip_state) return std::nullopt;
   auto sit = db.collection_by_symbol.find(symbol);
   if(sit == db.collection_by_symbol.end()) return std::nullopt;
   auto it = db.collections.find(sit->second);
   if(it == db.collections.end()) return std::nullopt;
   return it->second;
}

std::vector<chain::nft_object>
list_nfts_by_owner(const chain::database& db, const account_name_type& owner, list_args args) {
   std::vector<chain::nft_object> out;
   if(db.config.nft_skip_state) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, n] : db.nfts) {
      (void)id;
      if(n.owner != owner) continue;
      if(n.id < args.start) continue;
      out.push_back(maybe_light_nft(n, args.light));
      if(out.size() >= lim) break;
   }
   return out;
}

std::vector<chain::nft_object>
list_nfts_by_collection(const chain::database& db, protocol::collection_id_type c, list_args args) {
   std::vector<chain::nft_object> out;
   if(db.config.nft_skip_state) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, n] : db.nfts) {
      (void)id;
      if(n.collection != c) continue;
      if(n.id < args.start) continue;
      out.push_back(maybe_light_nft(n, args.light));
      if(out.size() >= lim) break;
   }
   return out;
}

// ---- HTLC ----

std::optional<chain::htlc_object>
get_htlc(const chain::database& db, protocol::htlc_id_type id, bool light) {
   if(db.config.htlc_skip_state) return std::nullopt;
   auto it = db.htlcs.find(id);
   if(it == db.htlcs.end()) return std::nullopt;
   return maybe_light_htlc(it->second, light);
}

std::vector<chain::htlc_object>
list_htlcs_by_from(const chain::database& db, const account_name_type& from, list_args args) {
   std::vector<chain::htlc_object> out;
   if(db.config.htlc_skip_state) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, h] : db.htlcs) {
      (void)id;
      if(h.from != from) continue;
      if(h.id < args.start) continue;
      out.push_back(maybe_light_htlc(h, args.light));
      if(out.size() >= lim) break;
   }
   return out;
}

std::vector<chain::htlc_object>
list_htlcs_by_to(const chain::database& db, const account_name_type& to, list_args args) {
   std::vector<chain::htlc_object> out;
   if(db.config.htlc_skip_state) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, h] : db.htlcs) {
      (void)id;
      if(h.to != to) continue;
      if(h.id < args.start) continue;
      out.push_back(maybe_light_htlc(h, args.light));
      if(out.size() >= lim) break;
   }
   return out;
}

std::vector<chain::htlc_object>
list_htlcs_by_expiration(const chain::database& db, time_point_sec min_expiration, list_args args) {
   std::vector<chain::htlc_object> out;
   if(db.config.htlc_skip_state) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, h] : db.htlcs) {
      (void)id;
      if(h.expiration < min_expiration) continue;
      if(h.id < args.start) continue;
      // Default open-only for expiration listings (matches hive.htlcs_open use case)
      if(h.status != chain::htlc_status::open) continue;
      out.push_back(maybe_light_htlc(h, args.light));
      if(out.size() >= lim) break;
   }
   return out;
}

// ---- Contracts ----

std::optional<chain::contract_object>
get_contract(const chain::database& db, protocol::contract_id_type id, bool light) {
   if(db.config.contracts_skip) return std::nullopt;
   auto it = db.contracts.find(id);
   if(it == db.contracts.end()) return std::nullopt;
   return maybe_light_contract(it->second, light);
}

std::vector<chain::contract_object>
list_contracts_by_owner(const chain::database& db, const account_name_type& owner, list_args args) {
   std::vector<chain::contract_object> out;
   if(db.config.contracts_skip) return out;
   const uint32_t lim = clamp_limit(args.limit);
   for(const auto& [id, c] : db.contracts) {
      (void)id;
      if(c.owner != owner) continue;
      if(c.id < args.start) continue;
      // list defaults to light (strip code) unless light=false explicitly requested
      out.push_back(maybe_light_contract(c, args.light || true));
      // Always strip code on list responses to avoid multi-MB payloads:
      auto& back = out.back();
      if(args.light || true) {
         // Force strip: listing never returns WASM code blobs.
         (void)args;
         back.code.clear();
      }
      if(out.size() >= lim) break;
   }
   return out;
}

std::optional<std::vector<uint8_t>>
get_storage_key(const chain::database& db,
                protocol::contract_id_type id,
                const std::string& key) {
   if(db.config.contracts_skip) return std::nullopt;
   auto cit = db.contract_storage.find(id);
   if(cit == db.contract_storage.end()) return std::nullopt;
   auto kit = cit->second.find(key);
   if(kit == cit->second.end()) return std::nullopt;
   return kit->second;
}

} // namespace api
} // namespace hive_native
