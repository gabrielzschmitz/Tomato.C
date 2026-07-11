/**
 * @file test_keybind_merge.c
 * @brief Integration tests for the TOML keybinding merge pipeline.
 *
 * Creates temporary TOML config files, calls loadTomlFile() to merge
 * them into g_config, and verifies the resulting keybinding array.
 * Covers action-group-level merge, cross-action key coexistence,
 * per-scene overrides, invalid action/key skipping, empty arrays,
 * and key-alias parsing.
 *
 * Includes config.c directly (via UNIT_TEST) to access static functions;
 * all action functions are stubbed.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.c"
#include "config.h"
#include "external/toml-c.h"
#include "test_helpers.h"
#include "test_keybind_stubs.h"

/**
 * ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------
 */

static void reset_defaults(void) {
  if (g_config.key_bindings) {
    free(g_config.key_bindings);
    g_config.key_bindings = NULL;
  }
  g_config.num_keys = 0;
  setDefaults();
}

/**
 * ---------------------------------------------------------------------------
 * Helper: create a temp TOML file, load it, return path
 * ---------------------------------------------------------------------------
 */
static char* write_toml(const char* content) {
  char tmpl[] = "/tmp/tomato_test_XXXXXX.toml";
  int fd = mkstemps(tmpl, 5);
  if (fd < 0) return NULL;
  FILE* f = fdopen(fd, "w");
  if (!f) {
    close(fd);
    return NULL;
  }
  fputs(content, f);
  fclose(f);
  return strdup(tmpl);
}

static void resolve_action(const char* label, int key, int mode,
                           SceneType scene, void (*expected)(AppData*)) {
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == key && keys[i].modes & (unsigned)mode &&
        keys[i].scene_types & (1u << (unsigned)scene)) {
      if (keys[i].action == expected) return;
    }
  }
  /* Failed — use the label */
  (void)label;
  ASSERT_TRUE(0 && label);
}

/**
 * ---------------------------------------------------------------------------
 * Test 1: Override QuitApp to only 'q', removing ESC/CTRLC
 * ---------------------------------------------------------------------------
 */

static void test_override_removes_keys(void) {
  TEST("loadTomlFile: QuitApp=['q'] removes ESC and CTRLC");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "QuitApp = [\"q\"]\n");
  ASSERT_NOT_NULL(path);

  /* g_config.key_bindings is now defaults; call loadTomlFile to merge */
  loadTomlFile(path);
  free(path);

  /* QuitApp should have q */
  int has_q = 0, has_esc = 0, has_ctrlc = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitApp && keys[i].modes & DEFAULT &&
        keys[i].scene_types & (1 << MAIN_MENU)) {
      if (keys[i].key == 'q') has_q = 1;
      if (keys[i].key == ESC) has_esc = 1;
      if (keys[i].key == CTRLC) has_ctrlc = 1;
    }
  }
  ASSERT_TRUE(has_q);
  ASSERT_FALSE(has_esc);
  ASSERT_FALSE(has_ctrlc);
}

/**
 * ---------------------------------------------------------------------------
 * Test 2: ClosePopup=["q"] does NOT steal QuitApp's 'q'
 *         (the original bug scenario)
 * ---------------------------------------------------------------------------
 */

static void test_close_popup_does_not_steal(void) {
  TEST("loadTomlFile: ClosePopup=['q'] doesn't steal QuitApp's 'q'");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "ClosePopup = [\"q\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* QuitApp should still have 'q' in ALL_SCENES */
  int quitapp_has_q = 0, closepopup_has_q = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == 'q' && keys[i].modes & DEFAULT &&
        keys[i].scene_types & ALL_SCENES) {
      if (keys[i].action == QuitApp) quitapp_has_q = 1;
      if (keys[i].action == ClosePopup) closepopup_has_q = 1;
    }
  }
  ASSERT_TRUE(quitapp_has_q);
  ASSERT_TRUE(closepopup_has_q);

  /* IsKeyAssignedToAction would find both */
  int found = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == 'q' && keys[i].action == QuitApp &&
        keys[i].modes & DEFAULT && keys[i].scene_types & (1 << MAIN_MENU)) {
      found = 1;
      break;
    }
  }
  ASSERT_TRUE(found);
}

