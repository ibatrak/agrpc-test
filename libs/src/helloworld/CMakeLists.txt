set(LIB_NAME helloworld)

#protobuf section
set(protobuf_generate_PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/gen CACHE STRING "set protobuf generated files output dir")
file(MAKE_DIRECTORY ${protobuf_generate_PROTOC_OUT_DIR})

set(HELLOWORLD_PROTO_IMPORT_DIR ${CMAKE_SOURCE_DIR}/3dparty/helloworld)

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "** Using protobuf ${protobuf_VERSION}")
find_package(gRPC CONFIG REQUIRED)
message(STATUS "** Using gRPC ${gRPC_VERSION}")

set(HELLOWORLD_PROTO_FILES
    ${HELLOWORLD_PROTO_IMPORT_DIR}/helloworld.proto
)

add_library(me-controller_proto STATIC)
target_sources(me-controller_proto PRIVATE ${HELLOWORLD_PROTO_FILES})
target_link_libraries(me-controller_proto PUBLIC gRPC::grpc++)
target_include_directories(me-controller_proto PUBLIC ${protobuf_generate_PROTOC_OUT_DIR})

protobuf_generate(TARGET me-controller_proto PROTOS ${HELLOWORLD_PROTO_FILES} IMPORT_DIRS ${HELLOWORLD_PROTO_IMPORT_DIR} LANGUAGE cpp)

protobuf_generate(
    TARGET me-controller_proto
    LANGUAGE grpc
    IMPORT_DIRS ${HELLOWORLD_PROTO_IMPORT_DIR}
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

set(HEADER_PATH ${LIBRARY_DIR}/include)

set(HELLO_WORLD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_server.cpp
)

set(HELLO_WORLD_HEADERS
    ${HEADER_PATH}/${LIB_NAME}/hello_world_server.h
)

add_library(${LIB_NAME} STATIC
    ${HELLO_WORLD_SOURCES}
    ${HELLO_WORLD_HEADERS})

target_include_directories(${LIB_NAME} PRIVATE
    ${Boost_INCLUDE_DIR}
    ${Agrpc_INCLUDE_DIR}
    ${HEADER_PATH}
)

target_link_libraries(${LIB_NAME} PRIVATE
    me-controller_proto
    asio-grpc::asio-grpc
)
