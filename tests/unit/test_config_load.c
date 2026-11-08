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

/* Save an environment variable so a test can restore it afterwards. */
static char* save_env(const char* name) {
  const char* value = getenv(name);
  return value ? strdup(value) : NULL;
}

static void restore_env(const char* name, char* value) {
  if (value) {
    setenv(name, value, 1);
    free(value);
  } else {
    unsetenv(name);
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

static void test_save_preserves_alignment_spacing(void) {
  TEST("SaveConfigValues preserves key, value, and comment spacing");
  char tmp[] = "/tmp/t_cfg_spacing_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  write_file(tmp,
             "[visual]\n"
             "clock_24h            = 1            "
             "# 0/1 \xE2\x80\x94 use 24-hour clock\n");

  ConfigValue v = {"visual.clock_24h", "0"};
  ASSERT_EQ(SaveConfigValues(tmp, &v, 1), NO_ERROR);

  char buf[512];
  read_file(tmp, buf, sizeof(buf));
  ASSERT_STR_EQ(buf,
                "[visual]\n"
                "clock_24h            = 0            "
                "# 0/1 \xE2\x80\x94 use 24-hour clock\n");
  ASSERT_NULL(strstr(buf, "clock_24h = 0  #"));

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

static void test_user_config_override(void) {
  TEST("TOMATO_CONFIG_DIR overrides the user config path");
  char* old_override = save_env("TOMATO_CONFIG_DIR");
  char* old_xdg = save_env("XDG_CONFIG_HOME");
  setenv("TOMATO_CONFIG_DIR", "/tmp/t_cfg_override", 1);
  setenv("XDG_CONFIG_HOME", "/tmp/t_cfg_xdg", 1);

  char path[1024];
  ASSERT_TRUE(resolveUserConfigPath(path, sizeof(path)));
  ASSERT_STR_EQ(path, "/tmp/t_cfg_override/config.toml");

  restore_env("XDG_CONFIG_HOME", old_xdg);
  restore_env("TOMATO_CONFIG_DIR", old_override);
}

static void test_user_config_falls_back_to_xdg(void) {
  TEST("user config falls back to XDG_CONFIG_HOME");
  char* old_override = save_env("TOMATO_CONFIG_DIR");
  char* old_xdg = save_env("XDG_CONFIG_HOME");
  unsetenv("TOMATO_CONFIG_DIR");
  setenv("XDG_CONFIG_HOME", "/tmp/t_cfg_xdg", 1);

  char path[1024];
  ASSERT_TRUE(resolveUserConfigPath(path, sizeof(path)));
  ASSERT_STR_EQ(path, "/tmp/t_cfg_xdg/tomato/config.toml");

  restore_env("XDG_CONFIG_HOME", old_xdg);
  restore_env("TOMATO_CONFIG_DIR", old_override);
}

static void test_user_config_ignores_relative_override(void) {
  TEST("relative TOMATO_CONFIG_DIR is ignored");
  char* old_override = save_env("TOMATO_CONFIG_DIR");
  char* old_xdg = save_env("XDG_CONFIG_HOME");
  setenv("TOMATO_CONFIG_DIR", "relative/path", 1);
  setenv("XDG_CONFIG_HOME", "/tmp/t_cfg_xdg", 1);

  char path[1024];
  ASSERT_TRUE(resolveUserConfigPath(path, sizeof(path)));
  ASSERT_STR_EQ(path, "/tmp/t_cfg_xdg/tomato/config.toml");

  restore_env("XDG_CONFIG_HOME", old_xdg);
  restore_env("TOMATO_CONFIG_DIR", old_override);
}

static void test_state_and_runtime_overrides(void) {
  TEST("TOMATO_STATE_DIR and TOMATO_RUNTIME_DIR override XDG paths");
  char* old_state = save_env("TOMATO_STATE_DIR");
  char* old_runtime = save_env("TOMATO_RUNTIME_DIR");
  char* old_xdg_state = save_env("XDG_STATE_HOME");
  char* old_xdg_runtime = save_env("XDG_RUNTIME_DIR");
  setenv("TOMATO_STATE_DIR", "/tmp/t_cfg_state", 1);
  setenv("TOMATO_RUNTIME_DIR", "/tmp/t_cfg_runtime", 1);
  setenv("XDG_STATE_HOME", "/tmp/t_cfg_unused_state", 1);
  setenv("XDG_RUNTIME_DIR", "/tmp/t_cfg_unused_runtime", 1);

  setDefaults();
  ASSERT_STR_EQ(g_config.logging.pomodoro_log, "/tmp/t_cfg_state/pomodoro.bin");
  ASSERT_STR_EQ(g_config.logging.notes_log, "/tmp/t_cfg_state/notes.log");
  ASSERT_STR_EQ(g_config.logging.error_log, "/tmp/t_cfg_state/errors.log");
  ASSERT_STR_EQ(g_config.logging.timer_file,
                "/tmp/t_cfg_runtime/tomato_timer.sock");

  restore_env("XDG_RUNTIME_DIR", old_xdg_runtime);
  restore_env("XDG_STATE_HOME", old_xdg_state);
  restore_env("TOMATO_RUNTIME_DIR", old_runtime);
  restore_env("TOMATO_STATE_DIR", old_state);
  rmdir("/tmp/t_cfg_state");
  rmdir("/tmp/t_cfg_runtime");
}

static void test_state_dir_falls_back_to_passwd(void) {
  TEST("state dir uses passwd home when HOME is unset");
  char* old_home = save_env("HOME");
  char* old_xdg_state = save_env("XDG_STATE_HOME");
  char* old_override = save_env("TOMATO_STATE_DIR");
  unsetenv("HOME");
  unsetenv("XDG_STATE_HOME");
  unsetenv("TOMATO_STATE_DIR");

  char dir[4096];
  resolveStateDir(dir, sizeof(dir));

  struct passwd pw;
  struct passwd* result = NULL;
  char buf[1024];
  bool have_passwd =
    getpwuid_r(getuid(), &pw, buf, sizeof(buf), &result) == 0 && result &&
    result->pw_dir && result->pw_dir[0] == '/';
  if (have_passwd) {
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/.local/state/tomato",
             result->pw_dir);
    ASSERT_STR_EQ(dir, expected);
  } else {
    ASSERT_EQ(strncmp(dir, "/tmp/tomato-", 12), 0);
  }
  ASSERT_TRUE(strcmp(dir, "/tmp/tomato") != 0);

  restore_env("TOMATO_STATE_DIR", old_override);
  restore_env("XDG_STATE_HOME", old_xdg_state);
  restore_env("HOME", old_home);
}

static void test_private_dir_helper(void) {
  TEST("ensurePrivateDir creates and repairs owner-only directories");
  char parent[] = "/tmp/t_cfg_priv_XXXXXX";
  ASSERT_NOT_NULL(mkdtemp(parent));
  char dir[512];
  snprintf(dir, sizeof(dir), "%s/state", parent);

  ASSERT_TRUE(ensurePrivateDir(dir, getuid()));
  struct stat st;
  ASSERT_EQ(stat(dir, &st), 0);
  ASSERT_EQ(st.st_mode & 0777, 0700);

  ASSERT_EQ(chmod(dir, 0755), 0);
  ASSERT_TRUE(ensurePrivateDir(dir, getuid()));
  ASSERT_EQ(stat(dir, &st), 0);
  ASSERT_EQ(st.st_mode & 0777, 0700);

  char link[512];
  snprintf(link, sizeof(link), "%s/link", parent);
  ASSERT_EQ(symlink(dir, link), 0);
  ASSERT_FALSE(ensurePrivateDir(link, getuid()));
  ASSERT_EQ(unlink(link), 0);

  rmdir(dir);
  rmdir(parent);
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
  RUN_TEST(test_save_preserves_alignment_spacing,
           "SaveConfigValues preserves key, value, and comment spacing");
  RUN_TEST(test_save_adds_missing_key_to_its_table,
           "SaveConfigValues adds a missing key to an existing table");
  RUN_TEST(test_save_appends_missing_table,
           "SaveConfigValues appends a missing table");
  RUN_TEST(test_save_refuses_invalid_file,
           "SaveConfigValues refuses a file that does not parse");
  RUN_TEST(test_save_writes_booleans,
           "SaveConfigValues writes a boolean key as true/false");
  RUN_TEST(test_user_config_override,
           "TOMATO_CONFIG_DIR overrides the user config path");
  RUN_TEST(test_user_config_falls_back_to_xdg,
           "user config falls back to XDG_CONFIG_HOME");
  RUN_TEST(test_user_config_ignores_relative_override,
           "relative TOMATO_CONFIG_DIR is ignored");
  RUN_TEST(test_state_and_runtime_overrides,
           "TOMATO_STATE_DIR and TOMATO_RUNTIME_DIR override XDG paths");
  RUN_TEST(test_state_dir_falls_back_to_passwd,
           "state dir uses passwd home when HOME is unset");
  RUN_TEST(test_private_dir_helper,
           "ensurePrivateDir creates and repairs private directories");

  return test_end();
}
