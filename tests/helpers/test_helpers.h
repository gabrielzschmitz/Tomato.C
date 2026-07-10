/**
 * @file test_helpers.h
 * @brief Header-only test framework for the Tomato.C test suites.
 *
 * Provides TEST / RUN_TEST macros, assertion macros (ASSERT_EQ,
 * ASSERT_STR_EQ, ASSERT_TRUE, etc.), and suite-level begin/end
 * helpers.  All state is file-static; include once per test binary.
 *
 * Output is coloured by default when stdout is a TTY and neither
 * NO_COLOR nor CI are set.
 */

#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * ---------------------------------------------------------------------------
 * Suite-level counters (file-static)
 * ---------------------------------------------------------------------------
 */

static int g_test_passed;     /**< Number of passed tests in current suite. */
static int g_test_failed;     /**< Number of failed tests in current suite. */
static int g_test_skipped;    /**< Number of skipped tests in current suite. */
static int g_test_assertions; /**< Total assertions executed. */
static int
  g_test_failed_flag; /**< Non-zero once the current test has failed. */
static const char* g_test_name;  /**< Description of the current test. */
static const char* g_suite_name; /**< Name of the current suite. */

/**
 * ---------------------------------------------------------------------------
 * ANSI colour constants
 * ---------------------------------------------------------------------------
 */

#define ANSI_GREEN "\033[32m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_CYAN "\033[36m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RESET "\033[0m"

static int g_use_color = 1; /**< Global colour toggle (set by detect_color). */

/**
 * @brief Constructor that disables colour output when NO_COLOR, CI, or a
 *        "dumb" terminal are detected.
 */
__attribute__((constructor)) static void detect_color(void) {
  const char* no_color = getenv("NO_COLOR");
  const char* ci = getenv("CI");
  const char* term = getenv("TERM");
  if (no_color && no_color[0]) {
    g_use_color = 0;
    return;
  }
  if (ci && ci[0]) {
    g_use_color = 0;
    return;
  }
  if (!term || strcmp(term, "dumb") == 0) {
    g_use_color = 0;
    return;
  }
}

/**
 * @brief Return @p code if colour is enabled, empty string otherwise.
 * @param code  ANSI escape sequence.
 * @return The escape sequence or "".
 */
static const char* _C(const char* code) { return g_use_color ? code : ""; }

static const char* _cc_cyan(void) { return _C(ANSI_CYAN); }
static const char* _cc_red(void) { return _C(ANSI_RED); }
static const char* _cc_green(void) { return _C(ANSI_GREEN); }
static const char* _cc_yellow(void) { return _C(ANSI_YELLOW); }
static const char* _cc_bold(void) { return _C(ANSI_BOLD); }
static const char* _cc_reset(void) { return _C(ANSI_RESET); }

/**
 * ---------------------------------------------------------------------------
 * Suite begin / end
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Initialise counters and print the suite header.
 * @param suite  Name of the test suite (printed in the header).
 */
static void test_begin(const char* suite) {
  g_suite_name = suite;
  g_test_passed = 0;
  g_test_failed = 0;
  g_test_skipped = 0;
  g_test_assertions = 0;
  printf("\n%sSUITE: %s%s%s\n", _cc_cyan(), _cc_bold(), suite, _cc_cyan());
  printf("%s%s%s\n", _cc_cyan(),
         "====================================================", _cc_cyan());
}

/**
 * @brief Print the suite footer (pass/fail counts) and return the exit code.
 * @return 0 if all tests passed, 1 otherwise.
 */
static int test_end(void) {
  int total = g_test_passed + g_test_failed + g_test_skipped;
  (void)total;
  printf("%s%s%s\n", _cc_cyan(),
         "----------------------------------------------------", _cc_cyan());
  printf("  %d passed, %d failed, %d skipped\n", g_test_passed, g_test_failed,
         g_test_skipped);
  printf("  %d assertions\n", g_test_assertions);
  if (g_test_failed > 0)
    printf("%sRESULT: %s%s%s\n", _cc_red(), _cc_bold(), "FAIL", _cc_red());
  else
    printf("%sRESULT: %s%s%s\n", _cc_green(), _cc_bold(), "PASS", _cc_green());
  printf("\n");
  return g_test_failed > 0 ? 1 : 0;
}

/**
 * ---------------------------------------------------------------------------
 * Single-test macros
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Mark the start of a single test inside a test function.
 *
 * Sets @c g_test_name and resets @c g_test_failed_flag.  The test
 * description is printed immediately.
 *
 * @param desc  Human-readable test description.
 */
#define TEST(desc)        \
  g_test_name = desc;     \
  g_test_failed_flag = 0; \
  printf("  %-50s ", desc);

/**
 * @brief Print a PASS marker.
 */
#define PASS_MARK() \
  printf("%s%s%s%s\n", _cc_green(), _cc_bold(), "PASS", _cc_green())

/**
 * @brief Print a FAIL marker.
 */
#define FAIL_MARK() \
  printf("%s%s%s%s\n", _cc_red(), _cc_bold(), "FAIL", _cc_red())

/**
 * @brief Print a SKIP marker with an optional reason.
 * @param reason  Skipped-reason string.
 */
#define SKIP_MARK(reason)                                           \
  printf("%s%s %s%s%s\n", _cc_yellow(), _cc_bold(), "SKIP", reason, \
         _cc_yellow())

/**
 * ---------------------------------------------------------------------------
 * Assertion macros
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Assert that @p cond evaluates to true.
 * @param cond  Boolean expression.
 */
