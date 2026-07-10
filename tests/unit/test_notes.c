/**
 * @file test_notes.c
 * @brief Unit tests for the notes module.
 *
 * Tests CreateNotesData, AddNote, AddChildNote, AddNoteAfter, UpdateNote,
 * DeleteNote, ToggleTask, NoteUp, NoteDown, MoveNoteUp, MoveNoteDown,
 * CloneNotesData, RestoreNotesData, GetSelectedNoteIndex, PromoteNote,
 * DemoteNote, WrapText, GetNoteLines, GetNoteLinesFromText,
 * FreeClonedNotesData, and SaveNotesToHistory.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "gap_buffer.h"
#include "history.h"
#include "notes.h"
#include "test_helpers.h"
#include "ui.h"

typedef struct AppData {
  int dummy;
} AppData;
Config g_config;

void SetColor(short int fg, short int bg, chtype attr) {
  (void)fg;
  (void)bg;
  (void)attr;
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
void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

#define SetError(app, ctx, type) ((void)0)

#include "notes.c"

static AppData mock_app;

static void reset_notes(NotesData* notes) {
  notes->count = 0;
  notes->capacity = NOTES_INITIAL_CAPACITY;
  notes->items = malloc(sizeof(NoteItem*) * (size_t)notes->capacity);
  notes->current_id = 1;
}

/**
 * ---------------------------------------------------------------------------
 * Create / Free
 * ---------------------------------------------------------------------------
 */

static void test_create_free(void) {
  TEST("create and free NotesData");
  NotesData* nd = CreateNotesData();
  ASSERT_NOT_NULL(nd);
  ASSERT_EQ(nd->count, 0);
  ASSERT_EQ(nd->capacity, NOTES_INITIAL_CAPACITY);
  ASSERT_EQ(nd->current_id, -1);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * AddNote / AddChildNote / AddNoteAfter
 * ---------------------------------------------------------------------------
 */

static void test_add_note(void) {
  TEST("add a note");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "test note", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 1);
  ASSERT_STR_EQ(GapBufferToString(nd->items[0]->text), "test note");
  ASSERT_EQ(nd->items[0]->state, NOTE_PLAIN);
  ASSERT_EQ(nd->items[0]->parent_id, -1);
  FreeNotesData(nd);
}

static void test_add_task(void) {
  TEST("add a task (undone)");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "[ ] task", NOTE_UNDONE);
  ASSERT_EQ(nd->count, 1);
  ASSERT_EQ(nd->items[0]->state, NOTE_UNDONE);
  FreeNotesData(nd);
}

static void test_add_multiple(void) {
  TEST("add multiple notes triggers reallocation");
  NotesData* nd = CreateNotesData();
  for (int i = 0; i < 20; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "note %d", i);
    AddNote(&mock_app, nd, buf, NOTE_PLAIN);
  }
  ASSERT_EQ(nd->count, 20);
  ASSERT_GT(nd->capacity, NOTES_INITIAL_CAPACITY);
  FreeNotesData(nd);
}

static void test_add_child_note(void) {
  TEST("add child note under parent");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "parent", NOTE_PLAIN);
  int pid = nd->items[0]->id;
  AddChildNote(&mock_app, nd, pid, "child", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 2);
  ASSERT_EQ(nd->items[1]->parent_id, pid);
  ASSERT_EQ(nd->items[1]->depth, 1);
  FreeNotesData(nd);
}

static void test_add_child_invalid_parent(void) {
  TEST("add child with invalid parent id");
  NotesData* nd = CreateNotesData();
  AddChildNote(&mock_app, nd, 999, "orphan", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 0);
  FreeNotesData(nd);
}

static void test_add_note_after(void) {
  TEST("add note after specific note");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "first", NOTE_PLAIN);
  AddNote(&mock_app, nd, "third", NOTE_PLAIN);
  int after_id = nd->items[0]->id;
  AddNoteAfter(&mock_app, nd, after_id, "second", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 3);
  char* s = GapBufferToString(nd->items[1]->text);
  ASSERT_STR_EQ(s, "second");
  free(s);
  FreeNotesData(nd);
}

static void test_add_note_after_invalid(void) {
  TEST("add note after invalid id falls back to AddNote");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "only", NOTE_PLAIN);
  AddNoteAfter(&mock_app, nd, 999, "ghost", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 2);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * UpdateNote
 * ---------------------------------------------------------------------------
 */

static void test_update_note_text(void) {
  TEST("update note text");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "old", NOTE_PLAIN);
  int id = nd->items[0]->id;
  UpdateNote(nd, id, "new", -1);
  char* s = GapBufferToString(nd->items[0]->text);
  ASSERT_STR_EQ(s, "new");
  free(s);
  FreeNotesData(nd);
}

