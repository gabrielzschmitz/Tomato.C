#include "notes.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "gap_buffer.h"
#include "input.h"
#include "tomato.h"

void SetColor(short int fg, short int bg, chtype attr);
#define NO_COLOR -1

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

  return notes;
}

void SetNotesMaxLines(NotesData* notes, int max_lines) {
  if (notes) notes->max_lines = max_lines;
}

int GetNoteLines(NoteItem* item, int render_width) {
  if (!item || render_width <= 0) return 1;
  char* text = GapBufferToString(item->text);
  if (!text) return 1;
  int count = GetNoteLinesFromText(text, render_width);
  free(text);
  return count > 0 ? count : 1;
}

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

void FreeNotesData(NotesData* notes) {
  if (!notes) return;

  for (int i = 0; i < notes->count; i++) {
    GapBufferFree(notes->items[i]->text);
    free(notes->items[i]);
  }
  free(notes->items);
  free(notes);
}

static void grow_items(NotesData* notes) {
  notes->capacity *= 2;
  NoteItem** new_items =
    (NoteItem**)realloc(notes->items, sizeof(NoteItem*) * notes->capacity);
  if (new_items) notes->items = new_items;
}

static int get_next_id(NotesData* notes) {
  int max_id = 0;
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id > max_id) max_id = notes->items[i]->id;
  return max_id + 1;
}

static NoteItem* find_by_id(NotesData* notes, int id) {
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id == id) return notes->items[i];
  return NULL;
}

static int get_index_by_id(NotesData* notes, int id) {
  for (int i = 0; i < notes->count; i++)
    if (notes->items[i]->id == id) return i;
  return -1;
}

void AddNote(NotesData* notes, const char* text, NoteState state) {
  if (!notes || !text) return;

  int new_note_lines = GetNoteLinesFromText(text, notes->render_width);
  if (notes->max_lines > 0 &&
      notes->total_lines + new_note_lines > notes->max_lines)
    return;
  if (notes->count >= notes->capacity) grow_items(notes);

  NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
  if (!item) return;

  item->text = GapBufferCreate();
  if (!item->text) {
    free(item);
    return;
  }

  GapBufferSetText(item->text, text);
  item->state = state;
  item->id = get_next_id(notes);

  notes->items[notes->count++] = item;
  notes->current_id = item->id;
  notes->total_lines += new_note_lines;
}

void UpdateNote(NotesData* notes, int note_id, const char* text,
                NoteState state) {
  if (!notes || !text || note_id < 0) return;

  NoteItem* note = find_by_id(notes, note_id);
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

void DeleteNote(NotesData* notes) {
  if (!notes || notes->current_id < 0) return;

  int idx = get_index_by_id(notes, notes->current_id);
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

void ToggleTask(NotesData* notes) {
  if (!notes || notes->current_id < 0) return;

  NoteItem* item = find_by_id(notes, notes->current_id);
  if (!item || item->state == NOTE_PLAIN) return;

  item->state = (item->state == NOTE_UNDONE) ? NOTE_DONE : NOTE_UNDONE;
}

void NoteUp(NotesData* notes) {
  if (!notes || notes->count == 0) return;

  int idx = get_index_by_id(notes, notes->current_id);
  if (idx < 0) {
    notes->current_id = notes->items[0]->id;
    return;
  }

  if (idx > 0) notes->current_id = notes->items[idx - 1]->id;
}

void NoteDown(NotesData* notes) {
  if (!notes || notes->count == 0) return;

  int idx = get_index_by_id(notes, notes->current_id);
  if (idx < 0) {
    notes->current_id = notes->items[0]->id;
    return;
  }

  if (idx < notes->count - 1) notes->current_id = notes->items[idx + 1]->id;
}

void NoteUpApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  NoteUp(app->notes);
}

void NoteDownApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  NoteDown(app->notes);
}

int GetSelectedNoteIndex(NotesData* notes) {
  if (!notes) return -1;
  return get_index_by_id(notes, notes->current_id);
}

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

