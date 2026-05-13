#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT_DIR"

mkdir -p docs/output

echo "[DOCS] Generating documentation..."

doxygen docs/Doxyfile -w html

echo "[DOCS] Done."
echo "[DOCS] Open: docs/output/html/index.html"
