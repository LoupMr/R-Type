cmake_minimum_required(VERSION 3.16)
project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source and header files
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ECS/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Input/*.cpp
)

file(GLOB_RECURSE ENGINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ECS/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Input/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Library/*.hpp
)

# Create Engine static library
add_library(Engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Link raylib to the Engine
target_link_libraries(Engine PRIVATE raylib)

# Make sure all Engine files can access all Engine headers
target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # Includes all headers inside Engine
)
