#ifndef TEST_ANIM_STUBS_H_
#define TEST_ANIM_STUBS_H_

#include "features/animation/anim.h"

/**
 * @file test_anim_stubs.h
 * @brief Minimal Rollfilm dimension stubs for tests that link util.c.
 *
 * util.c queries cached sprite dimensions through anim.h. Tests that do not
 * exercise animation behavior can include this header once to satisfy those
 * references without linking anim.c.
 */

/** @brief Stub cached width query. */
int RollfilmWidth(const Rollfilm* animation) {
  (void)animation;
  return 0;
}

/** @brief Stub cached height query. */
int RollfilmHeight(const Rollfilm* animation) {
  (void)animation;
  return 0;
}

#endif /* TEST_ANIM_STUBS_H_ */
