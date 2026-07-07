#include "notes.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "gap_buffer.h"
#include "input.h"
#include "ui.h"
#include "util.h"

/* PRIVATE NOTES FUNCTIONS */
/* Utility */
static void growItems(AppData* app, NotesData* notes);
static int getNextID(NotesData* notes);
static NoteItem* findByID(NotesData* notes, int id);
static int getIndexByID(NotesData* notes, int id);
/* Move mode */
static int getPrevSiblingIndex(NotesData* notes, int idx);
static int getNextSiblingIndex(NotesData* notes, int idx);
static void getSubtreeRange(NotesData* notes, int idx, int* start, int* end);
static void reinsertNoteAt(NotesData* notes, int from_idx, int to_idx);
static int findRootAncestorIndex(NotesData* notes, int idx);

/**
 * ---------------------------------------------------------------------------
 * NotesData Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new NotesData container with default values.
 * @return Pointer to the created NotesData, or NULL on allocation failure
 */
NotesData* CreateNotesData(void) {
  NotesData* notes = (NotesData*)malloc(sizeof(NotesData));
  if (!notes) return NULL;

  notes->items = (NoteItem**)malloc(sizeof(NoteItem*) * NOTES_INITIAL_CAPACITY);
  if (!notes->items) {
    free(notes);
    return NULL;
  }

  notes->count = 0;
  notes->capacity = NOTES_INITIAL_CAPACITY;
  notes->current_id = -1;
  notes->total_lines = 0;
  notes->max_lines = 0;
  notes->render_width = 0;
  notes->is_move_mode = false;
  notes->history = CreateHistory();
  notes->last_affected_id = -1;
  notes->drag_note_id = -1;
  notes->drag_start_y = 0;
  notes->drag_moved = false;

  return notes;
}

/**
 * Free all memory associated with NotesData.
 * @param notes Pointer to the NotesData to free
 */
void FreeNotesData(NotesData* notes) {
  if (!notes) return;

  for (int i = 0; i < notes->count; i++) {
    GapBufferFree(notes->items[i]->text);
    free(notes->items[i]);
  }
  free(notes->items);
  FreeHistory(notes->history, FreeClonedNotesData);
  free(notes);
}

/**
 * Add a new note at the end of the list.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 * @param text Initial text content for the note
 * @param state Initial state of the note
 */
void AddNote(AppData* app, NotesData* notes, const char* text,
             NoteState state) {
  if (!notes || !text) return;

  SaveNotesToHistory(notes, -1);

  int new_note_lines = GetNoteLinesFromText(text, notes->render_width);
  if (notes->max_lines > 0 &&
      notes->total_lines + new_note_lines > notes->max_lines)
    return;
  if (notes->count >= notes->capacity) growItems(app, notes);

  NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
  if (!item) {
    SetError(app, "AddNote", MALLOC_ERROR);
    return;
  }

  item->text = GapBufferCreate();
  if (!item->text) {
    SetError(app, "AddNote", MALLOC_ERROR);
    free(item);
    return;
  }

  GapBufferSetText(item->text, text);
  item->state = state;
  item->id = getNextID(notes);
  item->parent_id = -1;
  item->depth = 0;

  notes->items[notes->count++] = item;
  notes->current_id = item->id;
  notes->total_lines += new_note_lines;
}

/**
 * Add a child note under a parent note.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 * @param parent_id ID of the parent note
 * @param text Initial text content
 * @param state Initial state
 */
void AddChildNote(AppData* app, NotesData* notes, int parent_id,
                  const char* text, NoteState state) {
  if (!notes || !text || parent_id < 0) return;

  SaveNotesToHistory(notes, -1);

  NoteItem* parent = findByID(notes, parent_id);
  if (!parent) return;

  if (parent->depth >= MAX_NOTE_DEPTH) return;

  int new_note_lines =
    GetNoteLinesFromText(text, notes->render_width - (parent->depth + 1) * 2);
  if (notes->max_lines > 0 &&
      notes->total_lines + new_note_lines > notes->max_lines)
    return;
  if (notes->count >= notes->capacity) growItems(app, notes);

  NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
  if (!item) {
    SetError(app, "AddChildNote", MALLOC_ERROR);
    return;
  }

  item->text = GapBufferCreate();
  if (!item->text) {
    SetError(app, "AddChildNote", MALLOC_ERROR);
    free(item);
    return;
  }

  GapBufferSetText(item->text, text);
  item->state = state;
  item->id = getNextID(notes);
  item->parent_id = parent_id;
  item->depth = parent->depth + 1;

  int parent_idx = getIndexByID(notes, parent_id);
  int insert_idx = parent_idx + 1;
  for (int i = parent_idx + 1; i < notes->count; i++) {
    if (notes->items[i]->parent_id == parent_id) insert_idx = i + 1;
    if (notes->items[i]->depth <= parent->depth) break;
  }

  for (int i = notes->count; i > insert_idx; i--) {
    notes->items[i] = notes->items[i - 1];
  }
  notes->items[insert_idx] = item;
  notes->count++;

  notes->current_id = item->id;
  notes->total_lines += new_note_lines;
}

/**
 * Add a note after a specific note in the list.
 * Used for inserting notes at a specific position.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 * @param after_id ID of the note to insert after
 * @param text Initial text content
 * @param state Initial state
 */
void AddNoteAfter(AppData* app, NotesData* notes, int after_id,
                  const char* text, NoteState state) {
  if (!notes || !text) return;
  if (after_id < 0) {
    AddNote(app, notes, text, state);
    return;
  }

  SaveNotesToHistory(notes, -1);

  int after_idx = getIndexByID(notes, after_id);
  if (after_idx < 0) {
    AddNote(app, notes, text, state);
    return;
  }

  NoteItem* after_note = notes->items[after_idx];

  int subtree_start, subtree_end;
  getSubtreeRange(notes, after_idx, &subtree_start, &subtree_end);

  int new_note_lines = GetNoteLinesFromText(text, notes->render_width);
  if (notes->max_lines > 0 &&
      notes->total_lines + new_note_lines > notes->max_lines)
    return;
  if (notes->count >= notes->capacity) growItems(app, notes);

  NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
  if (!item) {
    SetError(app, "AddNoteAfter", MALLOC_ERROR);
    return;
  }

  item->text = GapBufferCreate();
  if (!item->text) {
    SetError(app, "AddNoteAfter", MALLOC_ERROR);
    free(item);
    return;
  }

  GapBufferSetText(item->text, text);
  item->state = state;
  item->id = getNextID(notes);
  item->parent_id = after_note->parent_id;
  item->depth = after_note->depth;

  int insert_idx = subtree_end + 1;

  for (int i = notes->count; i > insert_idx; i--) {
    notes->items[i] = notes->items[i - 1];
  }
  notes->items[insert_idx] = item;
  notes->count++;

  notes->current_id = item->id;
  notes->total_lines += new_note_lines;
}

