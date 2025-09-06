cmake_minimum_required(VERSION 3.21)
project(MatrixScreensaver VERSION 1.0.0 LANGUAGES CXX)

# Prefer static libraries by default
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

# Ensure CMAKE_MSVC_RUNTIME_LIBRARY is honored (CMake 3.15+)
cmake_policy(SET CMP0091 NEW)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
        src/main.cpp
        src/matrix_renderer.cpp
        src/matrix_screensaver.cpp
)

set(HEADERS
        src/matrix_renderer.hpp
        src/matrix_screensaver.hpp
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
endif ()

# Include submodules
add_subdirectory(third_party/SDL)
add_subdirectory(third_party/SDL_ttf)

# Prefer static SDL targets if available, else fall back
if (TARGET SDL3::SDL3-static)
    set(SDL3_MAIN_TARGET SDL3::SDL3-static)
else ()
    set(SDL3_MAIN_TARGET SDL3::SDL3)
endif ()

if (TARGET SDL3_ttf::SDL3_ttf-static)
    set(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf-static)
else ()
    set(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_TTF_TARGET} ${SDL3_MAIN_TARGET})


# Compiler-specific options
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /utf-8)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# Set output name to .scr for screensaver
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "MatrixScreensaver")

# Automatically copy dependent runtime DLLs only if we built/linked shared
if (BUILD_SHARED_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime DLLs..."
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif ()


# Post-build: rename to .scr
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/MatrixScreensaver.scr"
)
