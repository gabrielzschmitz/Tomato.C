/**
 * @file test_anim_malloc.c
 * @brief OOM unit tests for the animation (rollfilm) module.
 *
 * Verifies that CreateRollfilm handles allocation failure gracefully while
 * sprite data stays owned by the sprite library. Linked with
 * -Wl,--wrap,malloc to intercept allocations.
 */

#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/error.h"
#include "features/animation/anim.h"
#include "features/animation/sprite.h"
#include "test_helpers.h"
#include "test_malloc.h"

/** @brief Stub wall clock used by CreateRollfilm. */
double GetCurrentTimeMS(void) { return 0.0; }

/** @brief Stub error logger. */
void LogError(const char* context, ErrorType error) {
  (void)context;
  (void)error;
}

/** @brief Stub color setter. */
void SetColor(short int fg, short int bg, chtype attr) {
  (void)fg;
  (void)bg;
  (void)attr;
}

/** @brief Stub ncurses writer. */
int mvprintw(int y, int x, const char* format, ...) {
  (void)y;
  (void)x;
  (void)format;
  return 0;
}

/**
 * Create a temporary sprite file.
 * @param path_out Output receiving the heap path.
 * @return true when the file was created.
 */
static bool createSpriteFile(char** path_out) {
  static const char* const templates[] = {
    "/tmp/opencode/tomato_anim_malloc_XXXXXX",
    "/tmp/tomato_anim_malloc_XXXXXX"};
  static const char sprite[] =
    "[animation.a]\n"
    "icon-type = \"ascii\"\n"
    "default-frame-duration = \"10ms\"\n"
    "[[frame]]\n"
    "[[content]]\n"
    "x\n"
    "[[/content]]\n"
    "[[/frame]]\n";

  for (size_t i = 0; i < sizeof(templates) / sizeof(templates[0]); i++) {
    char* path = (char*)malloc(strlen(templates[i]) + 1);
    if (path == NULL) return false;
    strcpy(path, templates[i]);
    int fd = mkstemp(path);
    if (fd < 0) {
      free(path);
      continue;
    }
    FILE* file = fdopen(fd, "wb");
    if (file == NULL) {
      close(fd);
      unlink(path);
      free(path);
      continue;
    }
    fputs(sprite, file);
    fclose(file);
    *path_out = path;
    return true;
  }
  return false;
}

/** @brief CreateRollfilm returns NULL when its allocation fails. */
static void test_create_rollfilm_malloc_fail(void) {
  TEST("CreateRollfilm returns NULL when malloc fails");

  char* path = NULL;
  ASSERT_TRUE(createSpriteFile(&path));
  if (path == NULL) return;

  const char* files[] = {path};
  SpriteLibrary* library = NULL;
  ASSERT_EQ(LoadSpriteLibrary(files, 1, &library), NO_ERROR);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    unlink(path);
    free(path);
    return;
  }

  TestMallocFailAfter(0);
  Rollfilm* film = CreateRollfilm(library, "a", 2);
  ASSERT_NULL(film);
  TestMallocReset();

  FreeSpriteLibrary(library);
  unlink(path);
  free(path);
}

/** @brief CreateRollfilms returns NULL when its allocation fails. */
static void test_create_rollfilms_malloc_fail(void) {
  TEST("CreateRollfilms returns NULL when malloc fails");

  char* path = NULL;
  ASSERT_TRUE(createSpriteFile(&path));
  if (path == NULL) return;

  const char* files[] = {path};
  SpriteLibrary* library = NULL;
  ASSERT_EQ(LoadSpriteLibrary(files, 1, &library), NO_ERROR);
  ASSERT_NOT_NULL(library);
  if (library == NULL) {
    unlink(path);
    free(path);
    return;
  }

  const RollfilmDefinition definitions[] = {{"a", 2}};
  Rollfilm* out[1] = {(Rollfilm*)1};
  TestMallocFailAfter(0);
  ASSERT_NULL(CreateRollfilms(library, definitions, 1, out));
  ASSERT_NULL(out[0]);
  TestMallocReset();

  FreeSpriteLibrary(library);
  unlink(path);
  free(path);
}

int main(void) {
  test_begin("anim_malloc");
  RUN_TEST(test_create_rollfilm_malloc_fail,
           "CreateRollfilm returns NULL when malloc fails");
  RUN_TEST(test_create_rollfilms_malloc_fail,
           "CreateRollfilms returns NULL when malloc fails");
  return test_end();
}
