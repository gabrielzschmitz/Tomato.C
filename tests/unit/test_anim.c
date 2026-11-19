/**
 * @file test_anim.c
 * @brief Unit tests for sprite parsing and rollfilm playback.
 *
 * Covers the section/option/content format, escape decoding, diagnostics,
 * cached dimensions, playback timing, seeking, blank queries, rendering
 * geometry, and color state. External ncurses/time/error calls are stubbed
 * so playback and rendering are deterministic.
 */

#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/error.h"
#include "core/util.h"
#include "features/animation/anim.h"
#include "features/animation/sprite.h"
#include "test_helpers.h"

#define MAX_RENDER_CALLS 128
#define RENDER_TEXT_SIZE 64

/** Captured ncurses write produced by RenderRollfilm. */
typedef struct {
  int y;                       /**< Terminal row. */
  int x;                       /**< Terminal column. */
  char text[RENDER_TEXT_SIZE]; /**< Printed UTF-8 text. */
  short foreground;            /**< Foreground color set for the write. */
  short background;            /**< Background color set for the write. */
  chtype attribute;            /**< Attribute set for the write. */
} RenderCall;

static double g_now_ms;                             /**< Fake wall clock. */
static RenderCall g_render_calls[MAX_RENDER_CALLS]; /**< Captured writes. */
static int g_render_count;        /**< Number of captured writes. */
static short g_active_foreground; /**< Last SetColor foreground. */
static short g_active_background; /**< Last SetColor background. */
static chtype g_active_attribute; /**< Last SetColor attribute. */
static char g_log_context[512];   /**< Last logged diagnostic context. */
static ErrorType g_log_error;     /**< Last logged error type. */

/** @brief Stub wall clock used by playback tests. */
double GetCurrentTimeMS(void) { return g_now_ms; }

/** @brief Capture error diagnostics from sprite parsing. */
void LogError(const char* context, ErrorType error) {
  snprintf(g_log_context, sizeof(g_log_context), "%s",
           context == NULL ? "" : context);
  g_log_error = error;
}

/** @brief Capture color changes from RenderRollfilm. */
void SetColor(short int fg, short int bg, chtype attr) {
  g_active_foreground = fg;
  g_active_background = bg;
  g_active_attribute = attr;
}

/** @brief Capture text writes from RenderRollfilm. */
int mvprintw(int y, int x, const char* format, ...) {
  va_list args;
  va_start(args, format);
  if (g_render_count < MAX_RENDER_CALLS) {
    RenderCall* call = &g_render_calls[g_render_count++];
    call->y = y;
    call->x = x;
    call->foreground = g_active_foreground;
    call->background = g_active_background;
    call->attribute = g_active_attribute;
    vsnprintf(call->text, sizeof(call->text), format, args);
  }
  va_end(args);
  return 0;
}

