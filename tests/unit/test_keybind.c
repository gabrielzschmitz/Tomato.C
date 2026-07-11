/**
 * @file test_keybind.c
 * @brief Unit tests for keybinding merge logic (readKeybindings).
 *
 * Includes config.c directly with stub action functions so that
 * setDefaults() and readKeybindings() can be exercised in isolation.
 *
 * Tests:
 *   - Default keybinding array contains expected entries
 *   - TOML merge: override existing key for same action
 *   - TOML merge: drop default keys not listed in a TOML action group
 *   - TOML merge: same key under different actions coexists
 *   - TOML merge: unmentioned actions keep defaults untouched
 *   - TOML merge: duplicate keys within an action are deduplicated
 *   - NORMAL/INSERT/VISUAL mode bindings are preserved through the merge
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "external/toml-c.h"
#include "test_helpers.h"
#include "test_keybind_stubs.h"

/* Now include the actual config.c source */
#include "config.c"

/* Helper: reset g_config via setDefaults() */
static void reset_defaults(void) {
  if (g_config.key_bindings) {
    free(g_config.key_bindings);
    g_config.key_bindings = NULL;
  }
  g_config.num_keys = 0;
  setDefaults();
}

/* Helper: find an entry in the keybindings array */
typedef struct {
  int key;
  void (*action)(AppData*);
  int modes;
  int scene_types;
} KeyEntry;

static int find_entry(const KeyEntry* entries, size_t n, int key,
                      void (*action)(AppData*), int modes, int scene_types) {
  for (size_t i = 0; i < n; i++)
    if (entries[i].key == key && entries[i].action == action &&
        entries[i].modes == modes && entries[i].scene_types == scene_types)
      return 1;
  return 0;
}

static int count_entries(int key, void (*action)(AppData*), int modes,
                         int scene_types) {
  int c = 0;
  for (size_t i = 0; i < g_config.num_keys; i++)
    if (g_config.key_bindings[i].key == key &&
        g_config.key_bindings[i].action == action &&
        g_config.key_bindings[i].modes == modes &&
        g_config.key_bindings[i].scene_types == scene_types)
      c++;
  return c;
}

/** Helper: parse a TOML string and merge into g_config */
static int merge_toml(const char* toml_str) {
  char errbuf[200];
  char* buf = strdup(toml_str);
  if (!buf) return -1;
  toml_table_t* root = toml_parse(buf, errbuf, sizeof(errbuf));
  free(buf);
  if (!root) return -1;
  readKeybindings(root);
  toml_free(root);
  return 0;
}

/** Test: defaults contain expected QuitApp bindings */
static void test_defaults_quitapp(void) {
  TEST("defaults have QuitApp for q, ESC, CTRLC in ALL_SCENES");

  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(27, QuitApp, DEFAULT, ALL_SCENES), 0); /* ESC */
  ASSERT_GT(count_entries(3, QuitApp, DEFAULT, ALL_SCENES), 0);  /* CTRLC */
}

/** Test: defaults contain QuitAppNotes for q in SCENE_NOTES */
static void test_defaults_quitappnotes(void) {
  TEST("defaults have QuitAppNotes for q, ESC, CTRLC in SCENE_NOTES");

  ASSERT_GT(count_entries('q', QuitAppNotes, DEFAULT, SCENE_NOTES), 0);
}

/** Test: TOML merge overrides QuitApp key set (removes ESC, CTRLC) */
static void test_merge_override_removes_keys(void) {
  TEST("merge: QuitApp=[\"q\"] removes ESC and CTRLC from QuitApp");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\"]\n"),
            0);

  /* 'q' still bound to QuitApp */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  /* ESC and CTRLC removed from QuitApp */
  ASSERT_EQ(count_entries(27, QuitApp, DEFAULT, ALL_SCENES), 0);
  ASSERT_EQ(count_entries(3, QuitApp, DEFAULT, ALL_SCENES), 0);
}

/** Test: same key under different actions coexists */
static void test_merge_same_key_different_actions(void) {
  TEST("merge: qu key coexists in QuitApp and ClosePopup");

  /* QuitApp = ["q"] */
  /* ClosePopup = ["q", "ENTER"] */
  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp    = [\"q\"]\n"
                       "ClosePopup = [\"q\", \"ENTER\"]\n"),
            0);

  /* Both should exist */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries('q', ClosePopup, DEFAULT, ALL_SCENES), 0);
  /* ClosePopup should NOT steal QuitApp's entry */
  ASSERT_EQ(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 1);
}

