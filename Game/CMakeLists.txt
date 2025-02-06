# Game/CMakeLists.txt

# --- Build the common game library ---
# Gather all .cpp files in Game/ except the client main and the plugin file.
file(GLOB_RECURSE GAME_STATIC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(REMOVE_ITEM GAME_STATIC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/r-type_client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RTypeGamePlugin.cpp"  # Exclude plugin source from this target
)

add_library(game STATIC ${GAME_STATIC_SOURCES})

# Expose the Game folder to anyone linking this library.
target_include_directories(game
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to engine and raylib (if game code uses them).
target_link_libraries(game
    PUBLIC
        engine
        raylib
)

# --- Build the game plugin as a shared library ---
add_library(RTypeGamePlugin SHARED RTypeGamePlugin.cpp)

# Make sure the plugin can find IGame.hpp (in Engine) and any other needed headers.
target_include_directories(RTypeGamePlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}      # for Game files if needed
        ${CMAKE_CURRENT_SOURCE_DIR}/../Engine  # for IGame.hpp
)

target_link_libraries(RTypeGamePlugin
    PUBLIC
        network
        engine
        raylib
)

# Optionally, set the output name so that the shared library is named (e.g.) libRTypeGamePlugin.so (or RTypeGamePlugin.dll on Windows).
set_target_properties(RTypeGamePlugin PROPERTIES OUTPUT_NAME "RTypeGamePlugin")