/** @brief Assert two floating point values are equal within a tolerance. */
#define ASSERT_DBL_NEAR(a, b)                                                  \
  do {                                                                         \
    g_test_assertions++;                                                       \
    double _a = (double)(a);                                                   \
    double _b = (double)(b);                                                   \
    if (fabs(_a - _b) > 0.0001) {                                              \
      if (!g_test_failed_flag) {                                               \
        printf("\n    %sFAIL: %s:%d: ASSERT_DBL_NEAR(%s, %s)%s%s\n",           \
               _cc_red(), __FILE__, __LINE__, #a, #b, _cc_red(), _cc_reset()); \
        printf("      expected %f, got %f\n", _b, _a);                         \
        g_test_failed_flag = 1;                                                \
      }                                                                        \
    }                                                                          \
  } while (0)

/**
 * ---------------------------------------------------------------------------
 * Test file helpers
 * ---------------------------------------------------------------------------
 */

/**
 * Create a temporary file path.
 * @return Heap path, or NULL when no template can be used.
 */
static char* makeTemporaryPath(void) {
  static const char* const templates[] = {"/tmp/opencode/tomato_sprite_XXXXXX",
                                          "/tmp/tomato_sprite_XXXXXX"};
  for (size_t i = 0; i < sizeof(templates) / sizeof(templates[0]); i++) {
    char* path = strdup(templates[i]);
    if (path == NULL) return NULL;
    int fd = mkstemp(path);
    if (fd >= 0) {
      close(fd);
      return path;
    }
    free(path);
  }
  return NULL;
}

/**
 * Write sprite text to a temporary file.
 * @param text Sprite file contents.
 * @return Heap path, or NULL on failure.
 */
static char* writeTemporarySprite(const char* text) {
  char* path = makeTemporaryPath();
  if (path == NULL) return NULL;
  FILE* file = fopen(path, "wb");
  if (file == NULL) {
    free(path);
    return NULL;
  }
  fputs(text, file);
  fclose(file);
  return path;
}

/**
 * Load one sprite library from in-memory text.
 * @param text Sprite file contents.
 * @param path_out Optional output receiving the temporary path.
 * @param error Optional output receiving the load status.
 * @return Loaded library, or NULL on failure.
 */
static SpriteLibrary* loadSpriteText(const char* text, char** path_out,
                                     ErrorType* error) {
  char* path = writeTemporarySprite(text);
  if (path == NULL) return NULL;
  const char* files[] = {path};
  SpriteLibrary* library = NULL;
  ErrorType status = LoadSpriteLibrary(files, 1, &library);
  if (error != NULL) *error = status;
  if (path_out != NULL) {
    *path_out = path;
  } else {
    unlink(path);
    free(path);
  }
  return library;
}

/**
 * Remove and free a temporary sprite path.
 * @param path Path returned by loadSpriteText.
 */
static void removeTemporarySprite(char* path) {
  if (path == NULL) return;
  unlink(path);
  free(path);
}

/**
 * Assert that the last logged diagnostic contains a fragment.
 * @param fragment Expected substring.
 */
static void assertLogContains(const char* fragment) {
  ASSERT_TRUE(g_log_error != NO_ERROR);
  ASSERT_TRUE(fragment != NULL && strstr(g_log_context, fragment) != NULL);
}

/**
 * Create a one-frame animation definition string.
 * @param name Animation name.
 * @param icons Icon-type option text.
 * @param content Content lines.
 * @return Static buffer holding the sprite file text.
 */
static const char* simpleSprite(const char* name, const char* icons,
                                const char* content) {
  static char buffer[2048];
  snprintf(buffer, sizeof(buffer),
           "[animation.%s]\n"
           "%s\n"
           "default-frame-duration = \"10ms\"\n"
           "loop = true\n"
           "default-color = 7\n"
           "\n"
           "[[frame]]\n"
           "[[content]]\n"
           "%s\n"
           "[[/content]]\n"
           "[[/frame]]\n",
           name, icons, content);
  return buffer;
}

/**
 * ---------------------------------------------------------------------------
 * Parsing tests
 * ---------------------------------------------------------------------------
 */

static void test_parse_comments_options_and_content(void) {
  TEST("parse comments, options, empty rows, and literal #");
  const char* text =
    "# outside comment\n"
    "[animation.demo]\n"
    "icon-type = [\"nerd-icons\", \"emojis\"]\n"
    "default-frame-duration = \"150ms\"\n"
    "loop = false\n"
    "default-color = 7\n"
    "\n"
    "[[frame]]\n"
    "[[content]]\n"
    "   #\\c{4}A\\c04B\\c{4.12}C\\u{41}\\u{1F600}\\s\\{2}Z\n"
    "[[/content]]\n"
    "[[/frame]]\n"
    "\n"
    "[[frame]]\n"
    "frame-duration = \"1.0s\"\n"
    "[[content]]\n"
    "\n"
    "  A  \n"
    "[[/content]]\n"
    "[[/frame]]\n";

  char* path = NULL;
  ErrorType error = NO_ERROR;
  SpriteLibrary* library = loadSpriteText(text, &path, &error);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    assertLogContains("sprite");
    removeTemporarySprite(path);
    return;
  }
  ASSERT_EQ(error, NO_ERROR);

  const SpriteAnimation* animation = SpriteLibraryFind(library, "demo", 0);
  ASSERT_NOT_NULL(animation);
  ASSERT_NOT_NULL(SpriteLibraryFind(library, "demo", 1));
  ASSERT_NULL(SpriteLibraryFind(library, "demo", 2));
  ASSERT_STR_EQ(SpriteAnimationName(animation), "demo");
  ASSERT_EQ(SpriteAnimationFrameCount(animation), 2);
  ASSERT_FALSE(SpriteAnimationLoop(animation));
  ASSERT_EQ(SpriteAnimationDefaultColor(animation), 7);
  ASSERT_DBL_NEAR(SpriteAnimationDefaultDurationMS(animation), 150.0);

  const SpriteFrame* first = SpriteAnimationFrame(animation, 0);
  const SpriteFrame* second = SpriteAnimationFrame(animation, 1);
  ASSERT_NOT_NULL(first);
  ASSERT_NOT_NULL(second);
  ASSERT_DBL_NEAR(SpriteFrameDurationMS(first), 150.0);
  ASSERT_DBL_NEAR(SpriteFrameDurationMS(second), 1000.0);
  ASSERT_EQ(SpriteFrameHeight(first), 1);
  ASSERT_EQ(SpriteFrameWidth(first), 14);
  ASSERT_EQ(SpriteFrameHeight(second), 2);
  ASSERT_EQ(SpriteFrameWidth(second), 5);
  ASSERT_EQ(SpriteAnimationWidth(animation), 14);
  ASSERT_EQ(SpriteAnimationHeight(animation), 2);

  const SpriteRow* row = SpriteFrameRow(first, 0);
  ASSERT_NOT_NULL(row);
  ASSERT_EQ(SpriteRowCellCount(row), 13);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 0)), ' ');
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 3)), '#');
  ASSERT_EQ(SpriteCellForeground(SpriteRowCell(row, 3)), 7);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 4)), 'A');
  ASSERT_EQ(SpriteCellForeground(SpriteRowCell(row, 4)), 4);
  ASSERT_EQ(SpriteCellBackground(SpriteRowCell(row, 4)), SPRITE_NO_COLOR);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 5)), 'B');
  ASSERT_EQ(SpriteCellForeground(SpriteRowCell(row, 5)), 4);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 6)), 'C');
  ASSERT_EQ(SpriteCellForeground(SpriteRowCell(row, 6)), 4);
  ASSERT_EQ(SpriteCellBackground(SpriteRowCell(row, 6)), 12);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 7)), 'A');
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 8)), 0x1F600);
  ASSERT_EQ(SpriteCellWidth(SpriteRowCell(row, 8)), 2);
  ASSERT_TRUE(SpriteCellIsBlank(SpriteRowCell(row, 9)));
  ASSERT_TRUE(SpriteCellIsBlank(SpriteRowCell(row, 10)));
  ASSERT_TRUE(SpriteCellIsBlank(SpriteRowCell(row, 11)));
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 12)), 'Z');

  const SpriteRow* empty_row = SpriteFrameRow(second, 0);
  const SpriteRow* padded_row = SpriteFrameRow(second, 1);
  ASSERT_NOT_NULL(empty_row);
  ASSERT_NOT_NULL(padded_row);
  ASSERT_EQ(SpriteRowCellCount(empty_row), 0);
  ASSERT_EQ(SpriteRowCellCount(padded_row), 5);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(padded_row, 0)), ' ');
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(padded_row, 4)), ' ');

  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_parse_shared_and_repeated_definitions(void) {
  TEST("parse three shared icon types and repeated distinct names");
  const char* text =
    "[animation.shared]\n"
    "icon-type = [\"nerd-icons\", \"emojis\", \"ascii\"]\n"
    "default-frame-duration = \"20ms\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "same\n"
    "[[/content]]\n"
    "[[/frame]]\n"
    "\n"
    "[animation.repeat]\n"
    "icon-type = \"nerd-icons\"\n"
    "default-frame-duration = \"20ms\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "nerd\n"
    "[[/content]]\n"
    "[[/frame]]\n"
    "\n"
    "[animation.repeat]\n"
    "icon-type = \"emojis\"\n"
    "default-frame-duration = \"20ms\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "emoji\n"
    "[[/content]]\n"
    "[[/frame]]\n";

  char* path = NULL;
  ErrorType error = NO_ERROR;
  SpriteLibrary* library = loadSpriteText(text, &path, &error);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  const SpriteAnimation* shared_nerd = SpriteLibraryFind(library, "shared", 0);
  ASSERT_NOT_NULL(shared_nerd);
  ASSERT_TRUE(shared_nerd == SpriteLibraryFind(library, "shared", 1));
  ASSERT_TRUE(shared_nerd == SpriteLibraryFind(library, "shared", 2));
  ASSERT_EQ(SpriteAnimationFrameCount(shared_nerd), 1);

  const SpriteAnimation* repeat_nerd = SpriteLibraryFind(library, "repeat", 0);
  const SpriteAnimation* repeat_emoji = SpriteLibraryFind(library, "repeat", 1);
  ASSERT_NOT_NULL(repeat_nerd);
  ASSERT_NOT_NULL(repeat_emoji);
  ASSERT_TRUE(repeat_nerd != repeat_emoji);
  ASSERT_NULL(SpriteLibraryFind(library, "repeat", 2));
  ASSERT_STR_EQ(SpriteAnimationName(repeat_emoji), "repeat");

  const SpriteRow* nerd_row =
    SpriteFrameRow(SpriteAnimationFrame(repeat_nerd, 0), 0);
  const SpriteRow* emoji_row =
    SpriteFrameRow(SpriteAnimationFrame(repeat_emoji, 0), 0);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(nerd_row, 0)), 'n');
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(emoji_row, 0)), 'e');

  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_parse_escape_variants(void) {
  TEST("parse color, Unicode, spacing, and raw UTF-8 escapes");
  const char* text =
    "[animation.escapes]\n"
    "icon-type = \"ascii\"\n"
    "default-color = 7\n"
    "[[frame]]\n"
    "[[content]]\n"
    "\\c{04}\\c04\\c{04.12}\\u{41}\\u{1F600}\\u{20000}\\u{10FFFF}\\s\\{5}"
    "\xCE\xA9\n"
    "[[/content]]\n"
    "[[/frame]]\n";

  char* path = NULL;
  ErrorType error = NO_ERROR;
  SpriteLibrary* library = loadSpriteText(text, &path, &error);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  const SpriteAnimation* animation = SpriteLibraryFind(library, "escapes", 2);
  const SpriteFrame* frame = SpriteAnimationFrame(animation, 0);
  const SpriteRow* row = SpriteFrameRow(frame, 0);
  ASSERT_NOT_NULL(row);
  ASSERT_EQ(SpriteRowCellCount(row), 11);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 0)), 0x41);
  ASSERT_EQ(SpriteCellForeground(SpriteRowCell(row, 0)), 4);
  ASSERT_EQ(SpriteCellBackground(SpriteRowCell(row, 0)), 12);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 1)), 0x1F600);
  ASSERT_EQ(SpriteCellWidth(SpriteRowCell(row, 1)), 2);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 2)), 0x20000);
  ASSERT_EQ(SpriteCellWidth(SpriteRowCell(row, 2)), 2);
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 3)), 0x10FFFF);
  for (size_t i = 4; i < 10; i++)
    ASSERT_TRUE(SpriteCellIsBlank(SpriteRowCell(row, i)));
  ASSERT_EQ(SpriteCellCodepoint(SpriteRowCell(row, 10)), 0x3A9);

  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

