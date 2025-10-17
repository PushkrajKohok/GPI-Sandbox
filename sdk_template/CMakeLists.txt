cmake_minimum_required(VERSION 3.22)
project(gpi_plugin_example)

# Find SDL2
find_package(SDL2 REQUIRED)

# Add plugin target
add_library(gpi_plugin_example SHARED
    plugin_example.cpp
)

target_include_directories(gpi_plugin_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gpi_plugin_example PRIVATE
    SDL2::SDL2
)

# Set output directory
set_target_properties(gpi_plugin_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# Compile options
target_compile_features(gpi_plugin_example PRIVATE cxx_std_20)
target_compile_options(gpi_plugin_example PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
