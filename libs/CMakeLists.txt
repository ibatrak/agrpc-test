set(LIB_NAME libs)

set(HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src/helloworld EXCLUDE_FROM_ALL)

add_library(${LIB_NAME} INTERFACE)

target_include_directories(${LIB_NAME} INTERFACE ${HEADER_PATH})

set(LINK_LIBRARIES
    helloworld
)

target_link_libraries(${LIB_NAME} INTERFACE ${LINK_LIBRARIES})
