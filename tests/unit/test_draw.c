/**
 * @file test_draw.c
 * @brief Unit tests for the drawing module.
 *
 * Links against the real draw.c + util.c.  All external dependencies
 * (ui.c, error.c, input.c, anim.c, init.c, notes.c, bar.c) are stubbed
 * so DrawScreen can be exercised without a full app context.
 *
 * Tests validate that:
 *   - Screen-too-small is NON-FATAL (DrawScreen returns NO_ERROR)
 *   - ValidateAndRenderScreenSize boundary conditions
 *   - block_input flag is set/unset correctly
 */

#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "anim.h"
#include "bar.h"
#include "config.h"
#include "draw.h"
#include "error.h"
#include "history.h"
#include "init.h"
#include "input.h"
#include "notes.h"
#include "test_helpers.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* Required global - draw.c (via config.h) and util.c reference g_config */
Config g_config;

/* -----------------------------------------------------------------------
 * Stubs for external functions called by draw.c / util.c
 * ----------------------------------------------------------------------- */

/* From ui.c */
void ClearClickRegions(AppData* app) {
  if (app) app->click_region_count = 0;
}
void RegisterClickRegion(AppData* app, int x, int y, int width, int height,
                         RegionType type, MenuAction action, int menu_index,
                         int item_index, int note_id) {
  (void)app;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  (void)type;
  (void)action;
  (void)menu_index;
  (void)item_index;
  (void)note_id;
}
void RenderScreenSizeError(Screen* screen) { (void)screen; }
void RenderPanelBorder(Panel panel, Border border) {
  (void)panel;
  (void)border;
}
void RenderAnimationAtPanelCenter(Panel* panel, Rollfilm* animation,
                                  Vector2D offset) {
  (void)panel;
  (void)animation;
  (void)offset;
}
void RenderStatusBar(const StatusBar* status_bar, const Screen* screen,
                     bool has_error_line) {
  (void)status_bar;
  (void)screen;
  (void)has_error_line;
}
void RenderPomodoroStatus(AppData* app, Dimensions anim_size,
                          Vector2D anim_pos) {
  (void)app;
  (void)anim_size;
  (void)anim_pos;
}
void RenderQuitConfirmation(AppData* app) { (void)app; }
void RenderSkipConfirmation(AppData* app) { (void)app; }
void RenderResetMenu(AppData* app) { (void)app; }
void PrintMenuAtCenter(AppData* app, Panel* panel, Menu* menu, Vector2D offset,
                       int line_spacing) {
  (void)app;
  (void)panel;
  (void)menu;
  (void)offset;
  (void)line_spacing;
}
void RenderFloatingDialog(AppData* app, FloatingDialog* dialog) {
  (void)app;
  (void)dialog;
}
void UpdateFloatingDialog(FloatingDialog* dialog, Screen* screen) {
  (void)dialog;
  (void)screen;
}

/* From notes.c */
void RenderNotes(AppData* app, NotesData* notes, int start_x, int start_y,
                 int end_x, int end_y, InputState* input, int mode) {
  (void)app;
  (void)notes;
  (void)start_x;
  (void)start_y;
  (void)end_x;
  (void)end_y;
  (void)input;
  (void)mode;
}
void RenderNotesHistoryDebug(NotesData* notes, int x, int y) {
  (void)notes;
  (void)x;
  (void)y;
}

/* From error.c */
void SetError(AppData* app, const char* context, ErrorType type) {
  (void)app;
  (void)context;
  (void)type;
}
void LogError(const char* context, ErrorType error) {
  (void)context;
  (void)error;
}
bool HasErrors(void) { return false; }
void RenderErrorLine(void) {}

/* From input.c */
int IsKeyAssignedToAction(int key, void (*action)(AppData*)) {
  (void)key;
  (void)action;
  return 0; /* no keys assigned - renderPopups becomes a no-op */
}

/* From anim.c */
int RollfilmLargest(Rollfilm** animations, int* indices, int count) {
  (void)animations;
  (void)indices;
  (void)count;
  return -1;
}
void RollfilmSeekFrame(Rollfilm* animation, int frame) {
  (void)animation;
  (void)frame;
}
bool RollfilmFirstBlank(Rollfilm* animation, int* x, int* y) {
  (void)animation;
  *x = 0;
  *y = 0;
  return false;
}
bool RollfilmLastBlank(Rollfilm* animation, int* x, int* y) {
  (void)animation;
  *x = 0;
  *y = 0;
  return false;
}

