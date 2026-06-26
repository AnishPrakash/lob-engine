cmake_minimum_required(VERSION 3.26)
project(lob_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG -mavx2")

# Macro to generate gRPC/Protobuf files cleanly
macro(grpc_generate_cpp SRCS HDRS PROTO)
    get_filename_component(PROTO_ABS ${PROTO} ABSOLUTE)
    get_filename_component(PROTO_NAME ${PROTO} NAME_WE)
    
    set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${GRPC_SRC}" "${GRPC_HDR}"
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out="${CMAKE_CURRENT_BINARY_DIR}" 
             --grpc_out="${CMAKE_CURRENT_BINARY_DIR}"
             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
             -I "${CMAKE_CURRENT_SOURCE_DIR}/proto" "${PROTO_ABS}"
        DEPENDS "${PROTO_ABS}"
    )

    list(APPEND ${SRCS} "${PROTO_SRC}" "${GRPC_SRC}")
    list(APPEND ${HDRS} "${PROTO_HDR}" "${GRPC_HDR}")
endmacro()

find_package(benchmark REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Generate code
set(PROTO_SRCS)
set(PROTO_HDRS)
grpc_generate_cpp(PROTO_SRCS PROTO_HDRS proto/metrics.proto)

# Main Library
add_library(lob_lib STATIC
    src/order_book.cpp
    src/matching_engine.cpp
    src/itch_parser.cpp
    src/replay_engine.cpp
    src/fix_output.cpp
    ${PROTO_SRCS}
)

# Crucial: Include the build dir so #include "metrics.proto.pb.h" works
target_include_directories(lob_lib PUBLIC include ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(lob_lib PUBLIC 
    fmt::fmt 
    spdlog::spdlog 
    gRPC::grpc++ 
    protobuf::libprotobuf
)

add_executable(lob_engine src/main.cpp)
target_link_libraries(lob_engine PRIVATE lob_lib ncurses)

add_executable(bench_engine bench/bench_engine.cpp)
target_link_libraries(bench_engine PRIVATE lob_lib benchmark::benchmark)

enable_testing()
add_executable(test_matching tests/test_matching.cpp)
target_link_libraries(test_matching PRIVATE lob_lib)
add_test(NAME matching COMMAND test_matching)