void RenderNotes(NotesData* notes, int start_x, int start_y, int end_x,
                 int end_y, InputState* input, int mode) {
  if (!notes) return;

  int max_width = end_x - start_x;
  int max_height = end_y - start_y;

  if (max_width <= 0 || max_height <= 0) return;

  notes->max_lines = max_height;
  notes->render_width = max_width;
  notes->total_lines = 0;

  bool is_editing = ((mode == NORMAL || mode == INSERT || mode == VISUAL) &&
                     notes->current_id >= 0);

  for (int i = 0; i < notes->count; i++) {
    bool is_selected = (notes->items[i]->id == notes->current_id);
    if (is_editing && is_selected) {
      const char* prefix;
      switch (notes->items[i]->state) {
        case NOTE_DONE:
        case NOTE_UNDONE:
          prefix = "[ ] ";
          break;
        case NOTE_PLAIN:
        default:
          prefix = " - ";
      }
      int prefix_len = (int)strlen(prefix);
      int item_wrap_width = max_width - prefix_len;
      notes->total_lines += GetNoteLines(notes->items[i], item_wrap_width);
      continue;
    }

    const char* prefix;
    switch (notes->items[i]->state) {
      case NOTE_DONE:
      case NOTE_UNDONE:
        prefix = "[ ] ";
        break;
      case NOTE_PLAIN:
      default:
        prefix = " - ";
    }
    int prefix_len = (int)strlen(prefix);
    int item_wrap_width = max_width - prefix_len;
    notes->total_lines += GetNoteLines(notes->items[i], item_wrap_width);
  }

  int render_width = notes->render_width;
  int render_y = start_y;

  for (int i = 0; i < notes->count && render_y < end_y; i++) {
    bool is_selected = (notes->items[i]->id == notes->current_id);

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
          int input_wrap_width = max_width - prefix_len;
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
              char buffer[max_width + 1];
              int left_len =
                (input->cursor <= input->len) ? input->cursor : input->len;
              char left_part[left_len + 1];
              strncpy(left_part, input->buffer, left_len);
              left_part[left_len] = '\0';
              const char* right_part = input->buffer + input->cursor;
              snprintf(buffer, max_width + 1, "%s%s|%s", prefix, left_part,
                       right_part);
              mvprintw(render_y, start_x, "%s", buffer);
            } else {
              const char* line =
                wrapped_input && wrapped_input[wl] ? wrapped_input[wl] : "";
              mvprintw(render_y, start_x + strlen(prefix), "%s", line);
            }
            render_y++;
          }
          for (int wl = 0; wl < num_input_lines; wl++)
            if (wrapped_input[wl]) free(wrapped_input[wl]);
          free(wrapped_input);
        } else {
          mvprintw(render_y, start_x, "%s", INSERT_CURSOR_ICON);
          render_y++;
        }
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        continue;
      }
      if (input && input->len > 0 && (mode == NORMAL || mode == VISUAL)) {
        if (input && input->len > 0) {
          const char* prefix = edit_prefix;
          int prefix_len = (int)strlen(prefix);
          int input_wrap_width = max_width - prefix_len;

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
            if (wl == 0) mvprintw(render_y, start_x, "%s", prefix);
            for (int ci = 0; ci < line_len; ci++) {
              int abs_pos = line_char_offset + ci;
              bool in_selection =
                (mode == VISUAL && abs_pos >= sel_start && abs_pos <= sel_end);
              bool is_cursor = (mode == NORMAL && abs_pos == input->cursor);
              if (is_cursor || in_selection)
                SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
              mvaddch(render_y, start_x + prefix_len + ci,
                      wrapped_input[wl][ci] ? wrapped_input[wl][ci] : ' ');
              if (is_cursor || in_selection)
                SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
            }
            int cursor_line_start = line_char_offset;
            int cursor_line_end = line_char_offset + line_len - 1;
            if (input->cursor > cursor_line_end && wl == num_input_lines - 1) {
              bool is_cursor = (mode == NORMAL);
              if (is_cursor) SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
              mvaddch(render_y, start_x + prefix_len + line_len, ' ');
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
    int item_wrap_width = max_width - prefix_len;

    char** wrapped_lines = NULL;
    int num_lines = WrapText(text, item_wrap_width, &wrapped_lines);
    free(text);

    if (num_lines == 0) {
      num_lines = 1;
      wrapped_lines = (char**)malloc(sizeof(char*));
      if (wrapped_lines) wrapped_lines[0] = (char*)"";
    }

    for (int wl = 0; wl < num_lines && render_y < end_y; wl++) {
      if (is_selected)
        SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      else
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);

      if (wl == 0) {
        char buffer[max_width + 1];
        snprintf(buffer, max_width + 1, "%s%s", prefix,
                 wrapped_lines[wl] ? wrapped_lines[wl] : "");
        mvprintw(render_y, start_x, "%s", buffer);
      } else
        mvprintw(render_y, start_x + prefix_len, "%s",
                 wrapped_lines[wl] ? wrapped_lines[wl] : "");
      render_y++;
    }
    for (int wl = 0; wl < num_lines; wl++)
      if (wrapped_lines[wl]) free(wrapped_lines[wl]);
    free(wrapped_lines);
  }

  const char* input_buffer = input ? input->buffer : NULL;
  int input_len = input ? input->len : 0;
  int input_cursor_pos = input ? input->cursor : 0;

  bool input_already_rendered = is_editing && notes->current_id >= 0 &&
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
        char display_buf[max_width + 1];
        int left_len =
          (input_cursor_pos <= input_len) ? input_cursor_pos : input_len;
        char left_part[left_len + 1];
        strncpy(left_part, input_buffer, left_len);
        left_part[left_len] = '\0';
        const char* right_part = input_buffer + input_cursor_pos;
        snprintf(display_buf, max_width + 1, "%s%s|%s", prefix, left_part,
                 right_part);
        mvprintw(render_y, start_x, "%s", display_buf);
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
