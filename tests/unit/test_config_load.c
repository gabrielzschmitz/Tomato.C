/**
 * @file test_config_load.c
 * @brief Unit tests for configuration file loading diagnostics.
 *
 * Includes config.c directly with stub action functions so that
 * loadTomlFile() can be run against temporary TOML files.
 *
 * Tests:
 *   - An unparseable file is ignored and queues an error diagnostic
 *   - A valid file queues no diagnostics
 *   - sample_config.toml loads cleanly and matches the built-in defaults
 */

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "external/toml-c.h"
#include "test_helpers.h"
#include "test_keybind_stubs.h"

/* Now include the actual config.c source */
#include "config.c"

static void write_file(const char* path, const char* text) {
  FILE* f = fopen(path, "w");
  if (f) {
    fputs(text, f);
    fclose(f);
  }
}

static void test_parse_error_is_queued(void) {
  TEST("unparseable config queues an error diagnostic");
  char tmp[] = "/tmp/t_cfg_load_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "[visual\nanimations = 1\n");

  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);

  ASSERT_EQ(GetConfigWarningCount(), 1);
  ErrorLevel level = ERROR_LEVEL_INFO;
  const char* msg = GetConfigWarning(0, &level);
  ASSERT_NOT_NULL(msg);
  ASSERT_EQ(level, ERROR_LEVEL_ERROR);
  ASSERT_NOT_NULL(strstr(msg, tmp));
  ASSERT_NOT_NULL(strstr(msg, "ignored"));
  remove(tmp);
}

static void test_valid_file_queues_nothing(void) {
  TEST("valid config queues no diagnostics");
  char tmp[] = "/tmp/t_cfg_ok_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "[visual]\nanimations = 0\n");

  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);

  ASSERT_EQ(GetConfigWarningCount(), 0);
  ASSERT_EQ(g_config.visual.animations, 0);
  remove(tmp);
}

static void test_sample_config_matches_defaults(void) {
  TEST("sample_config.toml loads cleanly and matches defaults");
  setDefaults();
  int default_clock = g_config.visual.clock_24h;
  g_config.visual.clock_24h = -1;
  resetConfigWarnings();
  loadTomlFile(SAMPLE_CONFIG_PATH);

  ASSERT_EQ(GetConfigWarningCount(), 0);
  ASSERT_EQ(g_config.visual.clock_24h, default_clock);
}

static void test_invalid_icons_fall_back_to_ascii(void) {
  TEST("invalid visual.icons falls back to ascii with a warning");
  char tmp[] = "/tmp/t_cfg_icons_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "[visual]\nicons = \"nerdfont\"\n");

  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);

  ASSERT_STR_EQ(g_config.visual.icons, "ascii");
  ASSERT_EQ(g_config.visual.icons_index, 2);
  ASSERT_EQ(GetConfigWarningCount(), 1);
  ErrorLevel level = ERROR_LEVEL_INFO;
  const char* msg = GetConfigWarning(0, &level);
  ASSERT_EQ(level, ERROR_LEVEL_WARNING);
  ASSERT_NOT_NULL(strstr(msg, "nerdfont"));
  remove(tmp);
}

static void read_file(const char* path, char* buf, size_t size) {
  FILE* f = fopen(path, "rb");
  size_t n = f ? fread(buf, 1, size - 1, f) : 0;
  if (f) fclose(f);
  buf[n] = '\0';
}

static void test_save_keeps_comment_and_updates_value(void) {
  TEST("SaveConfigValues updates a key and keeps its comment");
  char tmp[] = "/tmp/t_cfg_save_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "# my config\n[visual]\nanimations = 1  # enable anim\n");

  ConfigValue v = {"visual.animations", "0"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), NO_ERROR);

  char buf[512];
  read_file(tmp, buf, sizeof(buf));
  ASSERT_NOT_NULL(strstr(buf, "# my config"));
  ASSERT_NOT_NULL(strstr(buf, "animations = 0  # enable anim"));
  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);
  ASSERT_EQ(g_config.visual.animations, 0);
  remove(tmp);
}

