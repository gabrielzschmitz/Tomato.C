/**
 * @file test_ui.c
 * @brief Unit tests for the UI module.
 *
 * Tests RegisterClickRegion data storage, overflow protection,
 * ClearClickRegions reset, and CreateScreen panel layout math.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "util.h"

#define MAX_CLICK_REGIONS 32
#define MAX_PANELS 2

/**
 * ---------------------------------------------------------------------------
 * Mock types
 * ---------------------------------------------------------------------------
 */

typedef enum {
  REGION_DIRECT,
  REGION_MENU_ITEM,
  REGION_POPUP_ITEM,
  REGION_NOTE_ITEM,
  REGION_SLIDE_NAV,
} RegionType;

typedef void (*MenuAction)(void* app);

typedef struct {
  Vector2D pos;
  Dimensions size;
  RegionType type;
  MenuAction action;
  int menu_index;
  int item_index;
  int note_id;
} ClickRegion;

typedef struct {
  ClickRegion click_regions[MAX_CLICK_REGIONS];
  int click_region_count;
} MockApp;

/**
 * ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------
 */

/** @brief Mirror of real RegisterClickRegion. */
static void registerClickRegion(MockApp* app, int x, int y, int width,
                                int height, RegionType type, MenuAction action,
                                int menu_index, int item_index, int note_id) {
  if (app->click_region_count >= MAX_CLICK_REGIONS) return;
  ClickRegion* r = &app->click_regions[app->click_region_count++];
  r->pos.x = x;
  r->pos.y = y;
  r->size.width = width;
  r->size.height = height;
  r->type = type;
  r->action = action;
  r->menu_index = menu_index;
  r->item_index = item_index;
  r->note_id = note_id;
}

/** @brief Mirror of real ClearClickRegions. */
static void clearClickRegions(MockApp* app) { app->click_region_count = 0; }

/** @brief Mirror of real CreateScreen panel-width calculation. */
static int computePanelWidth(int screen_width, int panel_index) {
  int panels_width = screen_width / MAX_PANELS;
  int remainder_width = screen_width % MAX_PANELS;
  return panels_width + (panel_index < remainder_width ? 1 : 0);
}

/** @brief Mirror of real CreateScreen panel-X calculation. */
static int computePanelX(int screen_width, int panel_index) {
  int panels_width = screen_width / MAX_PANELS;
  int remainder_width = screen_width % MAX_PANELS;
  return (panels_width * panel_index) +
         (panel_index < remainder_width ? panel_index : remainder_width);
}

/**
 * ---------------------------------------------------------------------------
 * RegisterClickRegion
 * ---------------------------------------------------------------------------
 */

/** @brief Stored region data matches input values. */
static void test_register_click_region(void) {
  TEST("RegisterClickRegion stores region data");
  MockApp app;
  memset(&app, 0, sizeof(app));

  registerClickRegion(&app, 10, 20, 30, 40, REGION_MENU_ITEM, NULL, 1, 2, 3);
  ASSERT_EQ(app.click_region_count, 1);
  ASSERT_EQ(app.click_regions[0].pos.x, 10);
  ASSERT_EQ(app.click_regions[0].pos.y, 20);
  ASSERT_EQ(app.click_regions[0].size.width, 30);
  ASSERT_EQ(app.click_regions[0].size.height, 40);
  ASSERT_EQ(app.click_regions[0].type, REGION_MENU_ITEM);
  ASSERT_EQ(app.click_regions[0].menu_index, 1);
  ASSERT_EQ(app.click_regions[0].item_index, 2);
  ASSERT_EQ(app.click_regions[0].note_id, 3);
}

