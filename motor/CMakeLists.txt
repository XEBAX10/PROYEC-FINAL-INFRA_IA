cmake_minimum_required(VERSION 3.16)
project(mancala_motor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

# Download header-only dependencies
include(FetchContent)

FetchContent_Declare(httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(json)

# Common sources
set(MOTOR_SOURCES
    src/board.cpp
    src/minimax.cpp
    src/parallel.cpp
)

# ── Motor HTTP server ──────────────────────────────────────────────────────────
add_executable(mancala_server src/main.cpp ${MOTOR_SOURCES})
target_include_directories(mancala_server PRIVATE
    ${httplib_SOURCE_DIR}
    ${json_SOURCE_DIR}/single_include
)
target_link_libraries(mancala_server PRIVATE OpenMP::OpenMP_CXX)
target_compile_options(mancala_server PRIVATE -O2)

# ── Benchmark binary ───────────────────────────────────────────────────────────
add_executable(mancala_bench bench/mancala_bench.cpp ${MOTOR_SOURCES})
target_link_libraries(mancala_bench PRIVATE OpenMP::OpenMP_CXX)
target_compile_options(mancala_bench PRIVATE -O2)

# ── Unit tests ─────────────────────────────────────────────────────────────────
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(test_board tests/test_board.cpp ${MOTOR_SOURCES})
target_link_libraries(test_board PRIVATE gtest_main OpenMP::OpenMP_CXX)
target_compile_options(test_board PRIVATE -O2)

include(GoogleTest)
gtest_discover_tests(test_board)
