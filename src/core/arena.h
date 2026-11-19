#ifndef ARENA_H_
#define ARENA_H_

#include <stddef.h>

/** Alignment applied to every arena allocation. */
#define ARENA_ALIGNMENT 16u

/** Default payload size of a new arena chunk, in bytes. */
#define ARENA_CHUNK_SIZE 4096u

/** Opaque arena chunk node. */
struct arenaChunk;

/** Opaque arena free-list node. */
struct arenaFreeBlock;

/**
 * Bump allocator with a reusable free list for released blocks.
 * Initialize with ArenaInit before first use, then release with ArenaDestroy.
 * The arena owns every allocation until ArenaDestroy; callers must not free
 * individual pointers with free().
 */
typedef struct {
  struct arenaChunk* head;            /**< Most recently allocated chunk. */
  struct arenaFreeBlock* free_blocks; /**< Reusable released allocations. */
  size_t chunk_size;                  /**< Minimum new chunk payload size. */
} Arena;

/**
 * Initialize an arena.
 * @param arena Arena to initialize.
 * @param chunk_size Minimum payload size for new chunks. Zero selects
 *                   ARENA_CHUNK_SIZE.
 */
void ArenaInit(Arena* arena, size_t chunk_size);

/**
 * Allocate storage from an arena with default alignment.
 * @param arena Arena to allocate from.
 * @param size Number of bytes to allocate.
 * @return Aligned pointer, or NULL on allocation failure.
 */
void* ArenaAllocate(Arena* arena, size_t size);

/**
 * Allocate storage from an arena with explicit alignment.
 * @param arena Arena to allocate from.
 * @param size Number of bytes to allocate.
 * @param alignment Required power-of-two alignment. Values below
 *                  ARENA_ALIGNMENT are raised to ARENA_ALIGNMENT.
 * @return Aligned pointer, or NULL on allocation failure.
 */
void* ArenaAllocateAligned(Arena* arena, size_t size, size_t alignment);

/**
 * Return an allocation to the arena's reusable free list.
 * @param arena Arena owning the allocation.
 * @param pointer Allocation to release. NULL is ignored.
 * @param size Payload size originally passed to ArenaAllocate or
 *             ArenaAllocateAligned. A block is reused only when its address
 *             also satisfies the new allocation's alignment.
 */
void ArenaRelease(Arena* arena, void* pointer, size_t size);

/**
 * Release every chunk owned by an arena and reset its state.
 * @param arena Arena to destroy. NULL is ignored.
 */
void ArenaDestroy(Arena* arena);

#endif /* ARENA_H_ */
