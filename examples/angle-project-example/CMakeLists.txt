cmake_minimum_required(VERSION 3.11)
project(clay_angle_project)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG "3.4"
)

FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG "10.1.0"
)

FetchContent_Declare(
    angle
)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG "VER-2-13-3"
)

FetchContent_MakeAvailable(glfw harfbuzz freetype)
FetchContent_GetProperties(angle)

if (NOT angle_POPULATED) # Don't build angle, only grab the repo for the includes
    FetchContent_Populate(angle
        URL https://github.com/google/angle/archive/refs/heads/main.zip
    )
endif()

add_executable(clay-angle main.c)

set_property(TARGET clay-angle PROPERTY C_STANDARD 99)

target_link_libraries(clay-angle PUBLIC glfw harfbuzz EGL GLESv2 freetype)
target_include_directories(clay-angle PUBLIC "${angle_SOURCE_DIR}/include" "${harfbuzz_SOURCE_DIR}/src" "${glfw_SOURCE_DIR}/include" "${freetype_SOURCE_DIR}/include")
target_compile_definitions(clay-angle PUBLIC GLFW_INCLUDE_NONE)
