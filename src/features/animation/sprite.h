/**
 * @file sprite.h
 * @brief Immutable sprite data types and sprite library API.
 */

#ifndef SPRITE_H_
#define SPRITE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/error.h"

/** Sentinel used when no terminal color is set. */
#define SPRITE_NO_COLOR (-1)

/** Maximum accepted color index in sprite escapes. */
#define SPRITE_MAX_COLOR 255

/** Maximum accepted blank-run length in a sprite escape. */
#define SPRITE_MAX_BLANK_RUN 1024

/** Number of supported icon types. */
#define SPRITE_ICON_TYPE_COUNT 3

/** Cell flag: transparent blank cell. */
#define SPRITE_CELL_FLAG_BLANK UINT8_C(0x01)

/** Cell flag: foreground color was declared explicitly. */
#define SPRITE_CELL_FLAG_FOREGROUND UINT8_C(0x02)

/** Cell flag: background color was declared explicitly. */
#define SPRITE_CELL_FLAG_BACKGROUND UINT8_C(0x04)

/** Animation flag: playback repeats. */
#define SPRITE_ANIMATION_FLAG_LOOP UINT8_C(0x01)

_Static_assert(sizeof(uint8_t) == 1, "uint8_t must occupy one byte");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t must occupy two bytes");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must occupy four bytes");
_Static_assert(SPRITE_NO_COLOR >= INT16_MIN && SPRITE_NO_COLOR <= INT16_MAX,
               "SPRITE_NO_COLOR must fit in int16_t");
_Static_assert(SPRITE_MAX_COLOR <= UINT8_MAX,
               "SPRITE_MAX_COLOR must fit in uint8_t");
_Static_assert(SPRITE_MAX_BLANK_RUN <= UINT16_MAX,
               "SPRITE_MAX_BLANK_RUN must fit in uint16_t");

/**
 * Immutable decoded sprite cell.
 * The library owns every cell and keeps the data valid for the library lifetime.
 */
typedef struct SpriteCell {
  uint32_t codepoint; /**< Unicode codepoint to render. */
  uint8_t foreground; /**< Explicit foreground color, or zero when unset. */
  uint8_t background; /**< Explicit background color, or zero when unset. */
  uint8_t width;      /**< Terminal display width in columns. */
  uint8_t flags;      /**< Bit mask of SPRITE_CELL_FLAG_* values. */
} SpriteCell;

_Static_assert(sizeof(SpriteCell) == 8, "SpriteCell must occupy 8 bytes");

/**
 * Immutable sprite row: a contiguous span of cells in the owning cell array.
 * The cells pointer is resolved once at finalization; the fixed accessor
 * signature has no animation context from which to derive it.
 */
typedef struct SpriteRow {
  const SpriteCell* cells; /**< Owning animation cell array. */
  uint32_t cell_offset;    /**< First cell in this row. */
  uint32_t cell_count;     /**< Number of cells in this row. */
} SpriteRow;

/**
 * Immutable sprite frame metadata and offsets into the owning row array.
 * The rows pointer is resolved once at finalization for the fixed accessor API.
 */
typedef struct SpriteFrame {
  const SpriteRow* rows; /**< Owning animation row array. */
  uint32_t row_offset;   /**< First row in this frame. */
  uint32_t row_count;    /**< Number of rows in this frame. */
  uint16_t width;        /**< Maximum row width in terminal columns. */
  uint16_t height;       /**< Number of rows. */
  double duration_ms;    /**< Frame duration in milliseconds. */
} SpriteFrame;

/**
 * Immutable animation definition shared by one or more icon types.
 * All offsets are relative to the owning library allocation base.
 */
typedef struct SpriteAnimation {
  const void* base;       /**< Start of the owning library allocation. */
  uint32_t name_offset;   /**< Offset of the name in the string pool. */
  uint32_t source_offset; /**< Offset of the source filename in the pool. */
  uint32_t frame_offset;  /**< Offset of this animation's frame array. */
  uint32_t frame_count;   /**< Number of frames. */
  uint32_t row_offset;    /**< Offset of this animation's row array. */
  uint32_t row_count;     /**< Number of rows. */
  uint32_t cell_offset;   /**< Offset of this animation's cell array. */
  uint32_t cell_count;    /**< Number of cells. */
  uint32_t source_line;   /**< Declaration line for diagnostics. */
  uint16_t width;         /**< Maximum frame width. */
  uint16_t height;        /**< Maximum frame height. */
  uint8_t icon_mask;      /**< Bit mask of declared icon types. */
  uint8_t flags;          /**< Bit mask of SPRITE_ANIMATION_FLAG_* values. */
  int16_t default_color;  /**< Frame-start color, or SPRITE_NO_COLOR. */
  double default_duration_ms; /**< Default frame duration. */
  double cycle_duration_ms;   /**< Sum of all frame durations. */
} SpriteAnimation;

/**
 * Owning collection of all animation definitions loaded from sprite files.
 * The header and every referenced array live in one allocation.
 */
