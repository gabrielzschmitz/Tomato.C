#ifndef NOTES_H_
#define NOTES_H_

#include <stdbool.h>
#include <stddef.h>

#include "gap_buffer.h"

typedef struct AppData AppData;
typedef struct NoteItem NoteItem;
typedef struct InputState InputState;

typedef enum { NOTE_UNDONE, NOTE_DONE, NOTE_PLAIN } NoteState;

#define NOTES_INITIAL_CAPACITY 8

struct NoteItem {
  GapBuffer* text;
  NoteState state;
  int id;
};

typedef struct {
  NoteItem** items;
  int count;
  int capacity;
  int current_id;
  int max_lines;
  int total_lines;
  int render_width;
} NotesData;

NotesData* CreateNotesData(void);
void FreeNotesData(NotesData* notes);
void SetNotesMaxLines(NotesData* notes, int max_lines);
int GetNoteLines(NoteItem* item, int render_width);
int GetNoteLinesFromText(const char* text, int render_width);
void AddNote(NotesData* notes, const char* text, NoteState state);
void UpdateNote(NotesData* notes, int note_id, const char* text, NoteState state);
void DeleteNote(NotesData* notes);
void ToggleTask(NotesData* notes);
void NoteUp(NotesData* notes);
void NoteDown(NotesData* notes);
void NoteUpApp(AppData* app);
void NoteDownApp(AppData* app);
int GetSelectedNoteIndex(NotesData* notes);
void RenderNotes(NotesData* notes, int start_x, int start_y, int end_x,
                 int end_y, InputState* input, int mode);
int WrapText(const char* text, int max_width, char*** out_lines);

#endif /* NOTES_H_ */
