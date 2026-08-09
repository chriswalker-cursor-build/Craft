cmake_minimum_required(VERSION 3.5)

project(craft)

FILE(GLOB SOURCE_FILES src/*.c)

add_executable(
    craft
    ${SOURCE_FILES}
    deps/glew/src/glew.c
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c)

add_definitions(-std=c99 -O3)

add_subdirectory(deps/glfw)
include_directories(deps/glew/include)
include_directories(deps/glfw/include)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/sqlite)
include_directories(deps/tinycthread)

if(MINGW)
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
        "C:/Program Files/CURL/lib" "C:/Program Files (x86)/CURL/lib")
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
        "C:/Program Files/CURL/include" "C:/Program Files (x86)/CURL/include")
endif()

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

if(APPLE)
    target_link_libraries(craft glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
elseif(UNIX)
    target_link_libraries(craft dl glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(MINGW)
    target_link_libraries(craft ws2_32.lib glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

# Headless unit tests (no GLFW/GL runtime).
add_executable(
    test_runner
    tests/test_runner.c
    tests/test_matrix.c
    tests/test_util.c
    tests/test_db.c
    tests/test_client.c
    src/matrix.c
    src/util.c
    src/db.c
    src/map.c
    src/ring.c
    src/sign.c
    src/client.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c)

target_include_directories(test_runner PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps/glew/include
    ${CMAKE_SOURCE_DIR}/deps/glfw/include
    ${CMAKE_SOURCE_DIR}/deps/lodepng
    ${CMAKE_SOURCE_DIR}/deps/sqlite
    ${CMAKE_SOURCE_DIR}/deps/tinycthread)

target_compile_options(test_runner PRIVATE -std=c99 -O0
    -ffunction-sections -fdata-sections)

if(APPLE)
    target_link_options(test_runner PRIVATE -Wl,-dead_strip)
elseif(UNIX)
    target_link_options(test_runner PRIVATE -Wl,--gc-sections)
endif()

if(APPLE)
    target_link_libraries(test_runner m pthread)
elseif(UNIX)
    target_link_libraries(test_runner m dl pthread)
else()
    target_link_libraries(test_runner m)
endif()
