/**
 * @file test_tomato.c
 * @brief Unit tests for the CLI argument parser (tomato.c / main.c).
 *
 * Tests timer looping and one-shot modes, icon sets, and history.
 */

#include <stdbool.h>
#include <string.h>

#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * Helper: argument parser mirror
 * ---------------------------------------------------------------------------
 */

typedef enum {
  ICON_NONE = -1,
  ICON_DEFAULT,
  ICON_NERD,
  ICON_EMOJI,
  ICON_ASCII,
  ICON_NO_ICONS
} IconChoice;

typedef enum {
  CLI_MODE_NORMAL,
  CLI_MODE_TIMER,
  CLI_MODE_HISTORY,
  CLI_MODE_ERROR
} CliMode;

/** @brief Mirror of tomato.c icon-set parsing. */
static bool parseIconSet(const char* arg, IconChoice* icon_out) {
  if (strcmp(arg, "nerd-icons") == 0)
    *icon_out = ICON_NERD;
  else if (strcmp(arg, "emojis") == 0)
    *icon_out = ICON_EMOJI;
  else if (strcmp(arg, "ascii") == 0)
    *icon_out = ICON_ASCII;
  else if (strcmp(arg, "no-icons") == 0)
    *icon_out = ICON_NO_ICONS;
  else
    return false;
  return true;
}

/** @brief Mirror of tomato.c boolean option parsing. */
static bool parseBool(const char* arg, bool* value) {
  if (strcmp(arg, "true") == 0)
    *value = true;
  else if (strcmp(arg, "false") == 0)
    *value = false;
  else
    return false;
  return true;
}

/** @brief Mirror of tomato.c argument parsing logic. */
static CliMode parseArgs(int argc, char* argv[], IconChoice* icon_out,
                         bool* loop_out) {
  *icon_out = ICON_DEFAULT;
  *loop_out = true;
  if (argc < 2) return CLI_MODE_NORMAL;

  int timer_arg = 0;
  if (strcmp("-loop", argv[1]) == 0) {
    if (argc < 3 || !parseBool(argv[2], loop_out)) return CLI_MODE_ERROR;
    timer_arg = 3;
    if (timer_arg >= argc || strcmp("-t", argv[timer_arg]) != 0)
      return CLI_MODE_ERROR;
  } else if (strcmp("-t", argv[1]) == 0) {
    timer_arg = 1;
  }

  if (timer_arg != 0) {
    int next = timer_arg + 1;
    if (next < argc && strcmp("-loop", argv[next]) == 0) {
      if (next + 1 >= argc || !parseBool(argv[next + 1], loop_out))
        return CLI_MODE_ERROR;
      next += 2;
    }
    if (next < argc) {
      if (!parseIconSet(argv[next], icon_out)) return CLI_MODE_ERROR;
      next++;
    }
    if (next != argc) return CLI_MODE_ERROR;
    return CLI_MODE_TIMER;
  }

  if (strcmp("-h", argv[1]) == 0) {
    if (argc == 2) return CLI_MODE_HISTORY;
    if (argc == 3 &&
        (strcmp("overview", argv[2]) == 0 || strcmp("today", argv[2]) == 0))
      return CLI_MODE_HISTORY;
    return CLI_MODE_ERROR;
  }

  return CLI_MODE_ERROR;
}

/**
 * ---------------------------------------------------------------------------
 * No arguments
 * ---------------------------------------------------------------------------
 */

/** @brief No arguments returns normal mode. */
static void test_no_args(void) {
  TEST("parseArgs returns normal mode with no arguments");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", NULL};
  ASSERT_EQ(parseArgs(1, argv, &icon, &loop), CLI_MODE_NORMAL);
  ASSERT_TRUE(loop);
}

/**
 * ---------------------------------------------------------------------------
 * -t flag (timer status)
 * ---------------------------------------------------------------------------
 */

