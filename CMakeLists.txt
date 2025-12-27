cmake_minimum_required (VERSION 3.30)

project(grpc-asio-test LANGUAGES CXX)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set the C++ standard to C++20
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif()

# boost
set(Boost_ROOT $ENV{BOOST_ROOT_PRIVATE} CACHE PATH "BOOST PATH")
# set(Boost_NO_SYSTEM_PATHS ON)
# set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

# grpc
list(APPEND CMAKE_PREFIX_PATH "$ENV{GRPC_ROOT}")

message("** Building with CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}\n** CMake version=${CMAKE_VERSION}\n** CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")

find_package(Boost 1.89.0 REQUIRED COMPONENTS thread context cobalt)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Set the source and binary directories
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(3DPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3dparty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(GRPC_ASIO_SOURCES
    ${SOURCE_DIR}/main.cpp
)

set(GRPC_ASIO_HEADERS
)

# Add your include directories if needed
set(GRPC_ASIO_INCLUDE_DIRS
    # ${CMAKE_CURRENT_SOURCE_DIR}/include
    # ${3DPARTY_DIR}/vml/include
    # Add additional include directories here
)

# Add your library dependencies if needed
set(LIBRARIES
    Boost::context
    Boost::thread
    Boost::cobalt
    gRPC::grpc++

    3dparty
    libs
)

# 3dparty section
add_subdirectory(3dparty EXCLUDE_FROM_ALL)

# libs section
add_subdirectory(libs EXCLUDE_FROM_ALL)

# Create the executable
add_executable(${PROJECT_NAME} ${GRPC_ASIO_HEADERS} ${GRPC_ASIO_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${GRPC_ASIO_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
