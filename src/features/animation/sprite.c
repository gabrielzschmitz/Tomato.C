/**
 * @file sprite.c
 * @brief Section-format sprite parser and immutable sprite queries.
 */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "features/animation/sprite.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "core/error.h"

/** Size of the formatted diagnostic context buffer. */
#define SPRITE_DIAGNOSTIC_SIZE 512

/** Largest accepted frame duration in milliseconds. */
#define SPRITE_MAX_DURATION_MS 1.0e12

/** Private state while one frame is parsed. */
typedef struct {
  size_t first_row;     /**< First row index owned by this frame. */
  size_t first_cell;    /**< First cell index owned by this frame. */
  int foreground;       /**< Current foreground color. */
  int background;       /**< Current background color. */
  bool duration_set;    /**< True when a frame duration override was read. */
  double duration_ms;   /**< Frame duration override in milliseconds. */
  bool content_started; /**< True after [[content]]. */
  bool content_closed;  /**< True after [[/content]]. */
} spriteFrameState;

/** Private state while one animation definition is parsed. */
typedef struct {
  char* name;                 /**< Animation name. */
  char* source;               /**< Source filename for diagnostics. */
  size_t source_line;         /**< Declaration line for diagnostics. */
  unsigned icon_mask;         /**< Bit mask of declared icon types. */
  bool icon_set;              /**< True when icon-type was read. */
  bool duration_set;          /**< True when default-frame-duration was read. */
  bool loop_set;              /**< True when loop was read. */
  bool color_set;             /**< True when default-color was read. */
  double duration_ms;         /**< Default frame duration. */
  bool loop;                  /**< Playback loop setting. */
  int default_color;          /**< Foreground color at frame start. */
  struct SpriteFrame* frames; /**< Parsed frames. */
  size_t frame_count;         /**< Number of frames. */
  size_t frame_capacity;      /**< Capacity of frames. */
  struct SpriteRow* rows;     /**< Parsed rows. */
  size_t row_count;           /**< Number of rows. */
  size_t row_capacity;        /**< Capacity of rows. */
  struct SpriteCell* cells;   /**< Decoded cells. */
  size_t cell_count;          /**< Number of cells. */
  size_t cell_capacity;       /**< Capacity of cells. */
  spriteFrameState frame;     /**< State of the open frame. */
  bool frame_open;            /**< True between [[frame]] and [[/frame]]. */
} spritePendingAnimation;

/** Private list of completed definitions from one file. */
typedef struct {
  struct SpriteAnimation** items; /**< Parsed definitions. */
  size_t count;                   /**< Number of definitions. */
  size_t capacity;                /**< Capacity of items. */
} spriteDefinitionList;

/* PRIVATE SPRITE FUNCTIONS */
static bool reserveArray(void** data, size_t* capacity, size_t needed,
                         size_t element_size);
static char* duplicateStringRange(const char* start, size_t length);
static ErrorType reportDiagnostic(ErrorType code, ErrorType* error,
                                  const char* filename, size_t line,
                                  const char* format, ...);
static char* readLine(FILE* file);

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
static bool parseUnsignedRange(const char* value, size_t length, int maximum,
                               int* result);
static bool parseDurationRange(const char* value, size_t length,
                               double* result_ms);
static bool parseIconTypes(const char* value, size_t length, unsigned* mask);
static bool parseBoolRange(const char* value, size_t length, bool* result);

static bool appendCell(struct SpriteCell** cells, size_t* count,
                       size_t* capacity, uint32_t codepoint, int width,
                       int foreground, int background, bool blank);
static bool decodeUtf8(const unsigned char* input, size_t available,
                       uint32_t* codepoint, size_t* length);
static bool parseColorEscape(const char* input, const char** end,
                             int* foreground, int* background);
static bool parseUnicodeEscape(const char* input, const char** end,
                               uint32_t* codepoint);
static bool parseBlankRunEscape(const char* input, const char** end,
                                int* count);
static bool decodeContentLine(const char* line, spritePendingAnimation* pending,
                              const char* filename, size_t line_number,
                              ErrorType* error);

static void clearPendingFrame(spritePendingAnimation* pending);
static void clearPendingAnimation(spritePendingAnimation* pending);
static bool startFrame(spritePendingAnimation* pending, const char* filename,
                       size_t line_number, ErrorType* error);
static bool closeContent(spritePendingAnimation* pending, const char* filename,
                         size_t line_number, ErrorType* error);
