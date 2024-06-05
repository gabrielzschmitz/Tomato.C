#include "anim.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tomato.h"
#include "util.h"

/* Increments animation frames based on real-life seconds */
void FrameTimer(double* milliseconds, int* frame_seconds) {
  const clock_t sec = 60 * CLOCKS_PER_SEC;
  clock_t current_time = clock();
  const clock_t final_time = current_time + sec;
  if (final_time > current_time) {
    *milliseconds = *milliseconds + 1;
    if (*milliseconds >= REAL_SECONDS) {
      *milliseconds = 0;
      *frame_seconds += 1;
    }
  }
}

/* Creates a new Rollfilm with N frames of height M */
Rollfilm* CreateRollfilm(int N, int M) {
  Rollfilm* film = (Rollfilm*)malloc(sizeof(Rollfilm));
  if (film == NULL) return NULL;

  film->frame_count = N;
  film->frame_height = M;
  film->frame_width = 0;
  film->current_frame = 0;
  film->frames = NULL;

  return film;
}

/* Frees all memory associated with a Rollfilm */
void FreeRollfilm(Rollfilm* rollfilm) {
  if (rollfilm == NULL) return;
  FreeFrames(rollfilm->frames);
  free(rollfilm);
}

/* Creates a new frame */
Frame* CreateFrame() {
  Frame* newFrame = (Frame*)malloc(sizeof(Frame));
  if (newFrame == NULL) return NULL;
  newFrame->next = NULL;
  newFrame->prev = NULL;
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
FrameRow* CreateRow() {
  FrameRow* newRow = (FrameRow*)malloc(sizeof(FrameRow));
  if (newRow == NULL) return NULL;
  newRow->next = NULL;
  newRow->prev = NULL;
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
FrameToken* CreateToken() {
  FrameToken* newToken = (FrameToken*)malloc(sizeof(FrameToken));
  if (newToken == NULL) return NULL;
  newToken->next = NULL;
  newToken->prev = NULL;
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

  while (fgets(line, sizeof(line), file) != NULL) {
    if (IsIconsLine(line)) {
      int frame_count, frame_height;
      read = 1;
      if (ParseFrameSize(line, &frame_count, &frame_height) != 0) {
        fclose(file);
        return NULL;
      }
      rollfilm = CreateRollfilm(frame_count, frame_height);
      if (rollfilm == NULL) {
        fclose(file);
        return NULL;
      }
      head_frame = CreateFrame();
      if (head_frame == NULL) {
        FreeRollfilm(rollfilm);
        fclose(file);
        return NULL;
      }
      current_frame = head_frame;
      current_frame->id = current_frame_id;
      head_row = CreateRow();
      if (head_row == NULL) {
        FreeRollfilm(rollfilm);
        fclose(file);
        return NULL;
      }
      current_row = head_row;
    } else if (IsSeparatorLine(line) && read) {
      read = 0;
      break;
    } else if (read) {
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
      }
    }
  }
  fclose(file);

  if (current_frame != NULL) {
    current_frame->rows = head_row;
    current_frame->width = line_width;
  }
  rollfilm->frames = head_frame;

  if (head_frame != NULL && current_frame != NULL &&
      current_frame != head_frame) {
    current_frame->next = head_frame;
    head_frame->prev = current_frame;
  }
  rollfilm->frame_width = GetWidestFrame(rollfilm);

  return rollfilm;
}

/* Parses the frame height from a line. */
static int ParseFrameSize(const char* line, int* frame_count,
                          int* frame_height) {
  return sscanf(line, "%*[^/]/%d/%d", frame_count, frame_height) != 2 ? -1 : 0;
}

/* Checks if the line contains icons. */
static int IsIconsLine(const char* line) { return strstr(line, ICONS) != NULL; }

/* Checks if the line is a separator. */
static int IsSeparatorLine(const char* line) {
  return strstr(line, SEPARATOR) != NULL;
}

/* Processes a line of frame data and updates the current row. */
static int ProcessFrameLine(const char* line, FrameRow** current_row,
                            int* line_color, int* line_width) {
  FrameToken* line_token = DeserializeFrameLine(line, line_color);
  if (line_token == NULL) {
    return -1;
  }
  int current_line_width = 0;
  FrameToken* token = line_token;
  while (token != NULL) {
    current_line_width += token->length;
    token = token->next;
  }

  if (current_line_width > *line_width) *line_width = current_line_width;

  (*current_row)->tokens = line_token;
  FrameRow* new_row = CreateRow();
  if (new_row == NULL) {
    return -1;
  }
  (*current_row)->next = new_row;
  new_row->prev = *current_row;
  *current_row = new_row;
  return 0;
}

/* Links a new frame to the current frame and updates the frame ID. */
static void LinkNewFrame(Frame** current_frame, FrameRow* head_row,
                         int frame_id) {
  Frame* new_frame = CreateFrame();
  if (new_frame == NULL) {
    return;
  }
  (*current_frame)->rows = head_row;
  new_frame->prev = *current_frame;
  new_frame->id = frame_id;

  if ((*current_frame)->next == NULL) {
    new_frame->next = *current_frame;
    (*current_frame)->next = new_frame;
    new_frame->prev = *current_frame;
  } else {
    new_frame->next = (*current_frame)->next;
    (*current_frame)->next->prev = new_frame;
    (*current_frame)->next = new_frame;
  }

  *current_frame = new_frame;
}

/* Returns the widest frame from a rollfilm */
int GetWidestFrame(Rollfilm* rollfilm) {
  if (rollfilm == NULL || rollfilm->frames == NULL) {
    return 0;
  }

  Frame* current_frame = rollfilm->frames;
  int max_width = current_frame->width;

  do {
    if (current_frame->width > max_width) {
      max_width = current_frame->width;
    }
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
      if (current_token->prev != NULL) {
        FrameToken* prev = current_token;
        FrameToken* current = CreateToken();
        if (current == NULL) {
          FreeTokens(head);
          return NULL;
        }
        FrameToken* next = NULL;
        prev->next = current;
        current->prev = prev;
        current->next = next;
        current_token = current;
      }
      src += HandleColor(src, color);
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
  // Create a buffer to store the UTF-8 character (max 4 bytes for UTF-8 + 1 for
  // null terminator)
  char utf8_char[5] = {0};

  // Extract the 4 hexadecimal digits from the source string
  char utf_char[5];
  strncpy(utf_char, src + 2, 4);
  utf_char[4] = '\0';

  // Convert the hexadecimal digits to an integer
  int unicode_value = (int)strtol(utf_char, NULL, 16);

  // Determine the UTF-8 encoding
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

  // Append the UTF-8 character to the destination string
  size_t dest_len = strlen(*dest);
  for (int i = 0; i < utf8_length; ++i) (*dest)[dest_len + i] = utf8_char[i];
  (*dest)[dest_len + utf8_length] = '\0';

  // Return the number of characters processed
  return utf8_length;
}

/* Handles a color code in a line */
int HandleColor(const char* src, int* dest) {
  int color_code = src[2] - '0';
  if (color_code < 0 || color_code > MAX_COLOR_PAIRS) return 3;
  *dest = color_code;
  return 3;
}

/* Prints all frames from a Rollfilm */
void PrintAllFrames(Rollfilm* rollfilm) {
  if (rollfilm == NULL) {
    printf("Rollfilm is NULL\n");
    return;
  }

  Frame* head_frame = rollfilm->frames;
  if (head_frame == NULL) {
    printf("No frames in the Rollfilm\n");
    return;
  }

  printf("Rollfilm height: %d\n", rollfilm->frame_width);
  Frame* current_frame = head_frame;
  do {
    printf("Frame ID: %d | Frame Width: %d\n", current_frame->id,
           current_frame->width);

    FrameRow* current_row = current_frame->rows;
    while (current_row != NULL) {
      FrameToken* current_token = current_row->tokens;
      while (current_token != NULL) {
        printf("%d:%s\n", current_token->color, current_token->token);
        current_token = current_token->next;
      }

      current_row = current_row->next;
    }

    current_frame = current_frame->next;
  } while (current_frame != head_frame);
}

/* Draws the current frame of the Rollfilm at the specified coordinates */
void DrawCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x) {
  if (rollfilm == NULL || rollfilm->frames == NULL) {
    return;
  }

  Frame* current_frame = rollfilm->frames;
  FrameRow* current_row = current_frame->rows;
  int y = start_y;
  int color = NO_COLOR;

  // Loop through each row in the current frame
  while (current_row != NULL) {
    FrameToken* current_token = current_row->tokens;
    int x = start_x;

    // Loop through each token in the current row and print it
    while (current_token != NULL) {
      if (color != current_token->color && current_token->color != NO_COLOR) {
        color = current_token->color;
        SetColor(color, COLOR_BLACK, A_BOLD);
      }
      mvprintw(y, x, "%s", current_token->token);
      x += current_token->length;
      current_token = current_token->next;
    }

    current_row = current_row->next;
    y++;
  }
}