/**
 * Update an existing note's content and/or state.
 * @param notes Pointer to the NotesData
 * @param note_id ID of the note to update
 * @param text New text content (or NULL to keep existing)
 * @param state New state (or -1 to keep existing)
 */
void UpdateNote(NotesData* notes, int note_id, const char* text,
                NoteState state) {
  if (!notes || !text || note_id < 0) return;

  SaveNotesToHistory(notes, -1);

  NoteItem* note = findByID(notes, note_id);
  if (!note) return;

  int old_lines = GetNoteLines(note, notes->render_width);
  int new_lines = GetNoteLinesFromText(text, notes->render_width);

  if (notes->max_lines > 0 &&
      notes->total_lines - old_lines + new_lines > notes->max_lines)
    return;

  GapBufferSetText(note->text, text);
  note->state = state;
  notes->total_lines = notes->total_lines - old_lines + new_lines;
}

/**
 * Delete the currently selected note.
 * @param notes Pointer to the NotesData
 */
void DeleteNote(NotesData* notes) {
  if (!notes || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) return;

  int deleted_lines = GetNoteLines(notes->items[idx], notes->render_width);
  GapBufferFree(notes->items[idx]->text);
  free(notes->items[idx]);

  for (int i = idx; i < notes->count - 1; i++)
    notes->items[i] = notes->items[i + 1];
  notes->count--;
  notes->total_lines -= deleted_lines;

  if (notes->count == 0)
    notes->current_id = -1;
  else if (idx >= notes->count)
    notes->current_id = notes->items[notes->count - 1]->id;
  else
    notes->current_id = notes->items[idx]->id;
}

/**
 * Toggle the selected note between done and undone states.
 * @param notes Pointer to the NotesData
 */
void ToggleTask(NotesData* notes) {
  if (!notes || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  NoteItem* item = findByID(notes, notes->current_id);
  if (!item || item->state == NOTE_PLAIN) return;

  item->state = (item->state == NOTE_UNDONE) ? NOTE_DONE : NOTE_UNDONE;
}

/**
 * ---------------------------------------------------------------------------
 * Selection
 * ---------------------------------------------------------------------------
 */

/**
 * Move selection up one position in the notes list.
 * @param notes Pointer to the NotesData
 */
void NoteUp(NotesData* notes) {
  if (!notes || notes->count == 0) return;

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) {
    notes->current_id = notes->items[0]->id;
    return;
  }

  if (idx > 0) notes->current_id = notes->items[idx - 1]->id;
}

/**
 * Move selection down one position in the notes list.
 * @param notes Pointer to the NotesData
 */
void NoteDown(NotesData* notes) {
  if (!notes || notes->count == 0) return;

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) {
    notes->current_id = notes->items[0]->id;
    return;
  }

  if (idx < notes->count - 1) notes->current_id = notes->items[idx + 1]->id;
}

/**
 * Get the current note selection index.
 * @param notes Pointer to the NotesData
 * @return Index of the currently selected note, or -1 if none
 */
int GetSelectedNoteIndex(NotesData* notes) {
  if (!notes) return -1;
  return getIndexByID(notes, notes->current_id);
}

/**
 * ---------------------------------------------------------------------------
 * Move mode
 * ---------------------------------------------------------------------------
 */

/**
 * Get the index of the previous sibling of the note at the given index.
 * A sibling is a note at the same depth with the same parent.
 * @param notes Pointer to the NotesData
 * @param idx Index of the note to find previous sibling for
 * @return Index of previous sibling, or -1 if none exists
 */
static int getPrevSiblingIndex(NotesData* notes, int idx) {
  if (!notes || idx <= 0) return -1;
  int parent_id = notes->items[idx]->parent_id;
  int current_depth = notes->items[idx]->depth;
  for (int i = idx - 1; i >= 0; i--) {
    if (notes->items[i]->parent_id == parent_id &&
        notes->items[i]->depth == current_depth)
      return i;
    if (notes->items[i]->depth < current_depth) break;
  }
  return -1;
}

/**
 * Get the index of the next sibling of the note at the given index.
 * A sibling is a note at the same depth with the same parent.
 * @param notes Pointer to the NotesData
 * @param idx Index of the note to find next sibling for
 * @return Index of next sibling, or -1 if none exists
 */
static int getNextSiblingIndex(NotesData* notes, int idx) {
  if (!notes || idx >= notes->count - 1) return -1;
  int parent_id = notes->items[idx]->parent_id;
  int current_depth = notes->items[idx]->depth;
  for (int i = idx + 1; i < notes->count; i++) {
    if (notes->items[i]->parent_id == parent_id &&
        notes->items[i]->depth == current_depth)
      return i;
    if (notes->items[i]->depth < current_depth) break;
  }
  return -1;
}

/**
 * Get the range of indices covering a note and all its descendants.
 * @param notes Pointer to the NotesData
 * @param idx Index of the note to get range for
 * @param start Pointer to store the starting index of the subtree
 * @param end Pointer to store the ending index of the subtree
 */
static void getSubtreeRange(NotesData* notes, int idx, int* start, int* end) {
  *start = idx;
  *end = idx;
  if (!notes || idx < 0 || idx >= notes->count) return;
  int parent_depth = notes->items[idx]->depth;
  for (int i = idx + 1; i < notes->count; i++) {
    if (notes->items[i]->depth > parent_depth)
      *end = i;
    else
      break;
  }
}

/**
 * Move a note and its subtree to a new position in the list.
 * Handles both upward and downward moves.
 * @param notes Pointer to the NotesData
 * @param from_idx Index of the note to move
 * @param to_idx Target index to move the note to
 */