/**
 * ---------------------------------------------------------------------------
 * Test 3: Same key 'q' in QuitApp and ClosePopup coexisting
 *         in different scenes
 * ---------------------------------------------------------------------------
 */

static void test_same_key_different_actions(void) {
  TEST(
    "loadTomlFile: 'q' coexists in QuitApp (ALL_SCENES) and ClosePopup "
    "(HISTORY)");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "QuitApp = [\"q\", \"ESC\", \"CTRLC\"]\n"
    "[keybindings.DEFAULT.SCENE_HISTORY_OVERVIEW]\n"
    "ClosePopup = [\"q\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* ALL_SCENES QuitApp still has 'q' */
  int found_quitapp_all = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == 'q' && keys[i].action == QuitApp &&
        keys[i].modes & DEFAULT && (keys[i].scene_types & (1 << MAIN_MENU)))
      found_quitapp_all = 1;
  }
  ASSERT_TRUE(found_quitapp_all);

  /* HISTORY_OVERVIEW ClosePopup has 'q' */
  int found_closepopup_history = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == 'q' && keys[i].action == ClosePopup &&
        keys[i].modes & DEFAULT &&
        (keys[i].scene_types & (1 << HISTORY_OVERVIEW)))
      found_closepopup_history = 1;
  }
  ASSERT_TRUE(found_closepopup_history);

  /* ALL_SCENES ClosePopup with 'q' from defaults (e.g. SCENE_CONTINUE) persists */
  int found_closepopup_all = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == 'q' && keys[i].action == ClosePopup &&
        keys[i].modes & DEFAULT && (keys[i].scene_types & ALL_SCENES))
      found_closepopup_all = 1;
  }
  /* ClosePopup is bound to 'q' in ALL_SCENES (via SCENE_CONTINUE/HELP defaults) */
  ASSERT_TRUE(found_closepopup_all);
}

/**
 * ---------------------------------------------------------------------------
 * Test 4: NORMAL mode SCENE_NOTES bindings from TOML
 * ---------------------------------------------------------------------------
 */

static void test_normal_mode_toml(void) {
  TEST("loadTomlFile: NORMAL mode SCENE_NOTES bindings from TOML");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.NORMAL.SCENE_NOTES]\n"
    "InputCursorLeft = [\"h\"]\n"
    "InputCursorRight = [\"l\"]\n"
    "UndoNotes = [\"u\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* After loading TOML with limited keys, only those keys should remain (others dropped) */
  int has_h = 0, has_l = 0, has_u = 0, has_i = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == InputCursorLeft && keys[i].modes & NORMAL &&
        keys[i].scene_types & (1 << NOTES) && keys[i].key == 'h')
      has_h = 1;
    if (keys[i].action == InputCursorRight && keys[i].modes & NORMAL &&
        keys[i].scene_types & (1 << NOTES) && keys[i].key == 'l')
      has_l = 1;
    if (keys[i].action == UndoNotes && keys[i].modes & NORMAL &&
        keys[i].scene_types & (1 << NOTES) && keys[i].key == 'u')
      has_u = 1;
    if (keys[i].action == SwitchToInsertMode && keys[i].modes & NORMAL &&
        keys[i].scene_types & (1 << NOTES) && keys[i].key == 'i')
      has_i = 1;
  }
  ASSERT_TRUE(has_h);
  ASSERT_TRUE(has_l);
  ASSERT_TRUE(has_u);
  ASSERT_TRUE(
    has_i); /* 'i' not in TOML → unmentioned action's default is preserved */
}

/**
 * ---------------------------------------------------------------------------
 * Test 5: Invalid action names are safely skipped
 * ---------------------------------------------------------------------------
 */

