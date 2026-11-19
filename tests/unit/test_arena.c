/**
 * @file test_arena.c
 * @brief Unit tests for the core arena allocator.
 *
 * Tests default and explicit alignment, writable storage, block reuse,
 * multi-chunk growth, large allocations with small chunks, and null or
 * zero-sized inputs.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/arena.h"
#include "test_helpers.h"

static void test_allocate_writable_and_aligned(void) {
  TEST("allocate writable, aligned storage");
  Arena arena;
  ArenaInit(&arena, 0);

  unsigned char* first = (unsigned char*)ArenaAllocate(&arena, 1);
  ASSERT_NOT_NULL(first);
  ASSERT_EQ((uintptr_t)first % ARENA_ALIGNMENT, 0);
  first[0] = 0xAB;

  unsigned char* second = (unsigned char*)ArenaAllocate(&arena, 100);
  ASSERT_NOT_NULL(second);
  ASSERT_EQ((uintptr_t)second % ARENA_ALIGNMENT, 0);
  memset(second, 0x5A, 100);
  ASSERT_EQ(first[0], 0xAB);
  ASSERT_EQ(second[99], 0x5A);

  ArenaDestroy(&arena);
}

static void test_explicit_alignment(void) {
  TEST("ArenaAllocateAligned honours larger alignment");
  Arena arena;
  ArenaInit(&arena, 0);

  void* wide = ArenaAllocateAligned(&arena, 3, 64);
  ASSERT_NOT_NULL(wide);
  ASSERT_EQ((uintptr_t)wide % 64, 0);

  void* narrow = ArenaAllocateAligned(&arena, 7, 8);
  ASSERT_NOT_NULL(narrow);
  ASSERT_EQ((uintptr_t)narrow % ARENA_ALIGNMENT, 0);

  ArenaDestroy(&arena);
}

static void test_release_reuses_block(void) {
  TEST("released block is reused for the same size");
  Arena arena;
  ArenaInit(&arena, 0);

  unsigned char* first = (unsigned char*)ArenaAllocate(&arena, 48);
  ASSERT_NOT_NULL(first);
  memset(first, 1, 48);
  ArenaRelease(&arena, first, 48);

  unsigned char* second = (unsigned char*)ArenaAllocate(&arena, 48);
  ASSERT_NOT_NULL(second);
  ASSERT_EQ(second, first);

  ArenaDestroy(&arena);
}

static void test_release_honours_new_alignment(void) {
  TEST("released block is skipped when alignment is too weak");
  Arena arena;
  ArenaInit(&arena, 0);

  void* weak = ArenaAllocate(&arena, 16);
  ASSERT_NOT_NULL(weak);
  uintptr_t weak_mod = (uintptr_t)weak % 64;
  ArenaRelease(&arena, weak, 16);

  void* strict = ArenaAllocateAligned(&arena, 16, 64);
  ASSERT_NOT_NULL(strict);
  ASSERT_EQ((uintptr_t)strict % 64, 0);
  if (weak_mod != 0) ASSERT_NE(strict, weak);

  ArenaDestroy(&arena);
}

static void test_small_chunks_preserve_data(void) {
  TEST("allocations across many small chunks stay intact");
  Arena arena;
  ArenaInit(&arena, 64);

  void* blocks[64];
  for (size_t i = 0; i < 64; i++) {
    blocks[i] = ArenaAllocate(&arena, 24);
    ASSERT_NOT_NULL(blocks[i]);
    memset(blocks[i], (int)i, 24);
  }
  for (size_t i = 0; i < 64; i++) {
    unsigned char* data = (unsigned char*)blocks[i];
    ASSERT_EQ(data[0], (int)i);
    ASSERT_EQ(data[23], (int)i);
  }

  ArenaDestroy(&arena);
}

static void test_large_allocation_with_small_chunks(void) {
  TEST("large aligned allocation leaves room for following writes");
  Arena arena;
  ArenaInit(&arena, 64);

  const size_t size = 5000;
  unsigned char* large = (unsigned char*)ArenaAllocateAligned(&arena, size, 64);
  ASSERT_NOT_NULL(large);
  ASSERT_EQ((uintptr_t)large % 64, 0);
  memset(large, 0xA5, size);

  unsigned char* next = (unsigned char*)ArenaAllocate(&arena, 16);
  ASSERT_NOT_NULL(next);
  memset(next, 0, 16);
  ASSERT_EQ(large[0], 0xA5);
  ASSERT_EQ(large[size - 1], 0xA5);

  ArenaDestroy(&arena);
}

static void test_destroy_is_idempotent(void) {
  TEST("destroy resets arena and allows reinitialization");
  Arena arena;
  ArenaInit(&arena, 0);
  ASSERT_NOT_NULL(ArenaAllocate(&arena, 32));
  ArenaDestroy(&arena);
  ArenaDestroy(&arena);

  ArenaInit(&arena, 0);
  ASSERT_NOT_NULL(ArenaAllocate(&arena, 32));
  ArenaDestroy(&arena);
}

static void test_null_and_zero_inputs(void) {
  TEST("null and zero inputs are safe");
  Arena arena;
  ArenaInit(&arena, 0);

  void* empty = ArenaAllocate(&arena, 0);
  ASSERT_NOT_NULL(empty);
  ArenaRelease(&arena, NULL, 16);
  ArenaRelease(&arena, empty, 0);
  ArenaDestroy(NULL);
  ArenaDestroy(&arena);
}

int main(void) {
  test_begin("arena");
  RUN_TEST(test_allocate_writable_and_aligned,
           "allocate writable, aligned storage");
  RUN_TEST(test_explicit_alignment, "ArenaAllocateAligned honours alignment");
  RUN_TEST(test_release_reuses_block, "released block is reused");
  RUN_TEST(test_release_honours_new_alignment,
           "released block respects stricter alignment");
  RUN_TEST(test_small_chunks_preserve_data, "allocations span many chunks");
  RUN_TEST(test_large_allocation_with_small_chunks,
           "large allocation leaves alignment slack");
  RUN_TEST(test_destroy_is_idempotent, "destroy resets arena");
  RUN_TEST(test_null_and_zero_inputs, "null and zero inputs are safe");
  return test_end();
}
