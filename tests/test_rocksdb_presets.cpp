/**
 * Perf catalogue portable prototypes — RocksDB presets #23–#28 (+ #103–#105)
 */
#include "hive_native/perf/rocksdb_presets.hpp"
#include <iostream>
#include <string_view>

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

using namespace hive_native::perf;

static void test_struct_fields() {
   // Core fields exist and are readable on every preset
   CHECK(witness_nvme.name == "witness_nvme");
   CHECK(mobile_emmc.name == "mobile_emmc");
   CHECK(archive_hdd.name == "archive_hdd");

   CHECK(witness_nvme.block_cache_mb > 0);
   CHECK(witness_nvme.write_buffer_mb > 0);
   CHECK(witness_nvme.bloom_bits_per_key > 0);
   CHECK(mobile_emmc.block_cache_mb > 0);
   CHECK(archive_hdd.write_buffer_mb > 0);
}

static void test_catalogue_23_block_cache() {
   // #23 — witness high cache; mobile small; dynamic helper mid-band
   CHECK(witness_nvme.block_cache_mb >= 1024);
   CHECK(mobile_emmc.block_cache_mb <= 128);
   CHECK(archive_hdd.block_cache_mb > mobile_emmc.block_cache_mb);
   CHECK(archive_hdd.block_cache_mb < witness_nvme.block_cache_mb);

   // 25% of 4096 MiB free = 1024; clamps apply
   CHECK(block_cache_mb_from_free_ram(4096) == 1024);
   CHECK(block_cache_mb_from_free_ram(100) == 64);       // min clamp
   CHECK(block_cache_mb_from_free_ram(100000) == 8192);  // max clamp
}

static void test_catalogue_24_bloom() {
   // #24 — 10 bits/key on all production presets
   CHECK(witness_nvme.bloom_bits_per_key == 10);
   CHECK(mobile_emmc.bloom_bits_per_key == 10);
   CHECK(archive_hdd.bloom_bits_per_key == 10);
}

static void test_catalogue_25_write_buffer() {
   // #25 — bulk/replay band 64–128 on witness; archive larger; mobile smaller
   CHECK(witness_nvme.write_buffer_mb >= 64 && witness_nvme.write_buffer_mb <= 128);
   CHECK(archive_hdd.write_buffer_mb >= 128);
   CHECK(mobile_emmc.write_buffer_mb < witness_nvme.write_buffer_mb);
}

static void test_catalogue_26_compression() {
   // #26 — ZSTD cold, LZ4 hot (witness/archive); mobile ZSTD both (space)
   CHECK(witness_nvme.compression_hot == rocksdb_compression::lz4);
   CHECK(witness_nvme.compression_cold == rocksdb_compression::zstd);
   CHECK(archive_hdd.compression_hot == rocksdb_compression::lz4);
   CHECK(archive_hdd.compression_cold == rocksdb_compression::zstd);
   CHECK(mobile_emmc.compression_hot == rocksdb_compression::zstd);
   CHECK(mobile_emmc.compression_cold == rocksdb_compression::zstd);

   CHECK(compression_name(rocksdb_compression::lz4) == "lz4");
   CHECK(compression_name(rocksdb_compression::zstd) == "zstd");
}

static void test_catalogue_27_universal() {
   // #27 — universal compaction for history-like archive profile
   CHECK(archive_hdd.compaction_style == rocksdb_compaction::universal);
   CHECK(witness_nvme.compaction_style == rocksdb_compaction::level);
   CHECK(mobile_emmc.compaction_style == rocksdb_compaction::level);
   CHECK(compaction_name(rocksdb_compaction::universal) == "universal");
}

static void test_catalogue_28_l0() {
   // #28 — SSD-friendly lower L0 trigger on NVMe; mobile even lower; HDD higher
   CHECK(witness_nvme.level0_file_num_compaction_trigger == 4);
   CHECK(mobile_emmc.level0_file_num_compaction_trigger <= 4);
   CHECK(archive_hdd.level0_file_num_compaction_trigger >= 4);
}

static void test_lookup_and_table() {
   CHECK(k_rocksdb_preset_count == 3);
   CHECK(find_rocksdb_preset("witness_nvme") != nullptr);
   CHECK(find_rocksdb_preset("mobile_emmc") != nullptr);
   CHECK(find_rocksdb_preset("archive_hdd") != nullptr);
   CHECK(find_rocksdb_preset("nope") == nullptr);
   CHECK(find_rocksdb_preset("witness_nvme")->block_cache_mb == witness_nvme.block_cache_mb);

   for(size_t i = 0; i < k_rocksdb_preset_count; ++i)
      CHECK(preset_sane(k_rocksdb_presets[i]));
}

static void test_relative_ordering() {
   // Mobile is the memory-constrained profile
   CHECK(mobile_emmc.block_cache_mb < witness_nvme.block_cache_mb);
   CHECK(mobile_emmc.write_buffer_mb < witness_nvme.write_buffer_mb);
   // Archive prioritizes write buffer / sequential bulk
   CHECK(archive_hdd.write_buffer_mb >= witness_nvme.write_buffer_mb);
}

int main() {
   test_struct_fields();
   test_catalogue_23_block_cache();
   test_catalogue_24_bloom();
   test_catalogue_25_write_buffer();
   test_catalogue_26_compression();
   test_catalogue_27_universal();
   test_catalogue_28_l0();
   test_lookup_and_table();
   test_relative_ordering();
   std::cout << "rocksdb_presets_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