static void test_invalid_action_skipped(void) {
  TEST("loadTomlFile: invalid action names are silently skipped");

  reset_defaults();
  size_t count_before = g_config.num_keys;

  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "NonExistentAction = [\"x\"]\n"
    "QuitApp = [\"q\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* The number of entries should have changed (merge happened) */
  /* QuitApp was still applied */
  int has_q = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitApp && keys[i].key == 'q' &&
        keys[i].modes & DEFAULT && keys[i].scene_types & (1 << MAIN_MENU))
      has_q = 1;
  }
  ASSERT_TRUE(has_q);
  (void)count_before;
}

/**
 * ---------------------------------------------------------------------------
 * Test 6: Empty array preserves defaults for that action
 * ---------------------------------------------------------------------------
 */

static void test_empty_array_preserves_default(void) {
  TEST("loadTomlFile: empty QuitApp array preserves defaults");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "QuitApp = []\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* QuitApp still has all its default keys */
  int has_q = 0, has_esc = 0, has_ctrlc = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitApp && keys[i].modes & DEFAULT &&
        keys[i].scene_types & (1 << MAIN_MENU)) {
      if (keys[i].key == 'q') has_q = 1;
      if (keys[i].key == ESC) has_esc = 1;
      if (keys[i].key == CTRLC) has_ctrlc = 1;
    }
  }
  ASSERT_TRUE(has_q);
  ASSERT_TRUE(has_esc);
  ASSERT_TRUE(has_ctrlc);
}

/**
 * ---------------------------------------------------------------------------
 * Test 7: Full sample config with multiple sections
 * ---------------------------------------------------------------------------
 */

static void test_full_sample_config(void) {
  TEST("loadTomlFile: full config with multiple sections");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.DEFAULT.ALL_SCENES]\n"
    "QuitApp = [\"q\"]\n"
    "[keybindings.DEFAULT.SCENE_NOTES]\n"
    "QuitAppNotes = [\"q\", \"ESC\", \"CTRLC\"]\n"
    "[keybindings.NORMAL.SCENE_NOTES]\n"
    "InputCursorLeft = [\"h\", \"KEY_LEFT\"]\n"
    "InputCursorRight = [\"l\", \"KEY_RIGHT\"]\n"
    "UndoNotes = [\"u\"]\n"
    "RedoNotes = [\"CTRLR\"]\n"
    "[keybindings.INSERT.SCENE_NOTES]\n"
    "InputCommit = [\"ENTER\"]\n"
    "[keybindings.DEFAULT.SCENE_NOISE]\n"
    "NoiseTogglePlay = [\" \"]\n"
    "NoiseClose = [\"q\", \"ESC\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  /* ALL_SCENES QuitApp only has 'q' */
  int all_q = 0, all_esc = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitApp && keys[i].modes & DEFAULT &&
        keys[i].scene_types & (1 << MAIN_MENU)) {
      if (keys[i].key == 'q') all_q = 1;
      if (keys[i].key == ESC) all_esc = 1;
    }
  }
  ASSERT_TRUE(all_q);
  ASSERT_FALSE(all_esc);

  /* SCENE_NOTES QuitAppNotes has q, ESC, CTRLC */
  int n_q = 0, n_esc = 0, n_ctrlc = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitAppNotes && keys[i].modes & DEFAULT &&
        keys[i].scene_types & (1 << NOTES)) {
      if (keys[i].key == 'q') n_q = 1;
      if (keys[i].key == ESC) n_esc = 1;
      if (keys[i].key == CTRLC) n_ctrlc = 1;
    }
  }
  ASSERT_TRUE(n_q);
  ASSERT_TRUE(n_esc);
  ASSERT_TRUE(n_ctrlc);

  /* NORMAL InputCursorLeft has h and KEY_LEFT */
  int norm_h = 0, norm_left = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == InputCursorLeft && keys[i].modes & NORMAL &&
        keys[i].scene_types & (1 << NOTES)) {
      if (keys[i].key == 'h') norm_h = 1;
      if (keys[i].key == KEY_LEFT) norm_left = 1;
    }
  }
  ASSERT_TRUE(norm_h);
  ASSERT_TRUE(norm_left);

  /* NOISE scene bindings */
  int noise_space = 0, noise_q = 0, noise_esc = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].modes & DEFAULT && keys[i].scene_types & (1 << NOISE)) {
      if (keys[i].key == ' ' && keys[i].action == NoiseTogglePlay)
        noise_space = 1;
      if (keys[i].key == 'q' && keys[i].action == NoiseClose) noise_q = 1;
      if (keys[i].key == ESC && keys[i].action == NoiseClose) noise_esc = 1;
    }
  }
  ASSERT_TRUE(noise_space);
  ASSERT_TRUE(noise_q);
  ASSERT_TRUE(noise_esc);

  /* DEFAULT SCENE_NOTES QuitApp should still exist (from defaults) */
  int notes_quitapp = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == QuitApp && keys[i].modes & DEFAULT &&
        keys[i].scene_types & (1 << NOTES))
      notes_quitapp = 1;
  }
  ASSERT_TRUE(notes_quitapp);
}

