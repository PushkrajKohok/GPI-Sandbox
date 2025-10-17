cmake_minimum_required(VERSION 3.22)
project(gpi_plugin_pong LANGUAGES CXX)
add_library(plugin_pong SHARED pong.cpp)
target_include_directories(plugin_pong PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(plugin_pong PROPERTIES
  OUTPUT_NAME "plugin_pong"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/bin"
)
