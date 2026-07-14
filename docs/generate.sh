#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT_DIR"

mkdir -p docs/output

echo "[DOCS] Generating documentation..."

doxygen docs/Doxyfile

echo "[DOCS] Copying resources to output..."
mkdir -p docs/output/html/resources/icons
cp resources/demo.gif docs/output/html/resources/
cp resources/docs.png docs/output/html/resources/
cp resources/icons/tomato.svg docs/output/html/resources/icons/

echo "[DOCS] Done."
echo "[DOCS] Open: docs/output/html/index.html"