static void test_update_note_state(void) {
  TEST("update note state");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "task", NOTE_UNDONE);
  int id = nd->items[0]->id;
  UpdateNote(nd, id, "task", NOTE_DONE);
  ASSERT_EQ(nd->items[0]->state, NOTE_DONE);
  FreeNotesData(nd);
}

static void test_update_invalid_id(void) {
  TEST("update invalid id is no-op");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "keep", NOTE_PLAIN);
  UpdateNote(nd, 999, "lost", NOTE_DONE);
  char* s = GapBufferToString(nd->items[0]->text);
  ASSERT_STR_EQ(s, "keep");
  free(s);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * DeleteNote / ToggleTask
 * ---------------------------------------------------------------------------
 */

static void test_delete_note(void) {
  TEST("delete a note");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "delete me", NOTE_PLAIN);
  AddNote(&mock_app, nd, "keep me", NOTE_PLAIN);
  int del_id = nd->items[0]->id;
  nd->current_id = del_id;
  DeleteNote(nd);
  ASSERT_EQ(nd->count, 1);
  FreeNotesData(nd);
}

static void test_toggle_task(void) {
  TEST("toggle task between done/undone");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "[ ] task", NOTE_UNDONE);
  nd->current_id = nd->items[0]->id;
  ToggleTask(nd);
  ASSERT_EQ(nd->items[0]->state, NOTE_DONE);
  ToggleTask(nd);
  ASSERT_EQ(nd->items[0]->state, NOTE_UNDONE);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * Navigation (NoteUp / NoteDown / MoveNoteUp / MoveNoteDown)
 * ---------------------------------------------------------------------------
 */

static void test_note_up_down(void) {
  TEST("note up and down selection");
  NotesData* nd = CreateNotesData();
  for (int i = 0; i < 5; i++) {
    char buf[16];
    snprintf(buf, sizeof(buf), "note %d", i);
    AddNote(&mock_app, nd, buf, NOTE_PLAIN);
  }
  nd->current_id = nd->items[0]->id;
  NoteDown(nd);
  ASSERT_EQ(nd->current_id, nd->items[1]->id);
  NoteUp(nd);
  ASSERT_EQ(nd->current_id, nd->items[0]->id);
  FreeNotesData(nd);
}

static void test_move_note_up_down(void) {
  TEST("move note up and down in list");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "first", NOTE_PLAIN);
  AddNote(&mock_app, nd, "second", NOTE_PLAIN);
  AddNote(&mock_app, nd, "third", NOTE_PLAIN);

  nd->current_id = nd->items[2]->id;
  MoveNoteUp(nd);
  char* s = GapBufferToString(nd->items[1]->text);
  ASSERT_STR_EQ(s, "third");
  free(s);

  nd->current_id = nd->items[1]->id;
  MoveNoteDown(nd);
  s = GapBufferToString(nd->items[2]->text);
  ASSERT_STR_EQ(s, "third");
  free(s);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * Clone / Restore NotesData
 * ---------------------------------------------------------------------------
 */

static void test_clone_restore(void) {
  TEST("clone and restore NotesData");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "original", NOTE_PLAIN);

  NotesData* clone = CloneNotesData(nd);
  ASSERT_NOT_NULL(clone);
  ASSERT_EQ(clone->count, 1);

  AddNote(&mock_app, nd, "added later", NOTE_PLAIN);
  ASSERT_EQ(nd->count, 2);

  RestoreNotesData(nd, clone);
  ASSERT_EQ(nd->count, 1);
  char* s = GapBufferToString(nd->items[0]->text);
  ASSERT_STR_EQ(s, "original");
  free(s);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * GetSelectedNoteIndex
 * ---------------------------------------------------------------------------
 */

static void test_get_selected_note_index(void) {
  TEST("GetSelectedNoteIndex returns correct index");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "a", NOTE_PLAIN);
  AddNote(&mock_app, nd, "b", NOTE_PLAIN);
  AddNote(&mock_app, nd, "c", NOTE_PLAIN);
  nd->current_id = nd->items[1]->id;
  ASSERT_EQ(GetSelectedNoteIndex(nd), 1);
  nd->current_id = nd->items[2]->id;
  ASSERT_EQ(GetSelectedNoteIndex(nd), 2);
  FreeNotesData(nd);
}

static void test_get_selected_note_index_null(void) {
  TEST("GetSelectedNoteIndex returns -1 on NULL");
  ASSERT_EQ(GetSelectedNoteIndex(NULL), -1);
}

