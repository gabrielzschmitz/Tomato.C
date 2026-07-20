#!/bin/sh
#/**
# * @file run_tests.sh
# * @brief Tomato.C Test Runner.
# *
# * Builds and runs all unit and integration test suites, collects
# * results, and prints a formatted summary. Supports filtering,
# * coverage/ASan/UBSan builds, list-only mode, and color output.
# *
# * Usage: ./run_tests.sh [options]
# *
# * Options:
# *   --unit|-u           Run only unit tests
# *   --integration|-i    Run only integration tests
# *   --verbose|-v        Show full output per test
# *   --quiet|-q          Show only summary
# *   --filter <pattern>  Run only suites matching pattern
# *   --coverage          Build with coverage flags
# *   --asan              Build with AddressSanitizer
# *   --ubsan             Build with UndefinedBehaviorSanitizer
# *   --list              List discovered suites (no run)
# *   --clean             Remove build artifacts before build"
# *   --no-color          Disable ANSI colors
# *   --help|-h           Show this help
# *
# * Exit codes: 0 = all pass, nonzero = failure
# */

set -u

#/**
# * ---------------------------------------------------------------------------
# * Global state
# * ---------------------------------------------------------------------------
# */

RUN_UNIT=1
RUN_INTEGRATION=1
VERBOSE=0
QUIET=0
FILTER=""
COVERAGE=0
ASAN=0
UBSAN=0
LIST_ONLY=0
CLEAN=0
SKIP_BUILD=0

RED=''; GREEN=''; YELLOW=''; CYAN=''; BOLD=''; RESET=''

SCRIPT_DIR=""
PROJECT_DIR=""
TESTS_DIR=""

TOTAL_ASSERTIONS=0
UNIT_TOTAL=0; UNIT_PASS=0; UNIT_FAIL=0; UNIT_SKIP=0
INT_TOTAL=0; INT_PASS=0; INT_FAIL=0; INT_SKIP=0
FAILED_NAMES=""

#/**
# * ---------------------------------------------------------------------------
# * Color support
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Initialise ANSI colour variables if output is a TTY and
# *        neither NO_COLOR nor CI are set.
# */
init_colors() {
  if [ -t 1 ] && [ "${NO_COLOR:-}" = "" ] && [ "${CI:-}" = "" ]; then
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
# * @brief Parse command-line arguments and set corresponding global flags.
# * @param $@  Arguments from the command line.
# * @return  Exit 0 on success, 1 on unknown flag.
# */
parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --unit|-u) RUN_INTEGRATION=0 ;;
      --integration|-i) RUN_UNIT=0 ;;
      --verbose|-v) VERBOSE=1 ;;
      --quiet|-q) QUIET=1 ;;
      --filter) FILTER="$2"; shift ;;
      --coverage) COVERAGE=1 ;;
      --asan) ASAN=1 ;;
      --ubsan) UBSAN=1 ;;
      --list) LIST_ONLY=1 ;;
      --clean) CLEAN=1 ;;
      --no-color)
        RED=''; GREEN=''; YELLOW=''; CYAN=''; BOLD=''; RESET='' ;;
      --help|-h)
        echo "Tomato.C Test Runner"
        echo ""
        echo "Usage: $0 [options]"
        echo ""
        echo "  --unit|-u           Run only unit tests"
        echo "  --integration|-i    Run only integration tests"
        echo "  --verbose|-v        Show full output per test"
        echo "  --quiet|-q          Show only summary"
        echo "  --filter <pattern>  Run only suites matching pattern"
        echo "  --coverage          Build with coverage flags"
        echo "  --asan              Build with AddressSanitizer"
        echo "  --ubsan             Build with UndefinedBehaviorSanitizer"
        echo "  --list              List discovered suites (no run)"
        echo "  --clean             Remove build artifacts before build"
        echo "  --no-color          Disable ANSI colors"
        echo "  --help|-h           Show this help"
        echo ""
        echo "Exit codes: 0 = all pass, nonzero = failure"
        exit 0
        ;;
      *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
  done
}

