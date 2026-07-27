cmake_minimum_required(VERSION 3.16)
project(hive_native_features VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HIVE_NATIVE_WITH_WASMTIME "Link Wasmtime C API if available" OFF)
option(HIVE_NATIVE_BUILD_BENCH "Build microbenchmarks" ON)
# Catalogue #691 — light/mobile compile profile (default_profile → mobile_light)
option(HIVE_LIGHT_NODE "Default node profile to mobile_light (non-consensus, skip feature state)" OFF)

add_library(hive_native
  src/util/crypto.cpp
  src/protocol/validate.cpp
  src/chain/database.cpp
  src/chain/evaluators_nft.cpp
  src/chain/evaluators_htlc.cpp
  src/chain/evaluators_contracts.cpp
  src/rc/costs.cpp
  src/contracts/engine.cpp
  src/api/database_api_stubs.cpp
)
target_include_directories(hive_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(hive_native PRIVATE -Wall -Wextra -Wpedantic)

if(HIVE_LIGHT_NODE)
  target_compile_definitions(hive_native PUBLIC HIVE_LIGHT_NODE=1)
endif()

if(HIVE_NATIVE_WITH_WASMTIME)
  target_compile_definitions(hive_native PUBLIC HIVE_NATIVE_WITH_WASMTIME=1)
  # User supplies Wasmtime via WASMTIME_ROOT
  if(DEFINED WASMTIME_ROOT)
    target_include_directories(hive_native PUBLIC ${WASMTIME_ROOT}/include)
    target_link_directories(hive_native PUBLIC ${WASMTIME_ROOT}/lib)
    target_link_libraries(hive_native PUBLIC wasmtime)
  endif()
endif()

enable_testing()
add_executable(hive_native_tests tests/test_runner.cpp)
target_link_libraries(hive_native_tests PRIVATE hive_native)
add_test(NAME hive_native_tests COMMAND hive_native_tests)

add_executable(hive_native_perf_tests tests/test_perf.cpp)
target_link_libraries(hive_native_perf_tests PRIVATE hive_native)
# worker_pool needs threads
find_package(Threads REQUIRED)
target_link_libraries(hive_native_perf_tests PRIVATE Threads::Threads)
add_test(NAME hive_native_perf_tests COMMAND hive_native_perf_tests)

# #23–#28 RocksDB presets (header-only; no RocksDB link required)
add_executable(hive_native_rocksdb_preset_tests tests/test_rocksdb_presets.cpp)
target_link_libraries(hive_native_rocksdb_preset_tests PRIVATE hive_native)
add_test(NAME hive_native_rocksdb_preset_tests COMMAND hive_native_rocksdb_preset_tests)

add_executable(hive_native_selective_undo_tests tests/test_selective_undo.cpp)
target_link_libraries(hive_native_selective_undo_tests PRIVATE hive_native)
add_test(NAME hive_native_selective_undo_tests COMMAND hive_native_selective_undo_tests)

# #301 compact-block relay (portable)
add_executable(hive_native_compact_block_tests tests/test_compact_block.cpp)
target_link_libraries(hive_native_compact_block_tests PRIVATE hive_native)
add_test(NAME hive_native_compact_block_tests COMMAND hive_native_compact_block_tests)

# Catalogue #63 — primary-key account cache / AccountIndex
add_executable(hive_native_account_cache_tests tests/test_account_cache.cpp)
target_link_libraries(hive_native_account_cache_tests PRIVATE hive_native)
add_test(NAME hive_native_account_cache_tests COMMAND hive_native_account_cache_tests)

# #151 apply_scheduler: dependency-aware portable apply
add_executable(hive_native_apply_scheduler_tests tests/test_apply_scheduler.cpp)
target_link_libraries(hive_native_apply_scheduler_tests PRIVATE hive_native)
add_test(NAME hive_native_apply_scheduler_tests COMMAND hive_native_apply_scheduler_tests)

# #209 — SIMD / scalar batch arithmetic (vote-weight helper)
add_executable(hive_native_simd_math_tests tests/test_simd_math.cpp)
target_link_libraries(hive_native_simd_math_tests PRIVATE hive_native)
add_test(NAME hive_native_simd_math_tests COMMAND hive_native_simd_math_tests)

# Catalogue #204 — known_tx_set (bloom + exact set)
add_executable(hive_native_known_tx_tests tests/test_known_tx.cpp)
target_link_libraries(hive_native_known_tx_tests PRIVATE hive_native)
add_test(NAME hive_native_known_tx_tests COMMAND hive_native_known_tx_tests)

# #151 dependency-graph stress (random + worst-case serial)
add_executable(hive_native_dep_stress_tests tests/test_op_dependency_stress.cpp)
target_link_libraries(hive_native_dep_stress_tests PRIVATE hive_native)
add_test(NAME hive_native_dep_stress_tests COMMAND hive_native_dep_stress_tests)

# Catalogue #8 #691 — light node profile presets (full / api_pruned / mobile_light)
add_executable(hive_native_node_profiles_tests tests/test_node_profiles.cpp)
target_link_libraries(hive_native_node_profiles_tests PRIVATE hive_native)
add_test(NAME hive_native_node_profiles_tests COMMAND hive_native_node_profiles_tests)

# HTLC preimage fuzz — deterministic xorshift, short runtime (docs/05 §8).
# Disable with -DHIVE_NATIVE_BUILD_FUZZ=OFF if needed.
option(HIVE_NATIVE_BUILD_FUZZ "Build short-runtime deterministic fuzz targets" ON)
if(HIVE_NATIVE_BUILD_FUZZ)
  add_executable(hive_native_fuzz_htlc tests/fuzz_htlc_preimage.cpp)
  target_link_libraries(hive_native_fuzz_htlc PRIVATE hive_native)
  add_test(NAME hive_native_fuzz_htlc COMMAND hive_native_fuzz_htlc)
endif()

if(HIVE_NATIVE_BUILD_BENCH)
  add_executable(hive_native_bench benchmarks/bench_ops.cpp)
  target_link_libraries(hive_native_bench PRIVATE hive_native)

  # #891 RC calibration harness — optional; not a ctest hard-fail gate
  add_executable(hive_native_bench_rc benchmarks/bench_rc_calibrate.cpp)
  target_link_libraries(hive_native_bench_rc PRIVATE hive_native)
endif()

install(TARGETS hive_native ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
