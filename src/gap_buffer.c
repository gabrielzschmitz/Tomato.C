#include "gap_buffer.h"

#include <stdlib.h>
#include <string.h>

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

void GapBufferFree(GapBuffer* gb) {
  if (!gb) return;
  free(gb->buffer);
  gb->buffer = NULL;
  free(gb);
}

static void ensure_capacity(GapBuffer* gb, size_t needed) {
  if (gb->capacity >= needed) return;

  size_t new_cap = gb->capacity * 2;
  while (new_cap < needed) new_cap *= 2;

  char* new_buf = (char*)realloc(gb->buffer, new_cap);
  if (new_buf) {
    gb->buffer = new_buf;
    gb->capacity = new_cap;
  }
}

void GapBufferInsert(GapBuffer* gb, size_t pos, char c) {
  if (!gb) return;
  if (pos > gb->len) pos = gb->len;

  ensure_capacity(gb, gb->len + 2);

  memmove(gb->buffer + pos + 1, gb->buffer + pos, gb->len - pos + 1);
  gb->buffer[pos] = c;
  gb->len++;
}

void GapBufferDelete(GapBuffer* gb, size_t pos) {
  if (!gb) return;
  if (pos >= gb->len) return;

  memmove(gb->buffer + pos, gb->buffer + pos + 1, gb->len - pos);
  gb->len--;
  gb->buffer[gb->len] = '\0';
}

size_t GapBufferLength(const GapBuffer* gb) {
  if (!gb) return 0;
  return gb->len;
}

char* GapBufferToString(const GapBuffer* gb) {
  if (!gb) return NULL;

  char* result = (char*)malloc(gb->len + 1);
  if (!result) return NULL;

  memcpy(result, gb->buffer, gb->len);
  result[gb->len] = '\0';

  return result;
}

void GapBufferClear(GapBuffer* gb) {
  if (!gb) return;
  gb->len = 0;
  gb->buffer[0] = '\0';
}

size_t GapBufferCapacity(const GapBuffer* gb) {
  if (!gb) return 0;
  return gb->capacity;
}

void GapBufferSetText(GapBuffer* gb, const char* text) {
  if (!gb || !text) return;

  size_t len = strlen(text);
  ensure_capacity(gb, len + 1);

  memcpy(gb->buffer, text, len);
  gb->len = len;
  gb->buffer[len] = '\0';
}