#define ASSERT(cond)                                                         \
  do {                                                                       \
    g_test_assertions++;                                                     \
    if (!(cond)) {                                                           \
      if (!g_test_failed_flag) {                                             \
        printf("\n    %sFAIL: %s:%d: ASSERT(%s)%s%s\n", _cc_red(), __FILE__, \
               __LINE__, #cond, _cc_red(), _cc_reset());                     \
        g_test_failed_flag = 1;                                              \
      }                                                                      \
    }                                                                        \
  } while (0)

/**
 * @brief Assert that @p a == @p b (cast to @c long long).
 */
#define ASSERT_EQ(a, b)                                                   \
  do {                                                                    \
    g_test_assertions++;                                                  \
    long long _a = (long long)(a);                                        \
    long long _b = (long long)(b);                                        \
    if (_a != _b) {                                                       \
      if (!g_test_failed_flag) {                                          \
        printf("\n    %sFAIL: %s:%d: ASSERT_EQ(%s, %s)%s%s\n", _cc_red(), \
               __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset());       \
        printf("      expected %lld, got %lld\n", _b, _a);                \
        g_test_failed_flag = 1;                                           \
      }                                                                   \
    }                                                                     \
  } while (0)

/**
 * @brief Assert that @p a != @p b (cast to @c long long).
 */
#define ASSERT_NE(a, b)                                                   \
  do {                                                                    \
    g_test_assertions++;                                                  \
    long long _a = (long long)(a);                                        \
    long long _b = (long long)(b);                                        \
    if (_a == _b) {                                                       \
      if (!g_test_failed_flag) {                                          \
        printf("\n    %sFAIL: %s:%d: ASSERT_NE(%s, %s)%s%s\n", _cc_red(), \
               __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset());       \
        printf("      both are %lld\n", _a);                              \
        g_test_failed_flag = 1;                                           \
      }                                                                   \
    }                                                                     \
  } while (0)

/**
 * @brief Assert that @p a > @p b (cast to @c long long).
 */
#define ASSERT_GT(a, b)                                                   \
  do {                                                                    \
    g_test_assertions++;                                                  \
    long long _a = (long long)(a);                                        \
    long long _b = (long long)(b);                                        \
    if (!(_a > _b)) {                                                     \
      if (!g_test_failed_flag) {                                          \
        printf("\n    %sFAIL: %s:%d: ASSERT_GT(%s, %s)%s%s\n", _cc_red(), \
               __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset());       \
        printf("      %lld is not > %lld\n", _a, _b);                     \
        g_test_failed_flag = 1;                                           \
      }                                                                   \
    }                                                                     \
  } while (0)

/**
 * @brief Assert that @p a < @p b (cast to @c long long).
 */
#define ASSERT_LT(a, b)                                                   \
  do {                                                                    \
    g_test_assertions++;                                                  \
    long long _a = (long long)(a);                                        \
    long long _b = (long long)(b);                                        \
    if (!(_a < _b)) {                                                     \
      if (!g_test_failed_flag) {                                          \
        printf("\n    %sFAIL: %s:%d: ASSERT_LT(%s, %s)%s%s\n", _cc_red(), \
               __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset());       \
        printf("      %lld is not < %lld\n", _a, _b);                     \
        g_test_failed_flag = 1;                                           \
      }                                                                   \
    }                                                                     \
  } while (0)

/** Convenience alias: ASSERT(cond). */
#define ASSERT_TRUE(cond) ASSERT(cond)
/** Assert that @p cond is false. */
#define ASSERT_FALSE(cond) ASSERT(!(cond))
/** Assert that @p ptr is NULL. */
#define ASSERT_NULL(ptr) ASSERT((ptr) == NULL)
/** Assert that @p ptr is non-NULL. */
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL)

/**
 * @brief Assert that two C strings are equal (via strcmp).
 */
#define ASSERT_STR_EQ(a, b)                                                   \
  do {                                                                        \
    g_test_assertions++;                                                      \
    const char* _a = (const char*)(a);                                        \
    const char* _b = (const char*)(b);                                        \
    if (strcmp(_a ? _a : "(null)", _b ? _b : "(null)") != 0) {                \
      if (!g_test_failed_flag) {                                              \
        printf("\n    %sFAIL: %s:%d: ASSERT_STR_EQ(%s, %s)%s%s\n", _cc_red(), \
               __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset());           \
        printf("      expected \"%s\", got \"%s\"\n", _b ? _b : "(null)",     \
               _a ? _a : "(null)");                                           \
        g_test_failed_flag = 1;                                               \
      }                                                                       \
    }                                                                         \
  } while (0)

/**
 * ---------------------------------------------------------------------------
 * Test runner macro
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Execute a test function, increment pass/fail counters.
 *
 * Prints a PASS or FAIL marker depending on whether the function set
 * @c g_test_failed_flag.
 *
 * @param fn    Function pointer (void fn(void)).
 * @param desc  Human-readable description printed alongside the result.
 */
#define RUN_TEST(fn, desc)                                                \
  do {                                                                    \
    g_test_failed_flag = 0;                                               \
    g_test_name = desc;                                                   \
    printf("  %-50s ", desc);                                             \
    fflush(stdout);                                                       \
    fn();                                                                 \
    if (g_test_failed_flag) {                                             \
      g_test_failed++;                                                    \
    } else {                                                              \
      g_test_passed++;                                                    \
      printf("%s%s%s%s\n", _cc_green(), _cc_bold(), "PASS", _cc_green()); \
    }                                                                     \
  } while (0)

/**
 * @brief Skip a test with an optional reason string.
 * @param reason  Explanation for the skip.
 */
#define SKIP(reason)   \
  do {                 \
    g_test_skipped++;  \
    SKIP_MARK(reason); \
    return;            \
  } while (0)

#endif /* TEST_HELPERS_H_ */
