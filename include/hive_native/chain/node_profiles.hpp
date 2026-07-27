#pragma once
/**
 * Light / pruned / full node profile presets.
 * Catalogue: #8 (drop secondary indexes on pruned/mobile), #691 (HIVE_LIGHT_NODE).
 * Task: swarm-perf-p0 / perf-05-light-profiles
 *
 * Maps operational node classes onto portable `node_config` skip flags.
 * Witnesses / consensus nodes must never enable NFT or HTLC skip state.
 */
#include "hive_native/chain/database.hpp"
#include <string_view>

namespace hive_native {
namespace chain {

/** Named node profiles for portable / upstream light-node configuration. */
enum class node_profile : uint8_t {
   full = 0,         ///< Witness / full producer: consensus, all feature state
   api_pruned = 1,   ///< Non-consensus API: keep NFT/HTLC indexes; skip contracts engine
   mobile_light = 2  ///< Mobile / minimal: non-consensus, skip NFT + HTLC + contracts state
};

inline std::string_view to_string(node_profile p) {
   switch(p) {
      case node_profile::full:         return "full";
      case node_profile::api_pruned:   return "api_pruned";
      case node_profile::mobile_light: return "mobile_light";
   }
   return "unknown";
}

/**
 * True when config is legal for its role.
 * Consensus / witness nodes cannot enable nft_skip_state or htlc_skip_state.
 * (contracts_skip on consensus is allowed; evaluators ignore it for deploy/call.)
 */
inline bool config_ok_for_role(const node_config& cfg) {
   if(cfg.is_consensus_node && (cfg.nft_skip_state || cfg.htlc_skip_state))
      return false;
   return true;
}

/**
 * Enforce consensus invariants in-place: clear illegal NFT/HTLC skips on witnesses.
 * Returns true if no correction was needed.
 */
inline bool sanitize_consensus_skips(node_config& cfg) {
   if(!cfg.is_consensus_node)
      return true;
   bool ok = true;
   if(cfg.nft_skip_state) {
      cfg.nft_skip_state = false;
      ok = false;
   }
   if(cfg.htlc_skip_state) {
      cfg.htlc_skip_state = false;
      ok = false;
   }
   return ok;
}

/**
 * Apply a named profile to `cfg` (sets skip flags and consensus role).
 *
 * | Profile       | is_consensus | nft_skip | htlc_skip | contracts_skip |
 * |---------------|--------------|----------|-----------|----------------|
 * | full          | true         | false    | false     | false          |
 * | api_pruned    | false        | false    | false     | true           |
 * | mobile_light  | false        | true     | true      | true           |
 *
 * After apply, consensus nodes never retain nft/htlc skip (safety net).
 */
inline void apply_profile(node_config& cfg, node_profile p) {
   switch(p) {
      case node_profile::full:
         cfg.is_consensus_node = true;
         cfg.nft_skip_state    = false;
         cfg.htlc_skip_state   = false;
         cfg.contracts_skip    = false;
         break;
      case node_profile::api_pruned:
         // Non-consensus API: serve NFT/HTLC; drop heavy contracts path (#8 pruned tier).
         cfg.is_consensus_node = false;
         cfg.nft_skip_state    = false;
         cfg.htlc_skip_state   = false;
         cfg.contracts_skip    = true;
         break;
      case node_profile::mobile_light:
         // Mobile / light: no local secondary feature indexes (#8 #691).
         cfg.is_consensus_node = false;
         cfg.nft_skip_state    = true;
         cfg.htlc_skip_state   = true;
         cfg.contracts_skip    = true;
         break;
   }
   // Invariant: witnesses cannot enable nft/htlc skip.
   if(cfg.is_consensus_node) {
      cfg.nft_skip_state  = false;
      cfg.htlc_skip_state = false;
   }
}

/** Convenience: apply profile to a live database config. */
inline void apply_profile(database& db, node_profile p) {
   apply_profile(db.config, p);
}

/** Build a fresh node_config from a profile (hardfork left at struct default). */
inline node_config make_node_config(node_profile p) {
   node_config cfg;
   apply_profile(cfg, p);
   return cfg;
}

/**
 * Compile-time / startup default profile.
 * When built with -DHIVE_LIGHT_NODE (CMake option), defaults to mobile_light;
 * otherwise full (witness-safe).
 */
inline node_profile default_profile() {
#ifdef HIVE_LIGHT_NODE
   return node_profile::mobile_light;
#else
   return node_profile::full;
#endif
}

/** Optional: apply compile-time default profile to config (call at node startup). */
inline void apply_default_profile(node_config& cfg) {
   apply_profile(cfg, default_profile());
}

/**
 * Reject illegal manual flag combos (e.g. consensus + nft_skip).
 * Prefer apply_profile() over hand-setting skips on witnesses.
 */
inline void require_config_ok_for_role(const node_config& cfg) {
   if(!config_ok_for_role(cfg))
      throw protocol_error("consensus node cannot skip NFT/HTLC state");
}

} // namespace chain
} // namespace hive_native
