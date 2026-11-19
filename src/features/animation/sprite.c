/**
 * @file sprite.c
 * @brief Section-format sprite parser, builder, and immutable sprite queries.
 */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "features/animation/sprite.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "core/arena.h"
#include "core/error.h"

/** Size of the formatted diagnostic context buffer. */
#define SPRITE_DIAGNOSTIC_SIZE 512

/** Largest accepted frame duration in milliseconds. */
#define SPRITE_MAX_DURATION_MS 1.0e12

/** Alignment used for every finalized storage region. */
#define SPRITE_STORAGE_ALIGNMENT 16u

_Static_assert(sizeof(void*) <= SPRITE_STORAGE_ALIGNMENT,
               "storage alignment must cover pointers");

/** Deduplicating string pool used during finalization. */
typedef struct {
  char* data;        /**< NUL-terminated strings. */
  uint32_t length;   /**< Bytes used. */
  uint32_t capacity; /**< Bytes allocated. */
} spriteStringPool;

/** Temporary row span stored in a builder animation. */
typedef struct {
  uint32_t cell_offset; /**< First cell in this row. */
  uint32_t cell_count;  /**< Number of cells in this row. */
} spriteRowSpan;

/** Temporary decoded cell stored in a builder animation. */
typedef struct {
  uint32_t codepoint; /**< Unicode codepoint. */
  uint8_t foreground; /**< Explicit foreground color. */
  uint8_t background; /**< Explicit background color. */
  uint8_t width;      /**< Terminal display width. */
  uint8_t flags;      /**< SPRITE_CELL_FLAG_* bit mask. */
} spriteCellData;

_Static_assert(sizeof(spriteCellData) == sizeof(SpriteCell),
               "temporary and finalized cells must have equal size");

/** Temporary frame state while one frame is parsed. */
typedef struct {
  uint32_t first_row;   /**< First row index owned by this frame. */
  uint32_t first_cell;  /**< First cell index owned by this frame. */
  uint8_t foreground;   /**< Current foreground color. */
  uint8_t background;   /**< Current background color. */
  bool foreground_set;  /**< True when a foreground color is active. */
  bool background_set;  /**< True when a background color is active. */
  bool duration_set;    /**< True when a frame duration override was read. */
  double duration_ms;   /**< Frame duration override in milliseconds. */
  bool content_started; /**< True after [[content]]. */
  bool content_closed;  /**< True after [[/content]]. */
} spriteFrameState;

/** Temporary frame metadata stored in the builder. */
typedef struct {
  uint32_t row_offset; /**< First row index owned by this frame. */
  uint32_t row_count;  /**< Number of rows in this frame. */
  uint16_t width;      /**< Maximum row width in terminal columns. */
  uint16_t height;     /**< Number of rows. */
  double duration_ms;  /**< Frame duration in milliseconds. */
} spriteBuilderFrame;

/** Temporary animation definition stored in the builder. */
typedef struct {
  Arena* arena;               /**< Arena owning temporary arrays. */
  char* name;                 /**< Animation name. */
  char* source;               /**< Source filename for diagnostics. */
  uint32_t source_line;       /**< Declaration line for diagnostics. */
  uint32_t name_offset;       /**< Resolved name offset in the string pool. */
  uint32_t source_offset;     /**< Resolved source offset in the pool. */
  uint8_t icon_mask;          /**< Bit mask of declared icon types. */
  uint8_t flags;              /**< SPRITE_ANIMATION_FLAG_* values. */
  bool icon_set;              /**< True when icon-type was read. */
  bool duration_set;          /**< True when default duration was read. */
  bool loop_set;              /**< True when loop was read. */
  bool color_set;             /**< True when default-color was read. */
  bool loop;                  /**< Playback loop setting. */
  double duration_ms;         /**< Default frame duration. */
  int16_t default_color;      /**< Frame-start color or SPRITE_NO_COLOR. */
  spriteBuilderFrame* frames; /**< Parsed frames. */
  uint32_t frame_count;       /**< Number of frames. */
  uint32_t frame_capacity;    /**< Capacity of frames. */
  spriteRowSpan* rows;        /**< Parsed row spans. */
  uint32_t row_count;         /**< Number of rows. */
  uint32_t row_capacity;      /**< Capacity of rows. */
  spriteCellData* cells;      /**< Decoded cells. */
  uint32_t cell_count;        /**< Number of cells. */
  uint32_t cell_capacity;     /**< Capacity of cells. */
  spriteFrameState frame;     /**< State of the open frame. */
  bool frame_open;            /**< True between [[frame]] and [[/frame]]. */
} spriteBuilderAnimation;

/** Temporary list of builder definitions. */
typedef struct {
  Arena* arena;                   /**< Arena owning the item array. */
  spriteBuilderAnimation** items; /**< Definition pointers. */
  uint32_t count;                 /**< Number of definitions. */
  uint32_t capacity;              /**< Capacity of items. */
} spriteBuilderList;

/** Temporary library built from all sprite files. */
typedef struct {
  spriteBuilderList definitions; /**< Accepted definitions. */
} spriteBuilderLibrary;

/** Final storage layout offsets in bytes. */
typedef struct {
  uint32_t animations; /**< Offset of the animation array. */
  uint32_t frames;     /**< Offset of the frame array. */
  uint32_t rows;       /**< Offset of the row array. */
  uint32_t cells;      /**< Offset of the cell array. */
  uint32_t strings;    /**< Offset of the string pool. */
  uint32_t total;      /**< Total allocation size. */
} spriteStorageLayout;

/* PRIVATE SPRITE FUNCTIONS */
static bool checkedAddSize(size_t left, size_t right, size_t* result);
static bool checkedMulSize(size_t left, size_t right, size_t* result);
static size_t alignUpSize(size_t value, size_t alignment);
static bool arenaReserve(Arena* arena, void** data, uint32_t* capacity,
                         uint32_t needed, size_t element_size);
static char* arenaDuplicateRange(Arena* arena, const char* start,
                                 size_t length);
static uint32_t stringPoolAppend(Arena* arena, spriteStringPool* pool,
                                 const char* text, bool* ok);

static ErrorType reportDiagnostic(ErrorType code, ErrorType* error,
                                  const char* filename, uint32_t line,
                                  const char* format, ...);
static char* readLine(FILE* file, Arena* arena, char** buffer,
                      uint32_t* capacity, uint32_t* length, bool* failed);

static void trimRange(const char* line, const char** start, const char** end);
static size_t rangeLength(const char* start, const char* end);
static bool rangeEquals(const char* start, size_t length, const char* literal);
static bool lineEquals(const char* start, const char* end, const char* literal);
static bool parseAnimationLine(const char* line, const char** name,
                               size_t* name_length);
static bool isStructuralLine(const char* start, const char* end);
static bool splitOption(const char* line, const char** key, size_t* key_length,
                        const char** value, size_t* value_length);
static bool parseQuotedRange(const char* value, size_t length,
                             const char** inner, size_t* inner_length);
static bool parseUnsignedRange(const char* value, size_t length,
                               uint32_t maximum, uint32_t* result);
static bool parseDurationRange(const char* value, size_t length,
                               double* result_ms);
static bool parseIconTypes(const char* value, size_t length, uint8_t* mask);
static bool parseBoolRange(const char* value, size_t length, bool* result);

static bool appendCell(spriteBuilderAnimation* animation, uint32_t codepoint,
                       int32_t width, uint8_t foreground, uint8_t background,
                       uint8_t flags, const char* filename, uint32_t line,
                       ErrorType* error);
static bool decodeUtf8(const unsigned char* input, size_t available,
                       uint32_t* codepoint, size_t* length);
static bool parseColorEscape(const char* input, const char** end,
                             uint8_t* foreground, uint8_t* background,
                             bool* foreground_set, bool* background_set);
static bool parseUnicodeEscape(const char* input, const char** end,
                               uint32_t* codepoint);
static bool parseBlankRunEscape(const char* input, const char** end,
                                uint32_t* count);
static bool decodeContentLine(const char* line, spriteBuilderAnimation* pending,
                              const char* filename, uint32_t line_number,
                              ErrorType* error);

static void clearPendingFrame(spriteBuilderAnimation* pending);
static bool startFrame(spriteBuilderAnimation* pending, const char* filename,
                       uint32_t line_number, ErrorType* error);
static bool closeContent(spriteBuilderAnimation* pending, const char* filename,
                         uint32_t line_number, ErrorType* error);
static bool closeFrame(spriteBuilderAnimation* pending, const char* filename,
                       uint32_t line_number, ErrorType* error);
static bool finalizeAnimation(spriteBuilderAnimation* pending,
                              spriteBuilderList* definitions,
                              const char* filename, uint32_t line_number,
                              ErrorType* error);
static bool appendDefinition(spriteBuilderList* definitions,
                             spriteBuilderAnimation* animation);
static bool mergeDefinitions(spriteBuilderLibrary* library,
                             spriteBuilderList* definitions, ErrorType* error);
static bool parseSpriteFile(const char* filename, spriteBuilderLibrary* library,
                            Arena* arena, ErrorType* error);

static bool validateBuilder(const spriteBuilderLibrary* library,
                            const char* filename, ErrorType* error);