static bool closeFrame(spritePendingAnimation* pending, const char* filename,
                       size_t line_number, ErrorType* error);
static bool finalizeAnimation(spritePendingAnimation* pending,
                              spriteDefinitionList* definitions,
                              const char* filename, size_t line_number,
                              ErrorType* error);
static void freeSpriteAnimation(struct SpriteAnimation* animation);
static void freeDefinitionList(spriteDefinitionList* definitions);
static bool appendDefinition(spriteDefinitionList* definitions,
                             struct SpriteAnimation* animation);
static bool mergeDefinitions(SpriteLibrary* library,
                             spriteDefinitionList* definitions,
                             ErrorType* error);
static bool parseSpriteFile(const char* filename, SpriteLibrary* library,
                            ErrorType* error);

/**
 * Grow an array with overflow checks.
 * @param data Pointer to the array pointer.
 * @param capacity Pointer to the stored capacity.
 * @param needed Minimum number of elements required.
 * @param element_size Size of one element.
 * @return true when the array has enough capacity.
 */
static bool reserveArray(void** data, size_t* capacity, size_t needed,
                         size_t element_size) {
  if (needed <= *capacity) return true;
  if (element_size == 0 || needed > SIZE_MAX / element_size) return false;

  size_t new_capacity = *capacity == 0 ? 8 : *capacity;
  while (new_capacity < needed) {
    if (new_capacity > SIZE_MAX / 2) {
      new_capacity = needed;
      break;
    }
    new_capacity *= 2;
  }

  void* resized = realloc(*data, new_capacity * element_size);
  if (resized == NULL) return false;
  *data = resized;
  *capacity = new_capacity;
  return true;
}

/**
 * Duplicate a string range.
 * @param start First character to copy.
 * @param length Number of characters to copy.
 * @return Heap copy, or NULL on allocation failure.
 */
