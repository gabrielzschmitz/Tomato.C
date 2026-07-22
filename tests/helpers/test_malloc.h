/**
 * @file test_malloc.h
 * @brief Configurable malloc wrapper for OOM (out-of-memory) tests.
 *
 * Declares the interface for __wrap_malloc, allowing tests to simulate
 * memory allocation failures.  When linked with -Wl,--wrap,malloc, all
 * calls to malloc in the test binary are routed through the wrapper,
 * which can be configured to return NULL after a given number of calls.
 */

#ifndef TEST_MALLOC_H_
#define TEST_MALLOC_H_

#include <stdlib.h>

/**
 * Configure malloc to fail after N successful calls.
 * The next malloc call after count successful ones will return NULL.
 * Set to -1 (default) to never fail.
 * @param count Number of successful mallocs before the first failure
 */
void TestMallocFailAfter(int count);

/**
 * Reset malloc to never-fail mode.
 * Must be called after each test that uses TestMallocFailAfter to
 * avoid interfering with subsequent tests or cleanup code.
 */
void TestMallocReset(void);

#endif /* TEST_MALLOC_H_ */
