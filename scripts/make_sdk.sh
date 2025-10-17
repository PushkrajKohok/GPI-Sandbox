#!/bin/bash
# GPI SDK Packaging Script (macOS/Linux)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SDK_DIR="$PROJECT_ROOT/sdk"
OUT_DIR="$PROJECT_ROOT/out"
VERSION="1.0.0"

echo "Creating GPI SDK v$VERSION..."

# Clean and create SDK directory
rm -rf "$SDK_DIR"
mkdir -p "$SDK_DIR"

# Copy headers
echo "Copying headers..."
mkdir -p "$SDK_DIR/include/gpi"
cp "$PROJECT_ROOT/include/gpi/gpi_plugin.h" "$SDK_DIR/include/gpi/"

# Copy template
echo "Copying template..."
cp -r "$PROJECT_ROOT/sdk_template" "$SDK_DIR/"

# Copy CMake helpers
echo "Copying CMake helpers..."
mkdir -p "$SDK_DIR/cmake"
cp "$PROJECT_ROOT/cmake/Version.cmake" "$SDK_DIR/cmake/"

# Create SDK CMakeLists.txt
cat > "$SDK_DIR/CMakeLists.txt" << 'INNER_EOF'
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
INNER_EOF

# Create README
cat > "$SDK_DIR/README.md" << INNER_EOF
# GPI SDK v$VERSION

This is the GPI Sandbox SDK for creating plugins.

## Quick Start

1. Copy the template:
   \`\`\`bash
   cp -r template my_plugin
   cd my_plugin
   \`\`\`

2. Edit \`plugin_example.cpp\` to implement your plugin

3. Build:
   \`\`\`bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   \`\`\`

4. Copy the built plugin to your host's plugins directory

## Documentation

- [Quickstart Guide](docs/quickstart.md)
- [Architecture](docs/architecture.md)
- [ABI Reference](docs/abi_reference.md)

## Examples

See the \`template/\` directory for a minimal example.

## Support

- GitHub Issues: https://github.com/your-org/gpi-sandbox/issues
- Documentation: https://github.com/your-org/gpi-sandbox/wiki
INNER_EOF

# Create docs directory
mkdir -p "$SDK_DIR/docs"
cp "$PROJECT_ROOT/docs/quickstart.md" "$SDK_DIR/docs/"
cp "$PROJECT_ROOT/docs/architecture.md" "$SDK_DIR/docs/"
cp "$PROJECT_ROOT/docs/abi_reference.md" "$SDK_DIR/docs/"

# Create archive
echo "Creating archive..."
cd "$PROJECT_ROOT"
tar -czf "gpi-sdk-v$VERSION.tar.gz" -C "$(dirname "$SDK_DIR")" "$(basename "$SDK_DIR")"

echo "SDK created: gpi-sdk-v$VERSION.tar.gz"
echo "SDK directory: $SDK_DIR"