#/**
# * ---------------------------------------------------------------------------
# * Path setup
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Resolve SCRIPT_DIR, PROJECT_DIR, and TESTS_DIR relative to
# *        the script location.
# */
setup_paths() {
  SCRIPT_DIR=$(dirname "$0")
  cd "$SCRIPT_DIR" || exit 1
  PROJECT_DIR=$(pwd)
  TESTS_DIR="$PROJECT_DIR/tests"
}

#/**
# * ---------------------------------------------------------------------------
# * Build
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Remove all build artifacts via make clean.
# */
clean_artifacts() {
  echo " ${BOLD}Cleaning build artifacts...${RESET}"
  local clean_output
  clean_output=$(/usr/bin/make -C "$TESTS_DIR" clean 2>&1)

  local clean_exit=$?
  if [ "$clean_exit" -ne 0 ]; then
    echo "$clean_output" | sed 's/^/  /'
    echo "  ${RED}[ERR]${RESET}  Clean failed${RESET}"
    echo ""
    exit 1
  fi
  echo "  ${GREEN}[OK]${RESET}  Clean done"
  echo ""
}

#/**
# * @brief Build all test binaries with make.
# *
# * Honour COVERAGE, ASAN, and UBSAN flags.  Exits the script if the
# * build fails.
# */
build_tests() {
  local build_flags=""
  [ "$COVERAGE" = "1" ] && build_flags="$build_flags COVERAGE=1"
  [ "$ASAN" = "1" ] && build_flags="$build_flags ASAN=1"
  [ "$UBSAN" = "1" ] && build_flags="$build_flags UBSAN=1"

  echo " ${BOLD}Building tests...${RESET}"

  local build_output
  build_output=$(/usr/bin/make -B -C "$TESTS_DIR" build $build_flags 2>&1)

  local build_exit=$?
  if [ "$build_exit" -ne 0 ]; then
    echo "  ${RED}[ERR]${RESET}  Build failed${RESET}"
    echo "$build_output" | sed 's/^/   /'
    echo ""
    print_summary 0 0 0 0 0 0 0 0 0 0 ""
    exit 1
  fi
  echo "  ${GREEN}[OK]${RESET}  Build successful"
}

#/**
# * ---------------------------------------------------------------------------
# * Test discovery
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Print all discovered test binaries (list mode) and exit.
# */
list_tests() {
  local unit_bins int_bins b
  unit_bins=$(find "$TESTS_DIR/unit" -maxdepth 1 -type f -executable -name 'test_*' 2>/dev/null | sort)
  int_bins=$(find "$TESTS_DIR/integration" -maxdepth 1 -type f -executable -name 'test_*' 2>/dev/null | sort)
  echo " ${BOLD}Discovered test suites:${RESET}"
  echo ""
  for b in $unit_bins; do
    echo "   unit/$(basename "$b")"
  done
  for b in $int_bins; do
    echo "   integration/$(basename "$b")"
  done
  echo ""
  exit 0
}

#/**
# * @brief Find executable test binaries, applying the optional FILTER.
# *
# * @param $1  1 to discover unit tests, 0 to skip.
# * @param $2  1 to discover integration tests, 0 to skip.
# * @return  Prints space-separated paths to stdout (caller captures).
# */
discover_tests() {
  local run_unit=$1 run_int=$2
  local result=""
  if [ "$run_unit" = "1" ]; then
    result="$result $(find "$TESTS_DIR/unit" -maxdepth 1 -type f -executable 2>/dev/null | sort)"
  fi
  if [ "$run_int" = "1" ]; then
    result="$result $(find "$TESTS_DIR/integration" -maxdepth 1 -type f -executable 2>/dev/null | sort)"
  fi

  if [ -n "$FILTER" ]; then
    local filtered=""
    for b in $result; do
      name=$(basename "$b")
      case "$name" in
        *"$FILTER"*) filtered="$filtered $b" ;;
      esac
    done
    result="$filtered"
  fi

  echo "$result"
}

#/**
# * ---------------------------------------------------------------------------
# * Test execution helpers
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Print the top-level header banner.
# */
print_header() {
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo "${BOLD}  Tomato.C Test Suite${RESET}"
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo ""
}

