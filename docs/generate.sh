#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT_DIR"

VERSION="$(cat VERSION)"
if [ -z "$VERSION" ]; then
  echo "[DOCS] VERSION is empty" >&2
  exit 1
fi

DOXYFILE="docs/Doxyfile"
DOXYFILE_TMP="$(mktemp)"
awk -v version="$VERSION" '
  /^PROJECT_NUMBER[[:space:]]*=/ { print "PROJECT_NUMBER         = " version; next }
  { print }
' "$DOXYFILE" > "$DOXYFILE_TMP"
cat "$DOXYFILE_TMP" > "$DOXYFILE"
rm -f "$DOXYFILE_TMP"

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
