/**
 * @file test_e2e.c
 * @brief End-to-end tests for the tomato CLI.
 *
 * Tests the -h flag (GetPomodoroHistory with empty log) and the -t flag
 * (GetTimerLog when no server is running).
 *
 * Links against the actual log, util, and gap_buffer modules with stub
 * implementations for SetError, LogError, and RenderCriticalQuitConfirmation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "log.h"
#include "test_helpers.h"
#include "util.h"

Config g_config;

void SetError(AppData* app, const char* ctx, ErrorType type) {
  (void)app;
  (void)ctx;
  (void)type;
}
void LogError(const char* ctx, ErrorType type) {
  (void)ctx;
  (void)type;
}
void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }

/**
 * ---------------------------------------------------------------------------
 * -h flag: GetPomodoroHistory with empty log
 * ---------------------------------------------------------------------------
 */

static void test_h_flag_no_history(void) {
  TEST("tomato -h shows no history message with empty log");

  char tmpfile[] = "/tmp/tomato_e2e_h_XXXXXX";
  int fd = mkstemp(tmpfile);
  ASSERT_GT(fd, -1);
  close(fd);

  fflush(stdout);
  int old_stdout = dup(STDOUT_FILENO);
  ASSERT_GT(old_stdout, -1);
  FILE* new_out = freopen(tmpfile, "w", stdout);
  ASSERT_NOT_NULL(new_out);

  g_config.logging.pomodoro_log = tmpfile;
  g_config.logging.work_log = 1;
  GetPomodoroHistory(tmpfile);

  fflush(stdout);
  dup2(old_stdout, STDOUT_FILENO);
  close(old_stdout);

  FILE* f = fopen(tmpfile, "r");
  ASSERT_NOT_NULL(f);
  char buf[256];
  int found = 0;
  while (fgets(buf, sizeof(buf), f)) {
    if (strstr(buf, "No pomodoro history found.")) {
      found = 1;
      break;
    }
  }
  fclose(f);
  ASSERT_TRUE(found);
  remove(tmpfile);
}

/**
 * ---------------------------------------------------------------------------
 * -t flag: GetTimerLog with no server running
 * ---------------------------------------------------------------------------
 */

static void test_t_flag_no_server(void) {
  TEST("tomato -t handles missing timer log server");
  g_config.logging.timer_log = 1;
  g_config.logging.timer_file = "/tmp/tomato_e2e_t_nonexistent.sock";
  ErrorType err = GetTimerLog(g_config.logging.timer_file, false, ASCII);
  ASSERT_EQ((int)err, (int)SOCKET_CONNECTION_ERROR);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("e2e");
  RUN_TEST(test_h_flag_no_history, "tomato -h shows no history message");
  RUN_TEST(test_t_flag_no_server, "tomato -t handles missing server");
  return test_end();
}
