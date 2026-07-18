#include "anim.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "error.h"
#include "tomato.h"
#include "util.h"

/* PRIVATE ANIM FUNCTIONS */
/* Rollfilm Lifecycle */
static struct Frame* createFrame(void);
static void linkNewFrame(struct Frame** current_frame,
                         struct FrameRow* head_row, int frame_id);
static void freeFrame(struct Frame* frame);
static void freeFrames(struct Frame* frames);
static struct FrameRow* createRow(void);
static void freeRows(struct FrameRow* rows);
static struct FrameToken* createToken(void);
static void freeToken(struct FrameToken* token);
static void freeTokens(struct FrameToken* tokens);
/* Rollfilm Deserialization */
static struct FrameToken* deserializeFrameLine(const char* src, int* color);
static Rollfilm* cleanupAndReturn(FILE* file, Rollfilm* rollfilm,
                                  struct Frame* head_frame,
                                  struct Frame* current_frame,
                                  struct FrameRow* head_row, int line_width);
static int processIconsLine(const char* line, int* read, Rollfilm** rollfilm,
                            struct Frame** head_frame,
                            struct Frame** current_frame,
                            struct FrameRow** head_row,
                            struct FrameRow** current_row,
                            int* current_frame_id, FILE* file);
/* Parsing Helpers */
static int processFrameLine(const char* line, struct FrameRow** current_row,
                            int* line_color, int* line_width);
static int parseFrameSize(const char* line, int* frame_count,
                          int* frame_height, int* default_frame);
static int parseFrameTime(const char* line, double* frame_time);
static bool isIconsLine(const char* line);
static bool isSeparatorLine(const char* line);
static int handleUnicode(const char* src, char** dest);
static int handleColor(const char* src, int* dest);
/* Utility */
static int getWidestFrame(Rollfilm* rollfilm);
/* Animation callbacks */
static void updateAnimation(Rollfilm* rollfilm);
static void renderCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x);

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new Rollfilm with N frames of height M.
 * @param N Number of frames to allocate
 * @param M Height of each frame in lines
 * @return Pointer to the created Rollfilm, or NULL on allocation failure
 */
Rollfilm* CreateRollfilm(int N, int M) {
  Rollfilm* film = (Rollfilm*)malloc(sizeof(Rollfilm));
  if (film == NULL) {
    LogError("CreateRollfilm", MALLOC_ERROR);
    return NULL;
  }

  film->delta_frame_ms = GetCurrentTimeMS();
  film->current_frame = 0;
  film->frame_count = N;
  film->frame_height = M;
  film->frame_width = 0;
  film->default_frame = 0;
  film->loop = true;
  film->frames = NULL;
  film->render = renderCurrentFrame;
  film->update = updateAnimation;

  return film;
}

/**
 * Set the loop variable for a list of Rollfilms.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param film Pointer to array of rollfilm pointers
 * @param list_to_update Array of indices to update
 * @param list_size Number of indices in list_to_update
 * @param loop true to enable looping, false to disable
 */
void SetRollfilmLoop(AppData* app, Rollfilm** film, const int* list_to_update,
                     size_t list_size, bool loop) {
  for (size_t i = 0; i < list_size; i++) {
    int index = list_to_update[i];
    if (film[index] == NULL) {
      SetError(app, "SetRollfilmLoop", ANIMATION_EQUAL_NULL);
      continue;
    }
    film[index]->loop = loop;
  }
}

/**
 * Free all memory associated with a Rollfilm.
 * @param rollfilm Pointer to the Rollfilm to free
 */
void FreeRollfilm(Rollfilm* rollfilm) {
  if (rollfilm == NULL) return;
  freeFrames(rollfilm->frames);
  free(rollfilm);
}

/**
 * Create a new empty Frame.
 * @return Pointer to the created Frame, or NULL on allocation failure
 */
static struct Frame* createFrame(void) {
  struct Frame* newFrame = (struct Frame*)malloc(sizeof(struct Frame));
  if (newFrame == NULL) {
    LogError("createFrame", MALLOC_ERROR);
    return NULL;
  }
  newFrame->next = NULL;
  newFrame->rows = NULL;
  newFrame->width = 0;
  newFrame->id = 0;

  return newFrame;
}

