
set(ASIO_GRPC_BUILD_EXAMPLES ON CACHE BOOL "Enable asio-grpc examples")

add_subdirectory(asio-grpc EXCLUDE_FROM_ALL)

add_library(3dparty INTERFACE)

target_link_libraries(3dparty INTERFACE
    asio-grpc::asio-grpc
)