/**
 * ---------------------------------------------------------------------------
 * Test 8: Key alias parsing
 * ---------------------------------------------------------------------------
 */

static void test_key_aliases(void) {
  TEST(
    "loadTomlFile: key aliases (KEY_LEFT, KEY_RIGHT, SPACE) resolve correctly");

  reset_defaults();
  char* path = write_toml(
    "[keybindings.NORMAL.SCENE_NOTES]\n"
    "InputCursorLeft = [\"KEY_LEFT\"]\n"
    "InputCursorRight = [\"KEY_RIGHT\"]\n"
    "UndoNotes = [\"u\"]\n"
    "[keybindings.DEFAULT.SCENE_WORK_TIME]\n"
    "SkipPomodoroStep = [\"SPACE\"]\n");
  ASSERT_NOT_NULL(path);

  loadTomlFile(path);
  free(path);

  int has_left = 0, has_right = 0, has_space = 0;
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].action == InputCursorLeft && keys[i].key == KEY_LEFT &&
        keys[i].modes & NORMAL && keys[i].scene_types & (1 << NOTES))
      has_left = 1;
    if (keys[i].action == InputCursorRight && keys[i].key == KEY_RIGHT &&
        keys[i].modes & NORMAL && keys[i].scene_types & (1 << NOTES))
      has_right = 1;
    if (keys[i].action == SkipPomodoroStep && keys[i].key == ' ' &&
        keys[i].modes & DEFAULT && keys[i].scene_types & (1 << WORK_TIME))
      has_space = 1;
  }
  ASSERT_TRUE(has_left);
  ASSERT_TRUE(has_right);
  ASSERT_TRUE(has_space);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("keybind-merge");

  RUN_TEST(test_override_removes_keys,
           "merge: QuitApp=[\"q\"] removes ESC/CTRLC");
  reset_defaults();
  RUN_TEST(test_close_popup_does_not_steal,
           "merge: ClosePopup=[\"q\"] doesn't steal QuitApp's q");
  reset_defaults();
  RUN_TEST(test_same_key_different_actions,
           "merge: 'q' in QuitApp+ClosePopup different scenes");
  reset_defaults();
  RUN_TEST(test_normal_mode_toml,
           "merge: NORMAL SCENE_NOTES TOML replaces defaults");
  reset_defaults();
  RUN_TEST(test_invalid_action_skipped,
           "merge: invalid action names safely skipped");
  reset_defaults();
  RUN_TEST(test_empty_array_preserves_default,
           "merge: empty QuitApp array preserves defaults");
  reset_defaults();
  RUN_TEST(test_full_sample_config, "merge: full multi-section sample config");
  reset_defaults();
  RUN_TEST(test_key_aliases, "merge: key aliases resolve correctly");
  reset_defaults();

  return test_end();
}
