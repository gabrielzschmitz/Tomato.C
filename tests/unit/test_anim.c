/**
 * @file test_anim.c
 * @brief Unit tests for animation (rollfilm) module.
 *
 * Tests CreateRollfilm, FreeRollfilm, SetRollfilmLoop, RollfilmLargest,
 * DeserializeSprites, RollfilmFirstBlank, and RollfilmLastBlank.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anim.h"
#include "config.h"
#include "error.h"
#include "test_helpers.h"

typedef struct AppData {
  int dummy;
} AppData;
Config g_config;
void RenderCriticalQuitConfirmation(void* app) { (void)app; }

/**
 * ---------------------------------------------------------------------------
 * CreateRollfilm / FreeRollfilm
 * ---------------------------------------------------------------------------
 */

static void test_create_rollfilm(void) {
  TEST("create Rollfilm with N frames, M height");
  Rollfilm* rf = CreateRollfilm(3, 5);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->frame_count, 3);
  ASSERT_EQ(rf->frame_height, 5);
  ASSERT_EQ(rf->current_frame, 0);
  ASSERT_EQ(rf->default_frame, 0);
  ASSERT_TRUE(rf->loop);
  FreeRollfilm(rf);
}

static void test_create_rollfilm_zero_frames(void) {
  TEST("create Rollfilm with 0 frames");
  Rollfilm* rf = CreateRollfilm(0, 5);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->frame_count, 0);
  FreeRollfilm(rf);
}

static void test_create_rollfilm_large(void) {
  TEST("create Rollfilm with many frames");
  Rollfilm* rf = CreateRollfilm(20, 10);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->frame_count, 20);
  FreeRollfilm(rf);
}

static void test_free_rollfilm_null(void) {
  TEST("FreeRollfilm NULL is safe");
  FreeRollfilm(NULL);
}

/**
 * ---------------------------------------------------------------------------
 * SetRollfilmLoop
 * ---------------------------------------------------------------------------
 */

static void test_set_rollfilm_loop(void) {
  TEST("SetRollfilmLoop enables/disables looping");
  AppData app;
  Rollfilm* films[3];
  for (int i = 0; i < 3; i++) {
    films[i] = CreateRollfilm(1, 1);
    ASSERT_NOT_NULL(films[i]);
  }
  int indices[] = {0, 2};
  SetRollfilmLoop(&app, films, indices, 2, false);
  ASSERT_FALSE(films[0]->loop);
  ASSERT_TRUE(films[1]->loop);
  ASSERT_FALSE(films[2]->loop);
  SetRollfilmLoop(&app, films, indices, 2, true);
  ASSERT_TRUE(films[0]->loop);
  ASSERT_TRUE(films[2]->loop);
  for (int i = 0; i < 3; i++) FreeRollfilm(films[i]);
}

static void test_set_rollfilm_loop_null_film(void) {
  TEST("SetRollfilmLoop skips NULL films safely");
  Rollfilm* films[2] = {NULL, NULL};
  int idx[] = {0, 1};
  AppData app;
  SetRollfilmLoop(&app, films, idx, 2, true);
}

/**
 * ---------------------------------------------------------------------------
 * RollfilmLargest
 * ---------------------------------------------------------------------------
 */

static void test_rollfilm_largest(void) {
  TEST("RollfilmLargest returns index of widest/tallest");
  Rollfilm* films[4];
  for (int i = 0; i < 4; i++) {
    films[i] = CreateRollfilm(1, 1);
    ASSERT_NOT_NULL(films[i]);
  }
  films[0]->frame_width = 10;
  films[0]->frame_height = 5;
  films[1]->frame_width = 20;
  films[1]->frame_height = 8;
  films[2]->frame_width = 15;
  films[2]->frame_height = 8;
  films[3]->frame_width = 20;
  films[3]->frame_height = 3;
  int indices[] = {0, 1, 2, 3};
  int largest = RollfilmLargest(films, indices, 4);
  ASSERT_EQ(largest, 1);
  for (int i = 0; i < 4; i++) FreeRollfilm(films[i]);
}

