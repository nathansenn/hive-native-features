cmake_minimum_required(VERSION 3.16)
project(hive_native_features VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HIVE_NATIVE_WITH_WASMTIME "Link Wasmtime C API if available" OFF)
option(HIVE_NATIVE_BUILD_BENCH "Build microbenchmarks" ON)

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

if(HIVE_NATIVE_BUILD_BENCH)
  add_executable(hive_native_bench benchmarks/bench_ops.cpp)
  target_link_libraries(hive_native_bench PRIVATE hive_native)
endif()

install(TARGETS hive_native ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