static char* duplicateStringRange(const char* start, size_t length) {
  char* copy = (char*)malloc(length + 1);
  if (copy == NULL) return NULL;
  memcpy(copy, start, length);
  copy[length] = '\0';
  return copy;
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
                                  const char* filename, size_t line,
                                  const char* format, ...) {
  char context[SPRITE_DIAGNOSTIC_SIZE];
  int prefix = snprintf(context, sizeof(context), "%s:%zu: ", filename, line);
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
 * Read one line without a fixed buffer.
 * @param file Open file.
 * @return Heap line without newline, or NULL at EOF.
 */
static char* readLine(FILE* file) {
  size_t capacity = 128;
  size_t length = 0;
  char* line = (char*)malloc(capacity);
  if (line == NULL) return NULL;

  int character;
  while ((character = fgetc(file)) != EOF) {
    if (character == '\n') break;
    if (length + 1 >= capacity) {
      if (!reserveArray((void**)&line, &capacity, length + 2, sizeof(char))) {
        free(line);
        return NULL;
      }
    }
    line[length++] = (char)character;
  }

  if (character == EOF && length == 0) {
    free(line);
    return NULL;
  }
  if (length > 0 && line[length - 1] == '\r') length--;
  line[length] = '\0';
  return line;
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
static bool parseUnsignedRange(const char* value, size_t length, int maximum,
                               int* result) {
  if (length == 0) return false;
  int parsed = 0;
  for (size_t i = 0; i < length; i++) {
    if (!isdigit((unsigned char)value[i])) return false;
    int digit = value[i] - '0';
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

  char* number = duplicateStringRange(inner, number_length);
  if (number == NULL) return false;

  errno = 0;
  char* end = NULL;
  double parsed = strtod(number, &end);
  bool valid = end != number && *end == '\0' && errno != ERANGE &&
               isfinite(parsed) && parsed > 0.0 &&
               parsed <= SPRITE_MAX_DURATION_MS / multiplier;
  free(number);
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
static bool parseIconTypes(const char* value, size_t length, unsigned* mask) {
  static const char* const names[SPRITE_ICON_TYPE_COUNT] = {"nerd-icons",
                                                            "emojis", "ascii"};
  const char* p = value;
  const char* end = value + length;
  unsigned parsed_mask = 0;
  int count = 0;
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

    int index = -1;
    for (int i = 0; i < SPRITE_ICON_TYPE_COUNT; i++)
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

  *mask = parsed_mask;
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
 * Append one decoded cell to the animation cell array.
 * @param cells Cell array.
 * @param count Cell count.
 * @param capacity Cell capacity.
 * @param codepoint Unicode codepoint.
 * @param width Terminal width override, or -1 to use wcwidth.
 * @param foreground Foreground color.
 * @param background Background color.
 * @param blank True for a blank terminal cell.
 * @return true when appended.
 */
static bool appendCell(struct SpriteCell** cells, size_t* count,
                       size_t* capacity, uint32_t codepoint, int width,
                       int foreground, int background, bool blank) {
  if (!reserveArray((void**)cells, capacity, *count + 1,
                    sizeof(struct SpriteCell)))
    return false;

  int display_width = width;
  if (display_width < 0) {
    display_width = wcwidth((wchar_t)codepoint);
    if (display_width < 0) display_width = 1;
  }

  struct SpriteCell* cell = &(*cells)[(*count)++];
  cell->codepoint = codepoint;
  cell->width = display_width;
  cell->foreground = foreground;
  cell->background = background;
  cell->blank = blank;
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
 * @param background Output: background color or SPRITE_NO_COLOR.
 * @return true for a valid color escape.
 */
static bool parseColorEscape(const char* input, const char** end,
                             int* foreground, int* background) {
  const char* p = input + 2;
  int parsed_foreground;
  int parsed_background = SPRITE_NO_COLOR;

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

  *foreground = parsed_foreground;
  *background = parsed_background;
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
  size_t digits = 0;
  while (*p != '\0' && *p != '}') {
    unsigned char character = (unsigned char)*p;
    int digit;
    if (character >= '0' && character <= '9')
      digit = character - '0';
    else if (character >= 'a' && character <= 'f')
      digit = character - 'a' + 10;
    else if (character >= 'A' && character <= 'F')
      digit = character - 'A' + 10;
    else
      return false;
    if (++digits > 6) return false;
    value = (value << 4) | (uint32_t)digit;
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
                                int* count) {
  const char* p = input + 2;
  const char* digit_start = p;
  while (isdigit((unsigned char)*p)) p++;
  if (p == digit_start || *p != '}') return false;
  if (!parseUnsignedRange(digit_start, (size_t)(p - digit_start),
                          SPRITE_MAX_BLANK_RUN, count) ||
      *count <= 0)
    return false;
  *end = p + 1;
  return true;
}

/**
 * Decode one raw content line into cells and append one row.
 * @param line Raw content line.
 * @param pending Pending animation state.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when decoded.
 */
static bool decodeContentLine(const char* line, spritePendingAnimation* pending,
                              const char* filename, size_t line_number,
                              ErrorType* error) {
  size_t first_cell = pending->cell_count;
  const char* p = line;

  while (*p != '\0') {
    if (*p == '\\' && p[1] == 'c') {
      const char* after;
      if (!parseColorEscape(p, &after, &pending->frame.foreground,
                            &pending->frame.background)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "malformed color escape");
        return false;
      }
      p = after;
      continue;
    }
    if (*p == '\\' && p[1] == 'u') {
      const char* after;
      uint32_t codepoint;
      if (!parseUnicodeEscape(p, &after, &codepoint)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "invalid Unicode escape");
        return false;
      }
      if (!appendCell(&pending->cells, &pending->cell_count,
                      &pending->cell_capacity, codepoint, -1,
                      pending->frame.foreground, pending->frame.background,
                      false)) {
        reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                         "allocation failure");
        return false;
      }
      p = after;
      continue;
    }
    if (*p == '\\' && p[1] == '{') {
      const char* after;
      int count;
      if (!parseBlankRunEscape(p, &after, &count)) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "invalid blank-cell count");
        return false;
      }
      for (int i = 0; i < count; i++)
        if (!appendCell(&pending->cells, &pending->cell_count,
                        &pending->cell_capacity, ' ', 1,
                        pending->frame.foreground, pending->frame.background,
                        true)) {
          reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                           "allocation failure");
          return false;
        }
      p = after;
      continue;
    }
    if (*p == '\\' && p[1] == 's') {
      if (!appendCell(
            &pending->cells, &pending->cell_count, &pending->cell_capacity, ' ',
            1, pending->frame.foreground, pending->frame.background, true)) {
        reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                         "allocation failure");
        return false;
      }
      p += 2;
      continue;
    }

    uint32_t codepoint;
    size_t consumed;
    if (!decodeUtf8((const unsigned char*)p, strlen(p), &codepoint,
                    &consumed)) {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number, "invalid UTF-8 sequence");
      return false;
    }
    if (codepoint == 0) {
      reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                       line_number, "NUL in sprite content");
      return false;
    }
    if (!appendCell(&pending->cells, &pending->cell_count,
                    &pending->cell_capacity, codepoint, -1,
                    pending->frame.foreground, pending->frame.background,
                    false)) {
      reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                       "allocation failure");
      return false;
    }
    p += consumed;
  }

  if (!reserveArray((void**)&pending->rows, &pending->row_capacity,
                    pending->row_count + 1, sizeof(struct SpriteRow))) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }
  struct SpriteRow* row = &pending->rows[pending->row_count++];
  row->cell_offset = first_cell;
  row->cell_count = pending->cell_count - first_cell;
  return true;
}

