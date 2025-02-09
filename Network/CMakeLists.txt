cmake_minimum_required(VERSION 3.10)
project(Network)

# Gather sources only from the Server and System directories.
file(GLOB_RECURSE NETWORK_LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/System/*.cpp"
)

add_library(network ${NETWORK_LIB_SOURCES})

target_include_directories(network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # This will include Protocol, Server, and System headers.
)

target_link_libraries(network
    PUBLIC
        game
        engine
        raylib
)