static void reinsertNoteAt(NotesData* notes, int from_idx, int to_idx) {
  if (!notes || from_idx == to_idx) return;
  if (from_idx < 0 || to_idx < 0 || from_idx >= notes->count ||
      to_idx > notes->count)
    return;

  int subtree_start, subtree_end;
  getSubtreeRange(notes, from_idx, &subtree_start, &subtree_end);
  int subtree_size = subtree_end - subtree_start + 1;

  NoteItem** copy = (NoteItem**)malloc(sizeof(NoteItem*) * notes->count);
  if (!copy) return;
  memcpy(copy, notes->items, sizeof(NoteItem*) * notes->count);

  if (from_idx < to_idx) {
    int idx = 0;
    for (int i = 0; i < subtree_start; i++) notes->items[idx++] = copy[i];
    for (int i = subtree_end + 1; i < to_idx; i++)
      notes->items[idx++] = copy[i];
    for (int i = subtree_start; i <= subtree_end; i++)
      notes->items[idx++] = copy[i];
    for (int i = to_idx; i < notes->count; i++) notes->items[idx++] = copy[i];
  } else {
    int idx = 0;
    for (int i = 0; i < to_idx; i++) notes->items[idx++] = copy[i];
    for (int i = subtree_start; i <= subtree_end; i++)
      notes->items[idx++] = copy[i];
    for (int i = to_idx; i < subtree_start; i++) notes->items[idx++] = copy[i];
    for (int i = subtree_end + 1; i < notes->count; i++)
      notes->items[idx++] = copy[i];
  }

  free(copy);
}

/**
 * Find the index of the root ancestor of the note at the given index.
 * A root ancestor is a top-level note (depth 0) or the highest-level parent.
 * @param notes Pointer to the NotesData
 * @param idx Index of the note to find root ancestor for
 * @return Index of the root ancestor, or -1 if not found
 */
static int findRootAncestorIndex(NotesData* notes, int idx) {
  if (!notes || idx < 0 || idx >= notes->count) return -1;
  NoteItem* note = notes->items[idx];
  if (note->parent_id < 0) return idx;
  int parent_idx = getIndexByID(notes, note->parent_id);
  if (parent_idx < 0) return idx;
  NoteItem* parent = notes->items[parent_idx];
  if (parent->depth == 0) return parent_idx;
  return findRootAncestorIndex(notes, parent_idx);
}

/**
 * Move the selected note up one position in the list.
 * @param notes Pointer to the NotesData
 */
void MoveNoteUp(NotesData* notes) {
  if (!notes || notes->count == 0 || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) return;
  int prev_sibling = getPrevSiblingIndex(notes, idx);
  if (prev_sibling < 0) return;
  reinsertNoteAt(notes, idx, prev_sibling);
}

/**
 * Move the selected note down one position in the list.
 * @param notes Pointer to the NotesData
 */
void MoveNoteDown(NotesData* notes) {
  if (!notes || notes->count == 0 || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) return;
  int next_sibling = getNextSiblingIndex(notes, idx);
  int to_idx;
  if (next_sibling >= 0) {
    int sibling_subtree_start, sibling_subtree_end;
    getSubtreeRange(notes, next_sibling, &sibling_subtree_start,
                    &sibling_subtree_end);
    to_idx = sibling_subtree_end + 1;
  } else {
    int subtree_start, subtree_end;
    getSubtreeRange(notes, idx, &subtree_start, &subtree_end);
    to_idx = notes->count;
  }
  if (to_idx <= idx) return;
  reinsertNoteAt(notes, idx, to_idx);
}

/**
 * Promote the selected note (decrease depth, move to parent level).
 * @param notes Pointer to the NotesData
 */
void PromoteNote(NotesData* notes) {
  if (!notes || notes->count == 0 || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) return;
  NoteItem* note = notes->items[idx];
  if (note->parent_id < 0) return;
  int parent_idx = getIndexByID(notes, note->parent_id);
  if (parent_idx < 0) return;
  NoteItem* parent = notes->items[parent_idx];
  int grandparent_id = parent->parent_id;
  int new_depth = (grandparent_id < 0) ? 0 : parent->depth;
  int new_parent_id = grandparent_id;
  if (new_depth > 0 && grandparent_id >= 0) {
    int root_idx = findRootAncestorIndex(notes, parent_idx);
    if (root_idx >= 0 && root_idx != parent_idx)
      new_parent_id = notes->items[root_idx]->id;
  }
  note->parent_id = new_parent_id;
  note->depth = new_depth;
  int subtree_start, subtree_end;
  getSubtreeRange(notes, idx, &subtree_start, &subtree_end);
  for (int i = subtree_start + 1; i <= subtree_end; i++) {
    notes->items[i]->parent_id = note->id;
    notes->items[i]->depth = notes->items[i]->depth - 1;
  }
}

/**
 * Demote the selected note (increase depth, nest under previous sibling).
 * @param notes Pointer to the NotesData
 */
void DemoteNote(NotesData* notes) {
  if (!notes || notes->count == 0 || notes->current_id < 0) return;

  SaveNotesToHistory(notes, -1);

  int idx = getIndexByID(notes, notes->current_id);
  if (idx < 0) return;
  NoteItem* note = notes->items[idx];
  int prev_sibling = getPrevSiblingIndex(notes, idx);
  if (prev_sibling < 0) return;
  NoteItem* new_parent = notes->items[prev_sibling];
  if (new_parent->depth >= MAX_NOTE_DEPTH) return;
  note->parent_id = new_parent->id;
  note->depth = new_parent->depth + 1;
  int subtree_start, subtree_end;
  getSubtreeRange(notes, idx, &subtree_start, &subtree_end);
  for (int i = subtree_start + 1; i <= subtree_end; i++) {
    notes->items[i]->parent_id = note->id;
    notes->items[i]->depth = notes->items[i]->depth + 1;
  }
}

/**
 * ---------------------------------------------------------------------------
 * History
 * ---------------------------------------------------------------------------
 */

/**
 * Free cloned NotesData for history cleanup.
 * @param data Pointer to NotesData to free
 */
void FreeClonedNotesData(void* data) {
  NotesData* notes = (NotesData*)data;
  if (!notes) return;

  for (int i = 0; i < notes->count; i++) {
    GapBufferFree(notes->items[i]->text);
    free(notes->items[i]);
  }
  free(notes->items);
  free(notes);
}

/**
 * Create a deep copy of NotesData for history snapshots.
 * @param src Source NotesData to clone
 * @return New NotesData copy, or NULL on failure
 */