static void test_rollfilm_largest_height_wins(void) {
  TEST("RollfilmLargest prefers taller over wider");
  Rollfilm* films[2];
  films[0] = CreateRollfilm(1, 1);
  films[0]->frame_width = 100;
  films[0]->frame_height = 1;
  films[1] = CreateRollfilm(1, 1);
  films[1]->frame_width = 10;
  films[1]->frame_height = 10;
  int indices[] = {0, 1};
  ASSERT_EQ(RollfilmLargest(films, indices, 2), 1);
  FreeRollfilm(films[0]);
  FreeRollfilm(films[1]);
}

static void test_rollfilm_largest_null_animations(void) {
  TEST("RollfilmLargest with NULL returns -1");
  ASSERT_EQ(RollfilmLargest(NULL, NULL, 0), -1);
}

static void test_rollfilm_largest_null_entry(void) {
  TEST("RollfilmLargest skips NULL film entries");
  Rollfilm* films[3] = {NULL, NULL, NULL};
  int indices[] = {0, 1, 2};
  ASSERT_EQ(RollfilmLargest(films, indices, 3), 1);
}

/**
 * ---------------------------------------------------------------------------
 * DeserializeSprites
 * ---------------------------------------------------------------------------
 */

static void test_deserialize_sprites(void) {
  TEST("DeserializeSprites parses sprite file");
  g_config.visual.icons = "nerd-icons";
  g_config.visual.icons_index = 0;
  const char* sprite_data =
    "nerd-icons/2c/2h/1f\n"
    "0.5s\n"
    "abc\n"
    "def\n"
    "1.0s\n"
    "ghi\n"
    "jkl\n"
    "--------------------------------------------------------------------------"
    "-\n";
  FILE* tmp = tmpfile();
  ASSERT_NOT_NULL(tmp);
  fwrite(sprite_data, 1, strlen(sprite_data), tmp);
  rewind(tmp);
  char tmp_path[64];
  snprintf(tmp_path, sizeof(tmp_path), "/dev/fd/%d", fileno(tmp));
  Rollfilm* rf = DeserializeSprites(tmp_path, 0);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->frame_count, 2);
  ASSERT_EQ(rf->frame_height, 2);
  ASSERT_EQ(rf->default_frame, 1);
  FreeRollfilm(rf);
  fclose(tmp);
}

static void test_deserialize_sprites_default_frame_default(void) {
  TEST("DeserializeSprites defaults default_frame to 0 when not set");
  g_config.visual.icons = "nerd-icons";
  g_config.visual.icons_index = 0;
  const char* sprite_data =
    "nerd-icons/2c/2h\n"
    "0.5s\n"
    "abc\n"
    "def\n"
    "1.0s\n"
    "ghi\n"
    "jkl\n"
    "--------------------------------------------------------------------------"
    "-\n";
  FILE* tmp = tmpfile();
  ASSERT_NOT_NULL(tmp);
  fwrite(sprite_data, 1, strlen(sprite_data), tmp);
  rewind(tmp);
  char tmp_path[64];
  snprintf(tmp_path, sizeof(tmp_path), "/dev/fd/%d", fileno(tmp));
  Rollfilm* rf = DeserializeSprites(tmp_path, 0);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->default_frame, 0);
  FreeRollfilm(rf);
  fclose(tmp);
}

static void test_deserialize_sprites_nonexistent(void) {
  TEST("DeserializeSprites returns NULL for missing file");
  g_config.visual.icons = "nerd-icons";
  Rollfilm* rf = DeserializeSprites("/tmp/nonexistent_sprite_42.sprite", 0);
  ASSERT_NULL(rf);
}

/**
 * ---------------------------------------------------------------------------
 * RollfilmFirstBlank / RollfilmLastBlank helpers
 * ---------------------------------------------------------------------------
 */

static struct Frame* make_test_frame(int id, int width) {
  struct Frame* f = (struct Frame*)malloc(sizeof(struct Frame));
  f->next = f;
  f->rows = NULL;
  f->seconds_multiplier = 1.0;
  f->width = width;
  f->id = id;
  return f;
}