static bool calculateLayout(const spriteBuilderLibrary* library,
                            const spriteStringPool* pool,
                            spriteStorageLayout* layout, ErrorType* error);
static bool finalizeLibrary(const spriteBuilderLibrary* library, Arena* arena,
                            SpriteLibrary** output, ErrorType* error);
static bool validateLibrary(const SpriteLibrary* library);
static const char* stringAt(const SpriteAnimation* animation, uint32_t offset);
static const SpriteFrame* animationFrame(const SpriteAnimation* animation,
                                         size_t index);

/**
 * Add two sizes with overflow checks.
 * @param left First operand.
 * @param right Second operand.
 * @param result Output: checked sum.
 * @return true when the sum is representable.
 */
static bool checkedAddSize(size_t left, size_t right, size_t* result) {
  if (left > SIZE_MAX - right) return false;
  *result = left + right;
  return true;
}

/**
 * Multiply two sizes with overflow checks.
 * @param left First operand.
 * @param right Second operand.
 * @param result Output: checked product.
 * @return true when the product is representable.
 */
static bool checkedMulSize(size_t left, size_t right, size_t* result) {
  if (left != 0 && right > SIZE_MAX / left) return false;
  *result = left * right;
  return true;
}

/**
 * Round a size up to an alignment boundary.
 * @param value Size to align.
 * @param alignment Power-of-two alignment.
 * @return Aligned size, or SIZE_MAX on overflow.
 */
static size_t alignUpSize(size_t value, size_t alignment) {
  size_t mask = alignment - 1;
  if (value > SIZE_MAX - mask) return SIZE_MAX;
  return (value + mask) & ~mask;
}

/**
 * Grow an arena-backed array with overflow checks.
 * @param arena Arena owning the allocation.
 * @param data Pointer to the array pointer.
 * @param capacity Pointer to the stored element capacity.
 * @param needed Minimum element count required.
 * @param element_size Size of one element.
 * @return true when the array has enough capacity.
 */
static bool arenaReserve(Arena* arena, void** data, uint32_t* capacity,
                         uint32_t needed, size_t element_size) {
  if (needed <= *capacity) return true;

  uint32_t new_capacity = *capacity == 0 ? 8 : *capacity;
  while (new_capacity < needed) {
    if (new_capacity > UINT32_MAX / 2) {
      new_capacity = needed;
      break;
    }
    new_capacity *= 2;
  }

  size_t bytes;
  if (!checkedMulSize(new_capacity, element_size, &bytes)) return false;
  size_t old_bytes = 0;
  if (*data != NULL && *capacity > 0 &&
      !checkedMulSize(*capacity, element_size, &old_bytes))
    return false;

  void* resized = ArenaAllocate(arena, bytes);
  if (resized == NULL) return false;
  if (old_bytes > 0) {
    memcpy(resized, *data, old_bytes);
    ArenaRelease(arena, *data, old_bytes);
  }
  *data = resized;
  *capacity = new_capacity;
  return true;
}

/**
 * Copy a character range into arena-owned storage.
 * @param arena Arena to allocate from.
 * @param start First character to copy.
 * @param length Number of characters to copy.
 * @return NUL-terminated copy, or NULL on allocation failure.
 */
static char* arenaDuplicateRange(Arena* arena, const char* start,
                                 size_t length) {
  size_t bytes;
  if (!checkedAddSize(length, 1, &bytes)) return NULL;
  char* copy = (char*)ArenaAllocate(arena, bytes);
  if (copy == NULL) return NULL;
  memcpy(copy, start, length);
  copy[length] = '\0';
  return copy;
}

/**
 * Append a string to the deduplicating pool.
 * @param arena Arena to allocate from.
 * @param pool String pool to update.
 * @param text NUL-terminated string to append.
 * @param ok Output: false on allocation failure.
 * @return Offset of the string in the pool.
 */
static uint32_t stringPoolAppend(Arena* arena, spriteStringPool* pool,
                                 const char* text, bool* ok) {
  *ok = true;
  if (text == NULL) {
    *ok = false;
    return 0;
  }

  size_t text_length = strlen(text);
  if (text_length > UINT32_MAX) {
    *ok = false;
    return 0;
  }
  uint32_t length = (uint32_t)text_length;

  uint32_t offset = 0;
  while (offset < pool->length) {
    const char* existing = pool->data + offset;
    size_t existing_length = strlen(existing);
    if (existing_length == text_length &&
        memcmp(existing, text, text_length) == 0)
      return offset;
    offset += (uint32_t)existing_length + 1;
  }

  uint32_t needed = length + 1;
  if (pool->length > UINT32_MAX - needed) {
    *ok = false;
    return 0;
  }
  if (!arenaReserve(arena, (void**)&pool->data, &pool->capacity,
                    pool->length + needed, 1)) {
    *ok = false;
    return 0;
  }
  memcpy(pool->data + pool->length, text, needed);
  offset = pool->length;
  pool->length += needed;
  return offset;
}

/**
 * Log a formatted diagnostic with filename and line number.
 * @param code Error type to report and return.
 * @param error Optional output receiving the error type.
 * @param filename Source filename.
 * @param line Source line number.
 * @param format Printf-style message.
 * @return The reported error type.
 */
static ErrorType reportDiagnostic(ErrorType code, ErrorType* error,
                                  const char* filename, uint32_t line,
                                  const char* format, ...) {
  char context[SPRITE_DIAGNOSTIC_SIZE];
  int prefix =
    snprintf(context, sizeof(context), "%s:%" PRIu32 ": ", filename, line);
  if (prefix < 0 || (size_t)prefix >= sizeof(context)) return code;

  va_list args;
  va_start(args, format);
  vsnprintf(context + prefix, sizeof(context) - (size_t)prefix, format, args);
  va_end(args);

  LogError(context, code);
  if (error != NULL) *error = code;
  return code;
}

/**
 * Read one line into an arena-backed reusable buffer.
 * @param file Open file.
 * @param arena Arena owning the buffer.
 * @param buffer Reusable line buffer.
 * @param capacity Capacity of the reusable buffer.
 * @param length Output: bytes read before the newline.
 * @param failed Output: true on allocation failure.
 * @return Buffer containing the line, or NULL at EOF or failure.
 */
static char* readLine(FILE* file, Arena* arena, char** buffer,
                      uint32_t* capacity, uint32_t* length, bool* failed) {
  uint32_t used = 0;
  int character = EOF;
  while ((character = fgetc(file)) != EOF) {
    if (character == '\n') break;
    if (used + 1 >= *capacity) {
      uint32_t needed = *capacity == 0 ? 128 : *capacity * 2;
      if (needed <= *capacity) {
        *failed = true;
        return NULL;
      }
      char* resized = (char*)ArenaAllocate(arena, needed);
      if (resized == NULL) {
        *failed = true;
        return NULL;
      }
      if (*buffer != NULL) {
        if (used > 0) memcpy(resized, *buffer, used);
        ArenaRelease(arena, *buffer, *capacity);
      }
      *buffer = resized;
      *capacity = needed;
    }
    (*buffer)[used++] = (char)character;
  }

  if (character == EOF && used == 0) return NULL;
  if (used > 0 && (*buffer)[used - 1] == '\r') used--;
  (*buffer)[used] = '\0';
  *length = used;
  return *buffer;
}

/**
 * Trim ASCII whitespace from both ends of a line.
 * @param line Source line.
 * @param start Output: first non-space character.
 * @param end Output: one past the last non-space character.
 */
static void trimRange(const char* line, const char** start, const char** end) {
  const char* first = line;
  while (*first != '\0' && isspace((unsigned char)*first)) first++;
  const char* last = first;
  while (*last != '\0') last++;
  while (last > first && isspace((unsigned char)*(last - 1))) last--;
  *start = first;
  *end = last;
}

/**
 * Get the length of a character range.
 * @param start First character.
 * @param end One past the last character.
 * @return Range length.
 */
static size_t rangeLength(const char* start, const char* end) {
  return end > start ? (size_t)(end - start) : 0;
}

/**
 * Compare a character range with a literal string.
 * @param start First character.
 * @param length Range length.
 * @param literal Null-terminated literal.
 * @return true when equal.
 */
static bool rangeEquals(const char* start, size_t length, const char* literal) {
  return strlen(literal) == length && strncmp(start, literal, length) == 0;
}

/**
 * Compare a trimmed line with a literal.
 * @param start First character.
 * @param end One past the last character.
 * @param literal Null-terminated literal.
 * @return true when equal.
 */
static bool lineEquals(const char* start, const char* end,
                       const char* literal) {
  return rangeEquals(start, rangeLength(start, end), literal);
}

/**
 * Parse an animation declaration line.
 * @param line Source line.
 * @param name Output: name start.
 * @param name_length Output: name length.
 * @return true when the line is a valid declaration.
 */
