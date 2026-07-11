/**
 * @file test_keybind_resolution.c
 * @brief Integration tests for key-to-action resolution logic.
 *
 * Implements the same linear-scan search used by ProcessKeyInput
 * (input.c:47-78) to verify that every key/mode/scene combination
 * resolves to the correct action function pointer.  Also tests the
 * NORMAL-to-DEFAULT fallback and INSERT printable-character sentinel
 * dispatch.
 *
 * Includes config.c directly (via UNIT_TEST) to access setDefaults()
 * and the keys[] array; all action functions are stubbed.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

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

/*
 * Resolution helpers — mirror ProcessKeyInput's search logic from input.c:47-78
 *
 * resolve_raw:           core linear scan, matches (key, mode_bitmask, scene_index)
 * resolve_with_fallback: adds NORMAL→DEFAULT fallback when input_len==0
 * resolve_insert:        INSERT printable char sentinel lookup (key == -1)
 * resolve_process:       full ProcessKeyInput resolution path
 */
static void (*resolve_raw(int key, int mode, SceneType scene))(AppData*) {
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == key && keys[i].modes & (unsigned)mode &&
        keys[i].scene_types & (1u << (unsigned)scene)) {
      return keys[i].action;
    }
  }
  return NULL;
}

static void (*resolve_with_fallback(int key, int mode, SceneType scene,
                                    size_t input_len))(AppData*) {
  int effective_mode = mode;
  if (mode == NORMAL && input_len == 0) effective_mode = DEFAULT;
  return resolve_raw(key, effective_mode, scene);
}

static void (*resolve_insert(SceneType scene))(AppData*) {
  for (size_t i = 0; i < g_config.num_keys; i++) {
    if (keys[i].key == -1 && keys[i].modes & INSERT &&
        keys[i].scene_types & (1u << (unsigned)scene)) {
      return keys[i].action;
    }
  }
  return NULL;
}

static void (*resolve_process(int key, int mode, SceneType scene,
                              size_t input_len))(AppData*) {
  if (mode == INSERT && key >= ' ' && key <= '~') {
    void* sentinel = resolve_insert(scene);
    if (sentinel) return sentinel;
  }
  return resolve_with_fallback(key, mode, scene, input_len);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — ALL_SCENES DEFAULT bindings
 * ---------------------------------------------------------------------------
 */

static void test_default_quitapp_q(void) {
  TEST("DEFAULT 'q' in MAIN_MENU → QuitApp (ALL_SCENES)");
  ASSERT_EQ(resolve_raw('q', DEFAULT, MAIN_MENU), QuitApp);
}

static void test_default_quitapp_esc(void) {
  TEST("DEFAULT ESC in MAIN_MENU → QuitApp (ALL_SCENES)");
  ASSERT_EQ(resolve_raw(ESC, DEFAULT, MAIN_MENU), QuitApp);
}

static void test_default_quitapp_ctrlc(void) {
  TEST("DEFAULT CTRLC in MAIN_MENU → QuitApp (ALL_SCENES)");
  ASSERT_EQ(resolve_raw(CTRLC, DEFAULT, MAIN_MENU), QuitApp);
}

static void test_default_quitapp_all_scenes(void) {
  TEST("DEFAULT 'q' resolves QuitApp across ALL_SCENES (except NOTES)");
  ASSERT_EQ(resolve_raw('q', DEFAULT, MAIN_MENU), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, WORK_TIME), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, SHORT_PAUSE), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, LONG_PAUSE), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, HELP), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, CONTINUE), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, NOTES_TRANSITION), QuitApp);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — SCENE_NOTES overrides ALL_SCENES for same key
 * ---------------------------------------------------------------------------
 */

static void test_notes_quitappnotes(void) {
  TEST("DEFAULT 'q' in NOTES → QuitAppNotes (SCENE_NOTES override)");
  ASSERT_EQ(resolve_raw('q', DEFAULT, NOTES), QuitAppNotes);
}

