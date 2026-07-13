#!/bin/sh

# Function to display usage instructions
usage() {
  echo "Usage: $0 [--debug] [--no-move] [--verbose] [--output-dir=<dir>]"
  echo "            [--install] [--prefix=<dir>] [--destdir=<dir>]"
  echo "  --debug: Enable debug mode (set DEBUG=1)"
  echo "  --no-move: Skip moving the 'tomato' executable to the project root"
  echo "  --verbose: Enable verbose output during the build"
  echo "  --output-dir=<dir>: Specify a custom directory for build output"
  echo "  --install: Build and install the binary and data files"
  echo "  --prefix=<dir>: Installation prefix (default: /usr/local)"
  echo "  --destdir=<dir>: Staging directory for packaged installs"
  exit 1
}

# Check if the number of arguments is greater than 9
if [ $# -gt 9 ]; then
  usage
fi

# Default values
DEBUG=0
NO_MOVE=0
VERBOSE=""
OUTPUT_DIR="."
INSTALL=0
PREFIX="/usr/local"
DESTDIR=""

# Parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    --debug)
      DEBUG=1
      ;;
    --no-move)
      NO_MOVE=1
      ;;
    --verbose)
      VERBOSE=1
      ;;
    --output-dir=*)
      OUTPUT_DIR="${1#*=}"
      ;;
    --install)
      INSTALL=1
      ;;
    --prefix=*)
      PREFIX="${1#*=}"
      ;;
    --destdir=*)
      DESTDIR="${1#*=}"
      ;;
    *)
      usage
      ;;
  esac
  shift
done

# Determine the make target and enter build directory
MAKE_TARGET="clean-all"
cd build

# Check if bear is installed
if command -v bear >/dev/null 2>&1; then
  BEAR_INSTALLED=1
else
  BEAR_INSTALLED=0
fi

# Override DATAPREFIX for install builds so the compiled binary
# embeds the absolute data directory path.
if [ "$INSTALL" -eq 1 ]; then
  DATAPREFIX_OVERRIDE="DATAPREFIX=${DESTDIR}${PREFIX}/share/tomato"
else
  DATAPREFIX_OVERRIDE=""
fi

# Run the make command with the appropriate DATAPREFIX, DEBUG, VERBOSE, and TARGET values
if [ "$DEBUG" -eq 1 ]; then
  if [ "$BEAR_INSTALLED" -eq 1 ]; then
    if [ -n "$VERBOSE" ]; then
      bear -- make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET V=1
    else
      bear -- make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET
    fi
    if [ -f "compile_commands.json" ]; then
      mv compile_commands.json ..
    fi
  else
    echo "Bear not installed. Proceeding without it."
    if [ -n "$VERBOSE" ]; then
      make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET V=1
    else
      make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET
    fi
  fi
else
  if [ -n "$VERBOSE" ]; then
    make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET V=1
  else
    make $DATAPREFIX_OVERRIDE DEBUG=$DEBUG $MAKE_TARGET
  fi
fi

# Move the executable to the specified directory if no-move is not set
if [ $NO_MOVE -eq 0 ]; then
  if [ "$OUTPUT_DIR" = "." ]; then
    mv tomato ..
  else
    mv tomato "$OUTPUT_DIR"
  fi
fi

# Navigate back to the project root
cd ..

# ---------------------------------------------------------------------------
# Install step
# ---------------------------------------------------------------------------
if [ "$INSTALL" -eq 1 ]; then
  # Locate the built binary
  if [ "$NO_MOVE" -eq 1 ]; then
    TOMATO_SRC="build/tomato"
  elif [ "$OUTPUT_DIR" = "." ]; then
    TOMATO_SRC="tomato"
  else
    TOMATO_SRC="$OUTPUT_DIR/tomato"
  fi

  INSTALL_DIR="${DESTDIR}${PREFIX}"

  # Check write permission before attempting installation
  if ! mkdir -p "${INSTALL_DIR}" 2>/dev/null; then
    echo "Error: Permission denied. Cannot write to ${INSTALL_DIR}." >&2
    echo "Try: sudo ${0} --install" >&2
    exit 1
  fi

  echo "Installing to ${INSTALL_DIR}..."

  # Binary
  mkdir -p "${INSTALL_DIR}/bin" || exit 1
  cp "$TOMATO_SRC" "${INSTALL_DIR}/bin/tomato" || exit 1
  chmod 755 "${INSTALL_DIR}/bin/tomato" || exit 1

  # Data files
  DATADIR_INSTALL="${INSTALL_DIR}/share/tomato"
  mkdir -p "$DATADIR_INSTALL/sprites" || exit 1
  mkdir -p "$DATADIR_INSTALL/sounds" || exit 1
  mkdir -p "$DATADIR_INSTALL/icons" || exit 1
  cp resources/sprites/*.asc "$DATADIR_INSTALL/sprites/" || exit 1
  cp resources/sounds/*.mp3 "$DATADIR_INSTALL/sounds/" || exit 1
  cp resources/icons/* "$DATADIR_INSTALL/icons/" || exit 1
  chmod 644 "$DATADIR_INSTALL/sprites/"* || exit 1
  chmod 644 "$DATADIR_INSTALL/sounds/"* || exit 1
  chmod 644 "$DATADIR_INSTALL/icons/"* || exit 1

  # Desktop file (Linux only)
  if [ "$(uname -s)" = "Linux" ]; then
    mkdir -p "${INSTALL_DIR}/share/applications" || exit 1
    cp tomato.desktop "${INSTALL_DIR}/share/applications/tomato.desktop" || exit 1
    sed -i "s|Icon=.*|Icon=${INSTALL_DIR}/share/tomato/icons/tomato.svg|" \
      "${INSTALL_DIR}/share/applications/tomato.desktop" || exit 1
    chmod 644 "${INSTALL_DIR}/share/applications/tomato.desktop" || exit 1
    echo "Desktop entry installed to ${INSTALL_DIR}/share/applications/tomato.desktop"
  fi

  echo "Installation complete."
fi