static bool parseAnimationLine(const char* line, const char** name,
                               size_t* name_length) {
  const char* start;
  const char* end;
  trimRange(line, &start, &end);
  const char* prefix = "[animation.";
  size_t prefix_length = strlen(prefix);
  size_t length = rangeLength(start, end);
  if (length <= prefix_length + 1) return false;
  if (strncmp(start, prefix, prefix_length) != 0) return false;
  if (*(end - 1) != ']') return false;

  const char* name_start = start + prefix_length;
  const char* name_end = end - 1;
  for (const char* p = name_start; p < name_end; p++)
    if (isspace((unsigned char)*p)) return false;

  *name = name_start;
  *name_length = rangeLength(name_start, name_end);
  return *name_length > 0;
}

/**
 * Check whether a line is a reserved structural delimiter.
 * @param start First non-space character.
 * @param end One past the last non-space character.
 * @return true for animation, frame, and content delimiters.
 */
static bool isStructuralLine(const char* start, const char* end) {
  static const char* const delimiters[] = {"[[frame]]", "[[/frame]]",
                                           "[[content]]", "[[/content]]"};
  for (size_t i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]); i++)
    if (lineEquals(start, end, delimiters[i])) return true;

  const char* prefix = "[animation.";
  size_t length = rangeLength(start, end);
  return length > strlen(prefix) && strncmp(start, prefix, strlen(prefix)) == 0;
}

/**
 * Split a key/value option line.
 * @param line Source line.
 * @param key Output: key start.
 * @param key_length Output: key length.
 * @param value Output: value start.
 * @param value_length Output: value length.
 * @return true when both key and value are present.
 */
static bool splitOption(const char* line, const char** key, size_t* key_length,
                        const char** value, size_t* value_length) {
  const char* equals = strchr(line, '=');
  if (equals == NULL) return false;

  const char* key_end;
  const char* key_start;
  trimRange(line, &key_start, &key_end);
  while (key_end > key_start && isspace((unsigned char)*(key_end - 1)))
    key_end--;
  if (key_end > equals) key_end = equals;
  while (key_end > key_start && isspace((unsigned char)*(key_end - 1)))
    key_end--;

  const char* value_start;
  const char* value_end;
  trimRange(equals + 1, &value_start, &value_end);
  if (key_end == key_start || value_end == value_start) return false;

  *key = key_start;
  *key_length = rangeLength(key_start, key_end);
  *value = value_start;
  *value_length = rangeLength(value_start, value_end);
  return true;
}

/**
 * Extract the contents of a quoted option value.
 * @param value Value range.
 * @param length Value length.
 * @param inner Output: first character inside the quotes.
 * @param inner_length Output: inside length.
 * @return true when the value is quoted.
 */
static bool parseQuotedRange(const char* value, size_t length,
                             const char** inner, size_t* inner_length) {
  if (length < 2 || value[0] != '"' || value[length - 1] != '"') return false;
  *inner = value + 1;
  *inner_length = length - 2;
  return true;
}

/**
 * Parse a positive decimal integer range.
 * @param value Value range.
 * @param length Value length.
 * @param maximum Largest accepted value.
 * @param result Output: parsed value.
 * @return true when valid and in range.
 */
