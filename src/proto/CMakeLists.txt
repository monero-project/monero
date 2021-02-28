
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(monero_wallet_proto "./wallet.proto" ABSOLUTE)
get_filename_component(monero_wallet_proto_msg "./wallet_messages.proto" ABSOLUTE)
get_filename_component(monero_wallet_proto_path "${monero_wallet_proto}" PATH)

# Generated sources
set(monero_wallet_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/wallet.pb.cc")
set(monero_wallet_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wallet.pb.h")
set(monero_wallet_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/wallet.grpc.pb.cc")
set(monero_wallet_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wallet.grpc.pb.h")

add_custom_command(
        OUTPUT "${monero_wallet_proto_srcs}" "${monero_wallet_proto_hdrs}" "${monero_wallet_grpc_srcs}" "${monero_wallet_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${monero_wallet_proto_path}"
        -I "${CMAKE_SOURCE_DIR}/external/grpc/third_party/protobuf/src/"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${monero_wallet_proto}" "${monero_wallet_proto_msg}"
        DEPENDS "${monero_wallet_proto}" "${monero_wallet_proto_msg}"
)


include_directories("${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "${CMAKE_CURRENT_BINARY_DIR}")
set(wallet_grpc_proto_hdrs
        ${monero_wallet_proto_hdrs}
        ${monero_wallet_grpc_hdrs}
        )
set(wallet_grpc_proto_srcs
        ${monero_wallet_proto_srcs}
        ${monero_wallet_grpc_srcs}
        )

add_library(
        wallet_grpc_proto
        ${monero_wallet_proto_srcs}
        ${monero_wallet_grpc_srcs}
        ${monero_wallet_proto_hdrs}
        ${monero_wallet_grpc_hdrs}
)

get_filename_component(INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/.. ABSOLUTE)

target_include_directories(
        wallet_grpc_proto
        PUBLIC
        ${INCLUDE_DIR})


target_link_libraries(
        wallet_grpc_proto
        PUBLIC
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
)

set(ProtobufIncludePath ${CMAKE_CURRENT_BINARY_DIR}
        CACHE INTERNAL "Path to generated protobuf files.")