/**
 * Reset state for a new frame.
 * @param pending Pending animation state.
 */
static void clearPendingFrame(spritePendingAnimation* pending) {
  memset(&pending->frame, 0, sizeof(pending->frame));
  pending->frame.foreground = pending->default_color;
  pending->frame.background = SPRITE_NO_COLOR;
  pending->frame.duration_ms = pending->duration_ms;
  pending->frame_open = false;
}

/**
 * Release all state owned by a pending animation.
 * @param pending Pending animation state.
 */
static void clearPendingAnimation(spritePendingAnimation* pending) {
  free(pending->name);
  free(pending->source);
  free(pending->frames);
  free(pending->rows);
  free(pending->cells);
  memset(pending, 0, sizeof(*pending));
  pending->default_color = SPRITE_NO_COLOR;
  clearPendingFrame(pending);
}

/**
 * Open a new frame declaration.
 * @param pending Pending animation state.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when opened.
 */
static bool startFrame(spritePendingAnimation* pending, const char* filename,
                       size_t line_number, ErrorType* error) {
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
 * @param pending Pending animation state.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when closed.
 */
static bool closeContent(spritePendingAnimation* pending, const char* filename,
                         size_t line_number, ErrorType* error) {
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
 * Close a frame and append its metadata to the animation.
 * @param pending Pending animation state.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when closed.
 */
static bool closeFrame(spritePendingAnimation* pending, const char* filename,
                       size_t line_number, ErrorType* error) {
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

  size_t row_count = pending->row_count - pending->frame.first_row;
  if (row_count == 0) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "frame has no content rows");
    return false;
  }
  if (row_count > INT_MAX) {
    reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                     line_number, "frame height overflow");
    return false;
  }

  int width = 0;
  for (size_t i = pending->frame.first_row; i < pending->row_count; i++) {
    int row_width = 0;
    const struct SpriteRow* row = &pending->rows[i];
    for (size_t j = 0; j < row->cell_count; j++) {
      if (row_width > INT_MAX - pending->cells[row->cell_offset + j].width) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                         line_number, "frame width overflow");
        return false;
      }
      row_width += pending->cells[row->cell_offset + j].width;
    }
    if (row_width > width) width = row_width;
  }

  if (!reserveArray((void**)&pending->frames, &pending->frame_capacity,
                    pending->frame_count + 1, sizeof(struct SpriteFrame))) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }
  struct SpriteFrame* frame = &pending->frames[pending->frame_count++];
  frame->row_offset = pending->frame.first_row;
  frame->row_count = row_count;
  frame->cell_offset = pending->frame.first_cell;
  frame->cell_count = pending->cell_count - pending->frame.first_cell;
  frame->width = width;
  frame->height = (int)row_count;
  frame->duration_ms = pending->frame.duration_set ? pending->frame.duration_ms
                                                   : pending->duration_ms;

  clearPendingFrame(pending);
  return true;
}

/**
 * Finish one animation declaration and move it into the definition list.
 * @param pending Pending animation state.
 * @param definitions Completed definitions.
 * @param filename Source filename.
 * @param line_number Source line number.
 * @param error Optional diagnostic output.
 * @return true when finalized.
 */
