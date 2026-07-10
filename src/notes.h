#ifndef NOTES_H_
#define NOTES_H_

#include <stdbool.h>
#include <stddef.h>

#include "gap_buffer.h"
#include "history.h"

typedef struct AppData AppData;
typedef struct NoteItem NoteItem;
typedef struct InputState InputState;

/**
 * Enum representing the state of a note item.
 * Determines rendering style and behavior.
 */
typedef enum { NOTE_UNDONE, NOTE_DONE, NOTE_PLAIN } NoteState;

#define NOTES_INITIAL_CAPACITY 8

/**
 * Structure representing a single note or task item.
 * Contains text content, state, and hierarchical positioning.
 */
struct NoteItem {
  GapBuffer*
    text; /**< Text content stored as gap buffer for efficient editing */
  NoteState state; /**< Current state: undone, done, or plain */
  int id;          /**< Unique identifier for this note */
  int page_id;     /**< Page this note belongs to (0-indexed) */
  int parent_id;   /**< ID of parent note (-1 for root-level notes) */
  int depth; /**< Nesting depth level (0 for root, increments for children) */
};

/**
 * Container for all notes data and editor state.
 * Manages the collection of notes and rendering parameters.
 */
typedef struct {
  NoteItem** items;     /**< Dynamic array of note item pointers */
  int count;            /**< Number of notes currently stored */
  int capacity;         /**< Current capacity of the items array */
  int current_id;       /**< Next ID to assign to a new note */
  int max_lines;        /**< Maximum lines available for rendering */
  int total_lines;      /**< Total lines occupied by all notes when rendered */
  int render_width;     /**< Width available for rendering notes */
  bool is_move_mode;    /**< Whether move mode is currently active */
  History* history;     /**< History manager for undo/redo */
  int last_affected_id;   /**< ID of last affected note */
  int last_affected_page; /**< Page of last affected note */
  int saved_cursor;       /**< Cursor position at time of history save */
  int current_page;       /**< Currently viewed page (0-indexed) */
  int page_count;         /**< Total number of pages */
  int page_capacity;      /**< Allocated capacity for pages */
  bool transitioning;     /**< True while page transition animation plays */
  int transition_target;  /**< Page to navigate to after transition */
  int drag_note_id;       /**< Note ID being dragged (-1 = none) */
  int drag_start_y;       /**< Y position where drag started */
  bool drag_moved;        /**< true if actual movement happened during drag */
} NotesData;

/**
 * ---------------------------------------------------------------------------
 * NotesData Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new NotesData container with default values.
 * @return Pointer to the created NotesData, or NULL on allocation failure
 */
NotesData* CreateNotesData(void);

/**
 * Free all memory associated with NotesData.
 * @param notes Pointer to the NotesData to free
 */
void FreeNotesData(NotesData* notes);

/**
 * Add a new note at the end of the list.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 * @param text Initial text content for the note
 * @param state Initial state of the note
 */
void AddNote(AppData* app, NotesData* notes, const char* text, NoteState state);

/**
 * Add a child note under a parent note.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param notes Pointer to the NotesData
 * @param parent_id ID of the parent note
 * @param text Initial text content
 * @param state Initial state
 */
void AddChildNote(AppData* app, NotesData* notes, int parent_id,
                  const char* text, NoteState state);

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
                  const char* text, NoteState state);

/**
 * Update an existing note's content and/or state.
 * @param notes Pointer to the NotesData
 * @param note_id ID of the note to update
 * @param text New text content (or NULL to keep existing)
 * @param state New state (or -1 to keep existing)
 */
void UpdateNote(NotesData* notes, int note_id, const char* text,
                NoteState state);

/**
 * Delete the currently selected note.
 * @param notes Pointer to the NotesData
 */
void DeleteNote(NotesData* notes);

/**
 * Renumber pages so there are no gaps in page_id values.
 * Updates page_count and current_page accordingly.
 * @param notes Pointer to the NotesData
 */
void CompactPages(NotesData* notes);

/**
 * Toggle the selected note between done and undone states.
 * @param notes Pointer to the NotesData
 */
void ToggleTask(NotesData* notes);

/**
 * ---------------------------------------------------------------------------
 * Selection
 * ---------------------------------------------------------------------------
 */

/**
 * Move selection up one position in the notes list.
 * @param notes Pointer to the NotesData
 */
void NoteUp(NotesData* notes);

/**
 * Move selection down one position in the notes list.
 * @param notes Pointer to the NotesData
 */
void NoteDown(NotesData* notes);

/**
 * Get the current note selection index.
 * @param notes Pointer to the NotesData
 * @return Index of the currently selected note, or -1 if none
 */
int GetSelectedNoteIndex(NotesData* notes);

/**
 * ---------------------------------------------------------------------------
 * Move mode
 * ---------------------------------------------------------------------------
 */

/**
 * Move the selected note up one position in the list.
 * @param notes Pointer to the NotesData
 */
void MoveNoteUp(NotesData* notes);

/**
 * Move the selected note down one position in the list.
 * @param notes Pointer to the NotesData
 */
void MoveNoteDown(NotesData* notes);

/**
 * Promote the selected note (decrease depth, move to parent level).
 * @param notes Pointer to the NotesData
 */
void PromoteNote(NotesData* notes);

/**
 * Demote the selected note (increase depth, nest under previous sibling).
 * @param notes Pointer to the NotesData
 */
void DemoteNote(NotesData* notes);

/**
 * ---------------------------------------------------------------------------
 * History 
 * ---------------------------------------------------------------------------
 */

/**
 * Create a deep copy of NotesData for history snapshots.
 * @param src Source NotesData to clone
 * @return New NotesData copy, or NULL on failure
 */
NotesData* CloneNotesData(const NotesData* src);

/**
 * Restore NotesData from a history snapshot.
 * @param notes Pointer to NotesData to restore into
 * @param data Pointer to cloned NotesData snapshot
 */
void RestoreNotesData(NotesData* notes, void* data);

/**
 * Free cloned NotesData for history cleanup.
 * @param data Pointer to NotesData to free
 */
void FreeClonedNotesData(void* data);

/**
 * Save current notes state to history for undo.
 * @param notes Pointer to NotesData
 * @param cursor Cursor position at time of save
 */
void SaveNotesToHistory(NotesData* notes, int cursor);

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
                 int end_x, int end_y, InputState* input, int mode);

/**
 * Wrap text to fit within a maximum width.
 * @param text Text to wrap
 * @param max_width Maximum width for each line
 * @param out_lines Pointer to store array of wrapped lines (caller must free)
 * @return Number of lines created, or -1 on failure
 */
int WrapText(const char* text, int max_width, char*** out_lines);

/**
 * Render history debug info in top-right corner.
 * Only renders if DEBUG is defined.
 * @param notes Pointer to NotesData
 * @param start_x Left boundary of render area
 * @param start_y Top boundary of render area
 */
void RenderNotesHistoryDebug(NotesData* notes, int start_x, int start_y);

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
void SetNotesMaxLines(NotesData* notes, int max_lines);

/**
 * Calculate the number of visual lines a note will occupy.
 * @param item Pointer to the note item
 * @param render_width Available width for rendering
 * @return Number of lines the note will occupy when displayed
 */
int GetNoteLines(NoteItem* item, int render_width);

/**
 * Calculate lines for plain text (without NoteItem).
 * @param text Text to calculate lines for
 * @param render_width Available width for rendering
 * @return Number of lines the text will occupy
 */
int GetNoteLinesFromText(const char* text, int render_width);

#endif /* NOTES_H_ */