NotesData* CloneNotesData(const NotesData* src) {
  NotesData* dst = (NotesData*)malloc(sizeof(NotesData));
  if (!dst) return NULL;

  dst->items = (NoteItem**)malloc(sizeof(NoteItem*) * src->capacity);
  if (!dst->items) {
    free(dst);
    return NULL;
  }

  dst->count = src->count;
  dst->capacity = src->capacity;
  dst->current_id = src->current_id;
  dst->max_lines = src->max_lines;
  dst->total_lines = src->total_lines;
  dst->render_width = src->render_width;
  dst->is_move_mode = src->is_move_mode;
  dst->history = NULL;
  dst->last_affected_id = src->last_affected_id;
  dst->saved_cursor = src->saved_cursor;
  dst->drag_note_id = src->drag_note_id;
  dst->drag_start_y = src->drag_start_y;
  dst->drag_moved = src->drag_moved;

  for (int i = 0; i < src->count; i++) {
    NoteItem* srcItem = src->items[i];
    NoteItem* dstItem = (NoteItem*)malloc(sizeof(NoteItem));
    if (!dstItem) {
      for (int j = 0; j < i; j++) {
        GapBufferFree(dst->items[j]->text);
        free(dst->items[j]);
      }
      free(dst->items);
      free(dst);
      return NULL;
    }
    dstItem->text = GapBufferClone(srcItem->text);
    if (!dstItem->text) {
      free(dstItem);
      for (int j = 0; j < i; j++) {
        GapBufferFree(dst->items[j]->text);
        free(dst->items[j]);
      }
      free(dst->items);
      free(dst);
      return NULL;
    }
    dstItem->state = srcItem->state;
    dstItem->id = srcItem->id;
    dstItem->parent_id = srcItem->parent_id;
    dstItem->depth = srcItem->depth;
    dst->items[i] = dstItem;
  }

  return dst;
}

/**
 * Save current NotesData state to history for undo.
 * @param notes Pointer to NotesData
 * @param cursor Current cursor position
 */
void SaveNotesToHistory(NotesData* notes, int cursor) {
  if (!notes || !notes->history) return;
  NotesData* snapshot = CloneNotesData(notes);
  snapshot->last_affected_id = notes->current_id;
  snapshot->saved_cursor = cursor;
  HistoryPush(notes->history, snapshot, FreeClonedNotesData, false);
}

/**
 * Restore NotesData from a history snapshot.
 * @param notes Pointer to NotesData to restore into
 * @param data Pointer to cloned NotesData snapshot
 */
void RestoreNotesData(NotesData* notes, void* data) {
  NotesData* snapshot = (NotesData*)data;
  if (!notes || !snapshot) return;

  /* Free current items */
  for (int i = 0; i < notes->count; i++) {
    GapBufferFree(notes->items[i]->text);
    free(notes->items[i]);
  }

  /* Restore from snapshot - deep copy */
  notes->count = snapshot->count;
  notes->capacity = snapshot->capacity;
  notes->current_id = snapshot->current_id;
  notes->max_lines = snapshot->max_lines;
  notes->total_lines = snapshot->total_lines;
  notes->render_width = snapshot->render_width;
  notes->is_move_mode = snapshot->is_move_mode;

  /* Reallocate and deep copy items */
  NoteItem** new_items =
    (NoteItem**)malloc(sizeof(NoteItem*) * notes->capacity);
  if (!new_items) return;
  for (int i = 0; i < snapshot->count; i++) {
    NoteItem* srcItem = snapshot->items[i];
    NoteItem* dstItem = (NoteItem*)malloc(sizeof(NoteItem));
    if (!dstItem) {
      for (int j = 0; j < i; j++) {
        GapBufferFree(new_items[j]->text);
        free(new_items[j]);
      }
      free(new_items);
      return;
    }
    dstItem->text = GapBufferClone(srcItem->text);
    if (!dstItem->text) {
      free(dstItem);
      for (int j = 0; j < i; j++) {
        GapBufferFree(new_items[j]->text);
        free(new_items[j]);
      }
      free(new_items);
      return;
    }
    dstItem->state = srcItem->state;
    dstItem->id = srcItem->id;
    dstItem->parent_id = srcItem->parent_id;
    dstItem->depth = srcItem->depth;
    new_items[i] = dstItem;
  }
  free(notes->items);
  notes->items = new_items;
}

/**
 * ---------------------------------------------------------------------------
 * Rendering
 * ---------------------------------------------------------------------------
 */

/**
 * Render all notes in the specified area.
 * @param app Pointer to the application data (for click region registration)
 * @param notes Pointer to the NotesData
 * @param start_x Left boundary of render area
 * @param start_y Top boundary of render area
 * @param end_x Right boundary of render area
 * @param end_y Bottom boundary of render area
 * @param input Pointer to input state for cursor display
 * @param mode Current input mode affecting rendering
 */