typedef struct SpriteLibrary {
  SpriteAnimation* items; /**< Animations stored in the same allocation. */
  uint32_t count;         /**< Number of definitions. */
  uint32_t string_offset; /**< Offset of the string pool. */
  uint32_t storage_size;  /**< Total size of the allocation. */
} SpriteLibrary;

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
                            SpriteLibrary** library);

/**
 * Free a sprite library and its single backing allocation.
 * Every Rollfilm created from the library must be freed first.
 * @param library Pointer to the library to free.
 */
void FreeSpriteLibrary(SpriteLibrary* library);

/**
 * Find an animation definition for a name and icon type.
 * @param library Pointer to the library to search.
 * @param name Animation name without the [animation.] prefix.
 * @param icon_type Icon type index (0=nerd-icons, 1=emojis, 2=ascii).
 * @return Immutable definition, or NULL when not found.
 */
const SpriteAnimation* SpriteLibraryFind(const SpriteLibrary* library,
                                         const char* name, int icon_type);

/**
 * Get an animation name.
 * @param animation Pointer to the animation definition.
 * @return Name string owned by the library.
 */
const char* SpriteAnimationName(const SpriteAnimation* animation);

/**
 * Get an animation frame.
 * @param animation Pointer to the animation definition.
 * @param index Frame index.
 * @return Immutable frame, or NULL when index is out of range.
 */
const SpriteFrame* SpriteAnimationFrame(const SpriteAnimation* animation,
                                        size_t index);

/**
 * Get the number of frames in an animation.
 * @param animation Pointer to the animation definition.
 * @return Frame count.
 */
size_t SpriteAnimationFrameCount(const SpriteAnimation* animation);

/**
 * Get the cached animation width in terminal columns.
 * @param animation Pointer to the animation definition.
 * @return Maximum frame width.
 */
int SpriteAnimationWidth(const SpriteAnimation* animation);

/**
 * Get the cached animation height in rows.
 * @param animation Pointer to the animation definition.
 * @return Maximum frame height.
 */
int SpriteAnimationHeight(const SpriteAnimation* animation);

/**
 * Get the configured default duration for frames in milliseconds.
 * @param animation Pointer to the animation definition.
 * @return Default frame duration.
 */
double SpriteAnimationDefaultDurationMS(const SpriteAnimation* animation);

/**
 * Get the cached total duration of one animation cycle.
 * @param animation Pointer to the animation definition.
 * @return Sum of all frame durations, or zero when unavailable.
 */
double SpriteAnimationCycleDurationMS(const SpriteAnimation* animation);

/**
 * Get the configured loop flag.
 * @param animation Pointer to the animation definition.
 * @return true when playback repeats.
 */
bool SpriteAnimationLoop(const SpriteAnimation* animation);

/**
 * Get the default foreground color applied at frame start.
 * @param animation Pointer to the animation definition.
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteAnimationDefaultColor(const SpriteAnimation* animation);

/**
 * Get a frame row.
 * @param frame Pointer to the frame.
 * @param index Row index.
 * @return Immutable row, or NULL when index is out of range.
 */
const SpriteRow* SpriteFrameRow(const SpriteFrame* frame, size_t index);

/**
 * Get the number of rows in a frame.
 * @param frame Pointer to the frame.
 * @return Row count.
 */
size_t SpriteFrameRowCount(const SpriteFrame* frame);

/**
 * Get the cached frame width in terminal columns.
 * @param frame Pointer to the frame.
 * @return Frame width.
 */
int SpriteFrameWidth(const SpriteFrame* frame);

/**
 * Get the frame height in rows.
 * @param frame Pointer to the frame.
 * @return Frame height.
 */
int SpriteFrameHeight(const SpriteFrame* frame);

/**
 * Get the frame duration in milliseconds.
 * @param frame Pointer to the frame.
 * @return Frame duration.
 */
double SpriteFrameDurationMS(const SpriteFrame* frame);

/**
 * Get a row cell.
 * @param row Pointer to the row.
 * @param index Cell index.
 * @return Immutable cell, or NULL when index is out of range.
 */
const SpriteCell* SpriteRowCell(const SpriteRow* row, size_t index);

/**
 * Get the number of cells in a row.
 * @param row Pointer to the row.
 * @return Cell count.
 */
size_t SpriteRowCellCount(const SpriteRow* row);

/**
 * Get the codepoint stored in a cell.
 * @param cell Pointer to the cell.
 * @return Unicode codepoint.
 */
uint32_t SpriteCellCodepoint(const SpriteCell* cell);

/**
 * Get the terminal display width of a cell.
 * @param cell Pointer to the cell.
 * @return Number of terminal columns (0 for combining marks).
 */
int SpriteCellWidth(const SpriteCell* cell);

/**
 * Get the foreground color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR when not declared.
 */
int SpriteCellForeground(const SpriteCell* cell);

/**
 * Get the background color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR when not declared.
 */
int SpriteCellBackground(const SpriteCell* cell);

/**
 * Check whether a cell is a transparent blank terminal cell.
 * @param cell Pointer to the cell.
 * @return true when the renderer advances the column without writing.
 */
bool SpriteCellIsBlank(const SpriteCell* cell);

#endif /* SPRITE_H_ */
