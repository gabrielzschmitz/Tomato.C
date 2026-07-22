/**
 * @file test_malloc.c
 * @brief Configurable malloc wrapper for OOM tests.
 *
 * Provides the implementation of __wrap_malloc and the control
 * functions declared in test_malloc.h.
 */

#include "test_malloc.h"

static int g_fail_after = -1;
static int g_call_count = 0;

/** @brief Wrapped malloc — returns NULL after fail-after count is reached. */
void* __wrap_malloc(size_t size) {
  extern void* __real_malloc(size_t);
  g_call_count++;
  if (g_fail_after >= 0 && g_call_count > g_fail_after) return NULL;
  return __real_malloc(size);
}

/** @brief Configure __wrap_malloc to fail after @p count successful calls. */
void TestMallocFailAfter(int count) {
  g_fail_after = count;
  g_call_count = 0;
}

/** @brief Reset to never-fail mode. */
void TestMallocReset(void) {
  g_fail_after = -1;
  g_call_count = 0;
}