/**
 * Load a sprite file that must fail and check the diagnostic.
 * @param text Sprite file contents.
 * @param fragment Expected diagnostic fragment.
 */
static void expectParseFailure(const char* text, const char* fragment) {
  ErrorType error = NO_ERROR;
  char* path = NULL;
  SpriteLibrary* library = loadSpriteText(text, &path, &error);
  ASSERT_NULL(library);
  assertLogContains(fragment);
  if (path != NULL) {
    assertLogContains(path);
  }
  ASSERT_EQ(error, ANIMATION_DESERIALIZATION_ERROR);
  removeTemporarySprite(path);
}

static void test_parse_errors(void) {
  TEST("reject missing declarations and options");
  expectParseFailure("default-frame-duration = \"1ms\"\n",
                     "content outside animation");
  expectParseFailure(
    "[animation.a]\n[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "icon-type");
  expectParseFailure("[animation.a]\nicon-type = \"ascii\"\n",
                     "at least one frame");

  TEST("reject duplicate, unknown, and conflicting options");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = [\"ascii\", \"ascii\"]\n"
    "[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "invalid icon-type option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "default-frame-duration = \"2ms\"\n"
    "[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "duplicate default-frame-duration option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "unknown = 1\n"
    "[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "unknown animation option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "unknown animation option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "frame-duration = \"1ms\"\n"
    "frame-duration = \"2ms\"\n"
    "[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "duplicate frame-duration option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "duration = \"1ms\"\n"
    "[[content]]\nx\n[[/content]]\n[[/frame]]\n",
    "unknown frame option");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"nerd-icons\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nx\n[[/content]]\n[[/frame]]\n"
    "\n"
    "[animation.a]\n"
    "icon-type = \"nerd-icons\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\ny\n[[/content]]\n[[/frame]]\n",
    "conflicting icon types");

  TEST("reject misplaced, nested, and unclosed blocks");
  expectParseFailure("[[frame]]\n", "outside animation");
  expectParseFailure("[[content]]\n", "content outside animation");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "[[/frame]]\n",
    "exactly one closed content block");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "[[content]]\n"
    "[[/content]]\n"
    "[[/frame]]\n",
    "structural delimiter inside content");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "x\n",
    "unclosed content");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "x\n"
    "[[/content]]\n",
    "unclosed frame");

  TEST("reject malformed escapes and out-of-range values");
  static const char* const bad_content[] = {
    "\\c{300}A", "\\c{x}A",      "\\u{110000}", "\\u{D800}",
    "\\u{}",     "\\u{1234567}", "\\{0}A",      "\\{1025}A"};
  for (size_t i = 0; i < sizeof(bad_content) / sizeof(bad_content[0]); i++)
    expectParseFailure(
      simpleSprite("a", "icon-type = \"ascii\"", bad_content[i]), "sprite");

  TEST("report filename and line number for errors");
  expectParseFailure(
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "\n"
    "[[frame]]\n"
    "[[content]]\n"
    "\\u{D800}\n"
    "[[/content]]\n"
    "[[/frame]]\n",
    ":6:");
}

