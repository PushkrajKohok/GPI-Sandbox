cmake_minimum_required(VERSION 3.22)
include(${CMAKE_SOURCE_DIR}/cmake/Version.cmake)
project(gpi_sandbox LANGUAGES CXX VERSION ${GPI_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate version header
configure_file(${CMAKE_SOURCE_DIR}/src/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)

# Phase 11: Security hardening options
option(GPI_WITH_ASAN "Enable AddressSanitizer" OFF)
option(GPI_WITH_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(GPI_WITH_FUZZ "Build fuzzers (LLVM libFuzzer required)" OFF)
# Phase 15: Documentation generation
option(GPI_BUILD_DOCS "Build documentation with Doxygen" OFF)

if (GPI_WITH_ASAN OR GPI_WITH_UBSAN)
  if (MSVC)
    message(WARNING "Sanitizers are best on Clang/GCC; MSVC limited")
  else()
    add_compile_options(-fno-omit-frame-pointer)
    add_link_options(-fno-omit-frame-pointer)
    if (GPI_WITH_ASAN) 
      add_compile_options(-fsanitize=address) 
      add_link_options(-fsanitize=address) 
    endif()
    if (GPI_WITH_UBSAN) 
      add_compile_options(-fsanitize=undefined) 
      add_link_options(-fsanitize=undefined) 
    endif()
  endif()
endif()

include(FetchContent)

# ---- SDL2 (2.30+) ----
FetchContent_Declare(
  sdl2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.8
)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sdl2)

# ---- ImGui ----
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90.4
)
set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(imgui)

# ---- App ----
add_executable(gpi_host
  src/main.cpp
  src/metrics.cpp
  src/imgui_layer.cpp
  src/input.h
  src/config.cpp
  src/ui/toast.cpp
  src/runtime/watchdog.cpp
  src/runtime/plugin_loader.cpp
  src/runtime/plugin_runtime.cpp
  src/platform/fs.cpp
  src/services/save_store.cpp
  src/services/log_bus.cpp
  src/services/telemetry.cpp
  src/services/artifacts.cpp
  src/services/replay.cpp
  src/services/metrics_detail.cpp
  src/services/screenshot.cpp
  src/runtime/runner_inproc.cpp
  src/runtime/runner_child.cpp
  src/runtime/child_shm.cpp
  src/runtime/drawlist_shm.cpp
  src/platform/sandbox_win.cpp
  src/platform/sandbox_posix.cpp
  src/platform/seccomp_linux.cpp
  src/ui/log_panel.cpp
  src/ui/draw_prim.cpp
  src/ui/render_drawlist.cpp
  src/ui/font_atlas.cpp
  src/ui/store_panel.cpp
  src/ui/hud_perf.cpp
  src/ui/a11y.cpp
  src/ui/l10n.cpp
  # src/qa/golden.cpp
  # src/qa/diff.cpp
  # src/qa/diff_ssim.cpp
  src/store/plugin_manifest.cpp
  src/platform/proc.cpp
)

# Platform defines
if(APPLE)
  target_compile_definitions(gpi_host PRIVATE GPI_MAC=1)
elseif(WIN32)
  target_compile_definitions(gpi_host PRIVATE GPI_WIN=1)
else()
  target_compile_definitions(gpi_host PRIVATE GPI_LINUX=1)
endif()

# ImGui sources
set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_sources(gpi_host PRIVATE ${IMGUI_SOURCES})

target_include_directories(gpi_host PRIVATE
  include
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(gpi_host PRIVATE SDL2::SDL2)

# OpenGL linking
if(APPLE)
  target_link_libraries(gpi_host PRIVATE "-framework OpenGL")
elseif(WIN32)
  target_link_libraries(gpi_host PRIVATE opengl32)
else()
  target_link_libraries(gpi_host PRIVATE GL)
endif()

# Output directory
set_target_properties(gpi_host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Warnings
if(MSVC)
  target_compile_options(gpi_host PRIVATE /W4 /permissive- /Zc:preprocessor)
else()
  target_compile_options(gpi_host PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Child process app
add_executable(gpi_child
  child/gpi_child_main.cpp
  child/crash_report_win.cpp
  child/crash_report_posix.cpp
  src/runtime/plugin_loader.cpp
  src/runtime/child_shm.cpp
  src/runtime/drawlist_shm.cpp
  src/platform/sandbox_win.cpp
  src/platform/sandbox_posix.cpp
  src/platform/seccomp_linux.cpp
)
target_include_directories(gpi_child PRIVATE include src)
if(MSVC)
  target_compile_options(gpi_child PRIVATE /W4)
else()
  target_compile_options(gpi_child PRIVATE -Wall -Wextra -Wpedantic)
endif()
set_target_properties(gpi_child PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Plugins
add_subdirectory(plugins/template)
add_subdirectory(plugins/pong)
add_subdirectory(plugins/snake)

# Phase 11: Fuzzer target
if (GPI_WITH_FUZZ)
  add_executable(fuzz_gpi fuzz/fuzz_gpi_frame.cpp)
  target_compile_options(fuzz_gpi PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_options(fuzz_gpi PRIVATE -fsanitize=fuzzer,address,undefined)
  target_include_directories(fuzz_gpi PRIVATE include src)
endif()

# Phase 15: Doxygen documentation
if (GPI_BUILD_DOCS)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/api)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(docs
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
  else()
    message(WARNING "Doxygen not found, skipping documentation generation")
  endif()
endif()

# CPack configuration
include(CPack)
set(CPACK_PACKAGE_NAME "gpi-sandbox")
set(CPACK_PACKAGE_VERSION ${GPI_VERSION})
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
