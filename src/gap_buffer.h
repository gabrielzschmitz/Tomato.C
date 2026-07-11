#ifndef GAP_BUFFER_H_
#define GAP_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>

#define GAP_INITIAL_CAPACITY 64

/**
 * Gap buffer data structure for efficient text editing.
 * Maintains a gap at the cursor position for O(1) insertions.
 * The buffer is split into two parts: before and after the gap.
 */
typedef struct GapBuffer {
  char* buffer; /**< The underlying character buffer containing text and gap */
  size_t len;   /**< Total length of text (excluding gap) */
  size_t capacity; /**< Total allocated capacity (including gap) */
} GapBuffer;

/**
 * ---------------------------------------------------------------------------
 * Gap Buffer Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new empty gap buffer.
 * @return Pointer to the created gap buffer, or NULL on allocation failure
 */
GapBuffer* GapBufferCreate(void);

/**
 * Free a gap buffer and its contents.
 * @param gb Pointer to the gap buffer to free
 */
void GapBufferFree(GapBuffer* gb);

/**
 * Set the buffer contents to a specific text string.
 * Replaces existing content with the new text.
 * @param gb Pointer to the gap buffer
 * @param text New text to set
 */
bool GapBufferSetText(GapBuffer* gb, const char* text);

/**
 * Insert a character at the specified position.
 * Moves the gap to the position and inserts the character.
 * @param gb Pointer to the gap buffer
 * @param pos Position to insert at
 * @param c Character to insert
 */
bool GapBufferInsert(GapBuffer* gb, size_t pos, char c);

/**
 * Delete a character at the specified position.
 * @param gb Pointer to the gap buffer
 * @param pos Position to delete at
 */
void GapBufferDelete(GapBuffer* gb, size_t pos);

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
size_t GapBufferLength(const GapBuffer* gb);

/**
 * Get the total capacity of the buffer.
 * @param gb Pointer to the gap buffer
 * @return Total capacity including gap
 */
size_t GapBufferCapacity(const GapBuffer* gb);

/**
 * Convert the gap buffer to a contiguous string.
 * @param gb Pointer to the gap buffer
 * @return Newly allocated string (caller must free), or NULL on failure
 */
char* GapBufferToString(const GapBuffer* gb);

/**
 * Clear the buffer contents, resetting to empty state.
 * @param gb Pointer to the gap buffer
 */
void GapBufferClear(GapBuffer* gb);

/**
 * Create a deep copy of a gap buffer.
 * @param gb Pointer to the gap buffer to clone
 * @return New gap buffer with identical content, or NULL on failure
 */
GapBuffer* GapBufferClone(const GapBuffer* gb);

#endif /* GAP_BUFFER_H_ */
