cmake_minimum_required(VERSION 3.22)
project(gpi_plugin_snake LANGUAGES CXX)
add_library(plugin_snake SHARED snake.cpp)
target_include_directories(plugin_snake PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(plugin_snake PROPERTIES
  OUTPUT_NAME "plugin_snake"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
)
