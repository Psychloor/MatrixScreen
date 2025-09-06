cmake_minimum_required(VERSION 3.21)
project(MatrixScreensaver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Source files
set(SOURCES
    src/main.cpp
)

set(HEADERS

)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

find_package(SDL3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W3 /permissive- /Zc:__cplusplus /utf-8
    )
endif()

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
    COMMAND ${CMAKE_COMMAND} -E copy 
    "$<TARGET_FILE:${PROJECT_NAME}>" 
    "${CMAKE_BINARY_DIR}/MatrixScreensaver.scr"
)