static bool parseUnsignedRange(const char* value, size_t length,
                               uint32_t maximum, uint32_t* result) {
  if (length == 0) return false;
  uint32_t parsed = 0;
  for (size_t i = 0; i < length; i++) {
    if (!isdigit((unsigned char)value[i])) return false;
    uint32_t digit = (uint32_t)(value[i] - '0');
    if (parsed > (maximum - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  *result = parsed;
  return true;
}

/**
 * Parse a quoted duration such as "150ms" or "1.0s".
 * @param value Value range.
 * @param length Value length.
 * @param result_ms Output: duration in milliseconds.
 * @return true when valid.
 */
static bool parseDurationRange(const char* value, size_t length,
                               double* result_ms) {
  const char* inner;
  size_t inner_length;
  if (!parseQuotedRange(value, length, &inner, &inner_length) ||
      inner_length == 0)
    return false;

  size_t number_length = 0;
  bool seen_digit = false;
  while (number_length < inner_length &&
         (isdigit((unsigned char)inner[number_length]) ||
          inner[number_length] == '.')) {
    if (isdigit((unsigned char)inner[number_length])) seen_digit = true;
    number_length++;
  }
  if (!seen_digit || number_length == 0) return false;

  const char* suffix = inner + number_length;
  size_t suffix_length = inner_length - number_length;
  double multiplier;

  if (rangeEquals(suffix, suffix_length, "ms")) {
    multiplier = 1.0;
  } else if (rangeEquals(suffix, suffix_length, "s")) {
    multiplier = 1000.0;
  } else {
    return false;
  }

  char number[SPRITE_DIAGNOSTIC_SIZE];
  if (number_length >= sizeof(number)) return false;
  memcpy(number, inner, number_length);
  number[number_length] = '\0';

  errno = 0;
  char* end = NULL;
  double parsed = strtod(number, &end);
  bool valid = end != number && *end == '\0' && errno != ERANGE &&
               isfinite(parsed) && parsed > 0.0 &&
               parsed <= SPRITE_MAX_DURATION_MS / multiplier;
  if (!valid) return false;

  *result_ms = parsed * multiplier;
  return true;
}

/**
 * Parse one or more icon type names.
 * @param value Value range.
 * @param length Value length.
 * @param mask Output: bit mask of icon types.
 * @return true when all names are valid and distinct.
 */
static bool parseIconTypes(const char* value, size_t length, uint8_t* mask) {
  static const char* const names[SPRITE_ICON_TYPE_COUNT] = {"nerd-icons",
                                                            "emojis", "ascii"};
  const char* p = value;
  const char* end = value + length;
  uint32_t parsed_mask = 0;
  uint32_t count = 0;
  bool array = false;
  bool need_item = false;

  while (p < end && isspace((unsigned char)*p)) p++;
  if (p < end && *p == '[') {
    array = true;
    p++;
  }

  while (true) {
    while (p < end && isspace((unsigned char)*p)) p++;
    if (array && p < end && *p == ']') {
      if (need_item) return false;
      break;
    }

    if (p >= end || *p != '"') return false;
    const char* item_start = p + 1;
    const char* item_end = memchr(item_start, '"', (size_t)(end - item_start));
    if (item_end == NULL) return false;
    size_t item_length = (size_t)(item_end - item_start);
    p = item_end + 1;

    int32_t index = -1;
    for (int32_t i = 0; i < SPRITE_ICON_TYPE_COUNT; i++)
      if (rangeEquals(item_start, item_length, names[i])) index = i;
    if (index < 0 || (parsed_mask & (1u << index)) != 0) return false;

    parsed_mask |= 1u << index;
    count++;
    need_item = false;
    if (count > SPRITE_ICON_TYPE_COUNT) return false;

    while (p < end && isspace((unsigned char)*p)) p++;
    if (!array) break;
    if (p < end && *p == ',') {
      p++;
      need_item = true;
      continue;
    }
    if (p < end && *p == ']') break;
    return false;
  }

  if (array) {
    p++;
    while (p < end && isspace((unsigned char)*p)) p++;
  }
  if (p != end || count == 0) return false;

  *mask = (uint8_t)parsed_mask;
  return true;
}

/**
 * Parse a boolean option value.
 * @param value Value range.
 * @param length Value length.
 * @param result Output: parsed boolean.
 * @return true when the value is true or false.
 */
static bool parseBoolRange(const char* value, size_t length, bool* result) {
  if (rangeEquals(value, length, "true")) {
    *result = true;
    return true;
  }
  if (rangeEquals(value, length, "false")) {
    *result = false;
    return true;
  }
  return false;
}

/**
 * Append one decoded cell to a builder animation.
 * @param animation Target builder animation.
 * @param codepoint Unicode codepoint.
 * @param width Display width override, or -1 to use wcwidth.
 * @param foreground Foreground color.
 * @param background Background color.
 * @param flags Cell flags.
 * @param filename Source filename.
 * @param line Source line number.
 * @param error Optional diagnostic output.
 * @return true when appended.
 */
static bool appendCell(spriteBuilderAnimation* animation, uint32_t codepoint,
                       int32_t width, uint8_t foreground, uint8_t background,
                       uint8_t flags, const char* filename, uint32_t line,
                       ErrorType* error) {
  int32_t display_width = width;
  if (display_width < 0) {
    int decoded = wcwidth((wchar_t)codepoint);
    display_width = decoded < 0 ? 1 : decoded;
  }
  if (display_width < 0 || display_width > UINT8_MAX) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename, line,
                     "cell width overflow");
    return false;
  }

  if (!arenaReserve(animation->arena, (void**)&animation->cells,
                    &animation->cell_capacity, animation->cell_count + 1,
                    sizeof(spriteCellData)))
    return false;

  spriteCellData* cell = &animation->cells[animation->cell_count++];
  cell->codepoint = codepoint;
  cell->foreground = foreground;
  cell->background = background;
  cell->width = (uint8_t)display_width;
  cell->flags = flags;
  return true;
}

/**
 * Decode one UTF-8 sequence.
 * @param input Input bytes.
 * @param available Bytes remaining.
 * @param codepoint Output: decoded codepoint.
 * @param length Output: number of bytes consumed.
 * @return true for a valid sequence.
 */
static bool decodeUtf8(const unsigned char* input, size_t available,
                       uint32_t* codepoint, size_t* length) {
  if (available == 0) return false;
  unsigned char first = input[0];
  if (first < 0x80) {
    *codepoint = first;
    *length = 1;
    return true;
  }

  if (first >= 0xC2 && first <= 0xDF && available >= 2) {
    if ((input[1] & 0xC0) != 0x80) return false;
    *codepoint = ((uint32_t)(first & 0x1F) << 6) | (input[1] & 0x3F);
    *length = 2;
    return true;
  }

  if (first >= 0xE0 && first <= 0xEF && available >= 3) {
    if ((input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80) return false;
    if (first == 0xE0 && input[1] < 0xA0) return false;
    if (first == 0xED && input[1] >= 0xA0) return false;
    *codepoint = ((uint32_t)(first & 0x0F) << 12) |
                 ((uint32_t)(input[1] & 0x3F) << 6) | (input[2] & 0x3F);
    *length = 3;
    return true;
  }

  if (first >= 0xF0 && first <= 0xF4 && available >= 4) {
    if ((input[1] & 0xC0) != 0x80 || (input[2] & 0xC0) != 0x80 ||
        (input[3] & 0xC0) != 0x80)
      return false;
    if (first == 0xF0 && input[1] < 0x90) return false;
    if (first == 0xF4 && input[1] >= 0x90) return false;
    *codepoint = ((uint32_t)(first & 0x07) << 18) |
                 ((uint32_t)(input[1] & 0x3F) << 12) |
                 ((uint32_t)(input[2] & 0x3F) << 6) | (input[3] & 0x3F);
    *length = 4;
    return true;
  }

  return false;
}

/**
 * Parse a color escape at a backslash.
 * @param input Pointer to the backslash.
 * @param end Output: first character after the escape.
 * @param foreground Output: foreground color.
 * @param background Output: background color.
 * @param foreground_set Output: true when foreground was declared.
 * @param background_set Output: true when background was declared.
 * @return true for a valid color escape.
 */
static bool parseColorEscape(const char* input, const char** end,
                             uint8_t* foreground, uint8_t* background,
                             bool* foreground_set, bool* background_set) {
  const char* p = input + 2;
  uint32_t parsed_foreground;
  uint32_t parsed_background = 0;
  bool parsed_background_set = false;

  if (*p == '{') {
    const char* close = strchr(p + 1, '}');
    if (close == NULL) return false;
    const char* dot = memchr(p + 1, '.', (size_t)(close - (p + 1)));
    if (dot != NULL) {
      if (!parseUnsignedRange(p + 1, (size_t)(dot - (p + 1)), SPRITE_MAX_COLOR,
                              &parsed_foreground) ||
          !parseUnsignedRange(dot + 1, (size_t)(close - (dot + 1)),
                              SPRITE_MAX_COLOR, &parsed_background))
        return false;
      parsed_background_set = true;
    } else if (!parseUnsignedRange(p + 1, (size_t)(close - (p + 1)),
                                   SPRITE_MAX_COLOR, &parsed_foreground)) {
      return false;
    }
    p = close + 1;
  } else {
    const char* digit_start = p;
    while (isdigit((unsigned char)*p)) p++;
    if (p == digit_start) return false;
    if (!parseUnsignedRange(digit_start, (size_t)(p - digit_start),
                            SPRITE_MAX_COLOR, &parsed_foreground))
      return false;
  }

  *foreground = (uint8_t)parsed_foreground;
  *background = (uint8_t)parsed_background;
  *foreground_set = true;
  *background_set = parsed_background_set;
  *end = p;
  return true;
}

/**
 * Parse a Unicode escape such as \u{1F600}.
 * @param input Pointer to the backslash.
 * @param end Output: first character after the escape.
 * @param codepoint Output: decoded codepoint.
 * @return true for a valid escape.
 */
static bool parseUnicodeEscape(const char* input, const char** end,
                               uint32_t* codepoint) {
  const char* p = input + 2;
  if (*p != '{') return false;
  p++;

  uint32_t value = 0;
  uint32_t digits = 0;
  while (*p != '\0' && *p != '}') {
    unsigned char character = (unsigned char)*p;
    uint32_t digit;
    if (character >= '0' && character <= '9')
      digit = (uint32_t)(character - '0');
    else if (character >= 'a' && character <= 'f')
      digit = (uint32_t)(character - 'a' + 10);
    else if (character >= 'A' && character <= 'F')
      digit = (uint32_t)(character - 'A' + 10);
    else
      return false;
    if (++digits > 6) return false;
    value = (value << 4) | digit;
    p++;
  }

  if (*p != '}' || digits == 0) return false;
  if (value > 0x10FFFF) return false;
  if (value >= 0xD800 && value <= 0xDFFF) return false;

  *codepoint = value;
  *end = p + 1;
  return true;
}

/**
 * Parse a blank-run escape such as \{5}.
 * @param input Pointer to the backslash.
 * @param end Output: first character after the escape.
 * @param count Output: blank cell count.
 * @return true for a valid escape.
 */
static bool parseBlankRunEscape(const char* input, const char** end,
                                uint32_t* count) {
  const char* p = input + 2;
  const char* digit_start = p;
  while (isdigit((unsigned char)*p)) p++;
  if (p == digit_start || *p != '}') return false;
  if (!parseUnsignedRange(digit_start, (size_t)(p - digit_start),
                          SPRITE_MAX_BLANK_RUN, count) ||
      *count == 0)
    return false;
  *end = p + 1;
  return true;
}

/**
 * Decode one raw content line into cells and append one row.
 * @param line Raw content line.
 * @param pending Pending builder animation.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when decoded.
 */
static bool decodeContentLine(const char* line, spriteBuilderAnimation* pending,
                              const char* filename, uint32_t line_number,
                              ErrorType* error) {
  uint32_t first_cell = pending->cell_count;
  const unsigned char* p = (const unsigned char*)line;
  const unsigned char* end = p + strlen(line);

  while (p < end) {
    if (*p == '\\' && p + 1 < end && p[1] == 'c') {
      const char* after;
      if (!parseColorEscape((const char*)p, &after, &pending->frame.foreground,
                            &pending->frame.background,
                            &pending->frame.foreground_set,
                            &pending->frame.background_set)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "malformed color escape");
        return false;
      }
      p = (const unsigned char*)after;
      continue;
    }
    if (*p == '\\' && p + 1 < end && p[1] == 'u') {
      const char* after;
      uint32_t codepoint;
      if (!parseUnicodeEscape((const char*)p, &after, &codepoint)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "invalid Unicode escape");
        return false;
      }
      if (!appendCell(
            pending, codepoint, -1, pending->frame.foreground,
            pending->frame.background,
            (pending->frame.foreground_set ? SPRITE_CELL_FLAG_FOREGROUND : 0) |
              (pending->frame.background_set ? SPRITE_CELL_FLAG_BACKGROUND : 0),
            filename, line_number, error))
        return false;
      p = (const unsigned char*)after;
      continue;
    }
    if (*p == '\\' && p + 1 < end && p[1] == '{') {
      const char* after;
      uint32_t count;
      if (!parseBlankRunEscape((const char*)p, &after, &count)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "invalid blank-cell count");
        return false;
      }
      uint8_t flags = SPRITE_CELL_FLAG_BLANK;
      if (pending->frame.foreground_set) flags |= SPRITE_CELL_FLAG_FOREGROUND;
      if (pending->frame.background_set) flags |= SPRITE_CELL_FLAG_BACKGROUND;
      for (uint32_t i = 0; i < count; i++)
        if (!appendCell(pending, ' ', 1, pending->frame.foreground,
                        pending->frame.background, flags, filename, line_number,
                        error))
          return false;
      p = (const unsigned char*)after;
      continue;
    }
    if (*p == '\\' && p + 1 < end && p[1] == 's') {
      uint8_t flags = SPRITE_CELL_FLAG_BLANK;
      if (pending->frame.foreground_set) flags |= SPRITE_CELL_FLAG_FOREGROUND;
      if (pending->frame.background_set) flags |= SPRITE_CELL_FLAG_BACKGROUND;
      if (!appendCell(pending, ' ', 1, pending->frame.foreground,
                      pending->frame.background, flags, filename, line_number,
                      error))
        return false;
      p += 2;
      continue;
    }

    uint32_t codepoint;
    size_t consumed;
    if (!decodeUtf8(p, (size_t)(end - p), &codepoint, &consumed)) {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number, "invalid UTF-8 sequence");
      return false;
    }
    if (codepoint == 0) {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number, "NUL in sprite content");
      return false;
    }
    uint8_t flags = 0;
    if (pending->frame.foreground_set) flags |= SPRITE_CELL_FLAG_FOREGROUND;
    if (pending->frame.background_set) flags |= SPRITE_CELL_FLAG_BACKGROUND;
    if (!appendCell(pending, codepoint, -1, pending->frame.foreground,
                    pending->frame.background, flags, filename, line_number,
                    error))
      return false;
    p += consumed;
  }

  if (!arenaReserve(pending->arena, (void**)&pending->rows,
                    &pending->row_capacity, pending->row_count + 1,
                    sizeof(spriteRowSpan))) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }
  spriteRowSpan* row = &pending->rows[pending->row_count++];
  row->cell_offset = first_cell;
  row->cell_count = pending->cell_count - first_cell;
  return true;
}

