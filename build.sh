#!/bin/sh
#/**
# * @file build.sh
# * @brief Tomato.C Build Script.
# *
# * Compiles the Tomato.C Pomodoro timer from source using GNU Make.
# * Optionally generates compile_commands.json via bear for LSP support.
# *
# * Usage: ./build.sh [--debug] [--no-move] [--verbose] [--output-dir=<dir>]
# *
# * Options:
# *   --debug              Enable debug mode (DEBUG=1)
# *   --no-move            Keep the binary inside build/
# *   --verbose            Verbose make output
# *   --output-dir=<dir>   Custom output directory for the binary
# *   --help|-h            Show this help
# *
# * Environment:
# *   DATAPREFIX           Override embedded data directory path (set by install.sh)
# *
# * Exit codes: 0 = success, nonzero = failure
# */

set -u

#/**
# * ---------------------------------------------------------------------------
# * Global state
# * ---------------------------------------------------------------------------
# */

DEBUG=0
NO_MOVE=0
VERBOSE=""
OUTPUT_DIR="."

#/**
# * ---------------------------------------------------------------------------
# * Color support
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Initialise ANSI colour variables if output is a TTY and
# *        NO_COLOR is not set.
# */
init_colors() {
  RED=''; GREEN=''; YELLOW=''; MAGENTA=''; CYAN=''; BOLD=''; RESET=''
  if [ -t 1 ] && [ "${NO_COLOR:-}" = "" ]; then
    if command -v tput >/dev/null 2>&1 && tput colors 2>/dev/null | grep -q '[0-9]'; then
      RED=$(tput setaf 1)
      GREEN=$(tput setaf 2)
      YELLOW=$(tput setaf 3)
      MAGENTA=$(tput setaf 5)
      CYAN=$(tput setaf 6)
      BOLD=$(tput bold)
      RESET=$(tput sgr0)
    fi
  fi
}

#/**
# * ---------------------------------------------------------------------------
# * Argument parsing
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Print usage information and exit.
# */
usage() {
  echo ""
  echo "${BOLD}Tomato.C Build Script${RESET}"
  echo ""
  echo "Usage: $0 [--debug] [--no-move] [--verbose] [--output-dir=<dir>]"
  echo ""
  echo "  --debug              Enable debug mode (DEBUG=1)"
  echo "  --no-move            Keep the binary inside build/"
  echo "  --verbose            Verbose make output"
  echo "  --output-dir=<dir>   Custom output directory for the binary"
  echo "  --help|-h            Show this help"
  echo ""
  echo "Environment:"
  echo "  DATAPREFIX           Override embedded data directory path"
  echo ""
  exit 0
}

#/**
# * @brief Parse command-line arguments into global variables.
# * @param $@  Arguments from the command line.
# */
parse_args() {
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
      --help|-h)
        usage
        ;;
      *)
        echo "${RED} Unknown option: $1${RESET}" >&2
        echo " Try $0 --help for available options." >&2
        exit 1
        ;;
    esac
    shift
  done

  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo " ${BOLD}Tomato.C Build${RESET}"
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo ""
  echo " ${BOLD}Configuration:${RESET}"
  echo "   Debug     : ${DEBUG}"
  echo "   No-move   : ${NO_MOVE}"
  echo "   Verbose   : ${VERBOSE:-0}"
  echo "   Output-dir: ${OUTPUT_DIR}"
  echo "   DATAPREFIX: ${DATAPREFIX:-"(default: ./resources)"}"
  echo ""
}

#/**
# * ---------------------------------------------------------------------------
# * Build
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Compile the project with GNU Make.
# *
# * Changes into build/, invokes make with the selected flags, and
# * optionally generates compile_commands.json when bear is available.
# * Moves the resulting binary to the project root (or OUTPUT_DIR)
# * unless --no-move was requested.
# */
build() {
  local make_target="clean-all"

  echo " ${CYAN}[MAKE]${RESET} Entering build directory..."

  if ! cd build; then
    echo " ${RED}[ERR]${RESET}  build/ directory not found" >&2
    exit 1
  fi

  # Detect bear
  local bear_prefix=""
  if command -v bear >/dev/null 2>&1; then
    bear_prefix="bear --"
    echo " ${GREEN}[OK]${RESET}  bear detected — compile_commands.json will be generated"
  fi

  # Build command
  local make_cmd="$bear_prefix make"
  [ "$DEBUG" -eq 1 ] && make_cmd="$make_cmd DEBUG=$DEBUG"
  [ -n "$VERBOSE" ] && make_cmd="$make_cmd V=1"
  [ -n "${DATAPREFIX:-}" ] && make_cmd="$make_cmd DATAPREFIX=$DATAPREFIX"

  echo " ${CYAN}[MAKE]${RESET} Running: ${BOLD}$make_cmd $make_target${RESET}"
  echo ""

  if [ -n "$bear_prefix" ]; then
    $make_cmd $make_target
    if [ -f "compile_commands.json" ]; then
      mv compile_commands.json ..
      echo " ${GREEN}[OK]${RESET}  compile_commands.json moved to project root"
    fi
  else
    $make_cmd $make_target
  fi

  local build_exit=$?
  if [ "$build_exit" -ne 0 ]; then
    echo ""
    echo " ${RED}[ERR]${RESET}  Build failed (exit code $build_exit)" >&2
    cd ..
    exit 1
  fi

  # Move the executable
  if [ "$NO_MOVE" -eq 0 ]; then
    if [ "$OUTPUT_DIR" = "." ]; then
      mv tomato ..
      echo " ${GREEN}[OK]${RESET}  Binary moved to project root"
    else
      mv tomato "$OUTPUT_DIR"
      echo " ${GREEN}[OK]${RESET}  Binary moved to ${OUTPUT_DIR}"
    fi
  else
    echo " ${YELLOW}[WARN]${RESET}  Binary left in build/ (--no-move)"
  fi

  cd ..

  echo ""
  echo " ${GREEN}${BOLD}Build complete.${RESET}  Run ${BOLD}./tomato${RESET} to start."
}

#/**
# * ---------------------------------------------------------------------------
# * Main
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Main entry point.
# * @param $@  Command-line arguments forwarded from the shell.
# */
main() {
  init_colors
  parse_args "$@"
  build
}

main "$@"