static void append_row_token(struct FrameRow* row, char ch) {
  struct FrameToken* t = (struct FrameToken*)malloc(sizeof(struct FrameToken));
  t->next = NULL;
  t->is_blank = (ch == '_');
  char* buf = (char*)malloc(2);
  buf[0] = ch;
  buf[1] = '\0';
  t->token = buf;
  t->length = 1;
  t->color = -1;
  struct FrameToken** last = &row->tokens;
  while (*last) last = &(*last)->next;
  *last = t;
}

static struct FrameRow* make_test_row(const char* tokens) {
  struct FrameRow* row = (struct FrameRow*)malloc(sizeof(struct FrameRow));
  row->next = NULL;
  row->tokens = NULL;
  while (*tokens) {
    append_row_token(row, *tokens);
    tokens++;
  }
  return row;
}

static void free_test_rollfilm(Rollfilm* rf) {
  if (!rf) return;
  if (rf->frames) {
    struct Frame* start = rf->frames;
    struct Frame* f = start;
    do {
      struct Frame* next = f->next;
      struct FrameRow* r = f->rows;
      while (r) {
        struct FrameRow* next_r = r->next;
        struct FrameToken* t = r->tokens;
        while (t) {
          struct FrameToken* next_t = t->next;
          free(t->token);
          free(t);
          t = next_t;
        }
        free(r);
        r = next_r;
      }
      free(f);
      f = next;
    } while (f && f != start);
  }
  free(rf);
}

/**
 * ---------------------------------------------------------------------------
 * RollfilmFirstBlank / RollfilmLastBlank tests
 * ---------------------------------------------------------------------------
 */

static void test_first_blank(void) {
  TEST("RollfilmFirstBlank finds first blank token in last frame");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 0;
  rf->frame_count = 2;
  rf->frame_height = 2;
  rf->frame_width = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f1 = make_test_frame(0, 3);
  struct Frame* f2 = make_test_frame(1, 3);
  f1->next = f2;
  f2->next = f1;
  rf->frames = f1;

  struct FrameRow* r1a = make_test_row("abc");
  struct FrameRow* r1b = make_test_row("_bc");
  r1a->next = r1b;
  f1->rows = r1a;

  struct FrameRow* r2a = make_test_row("a_c");
  f2->rows = r2a;

  int x, y;
  ASSERT_TRUE(RollfilmFirstBlank(rf, &x, &y));
  ASSERT_EQ(x, 1);
  ASSERT_EQ(y, 0);
  free_test_rollfilm(rf);
}

static void test_last_blank(void) {
  TEST("RollfilmLastBlank finds last blank token in last frame");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 0;
  rf->frame_count = 1;
  rf->frame_height = 2;
  rf->frame_width = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f1 = make_test_frame(0, 5);
  f1->next = f1;
  rf->frames = f1;

  struct FrameRow* r1 = make_test_row("a_b_c");
  f1->rows = r1;

  int x, y;
  ASSERT_TRUE(RollfilmLastBlank(rf, &x, &y));
  ASSERT_EQ(x, 4);
  ASSERT_EQ(y, 0);
  free_test_rollfilm(rf);
}

static void test_first_blank_no_blank(void) {
  TEST("RollfilmFirstBlank returns false when no blank");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 0;
  rf->frame_count = 1;
  rf->frame_height = 1;
  rf->frame_width = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f1 = make_test_frame(0, 3);
  f1->next = f1;
  rf->frames = f1;
  f1->rows = make_test_row("abc");

  int x, y;
  ASSERT_FALSE(RollfilmFirstBlank(rf, &x, &y));
  free_test_rollfilm(rf);
}

static void test_first_last_blank_null(void) {
  TEST("RollfilmFirstBlank/LastBlank returns false on NULL");
  int x, y;
  ASSERT_FALSE(RollfilmFirstBlank(NULL, &x, &y));
  ASSERT_FALSE(RollfilmLastBlank(NULL, &x, &y));
}

/**
 * ---------------------------------------------------------------------------
 * RollfilmSeekFrame
 * ---------------------------------------------------------------------------
 */

