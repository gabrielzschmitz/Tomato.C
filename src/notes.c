#include "notes.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "input.h"
#include "tomato.h"

/* Forward declaration for SetColor */
void SetColor(short int fg, short int bg, chtype attr);
#define NO_COLOR -1

/* Initialize notes data */
NotesData* CreateNotesData(void) {
  NotesData* notes = (NotesData*)malloc(sizeof(NotesData));
  if (notes == NULL) return NULL;

  notes->head = NULL;
  notes->tail = NULL;
  notes->current = NULL;
  notes->count = 0;
  notes->insert_mode = false;

  return notes;
}

/* Free notes data */
void FreeNotesData(NotesData* notes) {
  if (notes == NULL) return;

  NoteItem* current = notes->head;
  while (current != NULL) {
    NoteItem* next = current->next;
    free(current->text);
    free(current);
    current = next;
  }

  free(notes);
}

/* Add a new note/task */
void AddNote(NotesData* notes, const char* text, bool is_task) {
  if (notes == NULL || text == NULL) return;

  NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
  if (item == NULL) return;

  /* Format: tasks have [ ] or [X] prefix */
  if (is_task) {
    size_t len = strlen(text) + 5; /* "[ ] " + text + null */
    item->text = (char*)malloc(len);
    if (item->text) snprintf(item->text, len, "[ ] %s", text);
    item->done = false;
  } else {
    size_t len = strlen(text) + 3; /* "- " + text + null */
    item->text = (char*)malloc(len);
    if (item->text) snprintf(item->text, len, "- %s", text);
    item->done = false;
  }

  if (item->text == NULL) {
    free(item);
    return;
  }

  item->next = NULL;
  item->prev = notes->tail;

  if (notes->tail != NULL) {
    notes->tail->next = item;
  } else {
    notes->head = item;
    notes->current = item;
  }
  notes->tail = item;
  notes->count++;
}

/* Delete the selected note/task */
void DeleteNote(NotesData* notes) {
  if (notes == NULL || notes->current == NULL) return;

  NoteItem* to_delete = notes->current;

  if (to_delete->prev != NULL)
    to_delete->prev->next = to_delete->next;
  else
    notes->head = to_delete->next;

  if (to_delete->next != NULL) {
    to_delete->next->prev = to_delete->prev;
    notes->current = to_delete->next;
  } else {
    notes->tail = to_delete->prev;
    notes->current = notes->tail;
  }

  free(to_delete->text);
  free(to_delete);
  notes->count--;

  if (notes->count == 0) notes->head = notes->tail = notes->current = NULL;
}

/* Toggle the done status of the selected task */
void ToggleTask(NotesData* notes) {
  if (notes == NULL || notes->current == NULL) return;

  NoteItem* item = notes->current;

  /* Only toggle if it's a task (starts with [ ] or [X]) */
  if (strstr(item->text, "[ ]") != NULL) {
    char* bracket = strstr(item->text, "[ ]");
    if (bracket) {
      bracket[1] = 'X';
      item->done = true;
    }
  } else if (strstr(item->text, "[X]") != NULL) {
    char* bracket = strstr(item->text, "[X]");
    if (bracket) {
      bracket[1] = ' ';
      item->done = false;
    }
  }
}

/* Move selection up */
void NoteUp(NotesData* notes) {
  if (notes == NULL || notes->current == NULL) return;

  if (notes->current->prev != NULL) notes->current = notes->current->prev;
}

/* Move selection down */
void NoteDown(NotesData* notes) {
  if (notes == NULL || notes->current == NULL) return;

  if (notes->current->next != NULL) notes->current = notes->current->next;
}

/* Render notes in a panel */
void RenderNotes(NotesData* notes, int start_x, int start_y, int width,
                 int height, const char* input_buffer, int mode) {
  if (notes == NULL) return;

  int y = start_y;
  NoteItem* item = notes->head;

  while (item != NULL && y < start_y + height - 1) {
    bool is_selected = (item == notes->current);

    if (is_selected)
      SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    else
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);

    int max_len = width - 2;
    char buffer[max_len + 1];
    strncpy(buffer, item->text, max_len);
    buffer[max_len] = '\0';

    mvprintw(y, start_x, "%s", buffer);
    y++;
    item = item->next;
  }

  /* Show input buffer if there's content OR if in INSERT mode (show cursor) */
  if ((input_len > 0 || mode == INSERT) && y < start_y + height - 1) {
    /* Get the prefix based on input_mode_type */
    const char* prefix = (input_mode_type == 0) ? "[ ] " : "- ";
    int prefix_len = strlen(prefix);

    /* Mode-specific cursor indicators */
    if (mode == INSERT) {
      if (input_len == 0) {
        /* Show cursor at start after prefix when buffer is empty */
        mvprintw(y, start_x, "%s%s", prefix, INSERT_CURSOR_ICON);
      } else {
        /* Show ▏ cursor at cursor position for INSERT mode */
        char left_part[input_cursor_pos + 1];
        char right_part[input_len - input_cursor_pos + 1];
        strncpy(left_part, input_buffer, input_cursor_pos);
        left_part[input_cursor_pos] = '\0';
        strcpy(right_part, input_buffer + input_cursor_pos);
        mvprintw(y, start_x, "%s%s▏%s", prefix, left_part, right_part);
      }
    } else if (mode == VISUAL) {
      /* Highlight selection from visual_start to cursor */
      int start_sel =
        (visual_start < input_cursor_pos) ? visual_start : input_cursor_pos;
      int end_sel =
        (visual_start < input_cursor_pos) ? input_cursor_pos : visual_start;
      end_sel++;
      /* Show prefix first */
      mvprintw(y, start_x, "%s", prefix);
      for (int i = 0; i < input_len; i++) {
        if (i >= start_sel && i < end_sel) {
          SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
          mvaddch(y, start_x + prefix_len + i, input_buffer[i]);
          SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        } else
          mvaddch(y, start_x + prefix_len + i, input_buffer[i]);
      }
    } else {
      /* NORMAL mode - only highlight character at cursor position */
      /* Show prefix first */
      mvprintw(y, start_x, "%s", prefix);
      for (int i = 0; i < input_len; i++) {
        if (i == input_cursor_pos) {
          SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
          mvaddch(y, start_x + prefix_len + i, input_buffer[i]);
          SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        } else
          mvaddch(y, start_x + prefix_len + i, input_buffer[i]);
      }
      if (input_cursor_pos >= input_len) {
        SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
        mvaddch(y, start_x + prefix_len + input_len, ' ');
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      }
    }
  }
}

/* Wrapper functions for keybinding (take AppData*) */
void NoteUpApp(AppData* app) {
  if (app == NULL || app->notes == NULL) return;
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  NoteUp(app->notes);
}

void NoteDownApp(AppData* app) {
  if (app == NULL || app->notes == NULL) return;
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  NoteDown(app->notes);
}
