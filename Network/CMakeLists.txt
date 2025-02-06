# Network/CMakeLists.txt

file(GLOB_RECURSE NETWORK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(network ${NETWORK_SOURCES})

target_include_directories(network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network
    PUBLIC
        game
        engine
        raylib
)
