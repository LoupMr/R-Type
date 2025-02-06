cmake_minimum_required(VERSION 3.16)
project(RTypeProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1) Use FetchContent to get Raylib
include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 4.5.0
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# 2) Add subdirectories to build the Engine, Game, and Network libraries
add_subdirectory(Engine)
add_subdirectory(Game)
add_subdirectory(Network)

# 3) Build the r-type_client executable (client code)
add_executable(r-type_client Game/r-type_client.cpp)
target_link_libraries(r-type_client
    PRIVATE
        engine
        game
        network
        raylib
)

# 4) Build the r-type_server executable (server code)
add_executable(r-type_server Network/r-type_server.cpp)
target_link_libraries(r-type_server
    PRIVATE
        engine
        network
        raylib
)

# 5) Copy the "assets" folder into the build directory
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMENT "Copying assets folder to build directory..."
)
add_dependencies(r-type_client copy_assets)
add_dependencies(r-type_server copy_assets)