static bool finalizeAnimation(spritePendingAnimation* pending,
                              spriteDefinitionList* definitions,
                              const char* filename, size_t line_number,
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

  struct SpriteAnimation* animation =
    (struct SpriteAnimation*)calloc(1, sizeof(*animation));
  if (animation == NULL) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    return false;
  }

  animation->name = pending->name;
  animation->source = pending->source;
  animation->source_line = pending->source_line;
  animation->icon_mask = pending->icon_mask;
  animation->frames = pending->frames;
  animation->frame_count = pending->frame_count;
  animation->rows = pending->rows;
  animation->row_count = pending->row_count;
  animation->cells = pending->cells;
  animation->cell_count = pending->cell_count;
  animation->default_duration_ms = pending->duration_ms;
  animation->loop = pending->loop;
  animation->default_color = pending->default_color;

  for (size_t i = 0; i < animation->row_count; i++)
    animation->rows[i].cells = animation->cells;
  for (size_t i = 0; i < animation->frame_count; i++)
    animation->frames[i].rows =
      animation->rows + animation->frames[i].row_offset;

  for (size_t i = 0; i < animation->frame_count; i++) {
    const struct SpriteFrame* frame = &animation->frames[i];
    if (frame->width > animation->width) animation->width = frame->width;
    if (frame->height > animation->height) animation->height = frame->height;
  }

  memset(pending, 0, sizeof(*pending));
  pending->default_color = SPRITE_NO_COLOR;
  clearPendingFrame(pending);

  if (!appendDefinition(definitions, animation)) {
    reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                     "allocation failure");
    freeSpriteAnimation(animation);
    return false;
  }
  return true;
}

/**
 * Free one animation definition.
 * @param animation Pointer to the animation to free.
 */
static void freeSpriteAnimation(struct SpriteAnimation* animation) {
  if (animation == NULL) return;
  free(animation->name);
  free(animation->source);
  free(animation->frames);
  free(animation->rows);
  free(animation->cells);
  free(animation);
}

/**
 * Free a definition list.
 * @param definitions Pointer to the list to free.
 */
static void freeDefinitionList(spriteDefinitionList* definitions) {
  if (definitions == NULL) return;
  for (size_t i = 0; i < definitions->count; i++)
    freeSpriteAnimation(definitions->items[i]);
  free(definitions->items);
  memset(definitions, 0, sizeof(*definitions));
}

/**
 * Append a definition to a list.
 * @param definitions Target list.
 * @param animation Owned animation definition.
 * @return true when appended.
 */
static bool appendDefinition(spriteDefinitionList* definitions,
                             struct SpriteAnimation* animation) {
  if (!reserveArray((void**)&definitions->items, &definitions->capacity,
                    definitions->count + 1, sizeof(struct SpriteAnimation*)))
    return false;
  definitions->items[definitions->count++] = animation;
  return true;
}

/**
 * Merge new definitions into a library and validate icon-type masks.
 * @param library Target library.
 * @param definitions Definitions parsed from one file.
 * @param error Optional diagnostic output.
 * @return true when every definition was accepted.
 */
static bool mergeDefinitions(SpriteLibrary* library,
                             spriteDefinitionList* definitions,
                             ErrorType* error) {
  for (size_t i = 0; i < definitions->count; i++) {
    struct SpriteAnimation* candidate = definitions->items[i];
    for (size_t j = 0; j < library->count; j++) {
      struct SpriteAnimation* existing = library->items[j];
      if (strcmp(existing->name, candidate->name) == 0 &&
          (existing->icon_mask & candidate->icon_mask) != 0) {
        reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error,
                         candidate->source, candidate->source_line,
                         "conflicting icon types for animation '%s'",
                         candidate->name);
        return false;
      }
    }
    if (!reserveArray((void**)&library->items, &library->capacity,
                      library->count + 1, sizeof(struct SpriteAnimation*))) {
      reportDiagnostic(MALLOC_ERROR, error, candidate->source,
                       candidate->source_line, "allocation failure");
      return false;
    }
    library->items[library->count++] = candidate;
    definitions->items[i] = NULL;
  }
  return true;
}

/**
 * Parse one sprite file into temporary definitions, then merge them.
 * @param filename Path to the file.
 * @param library Target library.
 * @param error Optional diagnostic output.
 * @return true on success.
 */
