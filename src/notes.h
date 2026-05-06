#ifndef NOTES_H_
#define NOTES_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct AppData AppData;
typedef struct NoteItem NoteItem;

struct NoteItem {
  char* text;
  bool done;
  NoteItem* next;
  NoteItem* prev;
};

typedef struct {
  NoteItem* head;
  NoteItem* tail;
  NoteItem* current;
  int count;
  bool insert_mode;
} NotesData;

/* Initialize notes data */
NotesData* CreateNotesData(void);

/* Free notes data */
void FreeNotesData(NotesData* notes);

/* Add a new note/task */
void AddNote(NotesData* notes, const char* text, bool is_task);

/* Delete the selected note/task */
void DeleteNote(NotesData* notes);

/* Toggle the done status of the selected task */
void ToggleTask(NotesData* notes);

/* Move selection up */
void NoteUp(NotesData* notes);

/* Move selection down */
void NoteDown(NotesData* notes);

/* Wrapper functions for keybinding (take AppData*) */
void NoteUpApp(AppData* app);
void NoteDownApp(AppData* app);

/* Get the selected note index */
int GetSelectedNoteIndex(NotesData* notes);

/* Render notes in a panel */
/* If input_buffer is not NULL, it will be rendered at the end (for INSERT mode) */
void RenderNotes(NotesData* notes, int start_x, int start_y, int width,
                 int height, const char* input_buffer, int mode);

#endif /* NOTES_H_ */