/**
 * Link a new frame to the current frame, maintaining circular list.
 * Initializes the new frame and connects it to the existing chain.
 * @param current_frame Pointer to current frame pointer (updated)
 * @param head_row Row to attach as the first row of the new frame
 * @param frame_id ID to assign to the new frame
 */
static void linkNewFrame(struct Frame** current_frame,
                         struct FrameRow* head_row, int frame_id) {
  struct Frame* new_frame = createFrame();
  if (new_frame == NULL) {
    LogError("linkNewFrame", MALLOC_ERROR);
    return;
  }
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

/**
 * Free all memory associated with a single Frame.
 * @param frame Pointer to the Frame to free
 */
static void freeFrame(struct Frame* frame) {
  if (frame == NULL) return;
  freeRows(frame->rows);
  free(frame);
}

/**
 * Free all frames in a circular linked list.
 * @param frames Pointer to the first frame in the circular list
 */
static void freeFrames(struct Frame* frames) {
  if (frames == NULL) return;

  struct Frame* current = frames;
  do {
    struct Frame* next = current->next;
    freeFrame(current);
    current = next;
  } while (current != frames);
}

/**
 * Create a new empty FrameRow.
 * @return Pointer to the created FrameRow, or NULL on allocation failure
 */
static struct FrameRow* createRow(void) {
  struct FrameRow* newRow = (struct FrameRow*)malloc(sizeof(struct FrameRow));
  if (newRow == NULL) {
    LogError("createRow", MALLOC_ERROR);
    return NULL;
  }
  newRow->next = NULL;
  newRow->tokens = NULL;
  return newRow;
}

/**
 * Free all rows in a linked list starting from the given row.
 * @param rows Pointer to the first row to free
 */
static void freeRows(struct FrameRow* rows) {
  struct FrameRow* current = rows;
  while (current != NULL) {
    struct FrameRow* next = current->next;
    freeTokens(current->tokens);
    free(current);
    current = next;
  }
}

/**
 * Create a new empty FrameToken.
 * @return Pointer to the created FrameToken, or NULL on allocation failure
 */
static struct FrameToken* createToken(void) {
  struct FrameToken* newToken =
    (struct FrameToken*)malloc(sizeof(struct FrameToken));
  if (newToken == NULL) {
    LogError("createToken", MALLOC_ERROR);
    return NULL;
  }
  newToken->next = NULL;
  newToken->token = (char*)calloc(MAX_FRAME_WIDTH, sizeof(char));
  if (newToken->token == NULL) {
    LogError("createToken", MALLOC_ERROR);
    free(newToken);
    return NULL;
  }
  newToken->length = 0;
  newToken->color = NO_COLOR;
  newToken->is_blank = false;
  return newToken;
}

/**
 * Free all memory associated with a single FrameToken.
 * @param token Pointer to the FrameToken to free
 */
static void freeToken(struct FrameToken* token) {
  if (token == NULL) return;
  free(token->token);
  free(token);
}

/**
 * Free all tokens in a linked list starting from the given token.
 * @param tokens Pointer to the first token to free
 */
static void freeTokens(struct FrameToken* tokens) {
  struct FrameToken* current = tokens;
  while (current != NULL) {
    struct FrameToken* next = current->next;
    freeToken(current);
    current = next;
  }
}

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Deserialization
 * ---------------------------------------------------------------------------
 */

/**
 * Deserialize sprites from a file into a Rollfilm structure.
 * Parses sprite file format with frame size, timing, and color codes.
 * @param filename Path to the sprite file
 * @return Pointer to the created Rollfilm, or NULL on failure
 */
Rollfilm* DeserializeSprites(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) return NULL;

  char line[MAX_FRAME_WIDTH];
  int lines_read = 0;
  Rollfilm* rollfilm = NULL;
  struct Frame* head_frame = NULL;
  struct Frame* current_frame = NULL;
  struct FrameRow* head_row = NULL;
  struct FrameRow* current_row = NULL;
  int current_frame_id = 0;
  int line_color = NO_COLOR;
  int read = 0;
  int line_width = 0;
  double seconds_multiplier = 0.0;

  while (fgets(line, sizeof(line), file) != NULL) {
    if (isSeparatorLine(line) && read) {
      break;
    } else if (isIconsLine(line)) {
      if (processIconsLine(line, &read, &rollfilm, &head_frame, &current_frame,
                           &head_row, &current_row, &current_frame_id,
                           file) != 0)
        return NULL;

    } else if (read) {
      if (lines_read == 0) {
        if (parseFrameTime(line, &seconds_multiplier) != 0) {
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
      if (processFrameLine(line, &current_row, &line_color, &line_width) != 0) {
        FreeRollfilm(rollfilm);
        fclose(file);
        return NULL;
      }
      lines_read++;
      if (lines_read % rollfilm->frame_height == 0 &&
          lines_read < rollfilm->frame_count * rollfilm->frame_height) {
        current_frame->width = line_width;
        line_width = 0;
        linkNewFrame(&current_frame, head_row, ++current_frame_id);
        head_row = createRow();
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
          if (parseFrameTime(line, &seconds_multiplier) != 0) {
            FreeRollfilm(rollfilm);
            fclose(file);
            return NULL;
          }
          current_frame->seconds_multiplier = seconds_multiplier;
        }
      }
    }
  }

  return cleanupAndReturn(file, rollfilm, head_frame, current_frame, head_row,
                          line_width);
}

/**
 * Deserialize a single line of frame data into tokens.
 * Handles color codes, UTF-8 characters, and text segments.
 * @param src Source line to deserialize
 * @param color Output: color code parsed from line
 * @return Pointer to the first token, or NULL on error
 */
static struct FrameToken* deserializeFrameLine(const char* src, int* color) {
  struct FrameToken* head = createToken();
  struct FrameToken* current_token = head;
  if (current_token == NULL) return NULL;

  current_token->color = *color;
  while (*src != '\0') {
    if (*src == '\\' && *(src + 1) == 'u') {
      if (current_token->color == NO_COLOR) current_token->color = *color;
      int length = handleUnicode(src, &current_token->token);
      src += length + 3;
      current_token->length += length;
    } else if (*src == '\\' && *(src + 1) == 'c') {
      src += handleColor(src, color);

      struct FrameToken* new_token = createToken();
      if (new_token == NULL) {
        freeTokens(head);
        return NULL;
      }

      current_token->next = new_token;
      current_token = new_token;
      current_token->color = *color;
    } else if (*src == '\\' && *(src + 1) == '*') {
      struct FrameToken* blank_token = createToken();
      if (blank_token == NULL) {
        freeTokens(head);
        return NULL;
      }
      blank_token->is_blank = true;
      blank_token->color = *color;

      int count = 0;
      while (*src == '\\' && *(src + 1) == '*') {
        count++;
        src += 2;
      }
      blank_token->length = count;

      current_token->next = blank_token;
      struct FrameToken* new_token = createToken();
      if (new_token == NULL) {
        freeTokens(head);
        return NULL;
      }
      new_token->color = *color;
      blank_token->next = new_token;
      current_token = new_token;
      continue;
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

/**
 * Cleanup and finalize the Rollfilm structure.
 * Closes file and returns the completed rollfilm.
 * @param file File handle to close
 * @param rollfilm Pointer to the rollfilm to finalize
 * @param head_frame Pointer to the head frame
 * @param current_frame Pointer to the current frame
 * @param head_row Pointer to the head row
 * @param line_width Width of the largest line processed
 * @return Pointer to the finalized Rollfilm
 */
static Rollfilm* cleanupAndReturn(FILE* file, Rollfilm* rollfilm,
                                  struct Frame* head_frame,
                                  struct Frame* current_frame,
                                  struct FrameRow* head_row, int line_width) {
  fclose(file);

  if (current_frame != NULL) {
    current_frame->rows = head_row;
    current_frame->width = line_width;
  }
  rollfilm->frames = head_frame;

  if (head_frame != NULL && current_frame != NULL &&
      current_frame != head_frame)
    current_frame->next = head_frame;
  rollfilm->frame_width = getWidestFrame(rollfilm);

  return rollfilm;
}

/**
 * Process a line containing icons in the sprite file.
 * @param line Line of text to parse
 * @param read Pointer to track bytes read
 * @param rollfilm Pointer to the rollfilm pointer
 * @param head_frame Pointer to the head frame pointer
 * @param current_frame Pointer to the current frame pointer
 * @param head_row Pointer to the head row pointer
 * @param current_row Pointer to the current row pointer
 * @param current_frame_id Pointer to current frame ID
 * @param file File handle for reading
 * @return 0 on success, non-zero on error
 */
static int processIconsLine(const char* line, int* read, Rollfilm** rollfilm,
                            struct Frame** head_frame,
                            struct Frame** current_frame,
                            struct FrameRow** head_row,
                            struct FrameRow** current_row,
                            int* current_frame_id, FILE* file) {
  int frame_count, frame_height, default_frame;
  *read = 1;
  if (parseFrameSize(line, &frame_count, &frame_height, &default_frame) != 0) {
    fclose(file);
    return -1;
  }
  *rollfilm = CreateRollfilm(frame_count, frame_height);
  if (*rollfilm == NULL) {
    fclose(file);
    return -1;
  }
  (*rollfilm)->default_frame = default_frame;
  *head_frame = createFrame();
  if (*head_frame == NULL) {
    LogError("processIconsLine", MALLOC_ERROR);
    FreeRollfilm(*rollfilm);
    fclose(file);
    return -1;
  }
  *current_frame = *head_frame;
  (*current_frame)->id = *current_frame_id;
  *head_row = createRow();
  if (*head_row == NULL) {
    FreeRollfilm(*rollfilm);
    fclose(file);
    return -1;
  }
  *current_row = *head_row;
  return 0;
}

/**
 * ---------------------------------------------------------------------------
 * Parsing Helpers
 * ---------------------------------------------------------------------------
 */

/**
 * Process a line of frame data and update the current row.
 * Parses color codes and text tokens from the line.
 * @param line Line to process
 * @param current_row Pointer to current row pointer
 * @param line_color Output: color code for the line
 * @param line_width Output: width of the line
 * @return 0 on success, non-zero on error
 */
static int processFrameLine(const char* line, struct FrameRow** current_row,
                            int* line_color, int* line_width) {
  struct FrameToken* line_token = deserializeFrameLine(line, line_color);
  if (line_token == NULL) {
    LogError("processFrameLine", MALLOC_ERROR);
    return -1;
  }
  int current_line_width = 0;
  struct FrameToken* token = line_token;
  while (token != NULL) {
    current_line_width += token->length;
    token = token->next;
  }

  if (current_line_width > *line_width) *line_width = current_line_width;

  (*current_row)->tokens = line_token;
  struct FrameRow* new_row = createRow();
  if (new_row == NULL) {
    LogError("processFrameLine", MALLOC_ERROR);
    return -1;
  }
  (*current_row)->next = new_row;
  *current_row = new_row;
  return 0;
}

/**
 * Parse the frame size from a line in the sprite file.
 * Format: "<iconset>/<N>c/<M>h[/<D>f]" where N is frame count, M is height,
 * and D (optional) is the default frame index for static display.
 * @param line Line to parse
 * @param frame_count Output: number of frames
 * @param frame_height Output: height of each frame
 * @param default_frame Output: default frame index (0 if not specified)
 * @return 0 on success, non-zero on error
 */
static int parseFrameSize(const char* line, int* frame_count,
                          int* frame_height, int* default_frame) {
  int result = sscanf(line, "%*[^/]/%dc/%dh/%df", frame_count, frame_height,
                      default_frame);
  if (result == 2) {
    *default_frame = 0;
    return 0;
  }
  return result == 3 ? 0 : -1;
}

/**
 * Parse the frame time multiplier from a line.
 * Format: "TIME: X.X" where X.X is the multiplier value.
 * @param line Line to parse
 * @param frame_time Output: time multiplier value
 * @return 0 on success, non-zero on error
 */
static int parseFrameTime(const char* line, double* frame_time) {
  return sscanf(line, "%lfs", frame_time) != 1 ? -1 : 0;
}

/**
 * Check if a line contains icon data (not a separator or metadata).
 * @param line Line to check
 * @return true if icons, false if not
 */
static bool isIconsLine(const char* line) {
  return strstr(line, ICONS) != NULL;
}

/**
 * Check if a line is a separator between sprite sections.
 * @param line Line to check
 * @return true if separator, false if not
 */
static bool isSeparatorLine(const char* line) {
  if (strlen(line) != strlen(SEPARATOR)) return 0;
  return strcmp(line, SEPARATOR) == 0;
}

/**
 * Handle a UTF-8 character in a line.
 * Copies the multi-byte character to the destination buffer.
 * @param src Source position in the line
 * @param dest Pointer to destination pointer (updated)
 * @return Number of bytes consumed
 */
static int handleUnicode(const char* src, char** dest) {
  /* Create a buffer to store the UTF-8 character
   * (max 4 bytes for UTF-8 + 1 for null terminator) */
  char utf8_char[5] = {0};

  /* Extract the 4 hexadecimal digits from the source string */
  char utf_char[5];
  strncpy(utf_char, src + 2, 4);
  utf_char[4] = '\0';

  /* Validate hex digits */
  for (int i = 0; i < 4; i++)
    if (!isxdigit((unsigned char)utf_char[i])) return 0;

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
  for (int i = 0; i < utf8_length; ++i) (*dest)[dest_len + i] = utf8_char[i];
  (*dest)[dest_len + utf8_length] = '\0';

  /* Return the number of characters processed */
  return utf8_length;
}

/**
 * Handle a color code in a line.
 * Parses escape sequence and sets the color value.
 * @param src Source position in the line
 * @param dest Output: color code
 * @return Number of bytes consumed
 */
static int handleColor(const char* src, int* dest) {
  int color_code = src[2] - '0';
  if (color_code < 0 || color_code > PALETTE_SIZE) return 3;
  *dest = color_code;
  return 3;
}

/**
 * ---------------------------------------------------------------------------
 * Utility 
 * ---------------------------------------------------------------------------
 */

/**
 * Get the width of the widest frame in a rollfilm.
 * @param rollfilm Pointer to the rollfilm
 * @return Width of the widest frame, or 0 if rollfilm is NULL
 */
static int getWidestFrame(Rollfilm* rollfilm) {
  if (rollfilm == NULL || rollfilm->frames == NULL) return 0;

  struct Frame* current_frame = rollfilm->frames;
  int max_width = current_frame->width;

  do {
    if (current_frame->width > max_width) max_width = current_frame->width;
    current_frame = current_frame->next;
  } while (current_frame != rollfilm->frames);

  return max_width;
}

/**
 * Find the rollfilm with the largest width among specified indices.
 * Used to size panels to fit the largest animation.
 * @param animations Pointer to array of rollfilm pointers
 * @param indices Array of indices to check
 * @param indices_count Number of indices
 * @return Index of the largest rollfilm, or -1 if none found
 */
int RollfilmLargest(Rollfilm* animations[], int* indices, int indices_count) {
  if (!animations || !indices || indices_count <= 0) return -1;

  int max_index = 1;
  int max_width = -1;
  int max_height = -1;

  for (int i = 0; i < indices_count; i++) {
    int index = indices[i];
    Rollfilm* rf = animations[index];

    if (rf == NULL) continue;

    if (rf->frame_height > max_height ||
        (rf->frame_height == max_height && rf->frame_width > max_width)) {
      max_width = rf->frame_width;
      max_height = rf->frame_height;
      max_index = index;
    }
  }

  return max_index;
}

/**
 * Find the position of the first blank token in the last frame.
 * Used for cursor positioning in text input.
 * @param rollfilm Pointer to the rollfilm
 * @param out_x Output: x coordinate of the blank
 * @param out_y Output: y coordinate of the blank
 * @return true if found, false if not found
 */
bool RollfilmFirstBlank(Rollfilm* rollfilm, int* out_x, int* out_y) {
  if (rollfilm == NULL || rollfilm->frames == NULL) return false;

  struct Frame* last_frame = rollfilm->frames;
  struct Frame* current = rollfilm->frames->next;
  while (current != rollfilm->frames) {
    if (current->id > last_frame->id) last_frame = current;
    current = current->next;
  }

  struct FrameRow* row = last_frame->rows;
  int y = 0;
  while (row != NULL && y < rollfilm->frame_height) {
    int x = 0;
    struct FrameToken* token = row->tokens;
    while (token != NULL) {
      if (token->is_blank) {
        *out_x = x;
        *out_y = y;
        return true;
      }
      x += token->length;
      token = token->next;
    }
    y++;
    row = row->next;
  }

  return false;
}

/**
 * Find the position of the last blank token in the last frame.
 * @param rollfilm Pointer to the rollfilm
 * @param out_x Output: x coordinate of the blank
 * @param out_y Output: y coordinate of the blank
 * @return true if found, false if not found
 */
bool RollfilmLastBlank(Rollfilm* rollfilm, int* out_x, int* out_y) {
  if (rollfilm == NULL || rollfilm->frames == NULL) return false;

  struct Frame* last_frame = rollfilm->frames;
  struct Frame* current = rollfilm->frames->next;
  while (current != rollfilm->frames) {
    if (current->id > last_frame->id) last_frame = current;
    current = current->next;
  }

  int max_x = -1;
  int max_y = -1;
  struct FrameRow* row = last_frame->rows;
  int y = 0;
  while (row != NULL && y < rollfilm->frame_height) {
    int x = 0;
    struct FrameToken* token = row->tokens;
    while (token != NULL) {
      if (token->is_blank) {
        int token_end_x = x + token->length;
        if (token_end_x > max_x) max_x = token_end_x;
        if (y > max_y) max_y = y;
      }
      x += token->length;
      token = token->next;
    }
    y++;
    row = row->next;
  }

  if (max_x >= 0) {
    *out_x = max_x;
    *out_y = max_y;
    return true;
  }

  return false;
}

/**
 * ---------------------------------------------------------------------------
 * Animation callbacks
 * ---------------------------------------------------------------------------
 */

/**
 * Render the current frame of the Rollfilm at the specified coordinates.
 * @param rollfilm Pointer to the rollfilm to render
 * @param start_y Starting y coordinate on screen
 * @param start_x Starting x coordinate on screen
 */
static void renderCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x) {
  if (rollfilm == NULL || rollfilm->frames == NULL) {
    LogError("renderCurrentFrame", NULL_POINTER_ERROR);
    return;
  }

  struct Frame* current_frame = rollfilm->frames;
  struct FrameRow* current_row = current_frame->rows;
  int y = start_y;
  int color = NO_COLOR;

  while (current_row != NULL) {
    struct FrameToken* current_token = current_row->tokens;
    int x = start_x;

    while (current_token != NULL) {
      if (color != current_token->color && current_token->color != NO_COLOR) {
        color = current_token->color;
        SetColor(color, NO_COLOR, A_BOLD);
      }

      if (!current_token->is_blank) mvprintw(y, x, "%s", current_token->token);
      x += current_token->length;
      current_token = current_token->next;
    }

    current_row = current_row->next;
    y++;
  }
}

/**
 * Advance the Rollfilm to the next frame.
 * Handles timing, looping, and frame index wrapping.
 * @param rollfilm Pointer to the rollfilm to update
 */
static void updateAnimation(Rollfilm* rollfilm) {
  if (rollfilm == NULL || rollfilm->frames == NULL) {
    LogError("updateAnimation", NULL_POINTER_ERROR);
    return;
  }
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

/**
 * Seek the rollfilm to a specific frame by index.
 * Properly resets the frames pointer, current_frame index, and frame timer
 * so no frame is skipped on the next update.
 * @param rollfilm Pointer to the rollfilm
 * @param frame_index Target frame index (0 <= frame_index < frame_count)
 */
void RollfilmSeekFrame(Rollfilm* rollfilm, int frame_index) {
  if (!rollfilm || rollfilm->frame_count <= 0) return;
  if (frame_index < 0 || frame_index >= rollfilm->frame_count) return;
  if (frame_index == rollfilm->current_frame) {
    rollfilm->delta_frame_ms = GetCurrentTimeMS();
    return;
  }

  /* Walk the circular linked list to the target frame */
  int steps = (frame_index - rollfilm->current_frame + rollfilm->frame_count) %
              rollfilm->frame_count;
  for (int i = 0; i < steps; i++) {
    if (rollfilm->frames) rollfilm->frames = rollfilm->frames->next;
  }

  rollfilm->current_frame = frame_index;
  rollfilm->delta_frame_ms = GetCurrentTimeMS();
}