/* From input.c - action function pointers (never dereferenced at runtime) */
void QuitApp(AppData* app) { (void)app; }
void ForcefullyQuitApp(AppData* app) { (void)app; }
void ResetPomodoroStep(AppData* app) { (void)app; }
void ResetPomodoroCycle(AppData* app) { (void)app; }
void SkipPomodoroStep(AppData* app) { (void)app; }
void ForcefullySkipPomodoroStep(AppData* app) { (void)app; }
void NotesNextPage(AppData* app) { (void)app; }
void NotesPrevPage(AppData* app) { (void)app; }
void RenderPomodoroControls(AppData* app, Vector2D pos) {
  (void)app;
  (void)pos;
}

/* From init.c - stub with safe defaults */
Border InitBorder(void) {
  Border border;
  memset(&border, 0, sizeof(border));
  border.top_left = "+";
  border.top_right = "+";
  border.bottom_left = "+";
  border.bottom_right = "+";
  border.horizontal = "-";
  border.vertical = "|";
  return border;
}

/* -----------------------------------------------------------------------
 * Test helpers - build a minimal AppData whose only purpose is to drive
 * DrawScreen through the screen-too-small path.
 * ----------------------------------------------------------------------- */

static History* createMockHistory(void) {
  History* h = (History*)malloc(sizeof(History));
  if (h) {
    h->past = NULL;
    h->future = NULL;
    h->present = MAIN_MENU; /* scene 0 */
  }
  return h;
}

static void setupAppData(AppData* app, Screen* screen, int width, int height,
                         int min_width, int min_height) {
  memset(app, 0, sizeof(AppData));
  memset(screen, 0, sizeof(Screen));

  screen->size.width = width;
  screen->size.height = height;
  screen->min_panel_size.width = min_width;
  screen->min_panel_size.height = min_height;
  screen->current_panel = 0;

  /* Panel 0 - minimal valid state */
  screen->panels[0].mode = DEFAULT; /* 1 */
  screen->panels[0].input = NULL;
  screen->panels[0].visible = true;
  screen->panels[0].scene_history = createMockHistory();

  app->screen = screen;
  app->notes = NULL;
  app->user_input = 0;
  app->last_input = 0;
  app->pomodoro_data.current_step = WORK_TIME;
  app->status_bar = NULL;
  app->popup_dialog = NULL;
  app->block_input = false;
  app->running = true;
}

/* -----------------------------------------------------------------------
 * Tests - DrawScreen return value with real code
 * ----------------------------------------------------------------------- */

/** @brief DrawScreen returns NO_ERROR (not DRAW_ERROR) when width too small. */
static void test_drawscreen_too_small_width_returns_no_error(void) {
  TEST("DrawScreen returns NO_ERROR when width too small");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 10, 40, 20, 10);

  ErrorType result = DrawScreen(&app);
  ASSERT_EQ((int)result, (int)NO_ERROR);
  ASSERT_NE((int)result, (int)DRAW_ERROR);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief DrawScreen returns NO_ERROR (not DRAW_ERROR) when height too small. */
static void test_drawscreen_too_small_height_returns_no_error(void) {
  TEST("DrawScreen returns NO_ERROR when height too small");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 30, 5, 20, 10);

  ErrorType result = DrawScreen(&app);
  ASSERT_EQ((int)result, (int)NO_ERROR);
  ASSERT_NE((int)result, (int)DRAW_ERROR);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief DrawScreen returns NO_ERROR at exact minimum dimensions. */
static void test_drawscreen_exact_minimum_returns_no_error(void) {
  TEST("DrawScreen returns NO_ERROR at exact minimum");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 20, 10, 20, 10);

  ErrorType result = DrawScreen(&app);
  ASSERT_EQ((int)result, (int)NO_ERROR);
  ASSERT_NE((int)result, (int)DRAW_ERROR);
  ASSERT_FALSE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief DrawScreen returns NO_ERROR at zero size (non-fatal edge case). */
