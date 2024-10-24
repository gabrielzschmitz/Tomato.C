#include "anim.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tomato.h"
#include "util.h"

/* Function to get the current time in milliseconds */
double GetCurrentTimeMS(void) {
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  double ms =
    (current_time.tv_sec * 1000.0) + (current_time.tv_nsec / 1000000.0);
  return ms;
}

/* Creates a new Rollfilm with N frames of height M */
Rollfilm* CreateRollfilm(int N, int M) {
  Rollfilm* film = (Rollfilm*)malloc(sizeof(Rollfilm));
  if (film == NULL) return NULL;

  film->delta_frame_ms = GetCurrentTimeMS();
  film->current_frame = 0;
  film->frame_count = N;
  film->frame_height = M;
  film->frame_width = 0;
  film->loop = true;
  film->frames = NULL;
  film->render = RenderCurrentFrame;
  film->update = UpdateAnimation;

  return film;
}

/* Frees all memory associated with a Rollfilm */
void FreeRollfilm(Rollfilm* rollfilm) {
  if (rollfilm == NULL) return;
  FreeFrames(rollfilm->frames);
  free(rollfilm);
}

/* Creates a new frame */
Frame* CreateFrame(void) {
  Frame* newFrame = (Frame*)malloc(sizeof(Frame));
  if (newFrame == NULL) return NULL;
  newFrame->next = NULL;
  newFrame->rows = NULL;
  newFrame->width = 0;
  newFrame->id = 0;

  return newFrame;
}

/* Frees all frames in a linked list */
void FreeFrames(Frame* frames) {
  if (frames == NULL) return;

  Frame* current = frames;
  do {
    Frame* next = current->next;
    FreeFrame(current);
    current = next;
  } while (current != frames);
}

/* Frees all memory associated with a single frame */
void FreeFrame(Frame* frame) {
  if (frame == NULL) return;
  FreeRows(frame->rows);
  free(frame);
}

/* Creates a new frame row */
FrameRow* CreateRow(void) {
  FrameRow* newRow = (FrameRow*)malloc(sizeof(FrameRow));
  if (newRow == NULL) return NULL;
  newRow->next = NULL;
  newRow->tokens = NULL;
  return newRow;
}

/* Frees all rows in a frame */
void FreeRows(FrameRow* rows) {
  FrameRow* current = rows;
  while (current != NULL) {
    FrameRow* next = current->next;
    FreeTokens(current->tokens);
    free(current);
    current = next;
  }
}

/* Creates a new frame token */
FrameToken* CreateToken(void) {
  FrameToken* newToken = (FrameToken*)malloc(sizeof(FrameToken));
  if (newToken == NULL) return NULL;
  newToken->next = NULL;
  newToken->token = (char*)calloc(MAX_FRAME_WIDTH, sizeof(char));
  if (newToken->token == NULL) {
    free(newToken);
    return NULL;
  }
  newToken->length = 0;
  newToken->color = NO_COLOR;
  return newToken;
}

/* Frees all tokens in a linked list */
void FreeTokens(FrameToken* tokens) {
  FrameToken* current = tokens;
  while (current != NULL) {
    FrameToken* next = current->next;
    FreeToken(current);
    current = next;
  }
}

/* Frees all memory associated with a single frame token */
void FreeToken(FrameToken* token) {
  if (token == NULL) return;
  free(token->token);
  free(token);
}