static void test_parse_failure_cleanup_and_missing_file(void) {
  TEST("fail cleanly for missing files and partial libraries");
  SpriteLibrary* library = NULL;
  const char* missing[] = {"/tmp/opencode/no_such_sprite_file.asc"};
  ASSERT_EQ(LoadSpriteLibrary(missing, 1, &library), FILE_ERROR);
  ASSERT_NULL(library);
  assertLogContains("cannot open sprite file");

  char* good_path =
    writeTemporarySprite(simpleSprite("good", "icon-type = \"ascii\"", "x"));
  char* bad_path = writeTemporarySprite("[animation.bad]\n");
  ASSERT_NOT_NULL(good_path);
  ASSERT_NOT_NULL(bad_path);
  const char* files[] = {good_path, bad_path};
  ASSERT_EQ(LoadSpriteLibrary(files, 2, &library),
            ANIMATION_DESERIALIZATION_ERROR);
  ASSERT_NULL(library);
  assertLogContains(":1:");
  unlink(good_path);
  unlink(bad_path);
  free(good_path);
  free(bad_path);
}

/**
 * ---------------------------------------------------------------------------
 * Playback and rendering tests
 * ---------------------------------------------------------------------------
 */

static SpriteLibrary* loadThreeFrameLibrary(char** path_out, bool loop) {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer),
           "[animation.play]\n"
           "icon-type = \"ascii\"\n"
           "default-frame-duration = \"10ms\"\n"
           "loop = %s\n"
           "default-color = 7\n"
           "[[frame]]\n[[content]]\nA\n[[/content]]\n[[/frame]]\n"
           "[[frame]]\n[[content]]\nBB\n[[/content]]\n[[/frame]]\n"
           "[[frame]]\n[[content]]\nCCC\n[[/content]]\n[[/frame]]\n",
           loop ? "true" : "false");
  return loadSpriteText(buffer, path_out, NULL);
}

