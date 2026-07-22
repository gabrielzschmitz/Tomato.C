/**
 * @file test_anim_malloc.c
 * @brief OOM unit tests for the animation (rollfilm) module.
 *
 * Verifies that CreateRollfilm handles malloc failure gracefully.
 * Linked with -Wl,--wrap,malloc to intercept all allocations.
 */

#include <stdlib.h>

#include "anim.h"
#include "config.h"
#include "error.h"
#include "test_helpers.h"
#include "test_malloc.h"

Config g_config;
void RenderCriticalQuitConfirmation(void* app) { (void)app; }

/**
 * ---------------------------------------------------------------------------
 * CreateRollfilm OOM
 * ---------------------------------------------------------------------------
 */

/** @brief CreateRollfilm returns NULL when the first malloc fails. */
static void test_create_rollfilm_malloc_fail(void) {
  TEST("CreateRollfilm returns NULL when malloc fails");
  TestMallocFailAfter(0);
  Rollfilm* rf = CreateRollfilm(3, 5);
  ASSERT_NULL(rf);
  TestMallocReset();
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("anim_malloc");
  RUN_TEST(test_create_rollfilm_malloc_fail,
           "CreateRollfilm returns NULL when malloc fails");
  return test_end();
}