/* Deserializes sprites from a file into a Rollfilm structure */
Rollfilm* DeserializeSprites(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) return NULL;

  char line[MAX_FRAME_WIDTH];
  int lines_read = 0;
  Rollfilm* rollfilm = NULL;
  Frame* head_frame = NULL;
  Frame* current_frame = NULL;
  FrameRow* head_row = NULL;
  FrameRow* current_row = NULL;
  int current_frame_id = 0;
  int line_color = NO_COLOR;
  int read = 0;
  int line_width = 0;
  double seconds_multiplier = 0.0;

  while (fgets(line, sizeof(line), file) != NULL) {
    if (IsSeparatorLine(line) && read) {
      break;
    } else if (IsIconsLine(line)) {
      if (ProcessIconsLine(line, &read, &rollfilm, &head_frame, &current_frame,
                           &head_row, &current_row, &current_frame_id,
                           file) != 0) {
        return NULL;
      }
    } else if (read) {
      if (lines_read == 0) {
        if (ParseFrameTime(line, &seconds_multiplier) != 0) {
          FreeRollfilm(rollfilm);
          fclose(file);
          return NULL;
        }
        if (fgets(line, sizeof(line), file) == NULL) {
          FreeRollfilm(rollfilm);
          fclose(file);
          return NULL;
        }
        current_frame->seconds_multiplier = seconds_multiplier;
      } else if (lines_read >= rollfilm->frame_count * rollfilm->frame_height)
        break;
      if (ProcessFrameLine(line, &current_row, &line_color, &line_width) != 0) {
        FreeRollfilm(rollfilm);
        fclose(file);
        return NULL;
      }
      lines_read++;
      if (lines_read % rollfilm->frame_height == 0 &&
          lines_read < rollfilm->frame_count * rollfilm->frame_height) {
        current_frame->width = line_width;
        line_width = 0;
        LinkNewFrame(&current_frame, head_row, ++current_frame_id);
        head_row = CreateRow();
        if (head_row == NULL) {
          FreeRollfilm(rollfilm);
          fclose(file);
          return NULL;
        }
        current_row = head_row;
        line_color = NO_COLOR;
        if (current_frame_id < rollfilm->frame_count) {
          if (fgets(line, sizeof(line), file) == NULL) {
            FreeRollfilm(rollfilm);
            fclose(file);
            return NULL;
          }
          if (ParseFrameTime(line, &seconds_multiplier) != 0) {
            FreeRollfilm(rollfilm);
            fclose(file);
            return NULL;
          }
          current_frame->seconds_multiplier = seconds_multiplier;
        }
      }
    }
  }

  return CleanupAndReturn(file, rollfilm, head_frame, current_frame, head_row,
                          line_width);
}

/* Process the line containing icons */
int ProcessIconsLine(const char* line, int* read, Rollfilm** rollfilm,
                     Frame** head_frame, Frame** current_frame,
                     FrameRow** head_row, FrameRow** current_row,
                     int* current_frame_id, FILE* file) {
  int frame_count, frame_height;
  *read = 1;
  if (ParseFrameSize(line, &frame_count, &frame_height) != 0) {
    fclose(file);
    return -1;
  }
  *rollfilm = CreateRollfilm(frame_count, frame_height);
  if (*rollfilm == NULL) {
    fclose(file);
    return -1;
  }
  *head_frame = CreateFrame();
  if (*head_frame == NULL) {
    FreeRollfilm(*rollfilm);
    fclose(file);
    return -1;
  }
  *current_frame = *head_frame;
  (*current_frame)->id = *current_frame_id;
  *head_row = CreateRow();
  if (*head_row == NULL) {
    FreeRollfilm(*rollfilm);
    fclose(file);
    return -1;
  }
  *current_row = *head_row;
  return 0;
}

/* Process the content of the frame */
int ProcessFrameContent(char* line, int* lines_read, Rollfilm* rollfilm,
                        Frame** current_frame, FrameRow** current_row,
                        int* line_color, int* line_width,
                        double* seconds_multiplier, FILE* file) {
  if (*lines_read == 0) {
    if (ParseFrameTime(line, seconds_multiplier) != 0) {
      FreeRollfilm(rollfilm);
      fclose(file);
      return -1;
    }
    if (fgets(line, MAX_FRAME_WIDTH, file) == NULL) {
      FreeRollfilm(rollfilm);
      fclose(file);
      return -1;
    }
    (*current_frame)->seconds_multiplier = *seconds_multiplier;
  } else if (*lines_read >= rollfilm->frame_count * rollfilm->frame_height)
    return -1;

  if (ProcessFrameLine(line, current_row, line_color, line_width) != 0) {
    FreeRollfilm(rollfilm);
    fclose(file);
    return -1;
  }

  (*lines_read)++;
  return 0;
}