static void resetRenderCapture(void) {
  g_render_count = 0;
  g_active_foreground = -99;
  g_active_background = -99;
  g_active_attribute = 0;
}

static void test_playback_large_elapsed_updates(void) {
  TEST("advance through multiple frames for one elapsed update");
  char* path = NULL;
  g_now_ms = 0;
  SpriteLibrary* library = loadThreeFrameLibrary(&path, true);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* film = CreateRollfilm(library, "play", 2);
  ASSERT_NOT_NULL(film);
  ASSERT_EQ(RollfilmFrameCount(film), 3);
  ASSERT_EQ(RollfilmFrameIndex(film), 0);

  g_now_ms = 35;
  UpdateRollfilm(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 0);
  ASSERT_EQ(RollfilmFrameWidth(film), 1);

  g_now_ms = 60;
  UpdateRollfilm(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 0);
  ASSERT_EQ(RollfilmFrameWidth(film), 1);

  RollfilmSeekFrame(film, 1);
  g_now_ms = 105;
  UpdateRollfilm(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 2);
  ASSERT_EQ(RollfilmFrameWidth(film), 3);

  FreeRollfilm(film);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_nonlooping_playback_and_override(void) {
  TEST("stop at the last frame when looping is disabled");
  char* path = NULL;
  g_now_ms = 0;
  SpriteLibrary* library = loadThreeFrameLibrary(&path, true);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* film = CreateRollfilm(library, "play", 2);
  ASSERT_NOT_NULL(film);
  Rollfilm* films[] = {film};
  int indices[] = {0};
  SetRollfilmLoop(films, indices, 1, false);

  g_now_ms = 100;
  UpdateRollfilm(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 2);

  g_now_ms = 500;
  UpdateRollfilm(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 2);

  ASSERT_FALSE(RollfilmTryAdvance(film, 1));
  ASSERT_EQ(RollfilmFrameIndex(film), 2);

  FreeRollfilm(film);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_try_advance_and_seeking(void) {
  TEST("advance once per elapsed frame and seek with bounds checks");
  char* path = NULL;
  g_now_ms = 0;
  SpriteLibrary* library = loadThreeFrameLibrary(&path, false);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* film = CreateRollfilm(library, "play", 2);
  ASSERT_NOT_NULL(film);
  ASSERT_FALSE(RollfilmTryAdvance(film, 1));
  g_now_ms = 10;
  ASSERT_TRUE(RollfilmTryAdvance(film, 1));
  ASSERT_EQ(RollfilmFrameIndex(film), 1);
  g_now_ms = 11;
  ASSERT_FALSE(RollfilmTryAdvance(film, -1));
  g_now_ms = 20;
  ASSERT_TRUE(RollfilmTryAdvance(film, -1));
  ASSERT_EQ(RollfilmFrameIndex(film), 0);

  RollfilmSetDefaultFrame(film, 2);
  RollfilmSeekDefault(film);
  ASSERT_EQ(RollfilmFrameIndex(film), 2);
  RollfilmSeekFrame(film, 99);
  ASSERT_EQ(RollfilmFrameIndex(film), 2);
  RollfilmSeekFrame(film, 0);
  ASSERT_EQ(RollfilmFrameIndex(film), 0);

  FreeRollfilm(film);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_dimensions_and_largest(void) {
  TEST("cache independent frame dimensions and select the largest film");
  const char* text =
    "[animation.a]\nicon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nAAAAAAAAAA\n[[/content]]\n[[/frame]]\n"
    "[[frame]]\n[[content]]\nB\nC\n[[/content]]\n[[/frame]]\n"
    "\n"
    "[animation.b]\nicon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nAAAAAAAAAAAAAAAAAAAA\n[[/content]]\n[[/frame]]\n"
    "[[frame]]\n[[content]]\nB\nC\n[[/content]]\n[[/frame]]\n"
    "\n"
    "[animation.c]\nicon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nA\nB\nC\nD\nE\n[[/content]]\n[[/frame]]\n";

  char* path = NULL;
  SpriteLibrary* library = loadSpriteText(text, &path, NULL);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* a = CreateRollfilm(library, "a", 2);
  Rollfilm* b = CreateRollfilm(library, "b", 2);
  Rollfilm* c = CreateRollfilm(library, "c", 2);
  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  ASSERT_NOT_NULL(c);
  ASSERT_EQ(RollfilmWidth(a), 10);
  ASSERT_EQ(RollfilmHeight(a), 2);
  ASSERT_EQ(RollfilmFrameWidth(a), 10);
  RollfilmSeekFrame(a, 1);
  ASSERT_EQ(RollfilmFrameWidth(a), 1);
  ASSERT_EQ(RollfilmFrameHeight(a), 2);

  Rollfilm* films[] = {a, b, c};
  int indices[] = {0, 1, 2};
  ASSERT_EQ(RollfilmLargest(films, indices, 3), 2);
  int tied[] = {0, 1};
  ASSERT_EQ(RollfilmLargest(films, tied, 2), 1);

  FreeRollfilm(a);
  FreeRollfilm(b);
  FreeRollfilm(c);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_blank_queries(void) {
  TEST("find first and last blank cells in the final frame");
  const char* text =
    "[animation.blanks]\nicon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "[[frame]]\n[[content]]\nignored\n[[/content]]\n[[/frame]]\n"
    "[[frame]]\n[[content]]\nA\\sB\\{2}C\n[[/content]]\n[[/frame]]\n";
  char* path = NULL;
  SpriteLibrary* library = loadSpriteText(text, &path, NULL);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* film = CreateRollfilm(library, "blanks", 2);
  ASSERT_NOT_NULL(film);
  int x = -1;
  int y = -1;
  ASSERT_TRUE(RollfilmFirstBlank(film, &x, &y));
  ASSERT_EQ(x, 1);
  ASSERT_EQ(y, 0);
  ASSERT_TRUE(RollfilmLastBlank(film, &x, &y));
  ASSERT_EQ(x, 5);
  ASSERT_EQ(y, 0);

  FreeRollfilm(film);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_rendering_geometry_and_colors(void) {
  TEST("render Unicode, skip blank cells, and track color state");
  const char* text =
    "[animation.render]\nicon-type = \"ascii\"\n"
    "default-frame-duration = \"1ms\"\n"
    "default-color = 7\n"
    "[[frame]]\n[[content]]\n"
    "A\\u{1F600}\\s\\{2}Z\n"
    "\\c{3.5}Q\n"
    "R\n"
    "[[/content]]\n[[/frame]]\n";
  char* path = NULL;
  SpriteLibrary* library = loadSpriteText(text, &path, NULL);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    removeTemporarySprite(path);
    return;
  }

  Rollfilm* film = CreateRollfilm(library, "render", 2);
  ASSERT_NOT_NULL(film);
  resetRenderCapture();
  RenderRollfilm(film, 2, 4);

  ASSERT_EQ(g_render_count, 5);
  ASSERT_EQ(g_render_calls[0].y, 2);
  ASSERT_EQ(g_render_calls[0].x, 4);
  ASSERT_STR_EQ(g_render_calls[0].text, "A");
  ASSERT_EQ(g_render_calls[0].foreground, 7);
  ASSERT_EQ(g_render_calls[1].x, 5);
  ASSERT_STR_EQ(g_render_calls[1].text, "\xF0\x9F\x98\x80");
  ASSERT_EQ(g_render_calls[2].x, 10);
  ASSERT_STR_EQ(g_render_calls[2].text, "Z");
  ASSERT_EQ(g_render_calls[3].y, 3);
  ASSERT_EQ(g_render_calls[3].x, 4);
  ASSERT_STR_EQ(g_render_calls[3].text, "Q");
  ASSERT_EQ(g_render_calls[3].foreground, 3);
  ASSERT_EQ(g_render_calls[3].background, 5);
  ASSERT_EQ(g_render_calls[3].attribute, A_BOLD);
  ASSERT_EQ(g_render_calls[4].y, 4);
  ASSERT_EQ(g_render_calls[4].x, 4);
  ASSERT_STR_EQ(g_render_calls[4].text, "R");
  ASSERT_EQ(g_render_calls[4].foreground, 3);
  ASSERT_EQ(g_render_calls[4].background, 5);

  FreeRollfilm(film);
  FreeSpriteLibrary(library);
  removeTemporarySprite(path);
}

static void test_create_rollfilm_missing_animation(void) {
  TEST("CreateRollfilm rejects missing animation data");
  ErrorType error = NO_ERROR;
  ASSERT_NULL(CreateRollfilm(NULL, "missing", 0));
  char* path = NULL;
  SpriteLibrary* library = loadSpriteText(
    simpleSprite("a", "icon-type = \"ascii\"", "x"), &path, &error);
  ASSERT_NOT_NULL(library);
  if (library != NULL) {
    ASSERT_NULL(CreateRollfilm(library, "missing", 0));
    ASSERT_NULL(CreateRollfilm(library, "a", 3));
    FreeSpriteLibrary(library);
  }
  removeTemporarySprite(path);
}

int main(void) {
  if (setlocale(LC_CTYPE, "C.UTF-8") == NULL) {
    setlocale(LC_CTYPE, "en_US.UTF-8");
  }
  test_begin("anim/sprite");

  RUN_TEST(test_parse_comments_options_and_content,
           "parse comments, options, content, and dimensions");
  RUN_TEST(test_parse_shared_and_repeated_definitions,
           "parse shared and repeated animation definitions");
  RUN_TEST(test_parse_escape_variants, "parse all supported escape forms");
  RUN_TEST(test_parse_errors, "reject malformed sprite files");
  RUN_TEST(test_parse_failure_cleanup_and_missing_file,
           "clean up after parse failures");
  RUN_TEST(test_playback_large_elapsed_updates,
           "advance multiple frames per elapsed update");
  RUN_TEST(test_nonlooping_playback_and_override,
           "stop non-looping playback at the final frame");
  RUN_TEST(test_try_advance_and_seeking, "advance and seek deterministically");
  RUN_TEST(test_dimensions_and_largest,
           "cache dimensions and pick largest film");
  RUN_TEST(test_blank_queries, "locate blank cell runs");
  RUN_TEST(test_rendering_geometry_and_colors,
           "render Unicode, skip blank cells, and track color state");
  RUN_TEST(test_create_rollfilm_missing_animation,
           "reject animations that are not loaded");

  return test_end();
}