static bool parseSpriteFile(const char* filename, SpriteLibrary* library,
                            ErrorType* error) {
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    reportDiagnostic(FILE_ERROR, error, filename, 0, "cannot open sprite file");
    return false;
  }

  spritePendingAnimation pending;
  spriteDefinitionList definitions;
  memset(&pending, 0, sizeof(pending));
  memset(&definitions, 0, sizeof(definitions));
  pending.default_color = SPRITE_NO_COLOR;
  clearPendingFrame(&pending);

  /** Parser state for the current input line. */
  enum {
    STATE_OUTSIDE_ANIMATION, /**< Outside any animation block. */
    STATE_IN_ANIMATION,      /**< Inside an animation block. */
    STATE_IN_FRAME,          /**< Inside a frame block. */
    STATE_IN_CONTENT         /**< Inside a content block. */
  } state = STATE_OUTSIDE_ANIMATION;

  size_t line_number = 0;
  char* line;
  bool success = true;

  while (success && (line = readLine(file)) != NULL) {
    line_number++;
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
      free(line);
      continue;
    }

    if (start == end || *start == '#') {
      free(line);
      continue;
    }

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
          pending.name = duplicateStringRange(name, name_length);
          pending.source = duplicateStringRange(filename, strlen(filename));
          pending.source_line = line_number;
          if (pending.name == NULL || pending.source == NULL) {
            reportDiagnostic(MALLOC_ERROR, error, filename, line_number,
                             "allocation failure");
            success = false;
          } else {
            pending.duration_ms = 1000.0;
            pending.loop = true;
            pending.default_color = SPRITE_NO_COLOR;
            state = STATE_IN_ANIMATION;
          }
        }
      }
      free(line);
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
          success = parseUnsignedRange(value, value_length, SPRITE_MAX_COLOR,
                                       &pending.default_color);
          if (!success) {
            reportDiagnostic(ANIMATION_DESERIALIZATION_ERROR, error, filename,
                             line_number, "invalid default-color option");
          } else {
            pending.color_set = true;
            pending.frame.foreground = pending.default_color;
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

    free(line);
  }

  if (success && ferror(file)) {
    reportDiagnostic(FILE_ERROR, error, filename, line_number,
                     "error reading sprite file");
    success = false;
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

  clearPendingAnimation(&pending);
  freeDefinitionList(&definitions);
  return success;
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

  SpriteLibrary* loaded = (SpriteLibrary*)calloc(1, sizeof(*loaded));
  if (loaded == NULL)
    return reportDiagnostic(MALLOC_ERROR, &error, "<library>", 0,
                            "allocation failure");

  for (size_t i = 0; i < count; i++) {
    if (filenames[i] == NULL) {
      FreeSpriteLibrary(loaded);
      return reportDiagnostic(INVALID_ARGUMENT_ERROR, &error, "<library>", 0,
                              "NULL sprite filename");
    }
    if (!parseSpriteFile(filenames[i], loaded, &error)) {
      FreeSpriteLibrary(loaded);
      return error == NO_ERROR ? ANIMATION_DESERIALIZATION_ERROR : error;
    }
  }

  *library = loaded;
  return NO_ERROR;
}

/**
 * Free a sprite library and all animation data it owns.
 * Every Rollfilm created from the library must be freed first.
 * @param library Pointer to the library to free.
 */
void FreeSpriteLibrary(SpriteLibrary* library) {
  if (library == NULL) return;
  for (size_t i = 0; i < library->count; i++)
    freeSpriteAnimation(library->items[i]);
  free(library->items);
  free(library);
}

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
  unsigned bit = 1u << icon_type;
  for (size_t i = 0; i < library->count; i++) {
    const struct SpriteAnimation* animation = library->items[i];
    if ((animation->icon_mask & bit) != 0 && strcmp(animation->name, name) == 0)
      return animation;
  }
  return NULL;
}

/**
 * Get an animation name.
 * @param animation Pointer to the animation definition.
 * @return Name string owned by the library.
 */
const char* SpriteAnimationName(const SpriteAnimation* animation) {
  return animation == NULL ? NULL : animation->name;
}

/**
 * Get an animation frame.
 * @param animation Pointer to the animation definition.
 * @param index Frame index.
 * @return Immutable frame, or NULL when index is out of range.
 */
const SpriteFrame* SpriteAnimationFrame(const SpriteAnimation* animation,
                                        size_t index) {
  if (animation == NULL || index >= animation->frame_count) return NULL;
  return &animation->frames[index];
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
 * Get the configured loop flag.
 * @param animation Pointer to the animation definition.
 * @return true when playback repeats.
 */
bool SpriteAnimationLoop(const SpriteAnimation* animation) {
  return animation != NULL && animation->loop;
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
 * Get the cached frame height in rows.
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
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteCellForeground(const SpriteCell* cell) {
  return cell == NULL ? SPRITE_NO_COLOR : cell->foreground;
}

/**
 * Get the background color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteCellBackground(const SpriteCell* cell) {
  return cell == NULL ? SPRITE_NO_COLOR : cell->background;
}

/**
 * Check whether a cell is a blank terminal cell.
 * @param cell Pointer to the cell.
 * @return true when the cell must be drawn as a space.
 */
bool SpriteCellIsBlank(const SpriteCell* cell) {
  return cell != NULL && cell->blank;
}