static void test_save_adds_missing_key_to_its_table(void) {
  TEST("SaveConfigValues adds a missing key to an existing table");
  char tmp[] = "/tmp/t_cfg_add_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp,
             "[visual]\nicons = \"nerd-icons\"\n\n[pomodoro]\namount = 4\n");

  ConfigValue v = {"visual.clock_24h", "1"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), NO_ERROR);

  char buf[512];
  read_file(tmp, buf, sizeof(buf));
  ASSERT_NOT_NULL(strstr(buf, "amount = 4"));
  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);
  ASSERT_EQ(g_config.visual.clock_24h, 1);
  remove(tmp);
}

static void test_save_appends_missing_table(void) {
  TEST("SaveConfigValues appends a missing table");
  char tmp[] = "/tmp/t_cfg_tbl_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "# only a comment\n");

  ConfigValue v = {"pomodoro.amount", "3"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), NO_ERROR);

  char buf[512];
  read_file(tmp, buf, sizeof(buf));
  ASSERT_NOT_NULL(strstr(buf, "# only a comment"));
  ASSERT_NOT_NULL(strstr(buf, "[pomodoro]"));
  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);
  ASSERT_EQ(g_config.pomodoro.amount, 3);
  remove(tmp);
}

static void test_save_refuses_invalid_file(void) {
  TEST("SaveConfigValues refuses a file that does not parse");
  char tmp[] = "/tmp/t_cfg_bad_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "[visual\nanimations = 1\n");

  ConfigValue v = {"visual.animations", "0"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), INVALID_CONFIG);

  char buf[128];
  read_file(tmp, buf, sizeof(buf));
  ASSERT_STR_EQ(buf, "[visual\nanimations = 1\n");
  remove(tmp);
}

static void test_save_writes_booleans(void) {
  TEST("SaveConfigValues writes a boolean key as true/false");
  char tmp[] = "/tmp/t_cfg_bool_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  write_file(tmp, "[notifications]\nenabled = true\n");

  ConfigValue v = {"notifications.enabled", "false"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), NO_ERROR);

  setDefaults();
  resetConfigWarnings();
  loadTomlFile(tmp);
  ASSERT_EQ(g_config.notifications.enabled, 0);
  remove(tmp);
}

static int stepper_action_for(int key) {
  for (size_t i = 0; i < g_config.num_keys; i++) {
    KeyFunction* k = &g_config.key_bindings[i];
    if (k->key == key && k->scene_types == SCENE_PREFS_STEPPER) {
      if (k->action == StepperClose) return 1;
      if (k->action == StepperCancel) return 2;
    }
  }
  return 0;
}

static void test_stepper_keys_commit_and_cancel(void) {
  TEST("stepper: Enter commits, Esc/q/Ctrl-C cancel");
  setDefaults();
  ASSERT_EQ(stepper_action_for(ENTER), 1);
  ASSERT_EQ(stepper_action_for(ESC), 2);
  ASSERT_EQ(stepper_action_for('q'), 2);
  ASSERT_EQ(stepper_action_for(CTRLC), 2);
}

int main(void) {
  test_begin("config_load");

  RUN_TEST(test_parse_error_is_queued,
           "unparseable config queues an error diagnostic");
  RUN_TEST(test_valid_file_queues_nothing,
           "valid config queues no diagnostics");
  RUN_TEST(test_sample_config_matches_defaults,
           "sample_config.toml loads cleanly and matches defaults");
  RUN_TEST(test_invalid_icons_fall_back_to_ascii,
           "invalid visual.icons falls back to ascii with a warning");
  RUN_TEST(test_stepper_keys_commit_and_cancel,
           "stepper: Enter commits, Esc/q/Ctrl-C cancel");
  RUN_TEST(test_save_keeps_comment_and_updates_value,
           "SaveConfigValues updates a key and keeps its comment");
  RUN_TEST(test_save_adds_missing_key_to_its_table,
           "SaveConfigValues adds a missing key to an existing table");
  RUN_TEST(test_save_appends_missing_table,
           "SaveConfigValues appends a missing table");
  RUN_TEST(test_save_refuses_invalid_file,
           "SaveConfigValues refuses a file that does not parse");
  RUN_TEST(test_save_writes_booleans,
           "SaveConfigValues writes a boolean key as true/false");

  return test_end();
}
