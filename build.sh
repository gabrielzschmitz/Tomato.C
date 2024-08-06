#!/bin/sh

# Function to display usage instructions
usage() {
  echo "Usage: $0 [debug] [no-move] [no-clean] [verbose] [target=<target_name>] [output-dir=<dir>] [clean]"
  echo "  debug: Enable debug mode (set DEBUG=1)"
  echo "  no-move: Skip moving the 'tomato' executable to the project root"
  echo "  verbose: Enable verbose output during the build"
  echo "  output-dir=<dir>: Specify a custom directory for build output"
  exit 1
}

# Check if the number of arguments is greater than 7
if [ $# -gt 7 ]; then
  usage
fi

# Default values
DEBUG=0
NO_MOVE=0
VERBOSE=""
OUTPUT_DIR="."

# Parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    debug)
      DEBUG=1
      ;;
    no-move)
      NO_MOVE=1
      ;;
    verbose)
      VERBOSE=1
      ;;
    output-dir=*)
      OUTPUT_DIR="${1#*=}"
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

# Run the make command with the appropriate DEBUG, VERBOSE, and TARGET values
if [ -n "$VERBOSE" ]; then
  make DEBUG=$DEBUG $MAKE_TARGET V=1
else
  make DEBUG=$DEBUG $MAKE_TARGET
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
