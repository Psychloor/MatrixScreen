cmake_minimum_required(VERSION 3.21)
project(MatrixScreensaver VERSION 1.0.0 LANGUAGES CXX)

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

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf SDL3::SDL3)


# Compiler-specific options
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# Set output name to .scr for screensaver
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "MatrixScreensaver")

# Automatically copy dependent runtime DLLs (CMake 3.21+)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime DLLs..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
)


# Post-build: rename to .scr
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/MatrixScreensaver.scr"
)