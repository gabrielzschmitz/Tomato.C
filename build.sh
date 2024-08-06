#!/bin/sh

# Function to display usage instructions
usage() {
  echo "Usage: $0 [debug] [no-move] [no-clean]"
  echo "  debug: Enable debug mode (set DEBUG=1)"
  echo "  no-move: Skip moving the 'tomato' executable to the project root"
  echo "  no-clean: Run make without the 'clean-all' target"
  exit 1
}

# Check if the number of arguments is greater than 3
if [ $# -gt 3 ]; then
  usage
fi

# Default values
DEBUG=0
NO_MOVE=0
NO_CLEAN=0

# Parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    debug)
      DEBUG=1
      ;;
    no-move)
      NO_MOVE=1
      ;;
    no-clean)
      NO_CLEAN=1
      ;;
    *)
      usage
      ;;
  esac
  shift
done

# Navigate to the build directory
cd build

# Determine the make target
MAKE_TARGET="clean-all"
if [ $NO_CLEAN -eq 1 ]; then
  MAKE_TARGET=""
fi

# Run the make command with the appropriate DEBUG value
if [ -z "$MAKE_TARGET" ]; then
  make DEBUG=$DEBUG
else
  make DEBUG=$DEBUG $MAKE_TARGET
fi

# Move the executable back to the project root if no-move is not set
if [ $NO_MOVE -eq 0 ]; then
  mv tomato ..
fi

# Navigate back to the project root
cd ..
