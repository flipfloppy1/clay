cmake_minimum_required(VERSION 3.11)
project(clay_angle_project C)

include(FetchContent)

set(GLFW_BUILD_WAYLAND ON)
set(GLFW_BUILD_X11 ON)

# Only include glfw if the raylib build isn't already adding it
if(NOT TARGET glfw)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG "3.4"
    )
    FetchContent_MakeAvailable(glfw)
endif()

FetchContent_Declare(
    angle
)

FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG "10.1.0"
)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG "VER-2-13-3"
)


FetchContent_MakeAvailable(harfbuzz freetype)

FetchContent_GetProperties(angle)
if (NOT angle_POPULATED)
    FetchContent_Populate(
        angle
        URL https://github.com/google/angle/archive/refs/heads/main.zip
    )
endif()

FetchContent_GetProperties(glfw)
if (NOT glfw_POPULATED)
    FetchContent_Populate(glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
    )
endif()

add_executable(clay_examples_angle_project main.c)

set_property(TARGET clay_examples_angle_project PROPERTY C_STANDARD 99)

set(CMAKE_C_FLAGS_DEBUG "-Wall -Werror -Wno-error=missing-braces -DCLAY_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3")

target_link_libraries(clay_examples_angle_project PRIVATE glfw m harfbuzz EGL GLESv2 freetype) # dunno why math ('m') is needed here
target_include_directories(clay_examples_angle_project PRIVATE "${angle_SOURCE_DIR}/include" "${harfbuzz_SOURCE_DIR}/src" "${glfw_SOURCE_DIR}/include" "${freetype_SOURCE_DIR}/include")