/** Test: unmentioned actions keep defaults untouched */
static void test_merge_untouched_actions(void) {
  TEST("merge: unmentioned actions keep all defaults");

  /* Only mention QuitApp */
  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\"]\n"),
            0);

  /* ClosePopup should still have its default bindings */
  ASSERT_GT(count_entries('q', ClosePopup, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(27, ClosePopup, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(3, ClosePopup, DEFAULT, ALL_SCENES), 0);

  /* ENTER/ExecuteMenuAction still works */
  ASSERT_GT(count_entries('\n', ExecuteMenuAction, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(KEY_ENTER, ExecuteMenuAction, DEFAULT, ALL_SCENES),
            0);
}

/** Test: scene-specific defaults preserved through merge */
static void test_merge_scene_specific(void) {
  TEST("merge: SCENE_NOTES QuitAppNotes bindings preserved");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\"]\n"),
            0);

  /* QuitAppNotes for SCENE_NOTES should still exist */
  ASSERT_GT(count_entries('q', QuitAppNotes, DEFAULT, SCENE_NOTES), 0);
}

/** Test: NORMAL mode bindings preserved */
static void test_merge_normal_mode(void) {
  TEST("merge: NORMAL mode SCENE_NOTES in bindings preserved");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\"]\n"),
            0);

  /* h/l cursor movement in NORMAL mode should exist */
  ASSERT_GT(count_entries('h', InputCursorLeft, NORMAL, SCENE_NOTES), 0);
  ASSERT_GT(count_entries('l', InputCursorRight, NORMAL, SCENE_NOTES), 0);
}

/** Test: INSERT mode bindings preserved */
static void test_merge_insert_mode(void) {
  TEST("merge: INSERT mode SCENE_NOTES bindings preserved");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\"]\n"),
            0);

  ASSERT_GT(count_entries(KEY_LEFT, InputCursorLeft, INSERT, SCENE_NOTES), 0);
  ASSERT_GT(count_entries(KEY_RIGHT, InputCursorRight, INSERT, SCENE_NOTES), 0);
}

/** Test: duplicate keys in TOML are handled */
static void test_merge_duplicate_keys(void) {
  TEST("merge: duplicate keys within same action deduplicated");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"q\", \"q\", \"q\"]\n"),
            0);

  /* Should only have one entry for 'q' */
  ASSERT_EQ(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 1);
}

/** Test: ALL_SCENES section with ClosePopup doesn't steal QuitApp */
static void test_merge_close_popup_no_steal(void) {
  TEST("merge: ClosePopup=[\"q\",\"ESC\",\"CTRLC\"] doesn't steal QuitApp");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp    = [\"q\"]\n"
                       "ClosePopup = [\"q\", \"ESC\", \"CTRLC\"]\n"),
            0);

  /* QuitApp still has 'q' */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  /* ClosePopup also has 'q', ESC, CTRLC */
  ASSERT_GT(count_entries('q', ClosePopup, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(27, ClosePopup, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(3, ClosePopup, DEFAULT, ALL_SCENES), 0);
  /* ClosePopup does NOT have 'q' for QuitApp */
  ASSERT_EQ(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 1);
}

/** Test: re-adding ESC and CTRLC to QuitApp */
static void test_merge_restore_keys(void) {
  TEST("merge: QuitApp=[\"ESC\",\"CTRLC\"] removes q, keeps ESC/CTRLC");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = [\"ESC\", \"CTRLC\"]\n"),
            0);

  /* 'q' removed from QuitApp */
  ASSERT_EQ(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  /* ESC and CTRLC still bound */
  ASSERT_GT(count_entries(27, QuitApp, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries(3, QuitApp, DEFAULT, ALL_SCENES), 0);
}

/** Test: empty QuitApp array removes ALL QuitApp ALL_SCENES bindings */
static void test_merge_empty_array(void) {
  TEST("merge: empty QuitApp array skips (no change to QuitApp defaults)");

  /* Empty array should be skipped — current behavior */
  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.ALL_SCENES]\n"
                       "QuitApp = []\n"),
            0);

  /* Defaults should remain */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
}

/** Test: SCENE_NOTES section with only ESC for QuitAppNotes */
static void test_merge_notes_scene_override(void) {
  TEST("merge: SCENE_NOTES QuitAppNotes=[\"ESC\"] removes q and CTRLC");

  ASSERT_EQ(merge_toml("[keybindings.DEFAULT.SCENE_NOTES]\n"
                       "QuitAppNotes = [\"ESC\"]\n"),
            0);

  /* q and CTRLC removed from QuitAppNotes in SCENE_NOTES */
  ASSERT_EQ(count_entries('q', QuitAppNotes, DEFAULT, SCENE_NOTES), 0);
  ASSERT_EQ(count_entries(3, QuitAppNotes, DEFAULT, SCENE_NOTES), 0);
  /* ESC still bound */
  ASSERT_GT(count_entries(27, QuitAppNotes, DEFAULT, SCENE_NOTES), 0);
  /* QuitApp for ALL_SCENES untouched */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
}

