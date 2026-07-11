/**
 * @file test_keybind_stubs.h
 * @brief Shared stub definitions for keybinding tests.
 *
 * Provides an AppData typedef and stub implementations for every action
 * function referenced in config.c's DEFAULT_KEYS array so that tests
 * which #include "config.c" can compile without linking against the
 * real input.o / ui.o modules.
 *
 * Also stubs EnsureDir() which is called by the config-loading path.
 */

#ifndef TEST_KEYBIND_STUBS_H_
#define TEST_KEYBIND_STUBS_H_

#include "error.h"

typedef struct AppData {
  int dummy;
} AppData;

void InputCursorLeft(AppData* app) { (void)app; }
void InputCursorRight(AppData* app) { (void)app; }
void InputDeleteChar(AppData* app) { (void)app; }
void InputESC(AppData* app) { (void)app; }
void InputCommit(AppData* app) { (void)app; }
void SwitchToInsertMode(AppData* app) { (void)app; }
void SwitchToInsertModeAppend(AppData* app) { (void)app; }
void SwitchToVisualMode(AppData* app) { (void)app; }
void UndoNotes(AppData* app) { (void)app; }
void RedoNotes(AppData* app) { (void)app; }
void InputBackspace(AppData* app) { (void)app; }
void InputInsertChar(AppData* app) { (void)app; }
void InputVisualDelete(AppData* app) { (void)app; }
void InputSwitchToInsertFromVisual(AppData* app) { (void)app; }
void ToggleMoveMode(AppData* app) { (void)app; }
void ExitMoveMode(AppData* app) { (void)app; }
void MoveNoteDownWrapper(AppData* app) { (void)app; }
void MoveNoteUpWrapper(AppData* app) { (void)app; }
void PromoteNoteWrapper(AppData* app) { (void)app; }
void DemoteNoteWrapper(AppData* app) { (void)app; }
void DeleteNoteAtNotes(AppData* app) { (void)app; }
void AddNewTask(AppData* app) { (void)app; }
void AddNewNote(AppData* app) { (void)app; }
void AddSubtask(AppData* app) { (void)app; }
void AddSubnote(AppData* app) { (void)app; }
void EditCurrentNote(AppData* app) { (void)app; }
void ToggleTaskAtNotes(AppData* app) { (void)app; }
void QuitAppNotes(AppData* app) { (void)app; }
void SelectNextItem(AppData* app) { (void)app; }
void SelectPreviousItem(AppData* app) { (void)app; }
void ExecuteMenuAction(AppData* app) { (void)app; }
void ChangeSelectedItemLeft(AppData* app) { (void)app; }
void ChangeSelectedItemRight(AppData* app) { (void)app; }
void SkipPomodoroStep(AppData* app) { (void)app; }
void TogglePause(AppData* app) { (void)app; }
void OpenResetMenu(AppData* app) { (void)app; }
void QuitApp(AppData* app) { (void)app; }
void GoPrevSlide(AppData* app) { (void)app; }
void GoNextSlide(AppData* app) { (void)app; }
void ClosePopup(AppData* app) { (void)app; }
void SelectPrevButton(AppData* app) { (void)app; }
void SelectNextButton(AppData* app) { (void)app; }
void ExecuteButtonAction(AppData* app) { (void)app; }
void OpenNoiseMenu(AppData* app) { (void)app; }
void NoiseClose(AppData* app) { (void)app; }
void NoiseSelectPrev(AppData* app) { (void)app; }
void NoiseSelectNext(AppData* app) { (void)app; }
void NoiseTogglePlay(AppData* app) { (void)app; }
void NoiseVolumeUp(AppData* app) { (void)app; }
void NoiseVolumeDown(AppData* app) { (void)app; }
void NoiseResetAll(AppData* app) { (void)app; }
void OpenHistoryPopup(AppData* app) { (void)app; }
void HistoryCursorLeft(AppData* app) { (void)app; }
void HistoryCursorRight(AppData* app) { (void)app; }
void HistoryCursorDown(AppData* app) { (void)app; }
void HistoryCursorUp(AppData* app) { (void)app; }
void HistoryOpenDayDetail(AppData* app) { (void)app; }
void HistoryOpenStatistics(AppData* app) { (void)app; }
void HistoryScrollDown(AppData* app) { (void)app; }
void HistoryScrollUp(AppData* app) { (void)app; }
void HistoryCloseToOverview(AppData* app) { (void)app; }
void NextPanel(AppData* app) { (void)app; }
void OpenPreferencesMenu(AppData* app) { (void)app; }
void PrefsSelectPrev(AppData* app) { (void)app; }
void PrefsSelectNext(AppData* app) { (void)app; }
void PrefsValueDown(AppData* app) { (void)app; }
void PrefsValueUp(AppData* app) { (void)app; }
void PrefsToggle(AppData* app) { (void)app; }
void PrefsEdit(AppData* app) { (void)app; }
void PrefsBack(AppData* app) { (void)app; }
void PrefsPreview(AppData* app) { (void)app; }
void PrefsScrollUp(AppData* app) { (void)app; }
void PrefsScrollDown(AppData* app) { (void)app; }
void StepperDecrement(AppData* app) { (void)app; }
void StepperIncrement(AppData* app) { (void)app; }
void StepperClose(AppData* app) { (void)app; }
void SelectApply(AppData* app) { (void)app; }
void SelectCancel(AppData* app) { (void)app; }
void SelectPrevOption(AppData* app) { (void)app; }
void SelectNextOption(AppData* app) { (void)app; }
void OpenHelp(AppData* app) { (void)app; }
void OpenHelpMenu(AppData* app) { (void)app; }
void HelpScrollUp(AppData* app) { (void)app; }
void HelpScrollDown(AppData* app) { (void)app; }
void NotesPrevPage(AppData* app) { (void)app; }
void NotesNextPage(AppData* app) { (void)app; }
void SwitchToNormalMode(AppData* app) { (void)app; }

int EnsureDir(const char* dir) {
  (void)dir;
  return 0;
}

void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

void SetError(AppData* app, const char* context, ErrorType type) {
  (void)app;
  (void)context;
  (void)type;
}

#endif /* TEST_KEYBIND_STUBS_H_ */