void RenderNotes(AppData* app, NotesData* notes, int start_x, int start_y,
                 int end_x, int end_y, InputState* input, int mode) {
  if (!notes) return;

  int max_width = end_x - start_x;
  int max_height = end_y - start_y;

  if (max_width <= 0 || max_height <= 0) return;

  notes->max_lines = max_height;
  notes->render_width = max_width;
  notes->total_lines = 0;

  bool is_editing =
    ((mode == NORMAL || mode == INSERT || mode == VISUAL) &&
     (notes->current_id >= 0 || (input && (input->pending_parent_id >= 0 ||
                                           input->insert_after_id >= 0))));

  int pending_depth = 0;
  int pending_already_rendered = 0;
  int pending_insert_after = -1;
  if (is_editing && input && input->pending_parent_id >= 0) {
    NoteItem* parent = findByID(notes, input->pending_parent_id);
    if (parent) pending_depth = parent->depth + 1;
  }

  for (int i = 0; i < notes->count; i++) {
    bool is_selected = (notes->items[i]->id == notes->current_id);
    int depth = notes->items[i]->depth;
    int indent = depth * 2;

    const char* prefix;
    switch (notes->items[i]->state) {
      case NOTE_DONE:
        prefix = "[ ] ";
        break;
      case NOTE_UNDONE:
        prefix = "[ ] ";
        break;
      case NOTE_PLAIN:
      default:
        prefix = " - ";
    }

    int prefix_len = (int)strlen(prefix);
    int item_wrap_width = max_width - prefix_len - indent;
    if (item_wrap_width <= 0) item_wrap_width = 1;

    if (is_editing && is_selected) {
      notes->total_lines += GetNoteLines(notes->items[i], item_wrap_width);
      continue;
    }

    notes->total_lines += GetNoteLines(notes->items[i], item_wrap_width);
  }

  if (pending_depth > 0 && input && input->len > 0) {
    const char* prefix = input->is_task ? "[ ] " : " - ";
    int prefix_len = (int)strlen(prefix);
    int indent = pending_depth * 2;
    int item_wrap_width = max_width - prefix_len - indent;
    if (item_wrap_width <= 0) item_wrap_width = 1;
    notes->total_lines += GetNoteLinesFromText(input->buffer, item_wrap_width);
  } else if (pending_depth > 0 && (!input || input->len == 0))
    notes->total_lines += 1;

  int render_width = notes->render_width;
  int render_y = start_y;

  for (int i = 0; i < notes->count && render_y < end_y; i++) {
    bool is_selected = (notes->items[i]->id == notes->current_id);
    int depth = notes->items[i]->depth;
    int indent = depth * 2;

    if (is_editing && is_selected) {
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      const char* edit_prefix;
      switch (notes->items[i]->state) {
        case NOTE_DONE:
          edit_prefix = "[X] ";
          break;
        case NOTE_UNDONE:
          edit_prefix = "[ ] ";
          break;
        default:
          edit_prefix = " - ";
          break;
      }
      if (mode == INSERT) {
        const char* prefix = edit_prefix;
        if (input && input->len > 0) {
          int prefix_len = (int)strlen(prefix);
          int input_wrap_width = max_width - prefix_len - indent;
          if (input_wrap_width <= 0) input_wrap_width = 1;
          char** wrapped_input = NULL;
          int num_input_lines =
            WrapText(input->buffer, input_wrap_width, &wrapped_input);
          if (num_input_lines == 0) {
            num_input_lines = 1;
            wrapped_input = (char**)malloc(sizeof(char*));
            if (wrapped_input) wrapped_input[0] = (char*)"";
          }
          for (int wl = 0; wl < num_input_lines && render_y < end_y; wl++) {
            if (wl == 0) {
              int left_len =
                (input->cursor <= input->len) ? input->cursor : input->len;
              char* buffer = (char*)malloc((size_t)(max_width + 1));
              char* left_part = (char*)malloc((size_t)(left_len + 1));
              if (buffer && left_part) {
                strncpy(left_part, input->buffer, left_len);
                left_part[left_len] = '\0';
                const char* right_part = input->buffer + input->cursor;
                snprintf(buffer, (size_t)(max_width + 1), "%s%s|%s", prefix, left_part,
                         right_part);
                mvprintw(render_y, start_x + indent, "%s", buffer);
              }
              free(buffer);
              free(left_part);
            } else {
              const char* line =
                wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "";
              mvprintw(render_y, start_x + indent + strlen(prefix), "%s", line);
            }
            render_y++;
          }
          for (int wl = 0; wl < num_input_lines; wl++)
            if (wrapped_input[wl]) free(wrapped_input[wl]);
          free(wrapped_input);
        } else {
          mvprintw(render_y, start_x + indent, "%s", INSERT_CURSOR_ICON);
          render_y++;
        }
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        continue;
      }
      if (input && input->len > 0 && (mode == NORMAL || mode == VISUAL)) {
        if (input && input->len > 0) {
          const char* prefix = edit_prefix;
          int prefix_len = (int)strlen(prefix);
          int input_wrap_width = max_width - prefix_len - indent;
          if (input_wrap_width <= 0) input_wrap_width = 1;

          char** wrapped_input = NULL;
          int num_input_lines =
            WrapText(input->buffer, input_wrap_width, &wrapped_input);

          if (num_input_lines == 0) {
            num_input_lines = 1;
            wrapped_input = (char**)malloc(sizeof(char*));
            if (wrapped_input) wrapped_input[0] = (char*)"";
          }

          int sel_start = 0;
          int sel_end = 0;
          if (mode == VISUAL && input) {
            sel_start = input->selection.start;
            sel_end = input->cursor;
            if (sel_start > sel_end) {
              int tmp = sel_start;
              sel_start = sel_end;
              sel_end = tmp;
            }
          }

          for (int wl = 0; wl < num_input_lines && render_y < end_y; wl++) {
            int line_char_offset = wl * input_wrap_width;
            int line_len = (int)strlen(
              wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "");
            if (wl == 0) mvprintw(render_y, start_x + indent, "%s", prefix);
            for (int ci = 0; ci < line_len; ci++) {
              int abs_pos = line_char_offset + ci;
              bool in_selection =
                (mode == VISUAL && abs_pos >= sel_start && abs_pos <= sel_end);
              bool is_cursor = (mode == NORMAL && abs_pos == input->cursor);
              if (is_cursor || in_selection)
                SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
              mvaddch(render_y, start_x + indent + prefix_len + ci,
                      wrapped_input[wl][ci] ? wrapped_input[wl][ci] : ' ');
              if (is_cursor || in_selection)
                SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
            }
            int cursor_line_start = line_char_offset;
            int cursor_line_end = line_char_offset + line_len - 1;
            if (input->cursor > cursor_line_end && wl == num_input_lines - 1) {
              bool is_cursor = (mode == NORMAL);
              if (is_cursor) SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
              mvaddch(render_y, start_x + indent + prefix_len + line_len, ' ');
              if (is_cursor) SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
            }
            render_y++;
          }

          for (int wl = 0; wl < num_input_lines; wl++)
            if (wrapped_input[wl]) free(wrapped_input[wl]);
          free(wrapped_input);
        }
      }
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      continue;
    }

    char* text = GapBufferToString(notes->items[i]->text);
    if (!text) continue;

    const char* prefix;
    switch (notes->items[i]->state) {
      case NOTE_DONE:
        prefix = "[X] ";
        break;
      case NOTE_UNDONE:
        prefix = "[ ] ";
        break;
      case NOTE_PLAIN:
      default:
        prefix = " - ";
        break;
    }
    int prefix_len = (int)strlen(prefix);
    int item_wrap_width = max_width - prefix_len - indent;
    if (item_wrap_width <= 0) item_wrap_width = 1;

    char** wrapped_lines = NULL;
    int num_lines = WrapText(text, item_wrap_width, &wrapped_lines);
    free(text);

    if (num_lines == 0) {
      num_lines = 1;
      wrapped_lines = (char**)malloc(sizeof(char*));
      if (wrapped_lines) wrapped_lines[0] = (char*)"";
    }

    for (int wl = 0; wl < num_lines && render_y < end_y; wl++) {
      if (is_selected && notes->is_move_mode)
        SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
      else if (is_selected)
        SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      else
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);

      if (wl == 0) {
        char* buffer = (char*)malloc((size_t)(max_width + 1));
        if (buffer) {
          snprintf(buffer, (size_t)(max_width + 1), "%s%s", prefix,
                   wrapped_lines[wl] ? wrapped_lines[wl] : "");
          mvprintw(render_y, start_x + indent, "%s", buffer);
          free(buffer);
        }
      } else
        mvprintw(render_y, start_x + indent + prefix_len, "%s",
                 wrapped_lines[wl] ? wrapped_lines[wl] : "");

      RegisterClickRegion(app, start_x + indent, render_y, max_width - indent,
                          1, REGION_NOTE_ITEM, NULL, -1, -1,
                          notes->items[i]->id);
      render_y++;
    }
    for (int wl = 0; wl < num_lines; wl++)
      if (wrapped_lines[wl]) free(wrapped_lines[wl]);
    free(wrapped_lines);

    if (input && input->pending_parent_id >= 0 &&
        notes->items[i]->id == input->pending_parent_id) {
      NoteItem* parent = notes->items[i];
      int parent_depth = parent->depth;
      int last_child_idx = i;
      for (int j = i + 1; j < notes->count; j++) {
        if (notes->items[j]->parent_id == input->pending_parent_id)
          last_child_idx = j;
        if (notes->items[j]->depth <= parent_depth) break;
      }
      pending_insert_after = last_child_idx;
    }

    if (input && input->insert_after_id >= 0 &&
        notes->items[i]->id == input->insert_after_id) {
      NoteItem* note = notes->items[i];
      int note_depth = note->depth;
      pending_depth = note_depth;
      int last_descendant_idx = i;
      for (int j = i + 1; j < notes->count; j++) {
        if (notes->items[j]->depth > note_depth)
          last_descendant_idx = j;
        else
          break;
      }
      pending_insert_after = last_descendant_idx;
    }

    if (input &&
        (input->pending_parent_id >= 0 || input->insert_after_id >= 0) &&
        pending_depth >= 0 && i == pending_insert_after &&
        !pending_already_rendered) {
      int indent = pending_depth * 2;
      const char* prefix = input->is_task ? "[ ] " : " - ";
      if (mode == INSERT) {
        if (input->len > 0) {
          int prefix_len = (int)strlen(prefix);
          int input_wrap_width = max_width - prefix_len - indent;
          if (input_wrap_width <= 0) input_wrap_width = 1;
          char** wrapped_input = NULL;
          int num_input_lines =
            WrapText(input->buffer, input_wrap_width, &wrapped_input);
          if (num_input_lines == 0) {
            num_input_lines = 1;
            wrapped_input = (char**)malloc(sizeof(char*));
            if (wrapped_input) wrapped_input[0] = (char*)"";
          }
          for (int wl = 0; wl < num_input_lines && render_y < end_y; wl++) {
            if (wl == 0) {
              int left_len =
                (input->cursor <= input->len) ? input->cursor : input->len;
              char* buffer = (char*)malloc((size_t)(max_width + 1));
              char* left_part = (char*)malloc((size_t)(left_len + 1));
              if (buffer && left_part) {
                strncpy(left_part, input->buffer, left_len);
                left_part[left_len] = '\0';
                const char* right_part = input->buffer + input->cursor;
                snprintf(buffer, (size_t)(max_width + 1), "%s%s|%s", prefix, left_part,
                         right_part);
                mvprintw(render_y, start_x + indent, "%s", buffer);
              }
              free(buffer);
              free(left_part);
            } else {
              const char* line =
                wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "";
              mvprintw(render_y, start_x + indent + strlen(prefix), "%s", line);
            }
            render_y++;
          }
          for (int wl = 0; wl < num_input_lines; wl++)
            if (wrapped_input[wl]) free(wrapped_input[wl]);
          free(wrapped_input);
        } else {
          char* buffer = (char*)malloc((size_t)(max_width + 1));
          if (buffer) {
            snprintf(buffer, (size_t)(max_width + 1), "%s%s", prefix,
                     INSERT_CURSOR_ICON);
            mvprintw(render_y, start_x + indent, "%s", buffer);
            free(buffer);
          }
          render_y++;
        }
      } else if ((mode == NORMAL || mode == VISUAL) && input) {
        if (input->len > 0) {
          int prefix_len = (int)strlen(prefix);
          int input_wrap_width = max_width - prefix_len - indent;
          if (input_wrap_width <= 0) input_wrap_width = 1;
          char** wrapped_input = NULL;
          int num_input_lines =
            WrapText(input->buffer, input_wrap_width, &wrapped_input);
          if (num_input_lines == 0) {
            num_input_lines = 1;
            wrapped_input = (char**)malloc(sizeof(char*));
            if (wrapped_input) wrapped_input[0] = (char*)"";
          }
          int sel_start = 0;
          int sel_end = 0;
          if (mode == VISUAL && input) {
            sel_start = input->selection.start;
            sel_end = input->cursor;
            if (sel_start > sel_end) {
              int tmp = sel_start;
              sel_start = sel_end;
              sel_end = tmp;
            }
          }
          for (int wl = 0; wl < num_input_lines && render_y < end_y; wl++) {
            int line_char_offset = wl * input_wrap_width;
            int line_len = (int)strlen(
              wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "");
            if (wl == 0) mvprintw(render_y, start_x + indent, "%s", prefix);
            for (int ci = 0; ci < line_len; ci++) {
              int abs_pos = line_char_offset + ci;
              bool is_cursor = (mode == NORMAL && abs_pos == input->cursor);
              bool in_selection =
                (mode == VISUAL && abs_pos >= sel_start && abs_pos <= sel_end);
              if (is_cursor || in_selection)
                SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
              mvaddch(render_y, start_x + indent + prefix_len + ci,
                      wrapped_input[wl][ci] ? wrapped_input[wl][ci] : ' ');
              if (is_cursor || in_selection)
                SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
            }
            render_y++;
          }
          for (int wl = 0; wl < num_input_lines; wl++)
            if (wrapped_input[wl]) free(wrapped_input[wl]);
          free(wrapped_input);
        } else {
          mvprintw(render_y, start_x + indent, "%s%s", prefix,
                   INSERT_CURSOR_ICON);
          render_y++;
        }
      }
      pending_already_rendered = 1;
    }
  }

  if (input && input->insert_after_id >= 0 && pending_insert_after < 0 &&
      notes->count == 0) {
    pending_depth = 0;
    pending_insert_after = -1;
  }

  if (input && input->insert_after_id >= 0 && pending_insert_after < 0 &&
      notes->count > 0) {
    for (int i = 0; i < notes->count; i++) {
      if (notes->items[i]->id == input->insert_after_id) {
        int note_depth = notes->items[i]->depth;
        pending_depth = note_depth;
        int last_descendant_idx = i;
        for (int j = i + 1; j < notes->count; j++) {
          if (notes->items[j]->depth > note_depth)
            last_descendant_idx = j;
          else
            break;
        }
        pending_insert_after = last_descendant_idx;
        break;
      }
    }
  }

  if (input && input->insert_after_id >= 0 && pending_insert_after < 0) {
    if (notes->count > 0) {
      pending_depth = 0;
      pending_insert_after = notes->count - 1;
    } else {
      pending_depth = 0;
      pending_insert_after = -1;
    }
  }

  const char* input_buffer = input ? input->buffer : NULL;
  int input_len = input ? input->len : 0;
  int input_cursor_pos = input ? input->cursor : 0;

  bool input_already_rendered =
    is_editing &&
    (notes->current_id >= 0 || (input && (input->pending_parent_id >= 0 ||
                                          input->insert_after_id >= 0))) &&
    ((input && input->len > 0) || mode == INSERT);

  if (!input_already_rendered && render_y < end_y &&
      (input_len > 0 || mode == INSERT)) {
    const char* prefix = input && input->is_task ? "[ ] " : " - ";
    int prefix_len = (int)strlen(prefix);
    int input_wrap_width = max_width - prefix_len;

    char** wrapped_input = NULL;
    int num_input_lines = 0;

    if (input_buffer && input_len > 0)
      num_input_lines =
        WrapText(input_buffer, input_wrap_width, &wrapped_input);

    if (num_input_lines == 0 && mode == INSERT) {
      wrapped_input = (char**)malloc(sizeof(char*));
      if (wrapped_input) {
        wrapped_input[0] = (char*)malloc(1);
        if (wrapped_input[0]) wrapped_input[0][0] = '\0';
      }
      num_input_lines = 1;
    }

    for (int wl = 0; wl < num_input_lines && render_y < end_y; wl++) {
      if (mode == INSERT && wl == 0 && input_len == 0) {
        mvprintw(render_y, start_x, "%s%s", prefix, INSERT_CURSOR_ICON);
        render_y++;
        continue;
      } else if (mode == INSERT && wl == 0 && input_len > 0) {
        int left_len =
          (input_cursor_pos <= input_len) ? input_cursor_pos : input_len;
        char* display_buf = (char*)malloc((size_t)(max_width + 1));
        char* left_part = (char*)malloc((size_t)(left_len + 1));
        if (display_buf && left_part) {
          strncpy(left_part, input_buffer, left_len);
          left_part[left_len] = '\0';
          const char* right_part = input_buffer + input_cursor_pos;
          snprintf(display_buf, (size_t)(max_width + 1), "%s%s|%s", prefix,
                   left_part, right_part);
          mvprintw(render_y, start_x, "%s", display_buf);
        }
        free(display_buf);
        free(left_part);
        render_y++;
        continue;
      } else if (mode == INSERT && wl > 0) {
        const char* line =
          wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "";
        mvprintw(render_y, start_x + prefix_len, "%s", line);
        render_y++;
        continue;
      } else if (mode == NORMAL || mode == VISUAL) {
        int sel_start = 0;
        int sel_end = 0;
        if (input) {
          if (mode == VISUAL) {
            sel_start = input->selection.start;
            sel_end = input->cursor;
            if (sel_start > sel_end) {
              int tmp = sel_start;
              sel_start = sel_end;
              sel_end = tmp;
            }
          }
        }
        int line_char_offset = wl * input_wrap_width;
        int line_len = (int)strlen(
          wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "");
        if (wl == 0) mvprintw(render_y, start_x, "%s", prefix);
        for (int ci = 0; ci < line_len; ci++) {
          int abs_pos = line_char_offset + ci;
          bool in_selection =
            (mode == VISUAL && abs_pos >= sel_start && abs_pos <= sel_end);
          bool is_cursor = (mode == NORMAL && abs_pos == input_cursor_pos);
          if (is_cursor || in_selection)
            SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
          mvaddch(render_y, start_x + prefix_len + ci, wrapped_input[wl][ci]);
          if (is_cursor || in_selection)
            SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        }
        if (wl == 0 && input_cursor_pos >= input_len) {
          bool is_cursor = (mode == NORMAL && input_cursor_pos >= input_len);
          if (is_cursor) SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
          mvaddch(render_y, start_x + prefix_len + input_len, ' ');
          if (is_cursor) SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        }
        render_y++;
      }
    }

    if (wrapped_input) {
      for (int wl = 0; wl < num_input_lines; wl++)
        if (wrapped_input[wl]) free(wrapped_input[wl]);
      free(wrapped_input);
    }
  }
}