static void test_get_selected_note_index_invalid(void) {
  TEST("GetSelectedNoteIndex returns -1 for unknown id");
  NotesData* nd = CreateNotesData();
  nd->current_id = 999;
  ASSERT_EQ(GetSelectedNoteIndex(nd), -1);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * PromoteNote
 * ---------------------------------------------------------------------------
 */

static void test_promote_note(void) {
  TEST("PromoteNote promotes a child note to sibling level");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "parent", NOTE_PLAIN);
  int pid = nd->items[0]->id;
  AddChildNote(&mock_app, nd, pid, "child", NOTE_PLAIN);
  nd->current_id = nd->items[1]->id;
  ASSERT_EQ(nd->items[1]->depth, 1);
  PromoteNote(nd);
  ASSERT_EQ(nd->items[1]->parent_id, -1);
  ASSERT_EQ(nd->items[1]->depth, 0);
  FreeNotesData(nd);
}

static void test_promote_note_no_parent(void) {
  TEST("PromoteNote on root note is no-op");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "root", NOTE_PLAIN);
  nd->current_id = nd->items[0]->id;
  PromoteNote(nd);
  ASSERT_EQ(nd->items[0]->parent_id, -1);
  FreeNotesData(nd);
}

static void test_promote_note_empty(void) {
  TEST("PromoteNote on empty notes is no-op");
  NotesData* nd = CreateNotesData();
  PromoteNote(nd);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * DemoteNote
 * ---------------------------------------------------------------------------
 */

static void test_demote_note(void) {
  TEST("DemoteNote demotes a note under previous sibling");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "sibling", NOTE_PLAIN);
  AddNote(&mock_app, nd, "target", NOTE_PLAIN);
  nd->current_id = nd->items[1]->id;
  DemoteNote(nd);
  ASSERT_EQ(nd->items[1]->parent_id, nd->items[0]->id);
  ASSERT_EQ(nd->items[1]->depth, 1);
  FreeNotesData(nd);
}

static void test_demote_note_no_prev(void) {
  TEST("DemoteNote on first note is no-op");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "first", NOTE_PLAIN);
  nd->current_id = nd->items[0]->id;
  DemoteNote(nd);
  ASSERT_EQ(nd->items[0]->parent_id, -1);
  FreeNotesData(nd);
}

/**
 * ---------------------------------------------------------------------------
 * WrapText
 * ---------------------------------------------------------------------------
 */

static void test_wrap_text_short(void) {
  TEST("WrapText returns single line for short text");
  char** lines = NULL;
  int count = WrapText("hello", 80, &lines);
  ASSERT_EQ(count, 1);
  ASSERT_STR_EQ(lines[0], "hello");
  for (int i = 0; i < count; i++) free(lines[i]);
  free(lines);
}

static void test_wrap_text_wrap(void) {
  TEST("WrapText wraps text at max_width boundary");
  char** lines = NULL;
  int count = WrapText("hello world foo bar", 10, &lines);
  ASSERT_GT(count, 1);
  ASSERT_STR_EQ(lines[0], "hello");
  for (int i = 0; i < count; i++) free(lines[i]);
  free(lines);
}

static void test_wrap_text_null(void) {
  TEST("WrapText returns 0 for NULL text");
  char** lines = (char**)0xdeadbeef;
  int count = WrapText(NULL, 80, &lines);
  ASSERT_EQ(count, 0);
  ASSERT_NULL(lines);
}

static void test_wrap_text_zero_width(void) {
  TEST("WrapText returns 0 for zero max_width");
  char** lines = (char**)0xdeadbeef;
  int count = WrapText("hello", 0, &lines);
  ASSERT_EQ(count, 0);
  ASSERT_NULL(lines);
}

/**
 * ---------------------------------------------------------------------------
 * GetNoteLines / GetNoteLinesFromText
 * ---------------------------------------------------------------------------
 */

static void test_get_note_lines_from_text(void) {
  TEST("GetNoteLinesFromText counts wrapped lines");
  int count = GetNoteLinesFromText("hello world", 5);
  ASSERT_EQ(count, 2);
}

static void test_get_note_lines_from_text_null(void) {
  TEST("GetNoteLinesFromText returns 0 for NULL");
  ASSERT_EQ(GetNoteLinesFromText(NULL, 80), 0);
}

static void test_get_note_lines_from_text_zero_width(void) {
  TEST("GetNoteLinesFromText returns 0 for zero width");
  ASSERT_EQ(GetNoteLinesFromText("hello", 0), 0);
}

static void test_get_note_lines_item(void) {
  TEST("GetNoteLines returns line count for NoteItem");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "hello world", NOTE_PLAIN);
  int count = GetNoteLines(nd->items[0], 5);
  ASSERT_EQ(count, 2);
  FreeNotesData(nd);
}

static void test_get_note_lines_null(void) {
  TEST("GetNoteLines returns 1 for NULL item");
  ASSERT_EQ(GetNoteLines(NULL, 80), 1);
}