/**
 * Reset state for a new frame.
 * @param pending Pending builder animation.
 */
static void clearPendingFrame(spriteBuilderAnimation* pending) {
  memset(&pending->frame, 0, sizeof(pending->frame));
  if (pending->default_color == SPRITE_NO_COLOR) {
    pending->frame.foreground_set = false;
  } else {
    pending->frame.foreground = (uint8_t)pending->default_color;
    pending->frame.foreground_set = true;
  }
  pending->frame.duration_ms = pending->duration_ms;
  pending->frame_open = false;
}

/**
 * Open a new frame declaration.
 * @param pending Pending builder animation.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when opened.
 */
static bool startFrame(spriteBuilderAnimation* pending, const char* filename,
                       uint32_t line_number, ErrorType* error) {
  if (pending->frame_open) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "nested frame declaration");
    return false;
  }
  clearPendingFrame(pending);
  pending->frame.first_row = pending->row_count;
  pending->frame.first_cell = pending->cell_count;
  pending->frame_open = true;
  return true;
}

/**
 * Close the content block of a frame.
 * @param pending Pending builder animation.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when closed.
 */
static bool closeContent(spriteBuilderAnimation* pending, const char* filename,
                         uint32_t line_number, ErrorType* error) {
  if (!pending->frame_open || !pending->frame.content_started) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "misplaced [[/content]]");
    return false;
  }
  if (pending->frame.content_closed) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "duplicate content block");
    return false;
  }
  pending->frame.content_closed = true;
  return true;
}

/**
 * Close a frame and append its metadata to the builder.
 * @param pending Pending builder animation.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when closed.
 */
static bool closeFrame(spriteBuilderAnimation* pending, const char* filename,
                       uint32_t line_number, ErrorType* error) {
  if (!pending->frame_open) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "misplaced [[/frame]]");
    return false;
  }
  if (!pending->frame.content_started || !pending->frame.content_closed) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number,
                     "frame must contain exactly one closed content block");
    return false;
  }

  uint32_t row_count = pending->row_count - pending->frame.first_row;
  if (row_count == 0) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "frame has no content rows");
    return false;
  }
  if (row_count > UINT16_MAX) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "frame height overflow");
    return false;
  }

  uint32_t width = 0;
  for (uint32_t i = pending->frame.first_row; i < pending->row_count; i++) {
    uint32_t row_width = 0;
    const spriteRowSpan* row = &pending->rows[i];
    for (uint32_t j = 0; j < row->cell_count; j++) {
      uint32_t cell_width = pending->cells[row->cell_offset + j].width;
      if (row_width > UINT16_MAX - cell_width) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "frame width overflow");
        return false;
      }
      row_width += cell_width;
    }
    if (row_width > width) width = row_width;
  }

  if (!arenaReserve(pending->arena, (void**)&pending->frames,
                    &pending->frame_capacity, pending->frame_count + 1,
                    sizeof(spriteBuilderFrame))) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }
  spriteBuilderFrame* frame = &pending->frames[pending->frame_count++];
  frame->row_offset = pending->frame.first_row;
  frame->row_count = row_count;
  frame->width = (uint16_t)width;
  frame->height = (uint16_t)row_count;
  frame->duration_ms = pending->frame.duration_set ? pending->frame.duration_ms
                                                   : pending->duration_ms;

  clearPendingFrame(pending);
  return true;
}

/**
 * Finish one animation declaration and move it into the definition list.
 * @param pending Pending builder animation.
 * @param definitions Completed definitions.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when finalized.
 */
static bool finalizeAnimation(spriteBuilderAnimation* pending,
                              spriteBuilderList* definitions,
                              const char* filename, uint32_t line_number,
                              ErrorType* error) {
  if (pending->frame_open) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "unclosed frame");
    return false;
  }
  if (pending->name == NULL || !pending->icon_set) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "animation requires a name and icon-type");
    return false;
  }
  if (pending->frame_count == 0) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "animation requires at least one frame");
    return false;
  }

  Arena* arena = pending->arena;
  spriteBuilderAnimation* finished =
    (spriteBuilderAnimation*)ArenaAllocate(arena, sizeof(*finished));
  if (finished == NULL) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }
  *finished = *pending;
  finished->flags = pending->loop ? SPRITE_ANIMATION_FLAG_LOOP : 0;
  if (!appendDefinition(definitions, finished)) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }

  memset(pending, 0, sizeof(*pending));
  pending->arena = arena;
  pending->default_color = SPRITE_NO_COLOR;
  clearPendingFrame(pending);
  return true;
}

/**
 * Append a builder definition to a list.
 * @param definitions Target list.
 * @param animation Owned builder animation.
 * @return true when appended.
 */
static bool appendDefinition(spriteBuilderList* definitions,
                             spriteBuilderAnimation* animation) {
  if (!arenaReserve(definitions->arena, (void**)&definitions->items,
                    &definitions->capacity, definitions->count + 1,
                    sizeof(spriteBuilderAnimation*)))
    return false;
  definitions->items[definitions->count++] = animation;
  return true;
}

/**
 * Merge new definitions into a builder library and validate icon masks.
 * @param library Target builder library.
 * @param definitions Definitions parsed from one file.
 * @param error Optional diagnostic output.
 * @return true when every definition was accepted.
 */
static bool mergeDefinitions(spriteBuilderLibrary* library,
                             spriteBuilderList* definitions, ErrorType* error) {
  for (uint32_t i = 0; i < definitions->count; i++) {
    spriteBuilderAnimation* candidate = definitions->items[i];
    for (uint32_t j = 0; j < library->definitions.count; j++) {
      spriteBuilderAnimation* existing = library->definitions.items[j];
      if (strcmp(existing->name, candidate->name) == 0 &&
          (existing->icon_mask & candidate->icon_mask) != 0) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error,
                         candidate->source, candidate->source_line,
                         "conflicting icon types for animation '%s'",
                         candidate->name);
        return false;
      }
    }
    if (!appendDefinition(&library->definitions, candidate)) {
      reportDiagnostic(MALLOC_ERROR, error, candidate->source,
                       candidate->source_line, "allocation failure");
      return false;
    }
  }
  return true;
}

/**
 * Parse one sprite file into temporary definitions, then merge them.
 * @param filename Path to the file.
 * @param library Target builder library.
 * @param arena Arena for temporary allocations.
 * @param error Optional diagnostic output.
 * @return true on success.
 */