/**
 * Wrap text to fit within a maximum width.
 * @param text Text to wrap
 * @param max_width Maximum width for each line
 * @param out_lines Pointer to store array of wrapped lines (caller must free)
 * @return Number of lines created, or -1 on failure
 */
int WrapText(const char* text, int max_width, char*** out_lines) {
  if (!text || max_width <= 0) {
    *out_lines = NULL;
    return 0;
  }

  int line_count = 0;
  int capacity = 4;
  char** lines = (char**)malloc(sizeof(char*) * capacity);
  if (!lines) return 0;

  const char* start = text;
  const char* ptr = text;

  while (*ptr) {
    int len = 0;
    const char* last_space = NULL;

    while (*ptr && len < max_width) {
      if (*ptr == ' ') last_space = ptr;
      len++;
      ptr++;
    }

    if (len >= max_width && last_space && last_space > start) {
      ptr = last_space + 1;
      len = last_space - start;
    }

    while (len > 0 && start[len - 1] == ' ') len--;

    if (len == 0) {
      char* line = (char*)malloc(1);
      if (!line) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        *out_lines = NULL;
        return 0;
      }
      line[0] = '\0';
      if (line_count >= capacity) {
        capacity *= 2;
        char** new_lines = (char**)realloc(lines, sizeof(char*) * capacity);
        if (!new_lines) {
          free(line);
          for (int i = 0; i < line_count; i++) free(lines[i]);
          free(lines);
          *out_lines = NULL;
          return 0;
        }
        lines = new_lines;
      }
      lines[line_count++] = line;
      if (*ptr == '\0') break;
      while (*ptr == ' ') ptr++;
      start = ptr;
      continue;
    }

    char* line = (char*)malloc(len + 1);
    if (!line) {
      for (int i = 0; i < line_count; i++) free(lines[i]);
      free(lines);
      *out_lines = NULL;
      return 0;
    }
    memcpy(line, start, len);
    line[len] = '\0';

    if (line_count >= capacity) {
      capacity *= 2;
      char** new_lines = (char**)realloc(lines, sizeof(char*) * capacity);
      if (!new_lines) {
        free(line);
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        *out_lines = NULL;
        return 0;
      }
      lines = new_lines;
    }
    lines[line_count++] = line;

    if (*ptr == '\0') break;
    while (*ptr == ' ') ptr++;
    start = ptr;
  }

  *out_lines = lines;
  return line_count;
}

