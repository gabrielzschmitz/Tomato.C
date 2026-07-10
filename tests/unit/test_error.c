/**
 * @file test_error.c
 * @brief Unit tests for the error module.
 *
 * Tests SetError, ClearErrors, HasErrors, IsCriticalError, ErrorType
 * enum consistency, and LogError file output.
 */

#include <string.h>
#include <time.h>

#include "config.h"
#include "error.h"
#include "test_helpers.h"
#include "tomato.h"

Config g_config;

void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }

static AppData dummy_app;

/**
 * ---------------------------------------------------------------------------
 * SetError / ClearErrors / HasErrors / IsCriticalError
 * ---------------------------------------------------------------------------
 */

static void test_set_error_adds_error(void) {
  TEST("SetError records an error");
  ClearErrors();
  SetError(&dummy_app, "test_context", FILE_ERROR);
  ASSERT_TRUE(HasErrors());
}

static void test_clear_errors(void) {
  TEST("ClearErrors clears all errors");
  ClearErrors();
  SetError(&dummy_app, "ctx", FILE_ERROR);
  ASSERT_TRUE(HasErrors());
  ClearErrors();
  ASSERT_FALSE(HasErrors());
}

static void test_is_critical_after_critical(void) {
  TEST("IsCriticalError true after critical SetError");
  ClearErrors();
  SetError(&dummy_app, "ctx", FILE_ERROR);
  ASSERT_TRUE(IsCriticalError());
}

static void test_is_critical_after_non_critical(void) {
  TEST("IsCriticalError false after non-critical");
  ClearErrors();
  SetError(&dummy_app, "ctx", INVALID_INPUT);
  ASSERT_FALSE(IsCriticalError());
}

static void test_has_errors_state(void) {
  TEST("HasErrors returns correct state");
  ClearErrors();
  ASSERT_FALSE(HasErrors());
  SetError(&dummy_app, "ctx", FILE_ERROR);
  ASSERT_TRUE(HasErrors());
}

static void test_error_type_enum(void) {
  TEST("ErrorType enum values are consistent");
  ASSERT_EQ(NO_ERROR, 0);
  ASSERT_EQ(MALLOC_ERROR, 1);
  ASSERT_GT(TEST_ERROR, 0);
}

/**
 * ---------------------------------------------------------------------------
 * LogError
 * ---------------------------------------------------------------------------
 */

static void test_log_error_writes_file(void) {
  TEST("LogError writes to error log file");
  const char* tmp_path = "/tmp/test_error_log.txt";
  remove(tmp_path);
  g_config.logging.error_log = tmp_path;
  LogError("test_log_context", FILE_ERROR);
  FILE* f = fopen(tmp_path, "r");
  ASSERT_NOT_NULL(f);
  char buf[512];
  int found = 0;
  while (fgets(buf, sizeof(buf), f)) {
    if (strstr(buf, "test_log_context")) {
      found = 1;
      break;
    }
  }
  fclose(f);
  ASSERT_TRUE(found);
  remove(tmp_path);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("error");
  RUN_TEST(test_set_error_adds_error, "SetError records an error");
  RUN_TEST(test_clear_errors, "ClearErrors clears all errors");
  RUN_TEST(test_is_critical_after_critical,
           "IsCriticalError true after critical SetError");
  RUN_TEST(test_is_critical_after_non_critical,
           "IsCriticalError false after non-critical");
  RUN_TEST(test_has_errors_state, "HasErrors returns correct state");
  RUN_TEST(test_error_type_enum, "ErrorType enum values are consistent");
  RUN_TEST(test_log_error_writes_file, "LogError writes to error log file");
  return test_end();
}