static void test_notes_esc_to_quitappnotes(void) {
  TEST("DEFAULT ESC in NOTES → QuitAppNotes (SCENE_NOTES override)");
  ASSERT_EQ(resolve_raw(ESC, DEFAULT, NOTES), QuitAppNotes);
}

static void test_notes_ctrlc_to_quitappnotes(void) {
  TEST("DEFAULT CTRLC in NOTES → QuitAppNotes (SCENE_NOTES override)");
  ASSERT_EQ(resolve_raw(CTRLC, DEFAULT, NOTES), QuitAppNotes);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — NORMAL mode in SCENE_NOTES
 * ---------------------------------------------------------------------------
 */

static void test_normal_h_cursor_left(void) {
  TEST("NORMAL 'h' in NOTES → InputCursorLeft");
  ASSERT_EQ(resolve_raw('h', NORMAL, NOTES), InputCursorLeft);
}

static void test_normal_l_cursor_right(void) {
  TEST("NORMAL 'l' in NOTES → InputCursorRight");
  ASSERT_EQ(resolve_raw('l', NORMAL, NOTES), InputCursorRight);
}

static void test_normal_x_delete(void) {
  TEST("NORMAL 'x' in NOTES → InputDeleteChar");
  ASSERT_EQ(resolve_raw('x', NORMAL, NOTES), InputDeleteChar);
}

static void test_normal_i_insert(void) {
  TEST("NORMAL 'i' in NOTES → SwitchToInsertMode");
  ASSERT_EQ(resolve_raw('i', NORMAL, NOTES), SwitchToInsertMode);
}

static void test_normal_u_undo(void) {
  TEST("NORMAL 'u' in NOTES → UndoNotes");
  ASSERT_EQ(resolve_raw('u', NORMAL, NOTES), UndoNotes);
}

static void test_normal_ctrlr_redo(void) {
  TEST("NORMAL CTRLR in NOTES → RedoNotes");
  ASSERT_EQ(resolve_raw(CTRLR, NORMAL, NOTES), RedoNotes);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — INSERT mode in SCENE_NOTES
 * ---------------------------------------------------------------------------
 */

static void test_insert_printable_char(void) {
  TEST("INSERT 'a' in NOTES → InputInsertChar (sentinel -1)");
  ASSERT_EQ(resolve_process('a', INSERT, NOTES, 0), InputInsertChar);
}

static void test_insert_esc(void) {
  TEST("INSERT ESC in NOTES → InputESC");
  ASSERT_EQ(resolve_process(ESC, INSERT, NOTES, 0), InputESC);
}

static void test_insert_ctrlc(void) {
  TEST("INSERT CTRLC in NOTES → InputESC");
  ASSERT_EQ(resolve_process(CTRLC, INSERT, NOTES, 0), InputESC);
}

static void test_insert_enter(void) {
  TEST("INSERT ENTER in NOTES → InputCommit");
  ASSERT_EQ(resolve_process(ENTER, INSERT, NOTES, 0), InputCommit);
}

static void test_insert_backspace(void) {
  TEST("INSERT KEY_BACKSPACE in NOTES → InputBackspace");
  ASSERT_EQ(resolve_process(KEY_BACKSPACE, INSERT, NOTES, 0), InputBackspace);
}

static void test_insert_key_left(void) {
  TEST("INSERT KEY_LEFT in NOTES → InputCursorLeft");
  ASSERT_EQ(resolve_process(KEY_LEFT, INSERT, NOTES, 0), InputCursorLeft);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — VISUAL mode in SCENE_NOTES
 * ---------------------------------------------------------------------------
 */

static void test_visual_h_cursor_left(void) {
  TEST("VISUAL 'h' in NOTES → InputCursorLeft");
  ASSERT_EQ(resolve_raw('h', VISUAL, NOTES), InputCursorLeft);
}

static void test_visual_i_insert(void) {
  TEST("VISUAL 'i' in NOTES → SwitchToInsertMode");
  ASSERT_EQ(resolve_raw('i', VISUAL, NOTES), SwitchToInsertMode);
}

static void test_visual_x_delete(void) {
  TEST("VISUAL 'x' in NOTES → InputVisualDelete");
  ASSERT_EQ(resolve_raw('x', VISUAL, NOTES), InputVisualDelete);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — unbound keys return NULL
 * ---------------------------------------------------------------------------
 */

static void test_unbound_default(void) {
  TEST("DEFAULT 'z' in MAIN_MENU → NULL (unbound)");
  ASSERT_NULL(resolve_raw('z', DEFAULT, MAIN_MENU));
}

static void test_unbound_normal_no_notes(void) {
  TEST("NORMAL 'q' in MAIN_MENU (no NORMAL binding) → NULL");
  ASSERT_NULL(resolve_raw('q', NORMAL, MAIN_MENU));
}

static void test_unbound_insert_no_notes(void) {
  TEST("INSERT 'q' in MAIN_MENU (no INSERT binding) → NULL");
  ASSERT_NULL(resolve_raw('q', INSERT, MAIN_MENU));
}

/**
 * ---------------------------------------------------------------------------
 * Tests — mode: scene isolation (key defined in only one mode/scene)
 * ---------------------------------------------------------------------------
 */

static void test_mode_isolation(void) {
  TEST("NORMAL 'h' resolves in NOTES, NOT in MAIN_MENU");
  ASSERT_EQ(resolve_raw('h', NORMAL, NOTES), InputCursorLeft);
  ASSERT_NULL(resolve_raw('h', NORMAL, MAIN_MENU));
}

static void test_different_action_same_key_diff_scene(void) {
  TEST("DEFAULT 'q' → QuitApp in MAIN_MENU but → QuitAppNotes in NOTES");
  ASSERT_EQ(resolve_raw('q', DEFAULT, MAIN_MENU), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, NOTES), QuitAppNotes);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — NORMAL→DEFAULT fallback (empty input buffer)
 * ---------------------------------------------------------------------------
 */

static void test_normal_empty_buffer_falls_to_default(void) {
  TEST(
    "NORMAL mode with empty buffer: 'q' in MAIN_MENU → QuitApp (DEFAULT "
    "fallback)");
  ASSERT_EQ(resolve_with_fallback('q', NORMAL, MAIN_MENU, 0), QuitApp);
}

static void test_normal_nonempty_buffer_stays_normal(void) {
  TEST(
    "NORMAL mode with non-empty buffer: 'q' in MAIN_MENU → NULL (no NORMAL "
    "binding)");
  ASSERT_NULL(resolve_with_fallback('q', NORMAL, MAIN_MENU, 1));
}

static void test_normal_empty_in_notes_quitappnotes(void) {
  TEST(
    "NORMAL empty buffer, 'q' in NOTES → QuitApp (DEFAULT SCENE_NOTES "
    "QuitAppNotes wins)");
  /* DEFAULT + SCENE_NOTES: 'q' → QuitAppNotes */
  ASSERT_EQ(resolve_with_fallback('q', NORMAL, NOTES, 0), QuitAppNotes);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — INSERT sentinel does NOT apply in DEFAULT mode
 * ---------------------------------------------------------------------------
 */

static void test_insert_sentinel_only_for_insert(void) {
  TEST("DEFAULT 'a' in NOTES → NULL (no sentinel for DEFAULT)");
  ASSERT_NULL(resolve_process('a', DEFAULT, NOTES, 0));
}

static void test_insert_sentinel_only_printable(void) {
  TEST("INSERT ESC (non-printable) uses normal lookup, not sentinel");
  ASSERT_EQ(resolve_process(ESC, INSERT, NOTES, 0), InputESC);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — POMODORO scene bindings
 * ---------------------------------------------------------------------------
 */

static void test_pomodoro_s_skip(void) {
  TEST("DEFAULT 's' in WORK_TIME → SkipPomodoroStep");
  ASSERT_EQ(resolve_raw('s', DEFAULT, WORK_TIME), SkipPomodoroStep);
}

static void test_pomodoro_p_toggle_pause(void) {
  TEST("DEFAULT 'p' in WORK_TIME → TogglePause");
  ASSERT_EQ(resolve_raw('p', DEFAULT, WORK_TIME), TogglePause);
}

static void test_pomodoro_key_left_right(void) {
  TEST("DEFAULT KEY_LEFT/KEY_RIGHT in WORK_TIME → select item");
  ASSERT_NOT_NULL(resolve_raw(KEY_LEFT, DEFAULT, WORK_TIME));
  ASSERT_NOT_NULL(resolve_raw(KEY_RIGHT, DEFAULT, WORK_TIME));
}

/**
 * ---------------------------------------------------------------------------
 * Tests — popup scene bindings (not in ALL_SCENES)
 * ---------------------------------------------------------------------------
 */

static void test_noise_bindings_isolated(void) {
  TEST("DEFAULT 'q' in MAIN_MENU → QuitApp (NOT NoiseClose)");
  ASSERT_EQ(resolve_raw('q', DEFAULT, MAIN_MENU), QuitApp);
}

static void test_noise_q_in_noise(void) {
  TEST("DEFAULT 'q' in NOISE scene → NoiseClose");
  ASSERT_EQ(resolve_raw('q', DEFAULT, NOISE), NoiseClose);
}

static void test_noise_space_not_skip(void) {
  TEST("DEFAULT ' ' in NOISE → NoiseTogglePlay, NOT SkipPomodoroStep");
  ASSERT_EQ(resolve_raw(' ', DEFAULT, NOISE), NoiseTogglePlay);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — history scene bindings
 * ---------------------------------------------------------------------------
 */

static void test_history_left_right(void) {
  TEST("DEFAULT KEY_LEFT in HISTORY_OVERVIEW → HistoryCursorLeft");
  ASSERT_EQ(resolve_raw(KEY_LEFT, DEFAULT, HISTORY_OVERVIEW),
            HistoryCursorLeft);
  ASSERT_EQ(resolve_raw(KEY_RIGHT, DEFAULT, HISTORY_OVERVIEW),
            HistoryCursorRight);
}

static void test_history_up_down(void) {
  TEST("DEFAULT KEY_UP in HISTORY_OVERVIEW → HistoryCursorUp");
  ASSERT_EQ(resolve_raw(KEY_UP, DEFAULT, HISTORY_OVERVIEW), HistoryCursorUp);
  ASSERT_EQ(resolve_raw(KEY_DOWN, DEFAULT, HISTORY_OVERVIEW),
            HistoryCursorDown);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — preferences scene bindings
 * ---------------------------------------------------------------------------
 */

static void test_prefs_up_down(void) {
  TEST(
    "DEFAULT KEY_UP/KEY_DOWN in PREFERENCES → PrefsSelectPrev/PrefsSelectNext");
  ASSERT_EQ(resolve_raw(KEY_UP, DEFAULT, PREFERENCES), PrefsSelectPrev);
  ASSERT_EQ(resolve_raw(KEY_DOWN, DEFAULT, PREFERENCES), PrefsSelectNext);
}

static void test_prefs_left_right(void) {
  TEST(
    "DEFAULT KEY_LEFT/KEY_RIGHT in PREFERENCES → PrefsValueDown/PrefsValueUp");
  ASSERT_EQ(resolve_raw(KEY_LEFT, DEFAULT, PREFERENCES), PrefsValueDown);
  ASSERT_EQ(resolve_raw(KEY_RIGHT, DEFAULT, PREFERENCES), PrefsValueUp);
}

/**
 * ---------------------------------------------------------------------------
 * Tests — ALL_SCENES vs NOISE/HISTORY isolation
 * ---------------------------------------------------------------------------
 */

static void test_all_scenes_does_not_leak_into_noise(void) {
  TEST("ALL_SCENES 's' (SkipPomodoroStep) does NOT resolve in NOISE");
  ASSERT_NE(resolve_raw('s', DEFAULT, NOISE), SkipPomodoroStep);
}

static void test_all_scenes_does_not_leak_into_history(void) {
  TEST("ALL_SCENES 'q' (QuitApp) does NOT resolve in HISTORY_OVERVIEW");
  ASSERT_NE(resolve_raw('q', DEFAULT, HISTORY_OVERVIEW), QuitApp);
  ASSERT_EQ(resolve_raw('q', DEFAULT, HISTORY_OVERVIEW), ClosePopup);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("keybind-resolution");

  /* ALL_SCENES DEFAULT */
  reset_defaults();
  RUN_TEST(test_default_quitapp_q, "DEFAULT 'q' in MAIN_MENU → QuitApp");
  reset_defaults();
  RUN_TEST(test_default_quitapp_esc, "DEFAULT ESC in MAIN_MENU → QuitApp");
  reset_defaults();
  RUN_TEST(test_default_quitapp_ctrlc, "DEFAULT CTRLC in MAIN_MENU → QuitApp");
  reset_defaults();
  RUN_TEST(test_default_quitapp_all_scenes,
           "DEFAULT 'q' → QuitApp across ALL_SCENES");

  /* SCENE_NOTES */
  reset_defaults();
  RUN_TEST(test_notes_quitappnotes, "DEFAULT 'q' in NOTES → QuitAppNotes");
  reset_defaults();
  RUN_TEST(test_notes_esc_to_quitappnotes, "ESC in NOTES → QuitAppNotes");
  reset_defaults();
  RUN_TEST(test_notes_ctrlc_to_quitappnotes, "CTRLC in NOTES → QuitAppNotes");

  /* NORMAL */
  reset_defaults();
  RUN_TEST(test_normal_h_cursor_left, "NORMAL 'h' in NOTES → InputCursorLeft");
  reset_defaults();
  RUN_TEST(test_normal_l_cursor_right,
           "NORMAL 'l' in NOTES → InputCursorRight");
  reset_defaults();
  RUN_TEST(test_normal_x_delete, "NORMAL 'x' in NOTES → InputDeleteChar");
  reset_defaults();
  RUN_TEST(test_normal_i_insert, "NORMAL 'i' in NOTES → SwitchToInsertMode");
  reset_defaults();
  RUN_TEST(test_normal_u_undo, "NORMAL 'u' in NOTES → UndoNotes");
  reset_defaults();
  RUN_TEST(test_normal_ctrlr_redo, "NORMAL CTRLR in NOTES → RedoNotes");

  /* INSERT */
  reset_defaults();
  RUN_TEST(test_insert_printable_char, "INSERT 'a' in NOTES → InputInsertChar");
  reset_defaults();
  RUN_TEST(test_insert_esc, "INSERT ESC in NOTES → InputESC");
  reset_defaults();
  RUN_TEST(test_insert_ctrlc, "INSERT CTRLC in NOTES → InputESC");
  reset_defaults();
  RUN_TEST(test_insert_enter, "INSERT ENTER in NOTES → InputCommit");
  reset_defaults();
  RUN_TEST(test_insert_backspace,
           "INSERT KEY_BACKSPACE in NOTES → InputBackspace");
  reset_defaults();
  RUN_TEST(test_insert_key_left, "INSERT KEY_LEFT in NOTES → InputCursorLeft");

  /* VISUAL */
  reset_defaults();
  RUN_TEST(test_visual_h_cursor_left, "VISUAL 'h' in NOTES → InputCursorLeft");
  reset_defaults();
  RUN_TEST(test_visual_i_insert, "VISUAL 'i' in NOTES → SwitchToInsertMode");
  reset_defaults();
  RUN_TEST(test_visual_x_delete, "VISUAL 'x' in NOTES → InputVisualDelete");

  /* Unbound */
  reset_defaults();
  RUN_TEST(test_unbound_default, "DEFAULT 'z' → NULL");
  reset_defaults();
  RUN_TEST(test_unbound_normal_no_notes, "NORMAL 'q' in MAIN_MENU → NULL");
  reset_defaults();
  RUN_TEST(test_unbound_insert_no_notes, "INSERT 'q' in MAIN_MENU → NULL");

  /* Mode/scene isolation */
  reset_defaults();
  RUN_TEST(test_mode_isolation, "NORMAL 'h' works in NOTES, not MAIN_MENU");
  reset_defaults();
  RUN_TEST(test_different_action_same_key_diff_scene,
           "'q'→QuitApp in menu, QuitAppNotes in NOTES");

  /* NORMAL→DEFAULT fallback */
  reset_defaults();
  RUN_TEST(test_normal_empty_buffer_falls_to_default,
           "NORMAL empty buf 'q' in MAIN_MENU → DEFAULT QuitApp");
  reset_defaults();
  RUN_TEST(test_normal_nonempty_buffer_stays_normal,
           "NORMAL nonempty buf 'q' in MAIN_MENU → NULL");
  reset_defaults();
  RUN_TEST(test_normal_empty_in_notes_quitappnotes,
           "NORMAL empty buf 'q' in NOTES → QuitAppNotes");

  /* INSERT sentinel isolation */
  reset_defaults();
  RUN_TEST(test_insert_sentinel_only_for_insert, "DEFAULT 'a' in NOTES → NULL");
  reset_defaults();
  RUN_TEST(test_insert_sentinel_only_printable,
           "INSERT ESC uses normal lookup");

  /* Pomodoro */
  reset_defaults();
  RUN_TEST(test_pomodoro_s_skip, "DEFAULT 's' in WORK_TIME → SkipPomodoroStep");
  reset_defaults();
  RUN_TEST(test_pomodoro_p_toggle_pause,
           "DEFAULT 'p' in WORK_TIME → TogglePause");
  reset_defaults();
  RUN_TEST(test_pomodoro_key_left_right, "KEY_LEFT/RIGHT in WORK_TIME exist");

  /* Noise isolation */
  reset_defaults();
  RUN_TEST(test_noise_bindings_isolated,
           "'q' in menu → QuitApp, not NoiseClose");
  reset_defaults();
  RUN_TEST(test_noise_q_in_noise, "DEFAULT 'q' in NOISE → NoiseClose");
  reset_defaults();
  RUN_TEST(test_noise_space_not_skip, "DEFAULT ' ' in NOISE → NoiseTogglePlay");

  /* History */
  reset_defaults();
  RUN_TEST(test_history_left_right, "KEY_LEFT/RIGHT in HISTORY_OVERVIEW");
  reset_defaults();
  RUN_TEST(test_history_up_down, "KEY_UP/DOWN in HISTORY_OVERVIEW");

  /* Preferences */
  reset_defaults();
  RUN_TEST(test_prefs_up_down, "KEY_UP/DOWN in PREFERENCES");
  reset_defaults();
  RUN_TEST(test_prefs_left_right, "KEY_LEFT/RIGHT in PREFERENCES");

  /* Isolation */
  reset_defaults();
  RUN_TEST(test_all_scenes_does_not_leak_into_noise,
           "ALL_SCENES SkipPomodoroStep 's' does not leak into NOISE");
  reset_defaults();
  RUN_TEST(test_all_scenes_does_not_leak_into_history,
           "ALL_SCENES q does not leak into HISTORY");

  return test_end();
}