/** @brief -t alone streams with the default icon set. */
static void test_t_flag_default(void) {
  TEST("parseArgs -t streams by default");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t"};
  ASSERT_EQ(parseArgs(2, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
  ASSERT_EQ(icon, ICON_DEFAULT);
}

/** @brief -t nerd-icons sets ICON_NERD. */
static void test_t_flag_nerd_icons(void) {
  TEST("parseArgs -t nerd-icons sets ICON_NERD");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "nerd-icons"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
  ASSERT_EQ(icon, ICON_NERD);
}

/** @brief -t emojis sets ICON_EMOJI. */
static void test_t_flag_emojis(void) {
  TEST("parseArgs -t emojis sets ICON_EMOJI");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "emojis"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
  ASSERT_EQ(icon, ICON_EMOJI);
}

/** @brief -t ascii sets ICON_ASCII. */
static void test_t_flag_ascii(void) {
  TEST("parseArgs -t ascii sets ICON_ASCII");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "ascii"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
  ASSERT_EQ(icon, ICON_ASCII);
}

/** @brief -t no-icons sets ICON_NO_ICONS. */
static void test_t_flag_no_icons(void) {
  TEST("parseArgs -t no-icons sets ICON_NO_ICONS");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "no-icons"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
  ASSERT_EQ(icon, ICON_NO_ICONS);
}

/** @brief -loop false before -t selects one-shot mode. */
static void test_loop_false_before_t(void) {
  TEST("parseArgs -loop false -t selects one-shot mode");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "-loop", "false", "-t"};
  ASSERT_EQ(parseArgs(4, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(!loop);
  ASSERT_EQ(icon, ICON_DEFAULT);
}

/** @brief -loop true before -t keeps streaming mode. */
static void test_loop_true_before_t(void) {
  TEST("parseArgs -loop true -t keeps streaming mode");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-loop", "true", "-t"};
  ASSERT_EQ(parseArgs(4, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(loop);
}

/** @brief -loop false accepts an icon set in either position. */
static void test_loop_false_with_iconset(void) {
  TEST("parseArgs -loop false -t ascii selects one-shot ASCII mode");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "-loop", "false", "-t", "ascii"};
  ASSERT_EQ(parseArgs(5, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(!loop);
  ASSERT_EQ(icon, ICON_ASCII);
}

/** @brief -loop may follow -t as well. */
static void test_loop_after_t(void) {
  TEST("parseArgs -t -loop false selects one-shot mode");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "-t", "-loop", "false", "ascii"};
  ASSERT_EQ(parseArgs(5, argv, &icon, &loop), CLI_MODE_TIMER);
  ASSERT_TRUE(!loop);
  ASSERT_EQ(icon, ICON_ASCII);
}

/** @brief -t with an unrecognized icon set returns an error. */
static void test_t_flag_invalid_iconset(void) {
  TEST("parseArgs -t with invalid icon set returns error");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "invalid"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_ERROR);
}

/** @brief -t with extra positional arg returns an error. */
static void test_t_with_extra_arg_after_iconset(void) {
  TEST("parseArgs -t nerd-icons extra returns error");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-t", "nerd-icons", "extra"};
  ASSERT_EQ(parseArgs(4, argv, &icon, &loop), CLI_MODE_ERROR);
}

/** @brief An invalid -loop value returns an error. */
static void test_loop_invalid_value(void) {
  TEST("parseArgs -loop maybe -t returns error");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "-loop", "maybe", "-t"};
  ASSERT_EQ(parseArgs(4, argv, &icon, &loop), CLI_MODE_ERROR);
}

/** @brief A missing -loop value returns an error. */
static void test_loop_missing_value(void) {
  TEST("parseArgs -loop false without -t returns error");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "-loop", "false"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_ERROR);
}

/** @brief --watch was removed and is now an error. */
static void test_watch_flag_removed(void) {
  TEST("parseArgs --watch returns error");
  IconChoice icon = ICON_NONE;
  bool loop = true;
  char* argv[] = {"tomato", "--watch"};
  ASSERT_EQ(parseArgs(2, argv, &icon, &loop), CLI_MODE_ERROR);
}

/**
 * ---------------------------------------------------------------------------
 * -h flag (history)
 * ---------------------------------------------------------------------------
 */

/** @brief -h alone returns history mode. */
static void test_h_flag(void) {
  TEST("parseArgs -h selects history mode");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-h"};
  ASSERT_EQ(parseArgs(2, argv, &icon, &loop), CLI_MODE_HISTORY);
}

/** @brief -h overview returns history mode. */
static void test_h_overview(void) {
  TEST("parseArgs -h overview selects history mode");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-h", "overview"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_HISTORY);
}

/** @brief -h today returns history mode. */
static void test_h_today(void) {
  TEST("parseArgs -h today selects history mode");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-h", "today"};
  ASSERT_EQ(parseArgs(3, argv, &icon, &loop), CLI_MODE_HISTORY);
}

/**
 * ---------------------------------------------------------------------------
 * Unknown / edge cases
 * ---------------------------------------------------------------------------
 */

/** @brief Unknown flag returns an error. */
static void test_unknown_flag(void) {
  TEST("parseArgs unknown flag returns error");
  IconChoice icon = ICON_NONE;
  bool loop = false;
  char* argv[] = {"tomato", "-x"};
  ASSERT_EQ(parseArgs(2, argv, &icon, &loop), CLI_MODE_ERROR);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("tomato");
  RUN_TEST(test_no_args, "no arguments returns normal mode");
  RUN_TEST(test_t_flag_default, "-t streams by default");
  RUN_TEST(test_t_flag_nerd_icons, "-t nerd-icons sets ICON_NERD");
  RUN_TEST(test_t_flag_emojis, "-t emojis sets ICON_EMOJI");
  RUN_TEST(test_t_flag_ascii, "-t ascii sets ICON_ASCII");
  RUN_TEST(test_t_flag_no_icons, "-t no-icons sets ICON_NO_ICONS");
  RUN_TEST(test_loop_false_before_t, "-loop false -t selects one-shot mode");
  RUN_TEST(test_loop_true_before_t, "-loop true -t keeps streaming mode");
  RUN_TEST(test_loop_false_with_iconset,
           "-loop false -t ascii selects one-shot ASCII mode");
  RUN_TEST(test_loop_after_t, "-t -loop false selects one-shot mode");
  RUN_TEST(test_t_flag_invalid_iconset, "-t with invalid icon set returns error");
  RUN_TEST(test_t_with_extra_arg_after_iconset,
           "-t with extra arg after iconset returns error");
  RUN_TEST(test_loop_invalid_value, "-loop invalid value returns error");
  RUN_TEST(test_loop_missing_value, "-loop without -t returns error");
  RUN_TEST(test_watch_flag_removed, "--watch returns error");
  RUN_TEST(test_h_flag, "-h selects history mode");
  RUN_TEST(test_h_overview, "-h overview selects history mode");
  RUN_TEST(test_h_today, "-h today selects history mode");
  RUN_TEST(test_unknown_flag, "unknown flag returns error");
  return test_end();
}