/* Cleanup and finalize the Rollfilm structure */
Rollfilm* CleanupAndReturn(FILE* file, Rollfilm* rollfilm, Frame* head_frame,
                           Frame* current_frame, FrameRow* head_row,
                           int line_width) {
  fclose(file);

  if (current_frame != NULL) {
    current_frame->rows = head_row;
    current_frame->width = line_width;
  }
  rollfilm->frames = head_frame;

  if (head_frame != NULL && current_frame != NULL &&
      current_frame != head_frame)
    current_frame->next = head_frame;
  rollfilm->frame_width = GetWidestFrame(rollfilm);

  return rollfilm;
}

/* Parses the frame height from a line. */
int ParseFrameSize(const char* line, int* frame_count, int* frame_height) {
  return sscanf(line, "%*[^/]/%dc/%dh", frame_count, frame_height) != 2 ? -1
                                                                        : 0;
}

/* Checks if the line contains icons. */
int IsIconsLine(const char* line) {
  return strstr(line, ICONS) != NULL;
}

/* Checks if the line is a separator. */
int IsSeparatorLine(const char* line) {
  if (strlen(line) != strlen(SEPARATOR)) return 0;
  return strcmp(line, SEPARATOR) == 0;
}

/* Parses the frame height from a line. */
int ParseFrameTime(const char* line, double* frame_time) {
  return sscanf(line, "%lfs", frame_time) != 1 ? -1 : 0;
}

/* Processes a line of frame data and updates the current row. */
int ProcessFrameLine(const char* line, FrameRow** current_row, int* line_color,
                     int* line_width) {
  FrameToken* line_token = DeserializeFrameLine(line, line_color);
  if (line_token == NULL) { return -1; }
  int current_line_width = 0;
  FrameToken* token = line_token;
  while (token != NULL) {
    current_line_width += token->length;
    token = token->next;
  }

  if (current_line_width > *line_width) *line_width = current_line_width;

  (*current_row)->tokens = line_token;
  FrameRow* new_row = CreateRow();
  if (new_row == NULL) { return -1; }
  (*current_row)->next = new_row;
  *current_row = new_row;
  return 0;
}

/* Links a new frame to the current frame and updates the frame ID. */
void LinkNewFrame(Frame** current_frame, FrameRow* head_row, int frame_id) {
  Frame* new_frame = CreateFrame();
  if (new_frame == NULL) { return; }
  (*current_frame)->rows = head_row;
  new_frame->id = frame_id;

  if ((*current_frame)->next == NULL) {
    new_frame->next = *current_frame;
    (*current_frame)->next = new_frame;
  } else {
    new_frame->next = (*current_frame)->next;
    (*current_frame)->next = new_frame;
  }

  *current_frame = new_frame;
}

/* Returns the widest frame from a rollfilm */
int GetWidestFrame(Rollfilm* rollfilm) {
  if (rollfilm == NULL || rollfilm->frames == NULL) { return 0; }

  Frame* current_frame = rollfilm->frames;
  int max_width = current_frame->width;

  do {
    if (current_frame->width > max_width) { max_width = current_frame->width; }
    current_frame = current_frame->next;
  } while (current_frame != rollfilm->frames);

  return max_width;
}

/* Deserializes a single line of frame data */
FrameToken* DeserializeFrameLine(const char* src, int* color) {
  FrameToken* head = CreateToken();
  FrameToken* current_token = head;
  if (current_token == NULL) return NULL;

  current_token->color = *color;
  while (*src != '\0') {
    if (*src == '\\' && *(src + 1) == 'u') {
      if (current_token->color == NO_COLOR) current_token->color = *color;
      int length = HandleUnicode(src, &current_token->token);
      src += length + 3;
      current_token->length += length;
    } else if (*src == '\\' && *(src + 1) == 'c') {
      src += HandleColor(src, color);

      FrameToken* new_token = CreateToken();
      if (new_token == NULL) {
        FreeTokens(head);
        return NULL;
      }

      current_token->next = new_token;
      current_token = new_token;
      current_token->color = *color;
    } else {
      if (current_token->color == NO_COLOR) current_token->color = *color;
      if (*src != '\n') {
        char buf[2] = {*src, '\0'};
        strcat(current_token->token, buf);
        current_token->length++;
      }
      src++;
    }
  }

  return head;
}

