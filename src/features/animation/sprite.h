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

/**
 * Immutable decoded sprite cell.
 * The library owns every cell and keeps the data valid for the library lifetime.
 */
typedef struct SpriteCell {
  uint32_t codepoint; /**< Unicode codepoint to render. */
  int width;          /**< Terminal display width in columns. */
  int foreground;     /**< Foreground color or SPRITE_NO_COLOR. */
  int background;     /**< Background color or SPRITE_NO_COLOR. */
  bool blank;         /**< True for a blank terminal cell. */
} SpriteCell;

/**
 * Immutable sprite row: a contiguous span of cells in the owning cell array.
 */
typedef struct SpriteRow {
  size_t cell_offset;      /**< Offset into the owning cell array. */
  size_t cell_count;       /**< Number of cells in this row. */
  const SpriteCell* cells; /**< Owning animation cell array. */
} SpriteRow;

/**
 * Immutable sprite frame metadata and offsets into row and cell arrays.
 */
typedef struct SpriteFrame {
  size_t row_offset;     /**< Offset into the owning row array. */
  size_t row_count;      /**< Number of rows in this frame. */
  size_t cell_offset;    /**< Offset into the owning cell array. */
  size_t cell_count;     /**< Number of cells in this frame. */
  const SpriteRow* rows; /**< Owning animation row array. */
  int width;             /**< Maximum row width in terminal columns. */
  int height;            /**< Number of rows. */
  double duration_ms;    /**< Frame duration in milliseconds. */
} SpriteFrame;

/**
 * Immutable animation definition shared by one or more icon types.
 */
typedef struct SpriteAnimation {
  char* name;                 /**< Animation name. */
  char* source;               /**< Source filename for diagnostics. */
  size_t source_line;         /**< Declaration line for diagnostics. */
  unsigned icon_mask;         /**< Bit mask of declared icon types. */
  SpriteFrame* frames;        /**< Frame metadata. */
  size_t frame_count;         /**< Number of frames. */
  SpriteRow* rows;            /**< Row spans. */
  size_t row_count;           /**< Number of rows. */
  SpriteCell* cells;          /**< Decoded cells. */
  size_t cell_count;          /**< Number of cells. */
  int width;                  /**< Maximum frame width. */
  int height;                 /**< Maximum frame height. */
  double default_duration_ms; /**< Default frame duration. */
  bool loop;                  /**< Loop setting. */
  int default_color;          /**< Foreground color at frame start. */
} SpriteAnimation;

/**
 * Owning collection of all animation definitions loaded from sprite files.
 */
typedef struct SpriteLibrary {
  SpriteAnimation** items; /**< Owned animation definitions. */
  size_t count;            /**< Number of definitions. */
  size_t capacity;         /**< Capacity of items. */
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
 * Free a sprite library and all animation data it owns.
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
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteCellForeground(const SpriteCell* cell);

/**
 * Get the background color of a cell.
 * @param cell Pointer to the cell.
 * @return Color index, or SPRITE_NO_COLOR.
 */
int SpriteCellBackground(const SpriteCell* cell);

/**
 * Check whether a cell is a blank terminal cell.
 * @param cell Pointer to the cell.
 * @return true when the cell must be drawn as a space.
 */
bool SpriteCellIsBlank(const SpriteCell* cell);

#endif /* SPRITE_H_ */
