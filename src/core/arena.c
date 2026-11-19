/**
 * @file arena.c
 * @brief General-purpose arena allocator with reusable released blocks.
 */

#include "core/arena.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/** Memory chunk owned by an arena. */
struct arenaChunk {
  struct arenaChunk* next; /**< Next allocated chunk. */
  size_t used;             /**< Bytes used in this chunk. */
  size_t capacity;         /**< Bytes available in this chunk. */
  unsigned char data[];    /**< Aligned payload storage. */
};

/** Free block stored inside a released arena allocation. */
struct arenaFreeBlock {
  struct arenaFreeBlock* next; /**< Next free block. */
  size_t size;                 /**< Usable bytes in this block. */
};

_Static_assert(sizeof(void*) <= ARENA_ALIGNMENT,
               "arena alignment must cover pointers");

_Static_assert(sizeof(struct arenaFreeBlock) <= ARENA_ALIGNMENT,
               "free block metadata must fit arena alignment");

/* PRIVATE ARENA FUNCTIONS */
static bool checkedAddSize(size_t left, size_t right, size_t* result);
static size_t alignUpSize(size_t value, size_t alignment);
static bool arenaBlockSize(size_t size, size_t alignment, size_t* block_size);
static void* arenaAllocateImpl(Arena* arena, size_t size, size_t alignment);

/**
 * Initialize an arena with a caller-selected chunk size.
 * @param arena Arena to initialize. Caller owns the Arena value.
 * @param chunk_size Minimum payload size for new chunks. Zero selects
 *                   ARENA_CHUNK_SIZE.
 */
void ArenaInit(Arena* arena, size_t chunk_size) {
  arena->head = NULL;
  arena->free_blocks = NULL;
  arena->chunk_size = chunk_size;
}

/**
 * Allocate storage from an arena with default alignment.
 * @param arena Arena to allocate from.
 * @param size Number of bytes to allocate.
 * @return Aligned pointer owned by the arena, or NULL on allocation failure.
 */
void* ArenaAllocate(Arena* arena, size_t size) {
  return arenaAllocateImpl(arena, size, ARENA_ALIGNMENT);
}

/**
 * Add two sizes without wrapping.
 * @param left First operand.
 * @param right Second operand.
 * @param result Output: sum.
 * @return true when the sum is representable.
 */
static bool checkedAddSize(size_t left, size_t right, size_t* result) {
  if (left > SIZE_MAX - right) return false;
  *result = left + right;
  return true;
}

/**
 * Round a size up to a power-of-two alignment.
 * @param value Size to round.
 * @param alignment Power-of-two alignment.
 * @return Aligned size, or SIZE_MAX on overflow.
 */
static size_t alignUpSize(size_t value, size_t alignment) {
  size_t mask = alignment - 1;
  if (value > SIZE_MAX - mask) return SIZE_MAX;
  return (value + mask) & ~mask;
}

/**
 * Calculate the padded size of one arena allocation.
 * @param size Requested payload size.
 * @param alignment Required power-of-two alignment.
 * @param block_size Output: padded allocation size.
 * @return true when the padded size is representable.
 */
static bool arenaBlockSize(size_t size, size_t alignment, size_t* block_size) {
  size_t minimum =
    size > sizeof(struct arenaFreeBlock) ? size : sizeof(struct arenaFreeBlock);
  size_t padded = alignUpSize(minimum, alignment);
  if (padded == SIZE_MAX) return false;
  *block_size = padded;
  return true;
}

/**
 * Allocate aligned storage from an arena.
 * @param arena Arena to allocate from.
 * @param size Number of bytes to allocate.
 * @param alignment Required power-of-two alignment.
 * @return Aligned pointer owned by the arena, or NULL on allocation failure.
 */
static void* arenaAllocateImpl(Arena* arena, size_t size, size_t alignment) {
  size_t effective_alignment =
    alignment > ARENA_ALIGNMENT ? alignment : ARENA_ALIGNMENT;
  size_t span;
  if (!arenaBlockSize(size, effective_alignment, &span)) return NULL;

  struct arenaFreeBlock** link = &arena->free_blocks;
  while (*link != NULL) {
    struct arenaFreeBlock* block = *link;
    if (block->size >= span &&
        ((uintptr_t)block & ((uintptr_t)effective_alignment - 1)) == 0) {
      *link = block->next;
      return block;
    }
    link = &block->next;
  }

  for (struct arenaChunk* chunk = arena->head; chunk != NULL;
       chunk = chunk->next) {
    uintptr_t raw = (uintptr_t)(chunk->data + chunk->used);
    uintptr_t aligned = (raw + (uintptr_t)effective_alignment - 1) &
                        ~((uintptr_t)effective_alignment - 1);
    size_t offset = (size_t)(aligned - (uintptr_t)chunk->data);
    size_t end;
    if (!checkedAddSize(offset, span, &end) || end > chunk->capacity) continue;
    chunk->used = end;
    return (void*)aligned;
  }

  size_t payload;
  if (!checkedAddSize(span, effective_alignment - 1, &payload)) return NULL;
  size_t minimum_chunk =
    arena->chunk_size == 0 ? ARENA_CHUNK_SIZE : arena->chunk_size;
  if (payload < minimum_chunk) payload = minimum_chunk;
  size_t chunk_size;
  if (!checkedAddSize(sizeof(struct arenaChunk), payload, &chunk_size))
    return NULL;
  struct arenaChunk* chunk = (struct arenaChunk*)malloc(chunk_size);
  if (chunk == NULL) return NULL;
  chunk->next = arena->head;
  chunk->used = 0;
  chunk->capacity = payload;
  arena->head = chunk;
  return arenaAllocateImpl(arena, size, alignment);
}

/**
 * Allocate storage with explicit alignment.
 * @param arena Arena to allocate from.
 * @param size Number of bytes to allocate.
 * @param alignment Required power-of-two alignment. Values below
 *                  ARENA_ALIGNMENT are raised to ARENA_ALIGNMENT.
 * @return Aligned pointer owned by the arena, or NULL on allocation failure.
 */
void* ArenaAllocateAligned(Arena* arena, size_t size, size_t alignment) {
  return arenaAllocateImpl(arena, size, alignment);
}

/**
 * Return an allocation to the arena's reusable free list.
 * @param arena Arena owning the allocation.
 * @param pointer Allocation to release. NULL is ignored.
 * @param size Payload size originally passed to ArenaAllocate or
 *             ArenaAllocateAligned. A block is reused only when its address
 *             also satisfies the new allocation's alignment.
 */
void ArenaRelease(Arena* arena, void* pointer, size_t size) {
  if (pointer == NULL || size == 0) return;
  size_t span;
  if (!arenaBlockSize(size, ARENA_ALIGNMENT, &span)) return;

  struct arenaFreeBlock* block = (struct arenaFreeBlock*)pointer;
  block->size = span;
  block->next = arena->free_blocks;
  arena->free_blocks = block;
}

/**
 * Release every chunk owned by an arena and reset its state.
 * @param arena Arena to destroy. NULL is ignored. The Arena value itself is
 *              caller-owned and remains valid for reuse after ArenaInit.
 */
void ArenaDestroy(Arena* arena) {
  if (arena == NULL) return;
  struct arenaChunk* chunk = arena->head;
  while (chunk != NULL) {
    struct arenaChunk* next = chunk->next;
    free(chunk);
    chunk = next;
  }
  arena->head = NULL;
  arena->free_blocks = NULL;
}
