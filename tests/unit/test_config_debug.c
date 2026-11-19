/**
 * @file test_config_debug.c
 * @brief Tests for project-local .tmp paths in debug builds.
 *
 * Includes config.c with DEBUG_FLAG and a fixed TOMATO_DEV_ROOT so path
 * resolution can be checked without building a debug binary.
 */

#define _POSIX_C_SOURCE 200809L
#define DEBUG_FLAG
#define TOMATO_DEV_ROOT "/tmp/tomato_debug_root"

#include <stdlib.h>
#include <string.h>

#include "core/config.h"
#include "external/toml-c.h"
#include "input/input.h"
#include "test_helpers.h"
#include "test_keybind_stubs.h"

/* Now include the actual config.c source. */
#include "core/config.c"

/** Remove every path override used by these tests. */
static void clear_path_env(void) {
  unsetenv("TOMATO_CONFIG_DIR");
  unsetenv("TOMATO_STATE_DIR");
  unsetenv("TOMATO_RUNTIME_DIR");
  unsetenv("XDG_CONFIG_HOME");
  unsetenv("XDG_STATE_HOME");
  unsetenv("XDG_RUNTIME_DIR");
  unsetenv("HOME");
}

/** Rebuild defaults without leaking the previous key-binding array. */
static void resetDefaults(void) {
  free(g_config.key_bindings);
  g_config.key_bindings = NULL;
  setDefaults();
}

static void test_debug_config_path(void) {
  TEST("debug config path uses project .tmp");
  clear_path_env();

  char path[1024];
  ASSERT_TRUE(resolveUserConfigPath(path, sizeof(path)));
  ASSERT_STR_EQ(path, "/tmp/tomato_debug_root/.tmp/config/tomato/config.toml");
}

static void test_debug_state_and_runtime_paths(void) {
  TEST("debug state and runtime paths use project .tmp");
  clear_path_env();

  resetDefaults();
  ASSERT_STR_EQ(g_config.logging.pomodoro_log,
                "/tmp/tomato_debug_root/.tmp/state/tomato/pomodoro.bin");
  ASSERT_STR_EQ(g_config.logging.notes_log,
                "/tmp/tomato_debug_root/.tmp/state/tomato/notes.log");
  ASSERT_STR_EQ(g_config.logging.error_log,
                "/tmp/tomato_debug_root/.tmp/state/tomato/errors.log");
  ASSERT_STR_EQ(g_config.logging.timer_file,
                "/tmp/tomato_debug_root/.tmp/runtime/tomato_timer.sock");
}

static void test_debug_overrides_still_win(void) {
  TEST("TOMATO_* overrides win in debug builds");
  clear_path_env();
  setenv("TOMATO_CONFIG_DIR", "/tmp/t_cfg_debug_config", 1);
  setenv("TOMATO_STATE_DIR", "/tmp/t_cfg_debug_state", 1);
  setenv("TOMATO_RUNTIME_DIR", "/tmp/t_cfg_debug_runtime", 1);

  char path[1024];
  ASSERT_TRUE(resolveUserConfigPath(path, sizeof(path)));
  ASSERT_STR_EQ(path, "/tmp/t_cfg_debug_config/config.toml");

  resetDefaults();
  ASSERT_STR_EQ(g_config.logging.pomodoro_log,
                "/tmp/t_cfg_debug_state/pomodoro.bin");
  ASSERT_STR_EQ(g_config.logging.timer_file,
                "/tmp/t_cfg_debug_runtime/tomato_timer.sock");

  clear_path_env();
}

int main(void) {
  test_begin("config-debug");
  RUN_TEST(test_debug_config_path, "debug config path uses project .tmp");
  RUN_TEST(test_debug_state_and_runtime_paths,
           "debug state and runtime paths use project .tmp");
  RUN_TEST(test_debug_overrides_still_win,
           "TOMATO_* overrides win in debug builds");
  return test_end();
}
