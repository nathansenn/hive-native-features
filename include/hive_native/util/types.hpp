#pragma once
/**
 * Portable type aliases for hive-native-features.
 * Mirrors Hive naming without requiring full chainbase.
 * Task-ID: phase-1 / foundation
 */
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace hive_native {

// ---- Hardfork placeholder (human-set before mainnet) ----
inline constexpr uint32_t HIVE_HARDFORK_NFT   = 9001; // TBD upstream number
inline constexpr uint32_t HIVE_HARDFORK_HTLC  = 9002;
inline constexpr uint32_t HIVE_HARDFORK_CONTRACTS = 9003; // Phase 3h only

// ---- Size caps (performance + DoS) ----
inline constexpr size_t MAX_ACCOUNT_NAME_LEN   = 16;
inline constexpr size_t MAX_NFT_SYMBOL_LEN     = 16;
inline constexpr size_t MAX_NFT_NAME_LEN       = 64;
inline constexpr size_t MAX_NFT_URI_LEN        = 256;
inline constexpr size_t MAX_MEMO_LEN           = 2048;
inline constexpr size_t MAX_HTLC_PREIMAGE_LEN  = 1024;
inline constexpr size_t MAX_CONTRACT_EXPORT    = 64;
inline constexpr size_t MAX_CONTRACT_ARGS      = 64 * 1024;
inline constexpr size_t MAX_CODE_BYTES         = 512 * 1024;

inline constexpr int64_t HTLC_MIN_DURATION_SEC = 60;
inline constexpr int64_t HTLC_MAX_DURATION_SEC = 30LL * 24 * 3600;
inline constexpr uint32_t HTLC_MAX_OPEN_PER_ACCOUNT = 256;

using account_name_type = std::string;
using time_point_sec    = uint32_t;
using share_type        = int64_t;

enum class asset_symbol : uint8_t { HIVE = 0, HBD = 1 };

struct asset {
   share_type   amount = 0;
   asset_symbol symbol = asset_symbol::HIVE;

   bool operator==(const asset& o) const {
      return amount == o.amount && symbol == o.symbol;
   }
   bool is_positive() const { return amount > 0; }
};

using sha256_t    = std::array<uint8_t, 32>;
using ripemd160_t = std::array<uint8_t, 20>;

enum class hash_algo : uint8_t { sha256 = 0, ripemd160 = 1 };

struct hash_digest {
   hash_algo algo = hash_algo::sha256;
   std::vector<uint8_t> bytes; // 32 or 20

   bool operator==(const hash_digest& o) const {
      return algo == o.algo && bytes == o.bytes;
   }
};

struct protocol_error : std::runtime_error {
   using std::runtime_error::runtime_error;
};

inline void assert_account_name(std::string_view name) {
   if(name.empty() || name.size() > MAX_ACCOUNT_NAME_LEN)
      throw protocol_error("invalid account name length");
   for(char c : name) {
      const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-';
      if(!ok) throw protocol_error("invalid account name character");
   }
}

inline void assert_len(std::string_view s, size_t max, const char* what) {
   if(s.size() > max) throw protocol_error(std::string(what) + " too long");
}

// Minimal FNV-1a for deterministic non-crypto ids in tests (not for HTLC).
inline uint64_t fnv1a64(std::string_view data) {
   uint64_t h = 14695981039346656037ull;
   for(unsigned char c : data) {
      h ^= c;
      h *= 1099511628211ull;
   }
   return h;
}

// Portable SHA-256 (compact implementation) — consensus-critical path candidate.
// Used for HTLC and metadata hashes in this portable library.
sha256_t sha256(const uint8_t* data, size_t len);
inline sha256_t sha256(std::string_view s) {
   return sha256(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}
inline sha256_t sha256(const std::vector<uint8_t>& v) {
   return sha256(v.data(), v.size());
}

// RIPEMD-160 (compact) for HTLC interop.
ripemd160_t ripemd160(const uint8_t* data, size_t len);
inline ripemd160_t ripemd160(const std::vector<uint8_t>& v) {
   return ripemd160(v.data(), v.size());
}

inline hash_digest digest_of(hash_algo algo, const std::vector<uint8_t>& preimage) {
   hash_digest d;
   d.algo = algo;
   if(algo == hash_algo::sha256) {
      auto h = sha256(preimage);
      d.bytes.assign(h.begin(), h.end());
   } else {
      auto h = ripemd160(preimage);
      d.bytes.assign(h.begin(), h.end());
   }
   return d;
}

inline bool constant_time_equal(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
   if(a.size() != b.size()) return false;
   uint8_t diff = 0;
   for(size_t i = 0; i < a.size(); ++i) diff |= uint8_t(a[i] ^ b[i]);
   return diff == 0;
}

} // namespace hive_native