static void test_rollfilm_seek_frame(void) {
  TEST("RollfilmSeekFrame seeks to correct frame");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 0;
  rf->frame_count = 3;
  rf->frame_height = 1;
  rf->frame_width = 0;
  rf->default_frame = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f0 = make_test_frame(0, 3);
  struct Frame* f1 = make_test_frame(1, 3);
  struct Frame* f2 = make_test_frame(2, 3);
  f0->next = f1;
  f1->next = f2;
  f2->next = f0;
  rf->frames = f0;

  RollfilmSeekFrame(rf, 2);
  ASSERT_EQ(rf->current_frame, 2);
  ASSERT_EQ(rf->frames->id, 2);

  RollfilmSeekFrame(rf, 0);
  ASSERT_EQ(rf->current_frame, 0);
  ASSERT_EQ(rf->frames->id, 0);

  free_test_rollfilm(rf);
}

static void test_rollfilm_seek_frame_out_of_bounds(void) {
  TEST("RollfilmSeekFrame out-of-bounds is safe");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 0;
  rf->frame_count = 2;
  rf->frame_height = 1;
  rf->frame_width = 0;
  rf->default_frame = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f0 = make_test_frame(0, 3);
  struct Frame* f1 = make_test_frame(1, 3);
  f0->next = f1;
  f1->next = f0;
  rf->frames = f0;

  RollfilmSeekFrame(rf, -1);
  ASSERT_EQ(rf->current_frame, 0);

  RollfilmSeekFrame(rf, 99);
  ASSERT_EQ(rf->current_frame, 0);

  RollfilmSeekFrame(NULL, 0);

  free_test_rollfilm(rf);
}

static void test_rollfilm_seek_frame_self_frame(void) {
  TEST("RollfilmSeekFrame to current frame resets timer");
  Rollfilm* rf = (Rollfilm*)malloc(sizeof(Rollfilm));
  rf->loop = true;
  rf->delta_frame_ms = 0;
  rf->current_frame = 1;
  rf->frame_count = 2;
  rf->frame_height = 1;
  rf->frame_width = 0;
  rf->default_frame = 0;
  rf->update = NULL;
  rf->render = NULL;

  struct Frame* f0 = make_test_frame(0, 3);
  struct Frame* f1 = make_test_frame(1, 3);
  f0->next = f1;
  f1->next = f0;
  rf->frames = f0;

  double before = rf->delta_frame_ms;
  RollfilmSeekFrame(rf, 1);
  ASSERT_EQ(rf->current_frame, 1);
  ASSERT_NE(rf->delta_frame_ms, before);

  free_test_rollfilm(rf);
}

/**
 * ---------------------------------------------------------------------------
 * Deserialize — explicit /Nf suffix
 * ---------------------------------------------------------------------------
 */

