# Engine/CMakeLists.txt

# Gather all .cpp files under Engine (including subfolders such as ECS, Core, Graphics)
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(engine ${ENGINE_SOURCES})

# Expose the parent directory so that headers (like "Engine/ECS/…") can be found.
target_include_directories(engine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# Link raylib if needed by Engine code.
target_link_libraries(engine
    PUBLIC
        raylib
)
