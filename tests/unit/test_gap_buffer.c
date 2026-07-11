/**
 * @file test_gap_buffer.c
 * @brief Unit tests for the gap-buffer data structure.
 *
 * Tests create/free, set text, insert, delete, clear, clone, and
 * large-text round-trips.
 */

#include <stdlib.h>

#include "error.h"
#include "gap_buffer.h"
#include "test_helpers.h"

/* Stub for LogError — gap_buffer.c calls it on allocation failure */
void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

static void test_create_free(void) {
  TEST("create and free gap buffer");
  GapBuffer* gb = GapBufferCreate();
  ASSERT_NOT_NULL(gb);
  ASSERT_EQ(GapBufferLength(gb), 0);
  ASSERT_GT(GapBufferCapacity(gb), 0);
  GapBufferFree(gb);
}

static void test_free_null(void) {
  TEST("free NULL gap buffer is safe");
  GapBufferFree(NULL);
}

static void test_set_text(void) {
  TEST("set text and verify content");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "hello");
  ASSERT_EQ(GapBufferLength(gb), 5);
  char* s = GapBufferToString(gb);
  ASSERT_STR_EQ(s, "hello");
  free(s);
  GapBufferFree(gb);
}

static void test_insert_char(void) {
  TEST("insert char at position");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "helo");
  (void)GapBufferInsert(gb, 2, 'l');
  char* s = GapBufferToString(gb);
  ASSERT_STR_EQ(s, "hello");
  free(s);
  GapBufferFree(gb);
}

static void test_delete_char(void) {
  TEST("delete char at position");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "hello");
  GapBufferDelete(gb, 1);
  char* s = GapBufferToString(gb);
  ASSERT_STR_EQ(s, "hllo");
  free(s);
  GapBufferFree(gb);
}

static void test_clear(void) {
  TEST("clear gap buffer");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "hello");
  GapBufferClear(gb);
  ASSERT_EQ(GapBufferLength(gb), 0);
  char* s = GapBufferToString(gb);
  ASSERT_STR_EQ(s, "");
  free(s);
  GapBufferFree(gb);
}

static void test_clone(void) {
  TEST("clone gap buffer");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "clone me");
  GapBuffer* clone = GapBufferClone(gb);
  ASSERT_NOT_NULL(clone);
  char* s = GapBufferToString(clone);
  ASSERT_STR_EQ(s, "clone me");
  free(s);
  GapBufferFree(clone);
  GapBufferFree(gb);
}

static void test_empty_after_clear(void) {
  TEST("gap buffer is empty after clear");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "something");
  GapBufferClear(gb);
  ASSERT_EQ(GapBufferLength(gb), 0);
  char* s = GapBufferToString(gb);
  ASSERT_STR_EQ(s, "");
  free(s);
  GapBufferFree(gb);
}

static void test_set_text_empty(void) {
  TEST("set text to empty string");
  GapBuffer* gb = GapBufferCreate();
  (void)GapBufferSetText(gb, "");
  ASSERT_EQ(GapBufferLength(gb), 0);
  GapBufferFree(gb);
}

static void test_large_text(void) {
  TEST("set and retrieve large text");
  GapBuffer* gb = GapBufferCreate();
  char big[2000];
  memset(big, 'x', sizeof(big) - 1);
  big[sizeof(big) - 1] = '\0';
  (void)GapBufferSetText(gb, big);
  ASSERT_EQ(GapBufferLength(gb), 1999);
  GapBufferFree(gb);
}

int main(void) {
  test_begin("gap_buffer");
  RUN_TEST(test_create_free, "create and free gap buffer");
  RUN_TEST(test_free_null, "free NULL gap buffer is safe");
  RUN_TEST(test_set_text, "set text and verify content");
  RUN_TEST(test_insert_char, "insert char at position");
  RUN_TEST(test_delete_char, "delete char at position");
  RUN_TEST(test_clear, "clear gap buffer");
  RUN_TEST(test_clone, "clone gap buffer");
  RUN_TEST(test_empty_after_clear, "gap buffer is empty after clear");
  RUN_TEST(test_set_text_empty, "set text to empty string");
  RUN_TEST(test_large_text, "set and retrieve large text");
  return test_end();
}
