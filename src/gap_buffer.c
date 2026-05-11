#include "gap_buffer.h"

#include <stdlib.h>
#include <string.h>

/* PRIVATE GAP BUFFER FUNCTIONS */
/* Utility */
static void ensureCapacity(GapBuffer* gb, size_t needed);

/**
 * ---------------------------------------------------------------------------
 * Gap Buffer Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new empty gap buffer.
 * @return Pointer to the created gap buffer, or NULL on allocation failure
 */
GapBuffer* GapBufferCreate(void) {
  GapBuffer* gb = (GapBuffer*)malloc(sizeof(GapBuffer));
  if (!gb) return NULL;

  gb->buffer = (char*)malloc(GAP_INITIAL_CAPACITY);
  if (!gb->buffer) {
    free(gb);
    return NULL;
  }

  gb->capacity = GAP_INITIAL_CAPACITY;
  gb->len = 0;
  gb->buffer[0] = '\0';

  return gb;
}

/**
 * Free a gap buffer and its contents.
 * @param gb Pointer to the gap buffer to free
 */
void GapBufferFree(GapBuffer* gb) {
  if (!gb) return;
  free(gb->buffer);
  gb->buffer = NULL;
  free(gb);
}

/**
 * Set the buffer contents to a specific text string.
 * Replaces existing content with the new text.
 * @param gb Pointer to the gap buffer
 * @param text New text to set
 */
void GapBufferSetText(GapBuffer* gb, const char* text) {
  if (!gb || !text) return;

  size_t len = strlen(text);
  ensureCapacity(gb, len + 1);

  memcpy(gb->buffer, text, len);
  gb->len = len;
  gb->buffer[len] = '\0';
}

/**
 * Insert a character at the specified position.
 * Moves the gap to the position and inserts the character.
 * @param gb Pointer to the gap buffer
 * @param pos Position to insert at
 * @param c Character to insert
 */
void GapBufferInsert(GapBuffer* gb, size_t pos, char c) {
  if (!gb) return;
  if (pos > gb->len) pos = gb->len;

  ensureCapacity(gb, gb->len + 2);

  memmove(gb->buffer + pos + 1, gb->buffer + pos, gb->len - pos + 1);
  gb->buffer[pos] = c;
  gb->len++;
}

/**
 * Delete a character at the specified position.
 * @param gb Pointer to the gap buffer
 * @param pos Position to delete at
 */
void GapBufferDelete(GapBuffer* gb, size_t pos) {
  if (!gb) return;
  if (pos >= gb->len) return;

  memmove(gb->buffer + pos, gb->buffer + pos + 1, gb->len - pos);
  gb->len--;
  gb->buffer[gb->len] = '\0';
}

/**
 * ---------------------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------------------
 */

/**
 * Get the length of text in the buffer (excluding gap).
 * @param gb Pointer to the gap buffer
 * @return Length of text content
 */
size_t GapBufferLength(const GapBuffer* gb) {
  if (!gb) return 0;
  return gb->len;
}

/**
 * Get the total capacity of the buffer.
 * @param gb Pointer to the gap buffer
 * @return Total capacity including gap
 */
size_t GapBufferCapacity(const GapBuffer* gb) {
  if (!gb) return 0;
  return gb->capacity;
}

/**
 * Clear the buffer contents, resetting to empty state.
 * @param gb Pointer to the gap buffer
 */
void GapBufferClear(GapBuffer* gb) {
  if (!gb) return;
  gb->len = 0;
  gb->buffer[0] = '\0';
}

/**
 * Convert the gap buffer to a contiguous string.
 * @param gb Pointer to the gap buffer
 * @return Newly allocated string (caller must free), or NULL on failure
 */
char* GapBufferToString(const GapBuffer* gb) {
  if (!gb) return NULL;

  char* result = (char*)malloc(gb->len + 1);
  if (!result) return NULL;

  memcpy(result, gb->buffer, gb->len);
  result[gb->len] = '\0';

  return result;
}

/**
 * Ensure the buffer has enough capacity for the needed bytes.
 * Grows buffer exponentially if necessary.
 * @param gb Pointer to the gap buffer
 * @param needed Number of bytes needed
 */
static void ensureCapacity(GapBuffer* gb, size_t needed) {
  if (gb->capacity >= needed) return;

  size_t new_cap = gb->capacity * 2;
  while (new_cap < needed) new_cap *= 2;

  char* new_buf = (char*)realloc(gb->buffer, new_cap);
  if (new_buf) {
    gb->buffer = new_buf;
    gb->capacity = new_cap;
  }
}