/**
 * Render history debug info in top-right corner.
 * Only renders if DEBUG is defined.
 * @param notes Pointer to NotesData
 * @param start_x Left boundary of render area
 * @param start_y Top boundary of render area
 */
void RenderNotesHistoryDebug(NotesData* notes, int start_x, int start_y) {
  if (!notes || !notes->history) return;

  History* h = notes->history;
  int x = start_x - 22;
  int y = start_y + 2;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(y, x, "HISTORY DEBUG");
  y++;

  /* Count stacks */
  const int max_per_stack = 5;
  int past_count = 0;
  HistoryNode* node = h->past;
  while (node) {
    past_count++;
    node = node->next;
  }

  int future_count = 0;
  node = h->future;
  while (node) {
    future_count++;
    node = node->next;
  }

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y++, x, "past: %d, future: %d", past_count, future_count);
  mvprintw(y++, x, "current_id: %d", notes->current_id);
  mvprintw(y++, x, "last_affected: %d", notes->last_affected_id);

  /* Show past stack (top is most recent) */
  if (past_count > 0) {
    SetColor(COLOR_CYAN, NO_COLOR, A_NORMAL);
    mvprintw(y++, x, "PAST:");
    node = h->past;
    int idx = 0;
    while (node && idx < max_per_stack) {
      NotesData* snap = (NotesData*)node->data;
      if (snap)
        mvprintw(y++, x, "  [%d] id:%d cnt:%d", idx, snap->last_affected_id,
                 snap->count);
      node = node->next;
      idx++;
    }
  }

  /* Show future stack */
  if (future_count > 0) {
    SetColor(COLOR_MAGENTA, NO_COLOR, A_NORMAL);
    mvprintw(y++, x, "FUTURE:");
    node = h->future;
    int idx = 0;
    while (node && idx < max_per_stack) {
      NotesData* snap = (NotesData*)node->data;
      if (snap)
        mvprintw(y++, x, "  [%d] id:%d cnt:%d", idx, snap->last_affected_id,
                 snap->count);
      node = node->next;
      idx++;
    }
  }
}

