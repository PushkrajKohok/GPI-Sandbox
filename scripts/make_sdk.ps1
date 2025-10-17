# GPI SDK Packaging Script (Windows)

$ErrorActionPreference = "Stop"

$SCRIPT_DIR = $PSScriptRoot
$PROJECT_ROOT = Split-Path $SCRIPT_DIR
$SDK_DIR = Join-Path $PROJECT_ROOT "sdk"
$VERSION_FILE = Join-Path $PROJECT_ROOT "cmake" "Version.cmake"
$VERSION = (Get-Content $VERSION_FILE | Select-String 'GPI_VERSION "([^"]+)"').Matches.Groups[1].Value

Write-Host "Creating GPI SDK v$VERSION..."

# Clean and create SDK directory
if (Test-Path $SDK_DIR) {
    Remove-Item -Recurse -Force $SDK_DIR
}
New-Item -ItemType Directory -Path $SDK_DIR | Out-Null

# Copy headers
Write-Host "Copying headers..."
New-Item -ItemType Directory -Path "$SDK_DIR\include\gpi" -Force | Out-Null
Copy-Item "$PROJECT_ROOT\include\gpi\gpi_plugin.h" "$SDK_DIR\include\gpi\"

# Copy template
Write-Host "Copying template..."
Copy-Item -Recurse "$PROJECT_ROOT\sdk\template" "$SDK_DIR\"

# Copy CMake helpers
Write-Host "Copying CMake helpers..."
New-Item -ItemType Directory -Path "$SDK_DIR\cmake" -Force | Out-Null
Copy-Item "$PROJECT_ROOT\cmake\Version.cmake" "$SDK_DIR\cmake\"

# Create SDK CMakeLists.txt
$CMAKE_CONTENT = @'
cmake_minimum_required(VERSION 3.22)
project(gpi_sdk)

# Find SDL2
find_package(SDL2 REQUIRED)

# Include version
include(cmake/Version.cmake)

# Add plugin target
add_library(gpi_plugin_example SHARED
    template/plugin_example.cpp
)

target_include_directories(gpi_plugin_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gpi_plugin_example PRIVATE
    SDL2::SDL2
)

# Set output directory
set_target_properties(gpi_plugin_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
)

# Compile options
target_compile_features(gpi_plugin_example PRIVATE cxx_std_20)
target_compile_options(gpi_plugin_example PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
'@
Set-Content "$SDK_DIR\CMakeLists.txt" $CMAKE_CONTENT

# Create README
$README_CONTENT = @"
# GPI SDK v$VERSION

This is the GPI Sandbox SDK for creating plugins.

## Quick Start

1. Copy the template:
   ``````powershell
   cp -r template my_plugin
   cd my_plugin
   ``````

2. Edit ``plugin_example.cpp`` to implement your plugin

3. Build:
   ``````powershell
   mkdir build; cd build
   cmake ..
   cmake --build .
   ``````

4. Copy the built plugin to your host's plugins directory

## Documentation

- [Quickstart Guide](docs/quickstart.md)
- [Architecture](docs/architecture.md)
- [ABI Reference](docs/abi_reference.md)

## Examples

See the ``template/`` directory for a minimal example.

## Support

- GitHub Issues: https://github.com/your-org/gpi-sandbox/issues
- Documentation: https://github.com/your-org/gpi-sandbox/wiki
"@
Set-Content "$SDK_DIR\README.md" $README_CONTENT

# Create docs directory
New-Item -ItemType Directory -Path "$SDK_DIR\docs" -Force | Out-Null
Copy-Item "$PROJECT_ROOT\docs\quickstart.md" "$SDK_DIR\docs\"
Copy-Item "$PROJECT_ROOT\docs\architecture.md" "$SDK_DIR\docs\"
Copy-Item "$PROJECT_ROOT\docs\abi_reference.md" "$SDK_DIR\docs\"

# Create archive
Write-Host "Creating archive..."
$ARCHIVE = "gpi-sdk-v$VERSION.zip"
if (Test-Path $ARCHIVE) {
    Remove-Item $ARCHIVE
}
Compress-Archive -Path $SDK_DIR -DestinationPath $ARCHIVE

Write-Host "SDK created: $ARCHIVE"
Write-Host "SDK directory: $SDK_DIR"