/** @brief Multiple registrations are stored sequentially. */
static void test_register_multiple_regions(void) {
  TEST("RegisterClickRegion stores multiple regions sequentially");
  MockApp app;
  memset(&app, 0, sizeof(app));

  registerClickRegion(&app, 0, 0, 10, 10, REGION_DIRECT, NULL, -1, -1, 0);
  registerClickRegion(&app, 5, 5, 20, 20, REGION_NOTE_ITEM, NULL, -1, -1, 42);
  registerClickRegion(&app, 0, 0, 15, 15, REGION_SLIDE_NAV, NULL, -1, -1, 0);

  ASSERT_EQ(app.click_region_count, 3);
  ASSERT_EQ(app.click_regions[1].type, REGION_NOTE_ITEM);
  ASSERT_EQ(app.click_regions[1].note_id, 42);
}

/** @brief Overflow past MAX_CLICK_REGIONS is silently ignored. */
static void test_register_region_overflow(void) {
  TEST("RegisterClickRegion stops at MAX_CLICK_REGIONS");
  MockApp app;
  memset(&app, 0, sizeof(app));

  for (int i = 0; i < MAX_CLICK_REGIONS + 5; i++)
    registerClickRegion(&app, i, 0, 1, 1, REGION_DIRECT, NULL, -1, -1, 0);

  ASSERT_EQ(app.click_region_count, MAX_CLICK_REGIONS);
}

/**
 * ---------------------------------------------------------------------------
 * ClearClickRegions
 * ---------------------------------------------------------------------------
 */

/** @brief ClearClickRegions resets count to 0. */
static void test_clear_click_regions(void) {
  TEST("ClearClickRegions resets count to 0");
  MockApp app;
  memset(&app, 0, sizeof(app));

  registerClickRegion(&app, 0, 0, 1, 1, REGION_DIRECT, NULL, -1, -1, 0);
  registerClickRegion(&app, 1, 0, 1, 1, REGION_DIRECT, NULL, -1, -1, 0);
  ASSERT_EQ(app.click_region_count, 2);

  clearClickRegions(&app);
  ASSERT_EQ(app.click_region_count, 0);
}

/**
 * ---------------------------------------------------------------------------
 * CreateScreen panel layout
 * ---------------------------------------------------------------------------
 */

/** @brief Even screen width divides equally between two panels. */
static void test_panel_width_even(void) {
  TEST("CreateScreen panel width: even screen width divides equally");
  ASSERT_EQ(computePanelWidth(80, 0), 40);
  ASSERT_EQ(computePanelWidth(80, 1), 40);
}

/** @brief Odd screen width gives first panel an extra pixel. */
static void test_panel_width_odd(void) {
  TEST("CreateScreen panel width: odd screen width gives first panel +1");
  ASSERT_EQ(computePanelWidth(81, 0), 41);
  ASSERT_EQ(computePanelWidth(81, 1), 40);
}

/** @brief Panel X positions start at 0 and span the screen width. */
static void test_panel_x_position(void) {
  TEST("CreateScreen panel X positions are calculated correctly");
  ASSERT_EQ(computePanelX(80, 0), 0);
  ASSERT_EQ(computePanelX(80, 1), 40);
}

/** @brief Panel X positions with odd width account for extra pixel. */
static void test_panel_x_position_odd(void) {
  TEST("CreateScreen panel X positions with odd width");
  ASSERT_EQ(computePanelX(81, 0), 0);
  ASSERT_EQ(computePanelX(81, 1), 41);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("ui");
  RUN_TEST(test_register_click_region,
           "RegisterClickRegion stores region data");
  RUN_TEST(test_register_multiple_regions,
           "RegisterClickRegion stores multiple regions");
  RUN_TEST(test_register_region_overflow,
           "RegisterClickRegion stops at MAX_CLICK_REGIONS");
  RUN_TEST(test_clear_click_regions,
           "ClearClickRegions resets count to 0");
  RUN_TEST(test_panel_width_even,
           "CreateScreen panel width: even screen width");
  RUN_TEST(test_panel_width_odd,
           "CreateScreen panel width: odd screen width");
  RUN_TEST(test_panel_x_position,
           "CreateScreen panel X positions");
  RUN_TEST(test_panel_x_position_odd,
           "CreateScreen panel X positions with odd width");
  return test_end();
}
