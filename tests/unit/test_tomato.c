/**
 * @file test_tomato.c
 * @brief Unit tests for the CLI argument parser (tomato.c / main.c).
 *
 * Tests -t flag with all icon-set values, -h flag, unknown flags,
 * --help, extra positional arguments, and invalid subcommands.
 */

#include <stdbool.h>
#include <string.h>

#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * Helper: argument parser mirror
 * ---------------------------------------------------------------------------
 */

typedef enum { ICON_NONE = -1, ICON_DEFAULT, ICON_NERD, ICON_EMOJI, ICON_ASCII, ICON_NO_ICONS } IconChoice;

/** @brief Mirror of tomato.c argument parsing logic. */
static int parseArgs(int argc, char* argv[], IconChoice* icon_out) {
  if (argc < 2) return 0;
  if (strcmp("-t", argv[1]) == 0) {
    if (argc == 2) {
      *icon_out = ICON_DEFAULT;
      return 1;
    }
    if (argc == 3) {
      if (strcmp(argv[2], "nerd-icons") == 0) *icon_out = ICON_NERD;
      else if (strcmp(argv[2], "emojis") == 0) *icon_out = ICON_EMOJI;
      else if (strcmp(argv[2], "ascii") == 0) *icon_out = ICON_ASCII;
      else if (strcmp(argv[2], "no-icons") == 0) *icon_out = ICON_NO_ICONS;
      else return -1;
      return 1;
    }
  } else if (strcmp("-h", argv[1]) == 0) {
    return 2;
  }
  return -1;
}

/**
 * ---------------------------------------------------------------------------
 * No arguments
 * ---------------------------------------------------------------------------
 */

/** @brief No arguments returns 0 (normal mode). */
static void test_no_args(void) {
  TEST("parseArgs returns 0 with no arguments");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", NULL};
  ASSERT_EQ(parseArgs(1, argv, &icon), 0);
}

/**
 * ---------------------------------------------------------------------------
 * -t flag
 * ---------------------------------------------------------------------------
 */

/** @brief -t alone sets ICON_DEFAULT. */
static void test_t_flag_default(void) {
  TEST("parseArgs -t flag sets ICON_DEFAULT");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t"};
  ASSERT_EQ(parseArgs(2, argv, &icon), 1);
  ASSERT_EQ(icon, ICON_DEFAULT);
}

/** @brief -t nerd-icons sets ICON_NERD. */
static void test_t_flag_nerd_icons(void) {
  TEST("parseArgs -t nerd-icons sets ICON_NERD");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "nerd-icons"};
  ASSERT_EQ(parseArgs(3, argv, &icon), 1);
  ASSERT_EQ(icon, ICON_NERD);
}

/** @brief -t emojis sets ICON_EMOJI. */
static void test_t_flag_emojis(void) {
  TEST("parseArgs -t emojis sets ICON_EMOJI");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "emojis"};
  ASSERT_EQ(parseArgs(3, argv, &icon), 1);
  ASSERT_EQ(icon, ICON_EMOJI);
}

/** @brief -t ascii sets ICON_ASCII. */
static void test_t_flag_ascii(void) {
  TEST("parseArgs -t ascii sets ICON_ASCII");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "ascii"};
  ASSERT_EQ(parseArgs(3, argv, &icon), 1);
  ASSERT_EQ(icon, ICON_ASCII);
}

/** @brief -t no-icons sets ICON_NO_ICONS. */
static void test_t_flag_no_icons(void) {
  TEST("parseArgs -t no-icons sets ICON_NO_ICONS");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "no-icons"};
  ASSERT_EQ(parseArgs(3, argv, &icon), 1);
  ASSERT_EQ(icon, ICON_NO_ICONS);
}

/** @brief -t with an unrecognized icon set returns -1. */
static void test_t_flag_invalid_iconset(void) {
  TEST("parseArgs -t with invalid icon set returns -1");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "invalid"};
  ASSERT_EQ(parseArgs(3, argv, &icon), -1);
}

/**
 * ---------------------------------------------------------------------------
 * -h flag
 * ---------------------------------------------------------------------------
 */

/** @brief -h alone returns 2 (history mode). */
static void test_h_flag(void) {
  TEST("parseArgs -h flag returns 2");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-h"};
  ASSERT_EQ(parseArgs(2, argv, &icon), 2);
}

/** @brief -h with a subcommand still returns 2. */
static void test_h_with_invalid_subcommand(void) {
  TEST("parseArgs -h stats returns 2");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-h", "stats"};
  ASSERT_EQ(parseArgs(3, argv, &icon), 2);
}

/**
 * ---------------------------------------------------------------------------
 * Unknown / edge cases
 * ---------------------------------------------------------------------------
 */

/** @brief Unknown flag returns -1. */
static void test_unknown_flag(void) {
  TEST("parseArgs unknown flag returns -1");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-x"};
  ASSERT_EQ(parseArgs(2, argv, &icon), -1);
}

/** @brief --help is not recognized and returns -1. */
static void test_long_help_flag(void) {
  TEST("parseArgs --help is treated as unknown flag");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "--help"};
  ASSERT_EQ(parseArgs(2, argv, &icon), -1);
}

/** @brief -t with extra positional arg after icon set returns -1. */
static void test_t_with_extra_arg_after_iconset(void) {
  TEST("parseArgs -t nerd-icons extra returns -1");
  IconChoice icon = ICON_NONE;
  char* argv[] = {"tomato", "-t", "nerd-icons", "extra"};
  ASSERT_EQ(parseArgs(4, argv, &icon), -1);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("tomato");
  RUN_TEST(test_no_args, "no arguments returns normal mode");
  RUN_TEST(test_t_flag_default, "-t flag sets ICON_DEFAULT");
  RUN_TEST(test_t_flag_nerd_icons, "-t nerd-icons sets ICON_NERD");
  RUN_TEST(test_t_flag_emojis, "-t emojis sets ICON_EMOJI");
  RUN_TEST(test_t_flag_ascii, "-t ascii sets ICON_ASCII");
  RUN_TEST(test_t_flag_no_icons, "-t no-icons sets ICON_NO_ICONS");
  RUN_TEST(test_t_flag_invalid_iconset, "-t with invalid icon set returns error");
  RUN_TEST(test_h_flag, "-h flag returns history mode");
  RUN_TEST(test_unknown_flag, "unknown flag returns error");
  RUN_TEST(test_long_help_flag, "--help treated as unknown flag");
  RUN_TEST(test_t_with_extra_arg_after_iconset,
           "-t with extra arg after iconset returns error");
  RUN_TEST(test_h_with_invalid_subcommand,
           "-h stats returns history mode");
  return test_end();
}
