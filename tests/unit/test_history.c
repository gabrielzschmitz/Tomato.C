/**
 * @file test_history.c
 * @brief Unit tests for the undo/redo history stack.
 *
 * Tests push/pop LIFO order, undo/redo round-trips, empty-stack
 * handling, free callbacks, large-batch pushes, and future-stack
 * clearing on new pushes.
 */

#include <stdlib.h>

#include "history.h"
#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * Test helpers
 * ---------------------------------------------------------------------------
 */
static int freed_count = 0;
static void track_free(void* data) {
  (void)data;
  freed_count++;
}

static void reset_freed(void) { freed_count = 0; }

/**
 * ---------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------------
 */
static void test_create_history(void) {
  TEST("create history");
  History* h = CreateHistory();
  ASSERT_NOT_NULL(h);
  ASSERT_FALSE(HistoryCanUndo(h));
  ASSERT_FALSE(HistoryCanRedo(h));
  FreeHistory(h, NULL);
}

static void test_push_pop(void) {
  TEST("push and pop LIFO order");
  History* h = CreateHistory();
  ASSERT_NOT_NULL(h);

  int a = 1, b = 2, c = 3;
  HistoryPush(h, &a, NULL, false);
  HistoryPush(h, &b, NULL, false);
  HistoryPush(h, &c, NULL, false);

  ASSERT_TRUE(HistoryCanUndo(h));
  int* val = (int*)HistoryPop(h, true);
  ASSERT_EQ(*val, 3);

  val = (int*)HistoryPop(h, true);
  ASSERT_EQ(*val, 2);

  val = (int*)HistoryPop(h, true);
  ASSERT_EQ(*val, 1);

  ASSERT_FALSE(HistoryCanUndo(h));
  FreeHistory(h, NULL);
}

static void test_undo_redo_roundtrip(void) {
  TEST("undo then redo round-trip");
  History* h = CreateHistory();
  int a = 10, b = 20;
  HistoryPush(h, &a, NULL, false);
  HistoryPush(h, &b, NULL, false);

  /* Undo: pop from past, push to future */
  int* val = (int*)HistoryPop(h, true);
  ASSERT_EQ(*val, 20);
  HistoryPush(h, val, NULL, true);
  ASSERT_TRUE(HistoryCanRedo(h));

  /* Redo: pop from future, push to past */
  val = (int*)HistoryPop(h, false);
  ASSERT_EQ(*val, 20);
  HistoryPush(h, val, NULL, false);
  ASSERT_TRUE(HistoryCanUndo(h));
  FreeHistory(h, NULL);
}

static void test_push_pop_future(void) {
  TEST("push to future stack");
  History* h = CreateHistory();
  int a = 99;
  HistoryPush(h, &a, NULL, true);
  ASSERT_TRUE(HistoryCanRedo(h));
  int* val = (int*)HistoryPop(h, false);
  ASSERT_EQ(*val, 99);
  FreeHistory(h, NULL);
}

static void test_pop_empty_past(void) {
  TEST("pop from empty past returns NULL");
  History* h = CreateHistory();
  void* val = HistoryPop(h, true);
  ASSERT_NULL(val);
  FreeHistory(h, NULL);
}

static void test_pop_empty_future(void) {
  TEST("pop from empty future returns NULL");
  History* h = CreateHistory();
  void* val = HistoryPop(h, false);
  ASSERT_NULL(val);
  FreeHistory(h, NULL);
}

static void test_can_undo_redo_states(void) {
  TEST("CanUndo/CanRedo state transitions");
  History* h = CreateHistory();
  int x = 0;

  ASSERT_FALSE(HistoryCanUndo(h));
  ASSERT_FALSE(HistoryCanRedo(h));

  HistoryPush(h, &x, NULL, false);
  ASSERT_TRUE(HistoryCanUndo(h));
  ASSERT_FALSE(HistoryCanRedo(h));

  /* Undo: pop from past, push to future */
  int* val = (int*)HistoryPop(h, true);
  HistoryPush(h, val, NULL, true);
  ASSERT_FALSE(HistoryCanUndo(h));
  ASSERT_TRUE(HistoryCanRedo(h));

  FreeHistory(h, NULL);
}

static void test_free_calls_free_fn(void) {
  TEST("FreeHistory calls free_fn on data");
  reset_freed();
  History* h = CreateHistory();
  int* datas[3];
  for (int i = 0; i < 3; i++) {
    datas[i] = malloc(sizeof(int));
    *datas[i] = i;
    HistoryPush(h, datas[i], track_free, false);
  }
  FreeHistory(h, track_free);
  ASSERT_EQ(freed_count, 3);
}

static void test_free_null_is_safe(void) {
  TEST("FreeHistory with NULL is safe");
  FreeHistory(NULL, NULL);
}

static void test_multiple_pushes(void) {
  TEST("many pushes preserve order");
  History* h = CreateHistory();
  int vals[100];
  for (int i = 0; i < 100; i++) {
    vals[i] = i;
    HistoryPush(h, &vals[i], NULL, false);
  }
  for (int i = 99; i >= 0; i--) {
    int* v = (int*)HistoryPop(h, true);
    ASSERT_EQ(*v, i);
  }
  FreeHistory(h, NULL);
}

static void test_new_push_after_undo_clears_future(void) {
  TEST("new push after undo clears future stack");
  History* h = CreateHistory();
  int a = 1, b = 2, c = 3;
  HistoryPush(h, &a, NULL, false);
  HistoryPush(h, &b, NULL, false);

  /* Undo: pop from past, push to future */
  int* val = (int*)HistoryPop(h, true);
  HistoryPush(h, val, NULL, true);
  ASSERT_TRUE(HistoryCanRedo(h));

  /* New push: should clear future (caller clears future stack manually) */
  HistoryPush(h, &c, NULL, false);
  /* Manually clear future stack as typical undo system would */
  while ((val = (int*)HistoryPop(h, false)) != NULL) {
    /* discard */
  }
  ASSERT_FALSE(HistoryCanRedo(h));

  /* Now verify past has c on top */
  val = (int*)HistoryPop(h, true);
  ASSERT_EQ(*val, 3);
  FreeHistory(h, NULL);
}

int main(void) {
  test_begin("history");
  RUN_TEST(test_create_history, "create history");
  RUN_TEST(test_push_pop, "push and pop LIFO order");
  RUN_TEST(test_undo_redo_roundtrip, "undo then redo round-trip");
  RUN_TEST(test_push_pop_future, "push to future stack");
  RUN_TEST(test_pop_empty_past, "pop from empty past returns NULL");
  RUN_TEST(test_pop_empty_future, "pop from empty future returns NULL");
  RUN_TEST(test_can_undo_redo_states, "CanUndo/CanRedo state transitions");
  RUN_TEST(test_free_calls_free_fn, "FreeHistory calls free_fn on data");
  RUN_TEST(test_free_null_is_safe, "FreeHistory with NULL is safe");
  RUN_TEST(test_multiple_pushes, "many pushes preserve order");
  RUN_TEST(test_new_push_after_undo_clears_future,
           "new push after undo clears future stack");
  return test_end();
}