static bool parseSpriteFile(const char* filename, spriteBuilderLibrary* library,
                            Arena* arena, ErrorType* error) {
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    reportDiagnostic(FILE_ERROR, error, filename, 0, "cannot open sprite file");
    return false;
  }

  spriteBuilderAnimation pending;
  spriteBuilderList definitions;
  memset(&pending, 0, sizeof(pending));
  memset(&definitions, 0, sizeof(definitions));
  pending.arena = arena;
  definitions.arena = arena;
  pending.default_color = SPRITE_NO_COLOR;
  clearPendingFrame(&pending);

  /** Parser state for the current input line. */
  enum {
    STATE_OUTSIDE_ANIMATION, /**< Outside any animation block. */
    STATE_IN_ANIMATION,      /**< Inside an animation block. */
    STATE_IN_FRAME,          /**< Inside a frame block. */
    STATE_IN_CONTENT         /**< Inside a content block. */
  } state = STATE_OUTSIDE_ANIMATION;

  uint32_t line_number = 0;
  char* buffer = NULL;
  uint32_t buffer_capacity = 0;
  bool success = true;

  while (success) {
    uint32_t line_length = 0;
    bool read_failed = false;
    char* line = readLine(file, arena, &buffer, &buffer_capacity, &line_length,
                          &read_failed);
    if (line == NULL) {
      if (read_failed || ferror(file)) {
        reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                         "allocation or read failure");
        success = false;
      }
      break;
    }
    line_number++;
    if (memchr(line, '\0', line_length) != NULL) {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number, "NUL in sprite content");
      success = false;
      break;
    }

    const char* start;
    const char* end;
    trimRange(line, &start, &end);

    if (state == STATE_IN_CONTENT) {
      if (lineEquals(start, end, "[[/content]]")) {
        success = closeContent(&pending, filename, line_number, error);
        state = STATE_IN_FRAME;
      } else if (isStructuralLine(start, end)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "structural delimiter inside content");
        success = false;
      } else {
        success =
          decodeContentLine(line, &pending, filename, line_number, error);
      }
      continue;
    }

    if (start == end || *start == '#') continue;

    const char* name;
    size_t name_length;
    if (parseAnimationLine(line, &name, &name_length)) {
      if (state == STATE_IN_FRAME) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "unclosed frame");
        success = false;
      } else {
        if (state == STATE_IN_ANIMATION)
          success = finalizeAnimation(&pending, &definitions, filename,
                                      line_number, error);
        if (success) {
          pending.name = arenaDuplicateRange(arena, name, name_length);
          pending.source =
            arenaDuplicateRange(arena, filename, strlen(filename));
          if (pending.name == NULL || pending.source == NULL) {
            reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                             "allocation failure");
            success = false;
          } else {
            pending.source_line = line_number;
            pending.duration_ms = 1000.0;
            pending.loop = true;
            pending.default_color = SPRITE_NO_COLOR;
            pending.icon_mask = 0;
            pending.icon_set = false;
            pending.duration_set = false;
            pending.loop_set = false;
            pending.color_set = false;
            clearPendingFrame(&pending);
            state = STATE_IN_ANIMATION;
          }
        }
      }
      continue;
    }

    const char* key;
    size_t key_length;
    const char* value;
    size_t value_length;
    bool has_option =
      splitOption(line, &key, &key_length, &value, &value_length);

    if (state == STATE_IN_ANIMATION && lineEquals(start, end, "[[frame]]")) {
      success = startFrame(&pending, filename, line_number, error);
      if (success) state = STATE_IN_FRAME;
    } else if (state == STATE_IN_FRAME &&
               lineEquals(start, end, "[[content]]")) {
      if (!pending.frame_open || pending.frame.content_started) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "misplaced or duplicate [[content]]");
        success = false;
      } else {
        pending.frame.content_started = true;
        state = STATE_IN_CONTENT;
      }
    } else if (state == STATE_IN_FRAME &&
               lineEquals(start, end, "[[/frame]]")) {
      success = closeFrame(&pending, filename, line_number, error);
      if (success) state = STATE_IN_ANIMATION;
    } else if (has_option && state == STATE_IN_ANIMATION) {
      if (rangeEquals(key, key_length, "icon-type")) {
        if (pending.icon_set) {
          reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                           line_number, "duplicate icon-type option");
          success = false;
        } else {
          success = parseIconTypes(value, value_length, &pending.icon_mask);
          if (!success)
            reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                             line_number, "invalid icon-type option");
          else
            pending.icon_set = true;
        }
      } else if (rangeEquals(key, key_length, "default-frame-duration")) {
        if (pending.duration_set) {
          reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                           line_number,
                           "duplicate default-frame-duration option");
          success = false;
        } else {
          success =
            parseDurationRange(value, value_length, &pending.duration_ms);
          if (!success)
            reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                             line_number,
                             "invalid default-frame-duration option");
          else
            pending.duration_set = true;
        }
      } else if (rangeEquals(key, key_length, "loop")) {
        if (pending.loop_set) {
          reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                           line_number, "duplicate loop option");
          success = false;
        } else {
          success = parseBoolRange(value, value_length, &pending.loop);
          if (!success)
            reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                             line_number, "invalid loop option");
          else
            pending.loop_set = true;
        }
      } else if (rangeEquals(key, key_length, "default-color")) {
        if (pending.color_set) {
          reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                           line_number, "duplicate default-color option");
          success = false;
        } else {
          uint32_t color;
          success =
            parseUnsignedRange(value, value_length, SPRITE_MAX_COLOR, &color);
          if (!success) {
            reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                             line_number, "invalid default-color option");
          } else {
            pending.color_set = true;
            pending.default_color = (int16_t)color;
            pending.frame.foreground = (uint8_t)color;
            pending.frame.foreground_set = true;
          }
        }
      } else {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "unknown animation option");
        success = false;
      }
    } else if (has_option && state == STATE_IN_FRAME) {
      if (!rangeEquals(key, key_length, "frame-duration")) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "unknown frame option");
        success = false;
      } else if (pending.frame.duration_set) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "duplicate frame-duration option");
        success = false;
      } else {
        success =
          parseDurationRange(value, value_length, &pending.frame.duration_ms);
        if (!success)
          reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                           line_number, "invalid frame-duration option");
        else
          pending.frame.duration_set = true;
      }
    } else {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number,
                       state == STATE_OUTSIDE_ANIMATION
                         ? "content outside animation"
                         : "misplaced frame or content delimiter");
      success = false;
    }
  }

  fclose(file);

  if (success && state == STATE_IN_CONTENT) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "unclosed content block");
    success = false;
  }
  if (success && state == STATE_IN_FRAME) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "unclosed frame");
    success = false;
  }
  if (success && state == STATE_IN_ANIMATION)
    success =
      finalizeAnimation(&pending, &definitions, filename, line_number, error);
  if (success && definitions.count == 0) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "no animation definitions");
    success = false;
  }
  if (success) success = mergeDefinitions(library, &definitions, error);
  return success;
}

/**
 * Validate builder data before finalization.
 * @param library Builder library to validate.
 * @param filename Source filename for diagnostics.
 * @param error Optional diagnostic output.
 * @return true when every definition is internally consistent.
 */
static bool validateBuilder(const spriteBuilderLibrary* library,
                            const char* filename, ErrorType* error) {
  for (uint32_t i = 0; i < library->definitions.count; i++) {
    const spriteBuilderAnimation* animation = library->definitions.items[i];
    if (animation->name == NULL || animation->source == NULL ||
        animation->icon_mask == 0 || animation->frame_count == 0)
      return reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                              animation->source_line,
                              "invalid animation metadata") == NO_ERROR;

    for (uint32_t f = 0; f < animation->frame_count; f++) {
      const spriteBuilderFrame* frame = &animation->frames[f];
      if (frame->row_count == 0 || frame->row_offset > animation->row_count ||
          frame->row_count > animation->row_count - frame->row_offset ||
          !isfinite(frame->duration_ms) || frame->duration_ms <= 0.0)
        return reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error,
                                filename, animation->source_line,
                                "invalid frame metadata") == NO_ERROR;
    }

    for (uint32_t r = 0; r < animation->row_count; r++) {
      const spriteRowSpan* row = &animation->rows[r];
      if (row->cell_offset > animation->cell_count ||
          row->cell_count > animation->cell_count - row->cell_offset)
        return reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error,
                                filename, animation->source_line,
                                "invalid row cell span") == NO_ERROR;
    }

    for (uint32_t c = 0; c < animation->cell_count; c++) {
      const spriteCellData* cell = &animation->cells[c];
      if (cell->codepoint > 0x10FFFF ||
          (cell->codepoint >= 0xD800 && cell->codepoint <= 0xDFFF) ||
          (cell->flags &
           ~(SPRITE_CELL_FLAG_BLANK | SPRITE_CELL_FLAG_FOREGROUND |
             SPRITE_CELL_FLAG_BACKGROUND)) != 0)
        return reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error,
                                filename, animation->source_line,
                                "invalid sprite cell") == NO_ERROR;
    }
  }
  return true;
}

/**
 * Calculate aligned offsets for the finalized storage block.
 * @param library Builder library.
 * @param pool Deduplicated string pool.
 * @param layout Output: calculated offsets.
 * @param error Optional diagnostic output.
 * @return true when the layout fits in uint32_t offsets.
 */
static bool calculateLayout(const spriteBuilderLibrary* library,
                            const spriteStringPool* pool,
                            spriteStorageLayout* layout, ErrorType* error) {
  uint64_t frames = 0;
  uint64_t rows = 0;
  uint64_t cells = 0;
  for (uint32_t i = 0; i < library->definitions.count; i++) {
    const spriteBuilderAnimation* animation = library->definitions.items[i];
    if (frames > UINT64_MAX - animation->frame_count ||
        rows > UINT64_MAX - animation->row_count ||
        cells > UINT64_MAX - animation->cell_count)
      return false;
    frames += animation->frame_count;
    rows += animation->row_count;
    cells += animation->cell_count;
  }

  uint64_t sizes[5] = {
    (uint64_t)library->definitions.count * sizeof(SpriteAnimation),
    frames * sizeof(SpriteFrame),
    rows * sizeof(SpriteRow),
    cells * sizeof(SpriteCell),
    pool->length,
  };
  uint64_t cursor =
    alignUpSize(sizeof(SpriteLibrary), SPRITE_STORAGE_ALIGNMENT);
  uint64_t offsets[6] = {cursor};
  for (size_t i = 0; i < 5; i++) {
    if (sizes[i] > UINT32_MAX) return false;
    cursor = alignUpSize((size_t)cursor, SPRITE_STORAGE_ALIGNMENT);
    if (cursor > UINT32_MAX - sizes[i]) return false;
    cursor += sizes[i];
    offsets[i + 1] = cursor;
  }
  cursor = alignUpSize((size_t)cursor, SPRITE_STORAGE_ALIGNMENT);
  if (cursor == SIZE_MAX || cursor > UINT32_MAX) {
    reportDiagnostic(MALLOC_ERROR, error, "<library>", 0,
                     "sprite storage too large");
    return false;
  }
  offsets[5] = cursor;

  layout->animations = (uint32_t)offsets[0];
  layout->frames = (uint32_t)offsets[1];
  layout->rows = (uint32_t)offsets[2];
  layout->cells = (uint32_t)offsets[3];
  layout->strings = (uint32_t)offsets[4];
  layout->total = (uint32_t)offsets[5];
  return true;
}

/**
 * Build the finalized one-allocation library from builder data.
 * @param library Builder library.
 * @param arena Arena holding builder data.
 * @param output Output receiving the finalized library.
 * @param error Optional diagnostic output.
 * @return true on success.
 */