static void test_drawscreen_zero_size_returns_no_error(void) {
  TEST("DrawScreen returns NO_ERROR at zero size (non-fatal)");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 0, 0, 20, 10);

  ErrorType result = DrawScreen(&app);
  ASSERT_EQ((int)result, (int)NO_ERROR);
  ASSERT_NE((int)result, (int)DRAW_ERROR);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/* -----------------------------------------------------------------------
 * Tests - ValidateAndRenderScreenSize direct
 * ----------------------------------------------------------------------- */

/** @brief ValidateAndRenderScreenSize returns false and blocks input for too-small width. */
static void test_validate_too_small_width(void) {
  TEST("ValidateAndRenderScreenSize false when width too small");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 10, 40, 20, 10);

  bool ok = ValidateAndRenderScreenSize(&app);
  ASSERT_FALSE(ok);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief ValidateAndRenderScreenSize returns false and blocks input for too-small height. */
static void test_validate_too_small_height(void) {
  TEST("ValidateAndRenderScreenSize false when height too small");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 30, 5, 20, 10);

  bool ok = ValidateAndRenderScreenSize(&app);
  ASSERT_FALSE(ok);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief ValidateAndRenderScreenSize returns true and unblocks input at exact minimum. */
static void test_validate_exact_minimum(void) {
  TEST("ValidateAndRenderScreenSize true at exact minimum");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 20, 10, 20, 10);
  app.block_input = true;

  bool ok = ValidateAndRenderScreenSize(&app);
  ASSERT_TRUE(ok);
  ASSERT_FALSE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief ValidateAndRenderScreenSize returns true when screen is larger than minimum. */
static void test_validate_larger_than_minimum(void) {
  TEST("ValidateAndRenderScreenSize true when screen is large enough");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 80, 24, 20, 10);

  bool ok = ValidateAndRenderScreenSize(&app);
  ASSERT_TRUE(ok);
  ASSERT_FALSE(app.block_input);

  free(screen.panels[0].scene_history);
}

/** @brief ValidateAndRenderScreenSize returns false at zero size. */
static void test_validate_zero_size(void) {
  TEST("ValidateAndRenderScreenSize false at zero size");
  AppData app;
  Screen screen;
  setupAppData(&app, &screen, 0, 0, 20, 10);

  bool ok = ValidateAndRenderScreenSize(&app);
  ASSERT_FALSE(ok);
  ASSERT_TRUE(app.block_input);

  free(screen.panels[0].scene_history);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  /* ncurses needs a TERM and initscr() for erase()/refresh()/mvprintw() */
  setenv("TERM", "dumb", 1);
  SCREEN* scr = newterm(NULL, stdout, stdin);
  if (scr == NULL) {
    test_begin("draw");
    printf("  (skipped: ncurses not available)\n");
    return test_end();
  }
  test_begin("draw");

  /* DrawScreen return-value tests (real code) */
  RUN_TEST(test_drawscreen_too_small_width_returns_no_error,
           "DrawScreen returns NO_ERROR when width too small");
  RUN_TEST(test_drawscreen_too_small_height_returns_no_error,
           "DrawScreen returns NO_ERROR when height too small");
  RUN_TEST(test_drawscreen_exact_minimum_returns_no_error,
           "DrawScreen returns NO_ERROR at exact minimum");
  RUN_TEST(test_drawscreen_zero_size_returns_no_error,
           "DrawScreen returns NO_ERROR at zero size (non-fatal)");

  /* ValidateAndRenderScreenSize tests (real code) */
  RUN_TEST(test_validate_too_small_width,
           "ValidateAndRenderScreenSize false when width too small");
  RUN_TEST(test_validate_too_small_height,
           "ValidateAndRenderScreenSize false when height too small");
  RUN_TEST(test_validate_exact_minimum,
           "ValidateAndRenderScreenSize true at exact minimum");
  RUN_TEST(test_validate_larger_than_minimum,
           "ValidateAndRenderScreenSize true when screen is large enough");
  RUN_TEST(test_validate_zero_size,
           "ValidateAndRenderScreenSize false at zero size");

  endwin();
  return test_end();
}