/* Handles a UTF-8 character in a line */
int HandleUnicode(const char* src, char** dest) {
  /* Create a buffer to store the UTF-8 character
   * (max 4 bytes for UTF-8 + 1 for null terminator) */
  char utf8_char[5] = {0};

  /* Extract the 4 hexadecimal digits from the source string */
  char utf_char[5];
  strncpy(utf_char, src + 2, 4);
  utf_char[4] = '\0';

  /* Convert the hexadecimal digits to an integer */
  int unicode_value = (int)strtol(utf_char, NULL, 16);

  /* Determine the UTF-8 encoding */
  int utf8_length = 0;
  if (unicode_value < 0x80) {
    utf8_char[0] = (char)unicode_value;
    utf8_length = 1;
  } else if (unicode_value < 0x800) {
    utf8_char[0] = (char)((unicode_value >> 6) | 0xC0);
    utf8_char[1] = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 2;
  } else if (unicode_value < 0x10000) {
    utf8_char[0] = (char)((unicode_value >> 12) | 0xE0);
    utf8_char[1] = (char)(((unicode_value >> 6) & 0x3F) | 0x80);
    utf8_char[2] = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 3;
  } else {
    utf8_char[0] = (char)((unicode_value >> 18) | 0xF0);
    utf8_char[1] = (char)(((unicode_value >> 12) & 0x3F) | 0x80);
    utf8_char[2] = (char)(((unicode_value >> 6) & 0x3F) | 0x80);
    utf8_char[3] = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 4;
  }

  /* Append the UTF-8 character to the destination string */
  size_t dest_len = strlen(*dest);
  for (int i = 0; i < utf8_length; ++i)
    (*dest)[dest_len + i] = utf8_char[i];
  (*dest)[dest_len + utf8_length] = '\0';

  /* Return the number of characters processed */
  return utf8_length;
}

/* Handles a color code in a line */
int HandleColor(const char* src, int* dest) {
  int color_code = src[2] - '0';
  if (color_code < 0 || color_code > PALETTE_SIZE) return 3;
  *dest = color_code;
  return 3;
}

/* Render the current frame of the Rollfilm at the specified coordinates */
void RenderCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x) {
  if (rollfilm == NULL || rollfilm->frames == NULL) { return; }

  Frame* current_frame = rollfilm->frames;
  FrameRow* current_row = current_frame->rows;
  int y = start_y;
  int color = NO_COLOR;

  while (current_row != NULL) {
    FrameToken* current_token = current_row->tokens;
    int x = start_x;

    while (current_token != NULL) {
      if (color != current_token->color && current_token->color != NO_COLOR) {
        color = current_token->color;
        SetColor(color, NO_COLOR, A_BOLD);
      }

      mvprintw(y, x, "%s", current_token->token);
      x += current_token->length;
      current_token = current_token->next;
    }

    current_row = current_row->next;
    y++;
  }
}

/* Updates the current frame of the Rollfilm to be next frame */
void UpdateAnimation(Rollfilm* rollfilm) {
  if (!rollfilm->loop && rollfilm->current_frame >= rollfilm->frame_count - 1)
    return;
  double current_time = GetCurrentTimeMS();
  double delta_time = current_time - rollfilm->delta_frame_ms;

  if (delta_time >= 1000.0 * rollfilm->frames->seconds_multiplier) {
    rollfilm->delta_frame_ms = current_time;
    rollfilm->frames = rollfilm->frames->next;
    rollfilm->current_frame =
      (rollfilm->current_frame + 1) % rollfilm->frame_count;
  }
}

/* Updates the loop variable of a given list of Rollfilms */
void SetAnimationsLoop(Rollfilm** film, const int* list_to_update,
                       size_t list_size, bool loop) {
  for (size_t i = 0; i < list_size; i++) {
    int index = list_to_update[i];
    film[index]->loop = loop;
  }
}