static bool finalizeLibrary(const spriteBuilderLibrary* library, Arena* arena,
                            SpriteLibrary** output, ErrorType* error) {
  if (library->definitions.count == 0) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, "<library>", 0,
                     "no animation definitions");
    return false;
  }
  if (!validateBuilder(library, "<library>", error)) return false;

  spriteStringPool pool = {0};
  for (uint32_t i = 0; i < library->definitions.count; i++) {
    spriteBuilderAnimation* animation = library->definitions.items[i];
    bool pool_ok;
    animation->name_offset =
      stringPoolAppend(arena, &pool, animation->name, &pool_ok);
    if (!pool_ok) {
      reportDiagnostic(MALLOC_ERROR, error, animation->source,
                       animation->source_line, "allocation failure");
      return false;
    }
    animation->source_offset =
      stringPoolAppend(arena, &pool, animation->source, &pool_ok);
    if (!pool_ok) {
      reportDiagnostic(MALLOC_ERROR, error, animation->source,
                       animation->source_line, "allocation failure");
      return false;
    }
  }

  spriteStorageLayout layout;
  if (!calculateLayout(library, &pool, &layout, error)) return false;

  void* storage = calloc(1, layout.total);
  if (storage == NULL) {
    reportDiagnostic(MALLOC_ERROR, error, "<library>", 0, "allocation failure");
    return false;
  }

  SpriteLibrary* finalized = (SpriteLibrary*)storage;
  finalized->items =
    (SpriteAnimation*)((unsigned char*)storage + layout.animations);
  finalized->count = library->definitions.count;
  finalized->string_offset = layout.strings;
  finalized->storage_size = layout.total;

  SpriteAnimation* animations = finalized->items;
  SpriteFrame* frames = (SpriteFrame*)((unsigned char*)storage + layout.frames);
  SpriteRow* rows = (SpriteRow*)((unsigned char*)storage + layout.rows);
  SpriteCell* cells = (SpriteCell*)((unsigned char*)storage + layout.cells);
  unsigned char* strings = (unsigned char*)storage + layout.strings;
  if (pool.length > 0) memcpy(strings, pool.data, pool.length);

  uint32_t frame_index = 0;
  uint32_t row_index = 0;
  uint32_t cell_index = 0;
  for (uint32_t i = 0; i < library->definitions.count; i++) {
    const spriteBuilderAnimation* source = library->definitions.items[i];
    SpriteAnimation* animation = &animations[i];
    double cycle_duration = 0.0;
    uint16_t width = 0;
    uint16_t height = 0;

    animation->base = storage;
    animation->name_offset = layout.strings + source->name_offset;
    animation->source_offset = layout.strings + source->source_offset;
    animation->frame_offset =
      layout.frames + (uint32_t)((size_t)frame_index * sizeof(SpriteFrame));
    animation->frame_count = source->frame_count;
    animation->row_offset =
      layout.rows + (uint32_t)((size_t)row_index * sizeof(SpriteRow));
    animation->row_count = source->row_count;
    animation->cell_offset =
      layout.cells + (uint32_t)((size_t)cell_index * sizeof(SpriteCell));
    animation->cell_count = source->cell_count;
    animation->source_line = source->source_line;
    animation->icon_mask = source->icon_mask;
    animation->flags = source->flags;
    animation->default_color = source->default_color;
    animation->default_duration_ms = source->duration_ms;

    if (source->cell_count > 0)
      memcpy(cells + cell_index, source->cells,
             (size_t)source->cell_count * sizeof(spriteCellData));

    for (uint32_t r = 0; r < source->row_count; r++) {
      const spriteRowSpan* source_row = &source->rows[r];
      SpriteRow* row = &rows[row_index + r];
      row->cells = cells + cell_index;
      row->cell_offset = source_row->cell_offset;
      row->cell_count = source_row->cell_count;
    }

    for (uint32_t f = 0; f < source->frame_count; f++) {
      const spriteBuilderFrame* source_frame = &source->frames[f];
      SpriteFrame* frame = &frames[frame_index + f];
      frame->rows = rows + row_index + source_frame->row_offset;
      frame->row_offset = source_frame->row_offset;
      frame->row_count = source_frame->row_count;
      frame->width = source_frame->width;
      frame->height = source_frame->height;
      frame->duration_ms = source_frame->duration_ms;
      cycle_duration += source_frame->duration_ms;
      if (frame->width > width) width = frame->width;
      if (frame->height > height) height = frame->height;
    }
    animation->width = width;
    animation->height = height;
    animation->cycle_duration_ms =
      isfinite(cycle_duration) ? cycle_duration : 0.0;

    frame_index += source->frame_count;
    row_index += source->row_count;
    cell_index += source->cell_count;
  }

  if (!validateLibrary(finalized)) {
    free(storage);
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, "<library>", 0,
                     "invalid finalized sprite library");
    return false;
  }

  *output = finalized;
  return true;
}

/**
 * Validate offsets, pointers, strings, and cells in a finalized library.
 * @param library Library to validate.
 * @return true when every invariant holds.
 */
static bool validateLibrary(const SpriteLibrary* library) {
  const unsigned char* base = (const unsigned char*)library;
  if (library->count == 0 || library->string_offset >= library->storage_size)
    return false;
  if ((const unsigned char*)library->items !=
      base + alignUpSize(sizeof(SpriteLibrary), SPRITE_STORAGE_ALIGNMENT))
    return false;

  for (uint32_t i = 0; i < library->count; i++) {
    const SpriteAnimation* animation = &library->items[i];
    size_t frame_bytes;
    if (animation->base != library || animation->icon_mask == 0 ||
        animation->icon_mask > ((1u << SPRITE_ICON_TYPE_COUNT) - 1) ||
        (animation->flags & ~SPRITE_ANIMATION_FLAG_LOOP) != 0 ||
        (animation->default_color != SPRITE_NO_COLOR &&
         animation->default_color > SPRITE_MAX_COLOR) ||
        animation->frame_count == 0 ||
        !checkedMulSize(animation->frame_count, sizeof(SpriteFrame),
                        &frame_bytes) ||
        animation->frame_offset > library->storage_size ||
        frame_bytes > library->storage_size - animation->frame_offset)
      return false;

    const char* name = stringAt(animation, animation->name_offset);
    const char* source = stringAt(animation, animation->source_offset);
    if (name == NULL || source == NULL) return false;
    if (memchr(name, '\0',
               (size_t)(base + library->storage_size -
                        (const unsigned char*)name)) == NULL)
      return false;
    if (memchr(source, '\0',
               (size_t)(base + library->storage_size -
                        (const unsigned char*)source)) == NULL)
      return false;

    const SpriteFrame* frame_array =
      (const SpriteFrame*)(base + animation->frame_offset);
    if (animation->row_offset > library->storage_size ||
        animation->row_count >
          (library->storage_size - animation->row_offset) / sizeof(SpriteRow) ||
        animation->cell_offset > library->storage_size ||
        animation->cell_count >
          (library->storage_size - animation->cell_offset) / sizeof(SpriteCell))
      return false;

    for (uint32_t f = 0; f < animation->frame_count; f++) {
      const SpriteFrame* frame = &frame_array[f];
      if (frame->rows != (const SpriteRow*)(base + animation->row_offset) +
                           frame->row_offset ||
          frame->row_offset > animation->row_count ||
          frame->row_count > animation->row_count - frame->row_offset ||
          !isfinite(frame->duration_ms) || frame->duration_ms <= 0.0)
        return false;
    }

    const SpriteRow* row_array =
      (const SpriteRow*)(base + animation->row_offset);
    for (uint32_t r = 0; r < animation->row_count; r++) {
      const SpriteRow* row = &row_array[r];
      if (row->cells != (const SpriteCell*)(base + animation->cell_offset) ||
          row->cell_offset > animation->cell_count ||
          row->cell_count > animation->cell_count - row->cell_offset)
        return false;
    }

    const SpriteCell* cell_array =
      (const SpriteCell*)(base + animation->cell_offset);
    for (uint32_t c = 0; c < animation->cell_count; c++) {
      const SpriteCell* cell = &cell_array[c];
      if (cell->codepoint > 0x10FFFF ||
          (cell->codepoint >= 0xD800 && cell->codepoint <= 0xDFFF) ||
          (cell->flags &
           ~(SPRITE_CELL_FLAG_BLANK | SPRITE_CELL_FLAG_FOREGROUND |
             SPRITE_CELL_FLAG_BACKGROUND)) != 0)
        return false;
    }
  }
  return true;
}

/**
 * Load every sprite file into one library.
 * Duplicate icon-type assignments for the same animation name are rejected.
 * Detailed file and line diagnostics are logged through LogError.
 * @param filenames Array of sprite file paths.
 * @param count Number of paths in filenames.
 * @param library Output receiving the owning library. Set to NULL on failure.
 * @return NO_ERROR on success, or an ErrorType code on failure.
 */