/**
 * ---------------------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------------------
 */

/**
 * Set the maximum number of lines for note rendering.
 * Used to calculate visible area and scrolling.
 * @param notes Pointer to the NotesData
 * @param max_lines Maximum lines available
 */
void SetNotesMaxLines(NotesData* notes, int max_lines) {
  if (notes) notes->max_lines = max_lines;
}

/**
 * Calculate the number of visual lines a note will occupy.
 * @param item Pointer to the note item
 * @param render_width Available width for rendering
 * @return Number of lines the note will occupy when displayed
 */
int GetNoteLines(NoteItem* item, int render_width) {
  if (!item || render_width <= 0) return 1;
  char* text = GapBufferToString(item->text);
  if (!text) return 1;
  int count = GetNoteLinesFromText(text, render_width);
  free(text);
  return count > 0 ? count : 1;
}

/**
 * Calculate lines for plain text (without NoteItem).
 * @param text Text to calculate lines for
 * @param render_width Available width for rendering
 * @return Number of lines the text will occupy
 */
int GetNoteLinesFromText(const char* text, int render_width) {
  if (!text || render_width <= 0) return 0;
  int text_len = strlen(text);
  if (text_len == 0) return 0;
  char** lines = NULL;
  int count = WrapText(text, render_width, &lines);
  if (lines) {
    for (int i = 0; i < count; i++) free(lines[i]);
    free(lines);
  }
  return count;
}

/**
 * Double the capacity of the items array to accommodate more notes.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 */
static void growItems(AppData* app, NotesData* notes) {
  int new_capacity = notes->capacity * 2;
  NoteItem** new_items =
    (NoteItem**)realloc(notes->items, sizeof(NoteItem*) * new_capacity);
  if (new_items) {
    notes->items = new_items;
    notes->capacity = new_capacity;
  } else {
    SetError(app, "growItems", MALLOC_ERROR);
  }
}

/**
 * Generate a unique ID for a new note.
 * @param notes Pointer to the NotesData
 * @return A unique integer ID for the next note
 */
static int getNextID(NotesData* notes) {
  int max_id = 0;
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id > max_id) max_id = notes->items[i]->id;
  return max_id + 1;
}

/**
 * Find a note by its ID.
 * @param notes Pointer to the NotesData
 * @param id ID of the note to find
 * @return Pointer to the NoteItem, or NULL if not found
 */
static NoteItem* findByID(NotesData* notes, int id) {
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id == id) return notes->items[i];
  return NULL;
}

/**
 * Get the array index of a note by its ID.
 * @param notes Pointer to the NotesData
 * @param id ID of the note to find
 * @return Index in the items array, or -1 if not found
 */
static int getIndexByID(NotesData* notes, int id) {
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id == id) return i;
  return -1;
}
