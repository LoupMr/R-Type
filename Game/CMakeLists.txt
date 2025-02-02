cmake_minimum_required(VERSION 3.16)
project(Game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all Game source and header files
file(GLOB_RECURSE GAME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

file(GLOB_RECURSE GAME_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.hpp
)

# Create Game executable
add_executable(Game ${GAME_SOURCES} ${GAME_HEADERS})

# Link Game with Engine and raylib
target_link_libraries(Game PRIVATE Engine raylib)

# Include directories for Game (PUBLIC so Engine headers are accessible)
target_include_directories(Game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine
)

# Ensure assets folder is copied to the build/ directory
add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Game/assets ${CMAKE_BINARY_DIR}/assets
)
