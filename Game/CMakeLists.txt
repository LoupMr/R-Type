file(GLOB_RECURSE GAME_STATIC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(REMOVE_ITEM GAME_STATIC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/r-type_client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RType/RTypeGamePlugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Snake/SnakeGamePlugin.cpp"
)

add_library(game STATIC ${GAME_STATIC_SOURCES})

target_include_directories(game
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(game
    PUBLIC
        engine
        raylib
)

# Specify the correct relative path to the plugin source files.
add_library(RTypeGamePlugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/RType/RTypeGamePlugin.cpp)

target_include_directories(RTypeGamePlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Engine
)

target_link_libraries(RTypeGamePlugin
    PUBLIC
        network
        engine
        raylib
)

set_target_properties(RTypeGamePlugin PROPERTIES OUTPUT_NAME "RTypeGamePlugin")

add_library(SnakeGamePlugin SHARED ${CMAKE_CURRENT_SOURCE_DIR}/Snake/SnakeGamePlugin.cpp)

target_include_directories(SnakeGamePlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Engine
)

target_link_libraries(SnakeGamePlugin
    PUBLIC
        network
        engine
        raylib
)

set_target_properties(SnakeGamePlugin PROPERTIES OUTPUT_NAME "SnakeGamePlugin")
