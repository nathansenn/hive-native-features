#pragma once
/**
 * #23–#28 (+ #103–#105) — portable RocksDB tuning presets.
 *
 * Header-only config sketches for witness NVMe, mobile eMMC, and archive HDD
 * deployments. Maps catalogue items without linking RocksDB itself:
 *   #23 block_cache_mb, #24 bloom_bits_per_key, #25 write_buffer_mb,
 *   #26 compression_hot / compression_cold, #27/#28 documented per-preset
 *   via compaction_style and level0_file_num_compaction_trigger helpers.
 *
 * Upstream mapping: account_history_rocksdb / external_storage Options.
 */
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hive_native {
namespace perf {

/** Portable compression labels (map to rocksdb::CompressionType upstream). */
enum class rocksdb_compression : uint8_t {
   none = 0,
   snappy = 1,
   lz4 = 2,
   zstd = 3,
};

/** Compaction style hint (#27 universal for history-like CFs). */
enum class rocksdb_compaction : uint8_t {
   level = 0,
   universal = 1,
   fifo = 2,
};

/**
 * Portable RocksDB preset — memory, compression, bloom, compaction knobs.
 * Values are megabytes / bits / counts; apply via rocksdb::Options upstream.
 */
struct rocksdb_preset {
   std::string_view name;
   /** #23 — block cache size in MiB (target ~20–30% free RAM when dynamic). */
   uint32_t block_cache_mb;
   /** #25 — memtable write_buffer_size in MiB. */
   uint32_t write_buffer_mb;
   /** #26 — compression for hot / recently written levels. */
   rocksdb_compression compression_hot;
   /** #26 — compression for cold / lower levels. */
   rocksdb_compression compression_cold;
   /** #24 — bloom filter bits per key (catalogue recommends 10). */
   uint32_t bloom_bits_per_key;
   /** #27 — compaction style (universal for history-like CFs). */
   rocksdb_compaction compaction_style;
   /** #28 — L0 file count that triggers compaction (SSD write-amp tune). */
   int level0_file_num_compaction_trigger;
};

/** String label for compression enum (logging / config dump). */
inline constexpr std::string_view compression_name(rocksdb_compression c) {
   switch(c) {
      case rocksdb_compression::none: return "none";
      case rocksdb_compression::snappy: return "snappy";
      case rocksdb_compression::lz4: return "lz4";
      case rocksdb_compression::zstd: return "zstd";
   }
   return "unknown";
}

inline constexpr std::string_view compaction_name(rocksdb_compaction c) {
   switch(c) {
      case rocksdb_compaction::level: return "level";
      case rocksdb_compaction::universal: return "universal";
      case rocksdb_compaction::fifo: return "fifo";
   }
   return "unknown";
}

/**
 * #23 helper — recommended block cache MiB as 25% of free RAM MiB,
 * clamped to [min_mb, max_mb]. Default band matches catalogue 20–30%.
 */
inline constexpr uint32_t block_cache_mb_from_free_ram(
   uint32_t free_ram_mb,
   uint32_t min_mb = 64,
   uint32_t max_mb = 8192) {
   // 25% of free RAM (midpoint of 20–30% band)
   uint64_t v = (uint64_t(free_ram_mb) * 25) / 100;
   if(v < min_mb) return min_mb;
   if(v > max_mb) return max_mb;
   return uint32_t(v);
}

// ---------------------------------------------------------------------------
// Named presets (#103–#105 packaging of #23–#28)
// ---------------------------------------------------------------------------

/**
 * #103 witness_nvme — high cache, LZ4 hot / ZSTD cold, aggressive SSD L0.
 * Target: block-producing / full API nodes on NVMe.
 */
inline constexpr rocksdb_preset witness_nvme{
   /*name*/ "witness_nvme",
   /*block_cache_mb*/ 2048,
   /*write_buffer_mb*/ 128,
   /*compression_hot*/ rocksdb_compression::lz4,
   /*compression_cold*/ rocksdb_compression::zstd,
   /*bloom_bits_per_key*/ 10,
   /*compaction_style*/ rocksdb_compaction::level,
   /*level0_file_num_compaction_trigger*/ 4,
};

/**
 * #104 mobile_emmc — small cache, ZSTD both, lower L0 pressure on flash.
 * Target: light / personal / ARM nodes on eMMC or low-end SSD.
 */
inline constexpr rocksdb_preset mobile_emmc{
   /*name*/ "mobile_emmc",
   /*block_cache_mb*/ 64,
   /*write_buffer_mb*/ 16,
   /*compression_hot*/ rocksdb_compression::zstd,
   /*compression_cold*/ rocksdb_compression::zstd,
   /*bloom_bits_per_key*/ 10,
   /*compaction_style*/ rocksdb_compaction::level,
   /*level0_file_num_compaction_trigger*/ 2,
};

/**
 * #105 archive_hdd — large write buffer, universal compact, space-biased.
 * Target: history / archive nodes on spinning media.
 */
inline constexpr rocksdb_preset archive_hdd{
   /*name*/ "archive_hdd",
   /*block_cache_mb*/ 512,
   /*write_buffer_mb*/ 256,
   /*compression_hot*/ rocksdb_compression::lz4,
   /*compression_cold*/ rocksdb_compression::zstd,
   /*bloom_bits_per_key*/ 10,
   /*compaction_style*/ rocksdb_compaction::universal,
   /*level0_file_num_compaction_trigger*/ 8,
};

/** All built-in presets (stable order for tests / CLI enumeration). */
inline constexpr rocksdb_preset k_rocksdb_presets[] = {
   witness_nvme,
   mobile_emmc,
   archive_hdd,
};

inline constexpr size_t k_rocksdb_preset_count =
   sizeof(k_rocksdb_presets) / sizeof(k_rocksdb_presets[0]);

/** Lookup by name; returns nullptr if unknown. */
inline const rocksdb_preset* find_rocksdb_preset(std::string_view name) {
   for(size_t i = 0; i < k_rocksdb_preset_count; ++i) {
      if(k_rocksdb_presets[i].name == name) return &k_rocksdb_presets[i];
   }
   return nullptr;
}

/** Sanity: bloom ≥ 1, buffers > 0, known compression/compaction. */
inline constexpr bool preset_sane(const rocksdb_preset& p) {
   if(p.name.empty()) return false;
   if(p.block_cache_mb == 0) return false;
   if(p.write_buffer_mb == 0) return false;
   if(p.bloom_bits_per_key == 0) return false;
   if(p.level0_file_num_compaction_trigger < 1) return false;
   switch(p.compression_hot) {
      case rocksdb_compression::none:
      case rocksdb_compression::snappy:
      case rocksdb_compression::lz4:
      case rocksdb_compression::zstd:
         break;
      default: return false;
   }
   switch(p.compression_cold) {
      case rocksdb_compression::none:
      case rocksdb_compression::snappy:
      case rocksdb_compression::lz4:
      case rocksdb_compression::zstd:
         break;
      default: return false;
   }
   switch(p.compaction_style) {
      case rocksdb_compaction::level:
      case rocksdb_compaction::universal:
      case rocksdb_compaction::fifo:
         break;
      default: return false;
   }
   return true;
}

} // namespace perf
} // namespace hive_native