#/**
# * @brief Print the final summary block after all suites complete.
# *
# * @param $1  Unit suites total
# * @param $2  Unit suites passed
# * @param $3  Unit suites failed
# * @param $4  Unit suites skipped
# * @param $5  Integration suites total
# * @param $6  Integration suites passed
# * @param $7  Integration suites failed
# * @param $8  Integration suites skipped
# * @param $9  Total assertions
# * @param $10 Elapsed time (seconds)
# * @param $*  Space-separated list of failed suite names
# */
print_summary() {
  local unit_total=$1 unit_pass=$2 unit_fail=$3 unit_skip=$4
  local int_total=$5 int_pass=$6 int_fail=$7 int_skip=$8
  local assertions=$9 total_time=$10
  shift 10
  local failed_names="$*"

  local total=$((unit_pass + int_pass))
  local failed=$((unit_fail + int_fail))
  local skipped=$((unit_skip + int_skip))

  echo ""
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo " ${BOLD}Summary${RESET}"
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo ""
  echo "  Unit suites:        $unit_total"
  echo "  Integration suites: $int_total"
  echo "  ${BOLD}Total:              $((unit_total + int_total))${RESET}"
  echo ""
  if [ "$total" -gt 0 ]; then
    echo "  ${GREEN}Passed:${RESET}             $total"
  fi
  if [ "$failed" -gt 0 ]; then
    echo "  ${RED}Failed:${RESET}             $failed"
  fi
  if [ "$skipped" -gt 0 ]; then
    echo "  ${YELLOW}Skipped:${RESET}            $skipped"
  fi
  echo "  Assertions:         $assertions"
  echo "  Time:               ${total_time}s"

  if [ "$failed" -gt 0 ]; then
    echo "  ${RED}Failed suites:${RESET}"
    for f in $failed_names; do
      echo "  ${RED}[ERR]${RESET}  $f"
    done
    echo ""
  fi
}

#/**
# * @brief Run a single test binary and collect pass/fail/skip counts.
# *
# * @param $1  Path to the test binary.
# * @param $2  Display name for the suite.
# * @param $3  Category ("unit" or "integration").
# *
# * Updates the global accumulator variables (UNIT_*, INT_*,
# * TOTAL_ASSERTIONS, FAILED_NAMES) as a side effect.
# */
run_test_binary() {
  local binary="$1"
  local name="$2"
  local category="$3"
  local start_time end_time elapsed
  local output exit_code pass_count fail_count skip_count assert_count result

  start_time=$(date +%s 2>/dev/null || echo 0)

  output=$("$binary" 2>&1)
  exit_code=$?

  end_time=$(date +%s 2>/dev/null || echo 0)
  elapsed=$((end_time - start_time))
  [ "$elapsed" -lt 0 ] && elapsed=0

  pass_count=$(echo "$output" | grep -o '[0-9]\+ passed' | head -1 | cut -d' ' -f1)
  fail_count=$(echo "$output" | grep -o '[0-9]\+ failed' | head -1 | cut -d' ' -f1)
  skip_count=$(echo "$output" | grep -o '[0-9]\+ skipped' | head -1 | cut -d' ' -f1)
  assert_count=$(echo "$output" | grep -o '[0-9]\+ assertions' | head -1 | cut -d' ' -f1)

  [ -z "$pass_count" ] && pass_count=0
  [ -z "$fail_count" ] && fail_count=0
  [ -z "$skip_count" ] && skip_count=0
  [ -z "$assert_count" ] && assert_count=0

  if [ "$exit_code" -eq 0 ]; then
    result="PASS"
  elif [ "$exit_code" -eq 2 ] || [ "$exit_code" -ge 126 ]; then
    result="CRASH"
  else
    result="FAIL"
  fi

  local padded_name
  padded_name=$(printf "%-44s" "$name")
  if [ "$result" = "PASS" ]; then
    echo "  ${GREEN}[OK]${RESET}  ${BOLD}$padded_name${GREEN}PASS${RESET}  (${pass_count})"
  elif [ "$result" = "FAIL" ]; then
    echo "  ${RED}[ERR]${RESET}  ${BOLD}$padded_name${RED}FAIL${RESET}  (${pass_count})"
  elif [ "$result" = "CRASH" ]; then
    echo "  ${RED}[ERR]${RESET}  ${BOLD}$padded_name${RED}CRASH${RESET} (exit code $exit_code)"
  else
    echo "  ${YELLOW}?${RESET}  $padded_name${YELLOW}UNKNOWN${RESET}"
  fi

  if [ "$VERBOSE" = "1" ] && [ "$exit_code" -ne 0 ]; then
    echo "$output" | sed 's/^/    /'
  fi

  case "$category" in
    unit) _pfx=UNIT ;;
    integration) _pfx=INT ;;
    *) _pfx=$(echo "$category" | tr '[:lower:]' '[:upper:]') ;;
  esac
  eval "${_pfx}_TOTAL=\$((\${${_pfx}_TOTAL:-0} + 1))"
  if [ "$result" = "PASS" ]; then
    eval "${_pfx}_PASS=\$((\${${_pfx}_PASS:-0} + 1))"
  elif [ "$result" = "FAIL" ] || [ "$result" = "CRASH" ]; then
    eval "${_pfx}_FAIL=\$((\${${_pfx}_FAIL:-0} + 1))"
    FAILED_NAMES="$FAILED_NAMES $name"
  else
    eval "${_pfx}_SKIP=\$((\${${_pfx}_SKIP:-0} + 1))"
  fi
  TOTAL_ASSERTIONS=$((TOTAL_ASSERTIONS + assert_count))

}

