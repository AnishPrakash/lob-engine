cmake_minimum_required(VERSION 3.26)
project(lob_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Release flags - optimise hard
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

find_package(benchmark REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Main library (reused by tests & bench)
add_library(lob_lib STATIC
    src/order_book.cpp
    src/matching_engine.cpp
    src/itch_parser.cpp
    src/replay_engine.cpp
    src/fix_output.cpp
)
target_include_directories(lob_lib PUBLIC include)
target_link_libraries(lob_lib PUBLIC fmt::fmt spdlog::spdlog)

# Executable
add_executable(lob_engine src/main.cpp)
target_link_libraries(lob_engine PRIVATE lob_lib ncurses)

# Benchmarks
add_executable(bench_engine bench/bench_engine.cpp)
target_link_libraries(bench_engine PRIVATE lob_lib benchmark::benchmark)

# Tests
enable_testing()
add_executable(test_matching tests/test_matching.cpp)
target_link_libraries(test_matching PRIVATE lob_lib)
add_test(NAME matching COMMAND test_matching)