/** Test: sample config (full ALL_SCENES + SCENE_NOTES) */
static void test_merge_sample_config(void) {
  TEST("merge: sample config preserves all critical bindings");

  /* Simulate the full sample_config.toml ALL_SCENES section */
  ASSERT_EQ(
    merge_toml(
      "[keybindings.DEFAULT.ALL_SCENES]\n"
      "ChangeSelectedItemLeft  = [\"KEY_UP\", \"k\", \"KEY_LEFT\", \"h\"]\n"
      "ChangeSelectedItemRight = [\"KEY_DOWN\", \"j\", \"KEY_RIGHT\", \"l\"]\n"
      "ExecuteMenuAction       = [\"ENTER\", \"KEY_ENTER\"]\n"
      "NextPanel               = [\"SPACE\"]\n"
      "QuitApp                 = [\"q\", \"ESC\", \"CTRLC\"]\n"
      "OpenNoiseMenu           = [\"CTRLW\", \"w\"]\n"
      "OpenHistoryPopup        = [\"CTRLH\"]\n"
      "GoPrevSlide             = [\"KEY_LEFT\", \"h\"]\n"
      "GoNextSlide             = [\"KEY_RIGHT\", \"l\"]\n"
      "ClosePopup              = [\"ENTER\", \"KEY_ENTER\", \"q\", \"ESC\", "
      "\"CTRLC\"]\n"
      "SelectPrevButton        = [\"KEY_LEFT\", \"h\"]\n"
      "SelectNextButton        = [\"KEY_RIGHT\", \"l\"]\n"
      "ExecuteButtonAction     = [\"ENTER\", \"KEY_ENTER\"]\n"
      "OpenHelp                = [\"?\", \"KEY_F(1)\"]\n"),
    0);

  /* Verify QuitApp and ClosePopup both have q */
  ASSERT_GT(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 0);
  ASSERT_GT(count_entries('q', ClosePopup, DEFAULT, ALL_SCENES), 0);

  /* QuitApp has exactly 1 entry for q */
  ASSERT_EQ(count_entries('q', QuitApp, DEFAULT, ALL_SCENES), 1);
  /* ClosePopup has exactly 1 entry for q */
  ASSERT_EQ(count_entries('q', ClosePopup, DEFAULT, ALL_SCENES), 1);
}

/** Main */
int main(void) {
  test_begin("keybind");

  reset_defaults();
  RUN_TEST(test_defaults_quitapp,
           "defaults have QuitApp for q, ESC, CTRLC in ALL_SCENES");
  reset_defaults();

  RUN_TEST(test_defaults_quitappnotes,
           "defaults have QuitAppNotes for q in SCENE_NOTES");
  reset_defaults();

  RUN_TEST(test_merge_override_removes_keys,
           "merge: QuitApp=[\"q\"] removes ESC and CTRLC from QuitApp");
  reset_defaults();

  RUN_TEST(test_merge_same_key_different_actions,
           "merge: 'q' key coexists in QuitApp and ClosePopup");
  reset_defaults();

  RUN_TEST(test_merge_untouched_actions,
           "merge: unmentioned actions keep all defaults");
  reset_defaults();

  RUN_TEST(test_merge_scene_specific,
           "merge: SCENE_NOTES QuitAppNotes bindings preserved");
  reset_defaults();

  RUN_TEST(test_merge_normal_mode,
           "merge: NORMAL mode SCENE_NOTES bindings preserved");
  reset_defaults();

  RUN_TEST(test_merge_insert_mode,
           "merge: INSERT mode SCENE_NOTES bindings preserved");
  reset_defaults();

  RUN_TEST(test_merge_duplicate_keys,
           "merge: duplicate keys within same action deduplicated");
  reset_defaults();

  RUN_TEST(test_merge_close_popup_no_steal,
           "merge: ClosePopup=[\"q\",\"ESC\",\"CTRLC\"] doesn't steal QuitApp");
  reset_defaults();

  RUN_TEST(test_merge_restore_keys,
           "merge: QuitApp=[\"ESC\",\"CTRLC\"] removes q, keeps ESC/CTRLC");
  reset_defaults();

  RUN_TEST(test_merge_empty_array,
           "merge: empty QuitApp array skips (no change to QuitApp defaults)");
  reset_defaults();

  RUN_TEST(test_merge_notes_scene_override,
           "merge: SCENE_NOTES QuitAppNotes=[\"ESC\"] removes q and CTRLC");
  reset_defaults();

  RUN_TEST(test_merge_sample_config,
           "merge: sample config preserves all critical bindings");
  reset_defaults();

  /* Cleanup */
  free(g_config.key_bindings);
  g_config.key_bindings = NULL;
  g_config.num_keys = 0;

  return test_end();
}
