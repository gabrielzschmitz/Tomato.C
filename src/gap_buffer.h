#ifndef GAP_BUFFER_H_
#define GAP_BUFFER_H_

#include <stddef.h>

#define GAP_INITIAL_CAPACITY 64

typedef struct GapBuffer {
  char* buffer;
  size_t len;
  size_t capacity;
} GapBuffer;

GapBuffer* GapBufferCreate(void);
void GapBufferFree(GapBuffer* gb);
void GapBufferInsert(GapBuffer* gb, size_t pos, char c);
void GapBufferDelete(GapBuffer* gb, size_t pos);
size_t GapBufferLength(const GapBuffer* gb);
char* GapBufferToString(const GapBuffer* gb);
void GapBufferClear(GapBuffer* gb);
size_t GapBufferCapacity(const GapBuffer* gb);
void GapBufferSetText(GapBuffer* gb, const char* text);

#endif /* GAP_BUFFER_H_ */
