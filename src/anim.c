#include "anim.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tomato.h"

/* Increase animations frames in real life seconds */
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

/* Handle UTF-8 character in line */
int HandleUnicodeCharacter(const char* src, char** dest) {
  char utf_char[5];
  strncpy(utf_char, src + 2, 4);
  utf_char[4] = '\0';
  int unicode_value = (int)strtol(utf_char, NULL, 16);
  int utf8_length = 0;
  if (unicode_value < 0x80) {
    *((*dest)++) = (char)unicode_value;
    utf8_length = 1;
  } else if (unicode_value < 0x800) {
    *((*dest)++) = (char)((unicode_value >> 6) | 0xC0);
    *((*dest)++) = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 2;
  } else if (unicode_value < 0x10000) {
    *((*dest)++) = (char)((unicode_value >> 12) | 0xE0);
    *((*dest)++) = (char)(((unicode_value >> 6) & 0x3F) | 0x80);
    *((*dest)++) = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 3;
  } else {
    *((*dest)++) = (char)((unicode_value >> 18) | 0xF0);
    *((*dest)++) = (char)(((unicode_value >> 12) & 0x3F) | 0x80);
    *((*dest)++) = (char)(((unicode_value >> 6) & 0x3F) | 0x80);
    *((*dest)++) = (char)((unicode_value & 0x3F) | 0x80);
    utf8_length = 4;
  }
  /* Return the number of characters processed */
  return (utf8_length + 3);
}

/* Handle color character in line */
int HandleCharacterColor(const char* src, int** dest) {
  int color_code = src[2] - '0';
  if (color_code < 0 || color_code > MAX_COLOR_PAIRS)
    return 3;  // Invalid color code
  // Store color code in destination
  **dest = color_code;  // Assigning color code to the correct pointer
  (*dest)++;            // Move the pointer to the next location
  // Return the number of characters to jump from file
  return 3;
}

/* Escape special characters in a line */
void EscapeLine(const char* line, char* sprite_line, int* color_line) {
  char* src = (char*)line;
  char* sprite_dest = sprite_line;
  int* color_dest = color_line;

  while (*src != '\0') {
    if (*src == '\\' && *(src + 1) == 'u') {
      src += HandleUnicodeCharacter(src, &sprite_dest);
    } else if (*src == '\\' && *(src + 1) == 'c') {
      src += HandleCharacterColor(
        src, &color_dest);  // Pass the address of color_dest
    } else {
      *sprite_dest++ = *src++;
      *color_dest++ = NO_COLOR;
    }
  }
  *sprite_dest = '\0';
}

/* Copy escaped line to sprite frame */
void CopyLineToFrame(Sprite* sprite, const char* sprite_line,
                     const int* color_line, int* lines_read) {
  sprite->frames[*lines_read] =
    (char*)malloc((strlen(sprite_line) + 1) * sizeof(char));
  if (sprite->frames[*lines_read] == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(1);
  }

  strcpy(sprite->frames[*lines_read], sprite_line);
  for (int i = 0; i < strlen(sprite_line); i++)
    sprite->color_map[*lines_read][i - 1] = color_line[i];
  sprite->color_map[*lines_read][strlen(sprite_line)] = NO_COLOR - 1;
  (*lines_read)++;
}

/* Deserialize a single line of sprite */
void DeserializeSpriteLine(const char* line, Sprite* sprite, int* lines_read) {
  char* sprite_line = (char*)malloc((strlen(line) + 1) * sizeof(char));
  int color_line[MAX_FRAME_COLUMNS];
  if (sprite_line == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(1);
  }

  EscapeLine(line, sprite_line, color_line);
  CopyLineToFrame(sprite, sprite_line, color_line, lines_read);

  free(sprite_line);
}

/* Deserialize sprites from file to Sprite struct */
void DeserializeSprite(const char* filename, Sprite* sprite) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    exit(1);
  }

  int read = 0;
  char line[MAX_FRAME_COLUMNS];
  int lines_read = 0;

  while (fgets(line, sizeof(line), file) != NULL) {
    if (strstr(line, ICONS) != NULL) {
      read = 1;
      sscanf(line, "%*[^/]/%d/%d", &sprite->frame_height, &sprite->frame_count);
      if (fgets(line, sizeof(line), file) == NULL) break;
    }
    if (strstr(line, SEPARATOR) != NULL && read == 1) {
      read = 0;
      break;
    }
    if (read == 1) {
      DeserializeSpriteLine(line, sprite, &lines_read);
    }
  }

  fclose(file);
}

/* Break a string using color_map as tokens */
void BreakStringByColorMap(const char* input_string, const int* color_map,
                           size_t length, char*** output_strings,
                           int* num_strings) {
  size_t start = 0, end = 0;
  *num_strings = 0;

  /* Count the number of substrings */
  int i = 0;
  while (color_map[i] != -2) {
    if (color_map[i] != NO_COLOR || i == length - 1) {
      end = i;
      if (start <= end) (*num_strings)++;
      start = i + 1;
    }
    i++;
  }

  /* Allocate memory for output strings */
  *output_strings = (char**)malloc((*num_strings + 1) * sizeof(char*));
  if (*output_strings == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(1);
  }

  start = 0;
  size_t str_index = 0;
  /* Extract substrings */
  for (size_t i = 0; i < length; i++) {
    if (color_map[i] != NO_COLOR) {
      end = i;
      size_t substr_length = end - start + 1;
      (*output_strings)[str_index] =
        (char*)malloc((substr_length + 1) * sizeof(char));
      if ((*output_strings)[str_index] == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
      }
      strncpy((*output_strings)[str_index], input_string + start,
              substr_length);
      (*output_strings)[str_index][substr_length] =
        '\0';  // Null-terminate the string
      start = i + 1;
      str_index += 1;
    }
  }
}

/* Returns the length of a array of strings */
int LengthStringArray(char** string, int from, int to) {
  int length = 0;

  if (from < to)
    for (int i = from; i <= to; i++) length += strlen(string[i]);
  else
    for (int i = to; i <= from; i++) length += strlen(string[i]);

  return length;
}