#/**
# * @brief Run a set of test binaries for a given category.
# *
# * @param $1  Space-separated list of binary paths.
# * @param $2  Category label ("unit" or "integration").
# */
run_category() {
  local binaries="$1"
  local category="$2"
  local label
  case "$category" in
    unit) label="Unit Tests" ;;
    integration) label="Integration Tests" ;;
    *) label="$category" ;;
  esac

  if [ "$QUIET" != "1" ]; then
    echo ""
    echo " ${BOLD}Running ${label}...${RESET}"
  fi

  for binary in $binaries; do
    [ ! -x "$binary" ] && continue
    local name
    name=$(basename "$binary" | sed 's/^test_//')
    run_test_binary "$binary" "$name" "$category"
  done
}

#/**
# * ---------------------------------------------------------------------------
# * Main
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Orchestrate the full test workflow.
# *
# * Initialises colours, parses arguments, resolves paths, builds the
# * test binaries, lists them if --list was given, otherwise executes
# * each suite and prints results.
# */
main() {
  init_colors
  parse_args "$@"
  setup_paths

  print_header

  if [ "$CLEAN" = "1" ]; then
    clean_artifacts
  fi

  build_tests

  if [ "$LIST_ONLY" = "1" ]; then
    list_tests
  fi

  local all_bins
  all_bins=$(discover_tests "$RUN_UNIT" "$RUN_INTEGRATION")

  TOTAL_ASSERTIONS=0
  UNIT_TOTAL=0; UNIT_PASS=0; UNIT_FAIL=0; UNIT_SKIP=0
  INT_TOTAL=0; INT_PASS=0; INT_FAIL=0; INT_SKIP=0
  FAILED_NAMES=""

  local global_start global_end global_elapsed
  global_start=$(date +%s 2>/dev/null || echo 0)

  local unit_bins=""
  local int_bins=""
  for b in $all_bins; do
    case "$b" in
      *"/unit/"*) unit_bins="$unit_bins $b" ;;
      *"/integration/"*) int_bins="$int_bins $b" ;;
    esac
  done

  if [ "$RUN_UNIT" = "1" ] && [ -n "$unit_bins" ]; then
    run_category "$unit_bins" "unit"
  fi

  if [ "$RUN_INTEGRATION" = "1" ] && [ -n "$int_bins" ]; then
    run_category "$int_bins" "integration"
  fi

  global_end=$(date +%s 2>/dev/null || echo 0)
  global_elapsed=$((global_end - global_start))
  [ "$global_elapsed" -lt 0 ] && global_elapsed=0

  print_summary \
    "$UNIT_TOTAL" "$UNIT_PASS" "$UNIT_FAIL" "$UNIT_SKIP" \
    "$INT_TOTAL" "$INT_PASS" "$INT_FAIL" "$INT_SKIP" \
    "$TOTAL_ASSERTIONS" "$global_elapsed" \
    "$FAILED_NAMES"

  if [ "$((UNIT_FAIL + INT_FAIL))" -gt 0 ]; then
    exit 1
  fi
  exit 0
}

main "$@"
