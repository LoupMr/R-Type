cmake_minimum_required(VERSION 3.16)
project(Game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Client Executable ("r-type_client") ---
file(GLOB_RECURSE CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/r-type_client.cpp
)
file(GLOB_RECURSE CLIENT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.hpp
)
add_executable(r-type_client ${CLIENT_SOURCES} ${CLIENT_HEADERS})
target_link_libraries(r-type_client PRIVATE Engine raylib)
target_include_directories(r-type_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine
)
if(WIN32)
    target_link_libraries(r-type_client PRIVATE ws2_32)
endif()
add_custom_command(TARGET r-type_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:r-type_client>/assets
)

# --- Server Executable ("r-type_server") ---
add_executable(r-type_server ${CMAKE_CURRENT_SOURCE_DIR}/r-type_server.cpp)
target_link_libraries(r-type_server PRIVATE Engine raylib)
target_include_directories(r-type_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine
)
if(WIN32)
    target_link_libraries(r-type_server PRIVATE ws2_32)
endif()
