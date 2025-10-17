#!/usr/bin/env bash
set -euo pipefail
PRESET="${1:-release}"
REPO="$(cd "$(dirname "$0")/.."; pwd)"
OUT="$REPO/out/$PRESET"
DIST="$REPO/dist/linux"
B="$DIST/gpi-sandbox-$PRESET-linux"

mkdir -p "$B/plugins/bin" "$B/config"
cp "$OUT/gpi_host" "$B/"
cp "$OUT/gpi_child" "$B/"
cp -r "$OUT/plugins/bin/"* "$B/plugins/bin/" 2>/dev/null || true
cp "$REPO/config/"*.toml "$B/config/"
cp "$REPO/scripts/run_demo.sh" "$B/"
cp "$REPO/README.md" "$B/"

(cd "$DIST" && tar czf "gpi-sandbox-$PRESET-linux.tar.gz" "$(basename "$B")")
echo "Packaged: $DIST/gpi-sandbox-$PRESET-linux.tar.gz"
