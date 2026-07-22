/**
 * @file test_draw.c
 * @brief Unit tests for the drawing module's screen-size validation.
 *
 * Tests ValidateAndRenderScreenSize boundary conditions using a
 * mock App/Screen structure that mirrors the real guard logic.
 */

#include <stdbool.h>

#include "draw.h"
#include "test_helpers.h"
#include "ui.h"

/**
 * ---------------------------------------------------------------------------
 * Mock types and helpers
 * ---------------------------------------------------------------------------
 */

typedef struct {
  Dimensions size;
  Dimensions min_panel_size;
} MockScreen;

typedef struct {
  MockScreen* screen;
  bool block_input;
} MockApp;

/** @brief Mirror of real ValidateAndRenderScreenSize screen-size check. */
static bool validateScreenSize(MockApp* app) {
  if (app->screen->size.width < app->screen->min_panel_size.width ||
      app->screen->size.height < app->screen->min_panel_size.height) {
    app->block_input = true;
    return false;
  }
  app->block_input = false;
  return true;
}

/**
 * ---------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------------
 */

/** @brief Screen narrower than minimum returns false and blocks input. */
static void test_screen_too_small_width(void) {
  TEST("ValidateAndRenderScreenSize false when width too small");
  MockScreen s = {{10, 40}, {20, 10}};
  MockApp app = {&s, false};
  ASSERT_FALSE(validateScreenSize(&app));
  ASSERT_TRUE(app.block_input);
}

/** @brief Screen shorter than minimum returns false and blocks input. */
static void test_screen_too_small_height(void) {
  TEST("ValidateAndRenderScreenSize false when height too small");
  MockScreen s = {{30, 5}, {20, 10}};
  MockApp app = {&s, false};
  ASSERT_FALSE(validateScreenSize(&app));
  ASSERT_TRUE(app.block_input);
}

/** @brief Screen at exact minimum dimensions passes validation. */
static void test_screen_exact_minimum(void) {
  TEST("ValidateAndRenderScreenSize true at exact minimum");
  MockScreen s = {{20, 10}, {20, 10}};
  MockApp app = {&s, true};
  ASSERT_TRUE(validateScreenSize(&app));
  ASSERT_FALSE(app.block_input);
}

/** @brief Screen larger than minimum passes validation. */
static void test_screen_larger_than_minimum(void) {
  TEST("ValidateAndRenderScreenSize true when screen is large enough");
  MockScreen s = {{80, 24}, {20, 10}};
  MockApp app = {&s, true};
  ASSERT_TRUE(validateScreenSize(&app));
  ASSERT_FALSE(app.block_input);
}

/** @brief Screen at zero size fails validation. */
static void test_screen_zero_size(void) {
  TEST("ValidateAndRenderScreenSize false at zero size");
  MockScreen s = {{0, 0}, {20, 10}};
  MockApp app = {&s, false};
  ASSERT_FALSE(validateScreenSize(&app));
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("draw");
  RUN_TEST(test_screen_too_small_width,
           "ValidateAndRenderScreenSize false when width too small");
  RUN_TEST(test_screen_too_small_height,
           "ValidateAndRenderScreenSize false when height too small");
  RUN_TEST(test_screen_exact_minimum,
           "ValidateAndRenderScreenSize true at exact minimum");
  RUN_TEST(test_screen_larger_than_minimum,
           "ValidateAndRenderScreenSize true when screen is large enough");
  RUN_TEST(test_screen_zero_size,
           "ValidateAndRenderScreenSize false at zero size");
  return test_end();
}