/**
 * ---------------------------------------------------------------------------
 * FreeClonedNotesData
 * ---------------------------------------------------------------------------
 */

static void test_free_cloned_notes_data(void) {
  TEST("FreeClonedNotesData frees cloned NotesData");
  NotesData* nd = CreateNotesData();
  AddNote(&mock_app, nd, "test", NOTE_PLAIN);
  NotesData* clone = CloneNotesData(nd);
  ASSERT_NOT_NULL(clone);
  FreeClonedNotesData(clone);
  FreeNotesData(nd);
}

static void test_free_cloned_notes_data_null(void) {
  TEST("FreeClonedNotesData handles NULL");
  FreeClonedNotesData(NULL);
}

/**
 * ---------------------------------------------------------------------------
 * SaveNotesToHistory
 * ---------------------------------------------------------------------------
 */

static void test_save_notes_to_history(void) {
  TEST("SaveNotesToHistory pushes snapshot to history");
  NotesData* nd = CreateNotesData();
  nd->history = CreateHistory();
  ASSERT_NOT_NULL(nd->history);
  AddNote(&mock_app, nd, "before", NOTE_PLAIN);
  SaveNotesToHistory(nd, -1);
  ASSERT_TRUE(HistoryCanUndo(nd->history));
  FreeNotesData(nd);
}

static void test_save_notes_to_history_null(void) {
  TEST("SaveNotesToHistory handles NULL notes");
  SaveNotesToHistory(NULL, -1);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  g_config.misc.max_note_depth = 10;
  test_begin("notes");
  RUN_TEST(test_create_free, "create and free NotesData");
  RUN_TEST(test_add_note, "add a note");
  RUN_TEST(test_add_task, "add a task (undone)");
  RUN_TEST(test_add_multiple, "add multiple notes triggers reallocation");
  RUN_TEST(test_add_child_note, "add child note under parent");
  RUN_TEST(test_add_child_invalid_parent, "add child with invalid parent id");
  RUN_TEST(test_add_note_after, "add note after specific note");
  RUN_TEST(test_add_note_after_invalid, "add note after invalid id");
  RUN_TEST(test_update_note_text, "update note text");
  RUN_TEST(test_update_note_state, "update note state");
  RUN_TEST(test_update_invalid_id, "update invalid id is no-op");
  RUN_TEST(test_delete_note, "delete a note");
  RUN_TEST(test_toggle_task, "toggle task between done/undone");
  RUN_TEST(test_note_up_down, "note up and down selection");
  RUN_TEST(test_move_note_up_down, "move note up and down in list");
  RUN_TEST(test_clone_restore, "clone and restore NotesData");
  RUN_TEST(test_get_selected_note_index,
           "GetSelectedNoteIndex returns correct index");
  RUN_TEST(test_get_selected_note_index_null,
           "GetSelectedNoteIndex returns -1 on NULL");
  RUN_TEST(test_get_selected_note_index_invalid,
           "GetSelectedNoteIndex returns -1 for unknown id");
  RUN_TEST(test_promote_note, "PromoteNote promotes a child note");
  RUN_TEST(test_promote_note_no_parent, "PromoteNote on root note is no-op");
  RUN_TEST(test_promote_note_empty, "PromoteNote on empty is no-op");
  RUN_TEST(test_demote_note, "DemoteNote demotes under previous sibling");
  RUN_TEST(test_demote_note_no_prev, "DemoteNote on first note is no-op");
  RUN_TEST(test_wrap_text_short, "WrapText returns single line for short text");
  RUN_TEST(test_wrap_text_wrap, "WrapText wraps text at max_width");
  RUN_TEST(test_wrap_text_null, "WrapText returns 0 for NULL text");
  RUN_TEST(test_wrap_text_zero_width, "WrapText returns 0 for zero width");
  RUN_TEST(test_get_note_lines_from_text,
           "GetNoteLinesFromText counts wrapped lines");
  RUN_TEST(test_get_note_lines_from_text_null,
           "GetNoteLinesFromText returns 0 for NULL");
  RUN_TEST(test_get_note_lines_from_text_zero_width,
           "GetNoteLinesFromText returns 0 for zero width");
  RUN_TEST(test_get_note_lines_item, "GetNoteLines returns line count");
  RUN_TEST(test_get_note_lines_null, "GetNoteLines returns 1 for NULL");
  RUN_TEST(test_free_cloned_notes_data,
           "FreeClonedNotesData frees cloned data");
  RUN_TEST(test_free_cloned_notes_data_null,
           "FreeClonedNotesData handles NULL");
  RUN_TEST(test_save_notes_to_history, "SaveNotesToHistory pushes snapshot");
  RUN_TEST(test_save_notes_to_history_null,
           "SaveNotesToHistory handles NULL notes");
  return test_end();
}
