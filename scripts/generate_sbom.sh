#!/bin/bash
# Generate Software Bill of Materials (SBOM)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION=$(grep "GPI_VERSION" "$PROJECT_ROOT/cmake/Version.cmake" | cut -d'"' -f2)
OUT_FILE="$PROJECT_ROOT/SBOM.json"

echo "Generating SBOM for GPI Sandbox v$VERSION..."

# Create SBOM in SPDX-like JSON format
cat > "$OUT_FILE" << EOF
{
  "bomFormat": "CycloneDX",
  "specVersion": "1.4",
  "version": 1,
  "metadata": {
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "component": {
      "type": "application",
      "name": "GPI Sandbox",
      "version": "$VERSION",
      "description": "A Modular Game Platform Host in Modern C++"
    }
  },
  "components": [
    {
      "type": "library",
      "name": "SDL2",
      "version": "2.30.8",
      "licenses": [{"license": {"id": "Zlib"}}]
    },
    {
      "type": "library",
      "name": "Dear ImGui",
      "version": "1.90.4",
      "licenses": [{"license": {"id": "MIT"}}]
    },
    {
      "type": "library",
      "name": "stb_image",
      "version": "2.28",
      "licenses": [{"license": {"id": "MIT OR Unlicense"}}]
    },
    {
      "type": "library",
      "name": "stb_image_write",
      "version": "1.16",
      "licenses": [{"license": {"id": "MIT OR Unlicense"}}]
    }
  ]
}
EOF

echo "SBOM generated: $OUT_FILE"