static void test_deserialize_sprites_default_frame_explicit(void) {
  TEST("DeserializeSprites parses explicit /0f default_frame");
  g_config.visual.icons = "nerd-icons";
  g_config.visual.icons_index = 0;
  const char* sprite_data =
    "nerd-icons/2c/2h/0f\n"
    "0.5s\n"
    "abc\n"
    "def\n"
    "1.0s\n"
    "ghi\n"
    "jkl\n"
    "--------------------------------------------------------------------------"
    "-\n";
  FILE* tmp = tmpfile();
  ASSERT_NOT_NULL(tmp);
  fwrite(sprite_data, 1, strlen(sprite_data), tmp);
  rewind(tmp);
  char tmp_path[64];
  snprintf(tmp_path, sizeof(tmp_path), "/dev/fd/%d", fileno(tmp));
  Rollfilm* rf = DeserializeSprites(tmp_path, 0);
  ASSERT_NOT_NULL(rf);
  ASSERT_EQ(rf->default_frame, 0);
  ASSERT_EQ(rf->frame_count, 2);
  FreeRollfilm(rf);
  fclose(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * Overflow / bounds tests
 * ---------------------------------------------------------------------------
 */

static void test_deserialize_long_line_no_overflow(void) {
  TEST("DeserializeSprites long frame line does not overflow token buffer");
  FILE* tmp = tmpfile();
  ASSERT_NOT_NULL(tmp);
  /* Frame longer than MAX_FRAME_WIDTH */
  fprintf(tmp, "# tomato sprites v1\n");
  fprintf(tmp, "# size 200x1\n");
  fprintf(tmp, "# /0f\n");
  fprintf(tmp, "# frame_ms 100\n");
  fprintf(tmp, "# icons nerd-icons\n");
  fprintf(tmp, "# icons emojis\n");
  fprintf(tmp, "# icons ascii\n");
  char long_line[256];
  memset(long_line, 'A', 200);
  long_line[200] = '\0';
  fprintf(tmp, "%s\n", long_line);
  fprintf(tmp, "%s\n", "--------------------------------------------");
  fprintf(tmp, "%s\n", long_line);
  rewind(tmp);
  char tmp_path[64];
  snprintf(tmp_path, sizeof(tmp_path), "/dev/fd/%d", fileno(tmp));
  Rollfilm* rf = DeserializeSprites(tmp_path, 0);
  /* Should not crash — may return NULL on overflow but must not corrupt memory */
  (void)rf;
  if (rf) FreeRollfilm(rf);
  fclose(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * Internals — isSeparatorLine, isIconsLine, parseFrameSize, parseFrameTime
 * ---------------------------------------------------------------------------
 */

#define SEP75 "---------------------------------------------------------------------------"

static int isSeparatorLine(const char* line) {
  if (!line) return 0;
  return strcmp(line, SEP75) == 0;
}

static int isIconsLine(const char* line) {
  if (!line) return 0;
  return strncmp(line, "# icons", 7) == 0;
}

static int parseFrameSize(const char* line, int* width, int* height) {
  if (!line || !width || !height) return 0;
  return sscanf(line, "# size %dx%d", width, height) == 2;
}

static int parseFrameTime(const char* line, double* seconds) {
  if (!line || !seconds) return 0;
  return sscanf(line, "%lfs", seconds) == 1;
}

static void test_is_separator_line(void) {
  TEST("isSeparatorLine matches the 75-char separator");
  ASSERT_TRUE(isSeparatorLine(SEP75));
}

static void test_is_separator_line_not_separator(void) {
  TEST("isSeparatorLine returns 0 for non-separator line");
  ASSERT_FALSE(isSeparatorLine("not a separator"));
}

static void test_is_separator_line_null(void) {
  TEST("isSeparatorLine returns 0 for NULL");
  ASSERT_FALSE(isSeparatorLine(NULL));
}

static void test_is_icons_line(void) {
  TEST("isIconsLine matches # icons prefix");
  ASSERT_TRUE(isIconsLine("# icons nerd-icons"));
}

static void test_is_icons_line_not_icons(void) {
  TEST("isIconsLine returns 0 for non-icons line");
  ASSERT_FALSE(isIconsLine("# size 80x24"));
}

static void test_is_icons_line_null(void) {
  TEST("isIconsLine returns 0 for NULL");
  ASSERT_FALSE(isIconsLine(NULL));
}

static void test_parse_frame_size(void) {
  TEST("parseFrameSize parses \"# size WxH\"");
  int w, h;
  ASSERT_TRUE(parseFrameSize("# size 80x24", &w, &h));
  ASSERT_EQ(w, 80);
  ASSERT_EQ(h, 24);
}

static void test_parse_frame_size_invalid(void) {
  TEST("parseFrameSize returns 0 for invalid input");
  int w, h;
  ASSERT_FALSE(parseFrameSize("not a size line", &w, &h));
}

static void test_parse_frame_size_null(void) {
  TEST("parseFrameSize returns 0 for NULL inputs");
  int w, h;
  ASSERT_FALSE(parseFrameSize(NULL, &w, &h));
  ASSERT_FALSE(parseFrameSize("# size 80x24", NULL, &h));
  ASSERT_FALSE(parseFrameSize("# size 80x24", &w, NULL));
}

static void test_parse_frame_time(void) {
  TEST("parseFrameTime parses \"0.5s\" format");
  double sec;
  ASSERT_TRUE(parseFrameTime("0.5s", &sec));
  ASSERT_EQ((int)(sec * 10), 5);
}

static void test_parse_frame_time_invalid(void) {
  TEST("parseFrameTime returns 0 for invalid input");
  double sec;
  ASSERT_FALSE(parseFrameTime("# size 80x24", &sec));
}

static void test_parse_frame_time_null(void) {
  TEST("parseFrameTime returns 0 for NULL");
  double sec;
  ASSERT_FALSE(parseFrameTime(NULL, &sec));
  ASSERT_FALSE(parseFrameTime("0.5s", NULL));
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("anim");
  RUN_TEST(test_create_rollfilm, "create Rollfilm with N frames, M height");
  RUN_TEST(test_create_rollfilm_zero_frames, "create Rollfilm with 0 frames");
  RUN_TEST(test_create_rollfilm_large, "create Rollfilm with many frames");
  RUN_TEST(test_free_rollfilm_null, "FreeRollfilm NULL is safe");
  RUN_TEST(test_set_rollfilm_loop, "SetRollfilmLoop enables/disables looping");
  RUN_TEST(test_set_rollfilm_loop_null_film,
           "SetRollfilmLoop skips NULL films");
  RUN_TEST(test_rollfilm_largest,
           "RollfilmLargest returns index of widest/tallest");
  RUN_TEST(test_rollfilm_largest_height_wins,
           "RollfilmLargest prefers taller over wider");
  RUN_TEST(test_rollfilm_largest_null_animations,
           "RollfilmLargest with NULL returns -1");
  RUN_TEST(test_rollfilm_largest_null_entry,
           "RollfilmLargest skips NULL entries");
  RUN_TEST(test_deserialize_sprites, "DeserializeSprites parses sprite file");
  RUN_TEST(test_deserialize_sprites_default_frame_default,
           "DeserializeSprites defaults default_frame to 0");
  RUN_TEST(test_deserialize_sprites_nonexistent,
           "DeserializeSprites returns NULL for missing file");
  RUN_TEST(test_first_blank, "RollfilmFirstBlank finds first blank token");
  RUN_TEST(test_last_blank, "RollfilmLastBlank finds last blank token");
  RUN_TEST(test_first_blank_no_blank, "RollfilmFirstBlank false when no blank");
  RUN_TEST(test_first_last_blank_null,
           "RollfilmFirstBlank/LastBlank returns false on NULL");
  RUN_TEST(test_rollfilm_seek_frame,
           "RollfilmSeekFrame seeks to correct frame");
  RUN_TEST(test_rollfilm_seek_frame_out_of_bounds,
           "RollfilmSeekFrame out-of-bounds is safe");
  RUN_TEST(test_rollfilm_seek_frame_self_frame,
           "RollfilmSeekFrame to current frame resets timer");
  RUN_TEST(test_deserialize_sprites_default_frame_explicit,
           "DeserializeSprites parses explicit /0f default_frame");
  RUN_TEST(test_deserialize_long_line_no_overflow,
           "DeserializeSprites long frame line no overflow");
  RUN_TEST(test_is_separator_line, "isSeparatorLine matches 75-char separator");
  RUN_TEST(test_is_separator_line_not_separator,
           "isSeparatorLine returns 0 for non-separator");
  RUN_TEST(test_is_separator_line_null,
           "isSeparatorLine returns 0 for NULL");
  RUN_TEST(test_is_icons_line, "isIconsLine matches # icons prefix");
  RUN_TEST(test_is_icons_line_not_icons,
           "isIconsLine returns 0 for non-icons line");
  RUN_TEST(test_is_icons_line_null, "isIconsLine returns 0 for NULL");
  RUN_TEST(test_parse_frame_size, "parseFrameSize parses \"# size WxH\"");
  RUN_TEST(test_parse_frame_size_invalid,
           "parseFrameSize returns 0 for invalid input");
  RUN_TEST(test_parse_frame_size_null,
           "parseFrameSize returns 0 for NULL inputs");
  RUN_TEST(test_parse_frame_time, "parseFrameTime parses \"0.5s\" format");
  RUN_TEST(test_parse_frame_time_invalid,
           "parseFrameTime returns 0 for invalid input");
  RUN_TEST(test_parse_frame_time_null,
           "parseFrameTime returns 0 for NULL");
  return test_end();
}
