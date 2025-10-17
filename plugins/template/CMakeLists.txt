cmake_minimum_required(VERSION 3.22)
project(gpi_plugin_template LANGUAGES C)

add_library(gpi_plugin_template SHARED plugin_template.cpp)

if(MSVC)
  target_compile_options(gpi_plugin_template PRIVATE /W4)
else()
  target_compile_options(gpi_plugin_template PRIVATE -Wall -Wextra -Wpedantic -fvisibility=hidden)
endif()

target_include_directories(gpi_plugin_template PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(gpi_plugin_template PROPERTIES
  OUTPUT_NAME "plugin_template"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
)