ErrorType LoadSpriteLibrary(const char* const* filenames, size_t count,
                            SpriteLibrary** library) {
  if (library == NULL)
    return reportDiagnostic(INVALID_ARGUMENT_ERROR, NULL, "<library>", 0,
                            "NULL library output");
  *library = NULL;

  ErrorType error = NO_ERROR;
  if (filenames == NULL || count == 0)
    return reportDiagnostic(INVALID_ARGUMENT_ERROR, &error, "<library>", 0,
                            "no sprite files supplied");

  Arena arena;
  ArenaInit(&arena, ARENA_CHUNK_SIZE);
  spriteBuilderLibrary builder = {0};
  builder.definitions.arena = &arena;
  for (size_t i = 0; i < count; i++) {
    if (filenames[i] == NULL) {
      ArenaDestroy(&arena);
      return reportDiagnostic(INVALID_ARGUMENT_ERROR, &error, "<library>", 0,
                              "NULL sprite filename");
    }
    if (!parseSpriteFile(filenames[i], &builder, &arena, &error)) {
      ArenaDestroy(&arena);
      return error == NO_ERROR ? ANIMATION_DESERIALIZATION_ERROR : error;
    }
  }

  SpriteLibrary* loaded = NULL;
  bool finalized = finalizeLibrary(&builder, &arena, &loaded, &error);
  ArenaDestroy(&arena);
  if (!finalized)
    return error == NO_ERROR ? ANIMATION_DESERIALIZATION_ERROR : error;
  *library = loaded;
  return NO_ERROR;
}

/**
 * Free a sprite library and its single backing allocation.
 * Every Rollfilm created from the library must be freed first.
 * @param library Pointer to the library to free.
 */
void FreeSpriteLibrary(SpriteLibrary* library) { free(library); }

/**
 * Find an animation definition for a name and icon type.
 * @param library Pointer to the library to search.
 * @param name Animation name without the [animation.] prefix.
 * @param icon_type Icon type index (0=nerd-icons, 1=emojis, 2=ascii).
 * @return Immutable definition, or NULL when not found.
 */
const SpriteAnimation* SpriteLibraryFind(const SpriteLibrary* library,
                                         const char* name, int icon_type) {
  if (library == NULL || name == NULL || icon_type < 0 ||
      icon_type >= SPRITE_ICON_TYPE_COUNT)
    return NULL;
  uint8_t bit = (uint8_t)(1u << icon_type);
  for (uint32_t i = 0; i < library->count; i++) {
    const SpriteAnimation* animation = &library->items[i];
    if ((animation->icon_mask & bit) != 0 &&
        strcmp(SpriteAnimationName(animation), name) == 0)
      return animation;
  }
  return NULL;
}

/**
 * Resolve a pooled string offset.
 * @param animation Owning animation.
 * @param offset Offset in the string pool.
 * @return Pointer to the NUL-terminated string, or NULL for a bad offset.
 */
static const char* stringAt(const SpriteAnimation* animation, uint32_t offset) {
  const SpriteLibrary* library = (const SpriteLibrary*)animation->base;
  const unsigned char* base = (const unsigned char*)library;
  if (offset < library->string_offset || offset >= library->storage_size)
    return NULL;
  return (const char*)base + offset;
}

/**
 * Get an animation name.
 * @param animation Pointer to the animation definition.
 * @return Name string owned by the library.
 */
const char* SpriteAnimationName(const SpriteAnimation* animation) {
  return animation == NULL ? NULL : stringAt(animation, animation->name_offset);
}

/**
 * Get an animation frame.
 * @param animation Pointer to the animation definition.
 * @param index Frame index.
 * @return Immutable frame, or NULL when index is out of range.
 */
const SpriteFrame* SpriteAnimationFrame(const SpriteAnimation* animation,
                                        size_t index) {
  return animationFrame(animation, index);
}

/**
 * Get an animation frame.
 * @param animation Pointer to the animation definition.
 * @param index Frame index.
 * @return Immutable frame, or NULL when index is out of range.
 */
static const SpriteFrame* animationFrame(const SpriteAnimation* animation,
                                         size_t index) {
  if (animation == NULL || index >= animation->frame_count) return NULL;
  const SpriteFrame* frames =
    (const SpriteFrame*)((const unsigned char*)animation->base +
                         animation->frame_offset);
  return &frames[index];
}

/**
 * Get the number of frames in an animation.
 * @param animation Pointer to the animation definition.
 * @return Frame count.
 */
size_t SpriteAnimationFrameCount(const SpriteAnimation* animation) {
  return animation == NULL ? 0 : animation->frame_count;
}

/**
 * Get the cached animation width in terminal columns.
 * @param animation Pointer to the animation definition.
 * @return Maximum frame width.
 */
int SpriteAnimationWidth(const SpriteAnimation* animation) {
  return animation == NULL ? 0 : animation->width;
}

/**
 * Get the cached animation height in rows.
 * @param animation Pointer to the animation definition.
 * @return Maximum frame height.
 */
int SpriteAnimationHeight(const SpriteAnimation* animation) {
  return animation == NULL ? 0 : animation->height;
}

/**
 * Get the configured default duration for frames in milliseconds.
 * @param animation Pointer to the animation definition.
 * @return Default frame duration.
 */
double SpriteAnimationDefaultDurationMS(const SpriteAnimation* animation) {
  return animation == NULL ? 0.0 : animation->default_duration_ms;
}

/**
 * Get the cached total duration of one animation cycle.
 * @param animation Pointer to the animation definition.
 * @return Sum of all frame durations, or zero when unavailable.
 */
double SpriteAnimationCycleDurationMS(const SpriteAnimation* animation) {
  return animation == NULL ? 0.0 : animation->cycle_duration_ms;
}

/**
 * Get the configured loop flag.
 * @param animation Pointer to the animation definition.
 * @return true when playback repeats.
 */
bool SpriteAnimationLoop(const SpriteAnimation* animation) {
  return animation != NULL &&
         (animation->flags & SPRITE_ANIMATION_FLAG_LOOP) != 0;
}

/**
 * Get the default foreground color applied at frame start.
 * @param animation Pointer to the animation definition.
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteAnimationDefaultColor(const SpriteAnimation* animation) {
  return animation == NULL ? SPRITE_NO_COLOR : animation->default_color;
}

/**
 * Get a frame row.
 * @param frame Pointer to the frame.
 * @param index Row index.
 * @return Immutable row, or NULL when index is out of range.
 */
const SpriteRow* SpriteFrameRow(const SpriteFrame* frame, size_t index) {
  if (frame == NULL || index >= frame->row_count) return NULL;
  return &frame->rows[index];
}

/**
 * Get the number of rows in a frame.
 * @param frame Pointer to the frame.
 * @return Row count.
 */
size_t SpriteFrameRowCount(const SpriteFrame* frame) {
  return frame == NULL ? 0 : frame->row_count;
}

/**
 * Get the cached frame width in terminal columns.
 * @param frame Pointer to the frame.
 * @return Frame width.
 */
int SpriteFrameWidth(const SpriteFrame* frame) {
  return frame == NULL ? 0 : frame->width;
}

/**
 * Get the frame height in rows.
 * @param frame Pointer to the frame.
 * @return Frame height.
 */
int SpriteFrameHeight(const SpriteFrame* frame) {
  return frame == NULL ? 0 : frame->height;
}

/**
 * Get the frame duration in milliseconds.
 * @param frame Pointer to the frame.
 * @return Frame duration.
 */
double SpriteFrameDurationMS(const SpriteFrame* frame) {
  return frame == NULL ? 0.0 : frame->duration_ms;
}

/**
 * Get a row cell.
 * @param row Pointer to the row.
 * @param index Cell index.
 * @return Immutable cell, or NULL when index is out of range.
 */
const SpriteCell* SpriteRowCell(const SpriteRow* row, size_t index) {
  if (row == NULL || index >= row->cell_count) return NULL;
  return &row->cells[row->cell_offset + index];
}

/**
 * Get the number of cells in a row.
 * @param row Pointer to the row.
 * @return Cell count.
 */
size_t SpriteRowCellCount(const SpriteRow* row) {
  return row == NULL ? 0 : row->cell_count;
}

/**
 * Get the codepoint stored in a cell.
 * @param cell Pointer to the cell.
 * @return Unicode codepoint.
 */
uint32_t SpriteCellCodepoint(const SpriteCell* cell) {
  return cell == NULL ? 0 : cell->codepoint;
}

/**
 * Get the terminal display width of a cell.
 * @param cell Pointer to the cell.
 * @return Number of terminal columns (0 for combining marks).
 */
int SpriteCellWidth(const SpriteCell* cell) {
  return cell == NULL ? 0 : cell->width;
}

/**
 * Get the foreground color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR when not declared.
 */
int SpriteCellForeground(const SpriteCell* cell) {
  if (cell == NULL || (cell->flags & SPRITE_CELL_FLAG_FOREGROUND) == 0)
    return SPRITE_NO_COLOR;
  return cell->foreground;
}

/**
 * Get the background color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR when not declared.
 */
int SpriteCellBackground(const SpriteCell* cell) {
  if (cell == NULL || (cell->flags & SPRITE_CELL_FLAG_BACKGROUND) == 0)
    return SPRITE_NO_COLOR;
  return cell->background;
}

/**
 * Check whether a cell is a transparent blank terminal cell.
 * @param cell Pointer to the cell.
 * @return true when the renderer advances the column without writing.
 */
bool SpriteCellIsBlank(const SpriteCell* cell) {
  return cell != NULL && (cell->flags & SPRITE_CELL_FLAG_BLANK) != 0;
}
