/**
 * @file anim.c
 * @brief Mutable rollfilm playback and ncurses rendering.
 */

#include "features/animation/anim.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/error.h"
#include "core/util.h"
#include "features/animation/sprite.h"

/** Number of bytes reserved for one batched UTF-8 output run. */
#define RENDER_BATCH_SIZE 256u

_Static_assert(sizeof(uint32_t) == 4, "uint32_t must occupy four bytes");
_Static_assert(SPRITE_NO_COLOR >= INT16_MIN && SPRITE_NO_COLOR <= INT16_MAX,
               "SPRITE_NO_COLOR must fit in int16_t");
_Static_assert(SPRITE_MAX_COLOR <= INT16_MAX,
               "SPRITE_MAX_COLOR must fit in int16_t");

/** Mutable playback state for one sprite animation. */
struct Rollfilm {
  const SpriteAnimation* animation; /**< Immutable shared sprite data. */
  uint32_t current_frame;           /**< Index of the displayed frame. */
  uint32_t default_frame;           /**< Frame used for static display. */
  bool loop;                        /**< Mutable playback override. */
  double last_tick_ms;              /**< Time of the last frame transition. */
};

/* PRIVATE ANIM FUNCTIONS */
static uint32_t frameCount(const Rollfilm* rollfilm);
static const SpriteFrame* currentFrame(const Rollfilm* rollfilm);
static int16_t resolveForeground(const Rollfilm* rollfilm, int16_t foreground);
static size_t encodeUtf8(uint32_t codepoint, char output[5]);
static bool findBlank(const Rollfilm* rollfilm, bool last, int* out_x,
                      int* out_y);
static void flushRenderBatch(int y, int x, char* batch, size_t* length);
static double timeToWrap(const Rollfilm* rollfilm, double cycle_duration_ms);

/**
 * Get the frame count as an unsigned index limit.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame count, or zero when unavailable.
 */
static uint32_t frameCount(const Rollfilm* rollfilm) {
  if (rollfilm == NULL || rollfilm->animation == NULL) return 0;
  size_t count = SpriteAnimationFrameCount(rollfilm->animation);
  return count > UINT32_MAX ? 0 : (uint32_t)count;
}

/**
 * Get the current frame metadata.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame, or NULL when unavailable.
 */
static const SpriteFrame* currentFrame(const Rollfilm* rollfilm) {
  if (rollfilm == NULL) return NULL;
  return SpriteAnimationFrame(rollfilm->animation,
                              (size_t)rollfilm->current_frame);
}

/**
 * Resolve a cell color to a value ncurses can use.
 * @param rollfilm Pointer to the rollfilm.
 * @param foreground Cell foreground color or SPRITE_NO_COLOR.
 * @return Effective foreground color.
 */
static int16_t resolveForeground(const Rollfilm* rollfilm, int16_t foreground) {
  if (foreground != SPRITE_NO_COLOR) return foreground;
  int configured = SpriteAnimationDefaultColor(rollfilm->animation);
  if (configured < 0 || configured > SPRITE_MAX_COLOR) return 7;
  return (int16_t)configured;
}

/**
 * Encode a Unicode codepoint as UTF-8.
 * Codepoint zero writes no bytes and preserves the original empty output.
 * @param codepoint Codepoint to encode.
 * @param output Output buffer of at least five bytes.
 * @return Number of bytes written.
 */
static size_t encodeUtf8(uint32_t codepoint, char output[5]) {
  if (codepoint == 0) return 0;
  if (codepoint > 0x10FFFFu || (codepoint >= 0xD800u && codepoint <= 0xDFFFu))
    codepoint = '?';

  if (codepoint < 0x80u) {
    output[0] = (char)codepoint;
    return 1;
  }
  if (codepoint < 0x800u) {
    output[0] = (char)(0xC0u | (codepoint >> 6));
    output[1] = (char)(0x80u | (codepoint & 0x3Fu));
    return 2;
  }
  if (codepoint < 0x10000u) {
    output[0] = (char)(0xE0u | (codepoint >> 12));
    output[1] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
    output[2] = (char)(0x80u | (codepoint & 0x3Fu));
    return 3;
  }

  output[0] = (char)(0xF0u | (codepoint >> 18));
  output[1] = (char)(0x80u | ((codepoint >> 12) & 0x3Fu));
  output[2] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
  output[3] = (char)(0x80u | (codepoint & 0x3Fu));
  return 4;
}

/**
 * Find a blank cell in the last frame.
 * @param rollfilm Pointer to the rollfilm.
 * @param last true to return the last blank run end, false for the first cell.
 * @param out_x Output: x coordinate.
 * @param out_y Output: y coordinate.
 * @return true when a blank cell was found.
 */
static bool findBlank(const Rollfilm* rollfilm, bool last, int* out_x,
                      int* out_y) {
  if (rollfilm == NULL || out_x == NULL || out_y == NULL) return false;
  uint32_t count = frameCount(rollfilm);
  if (count == 0) return false;

  const SpriteFrame* frame =
    SpriteAnimationFrame(rollfilm->animation, (size_t)(count - 1));
  if (frame == NULL) return false;

  bool found = false;

  for (size_t row_index = 0; row_index < SpriteFrameRowCount(frame);
       row_index++) {
    const SpriteRow* row = SpriteFrameRow(frame, row_index);
    if (row == NULL) continue;

    int32_t x = 0;
    for (size_t cell_index = 0; cell_index < SpriteRowCellCount(row);
         cell_index++) {
      const SpriteCell* cell = SpriteRowCell(row, cell_index);
      if (cell == NULL) continue;
      if (SpriteCellIsBlank(cell)) {
        *out_x = last ? x + SpriteCellWidth(cell) : x;
        *out_y = (int)row_index;
        if (!last) return true;
        found = true;
      }
      x += SpriteCellWidth(cell);
    }
  }
  return found;
}

/**
 * Write a complete batched UTF-8 run.
 * @param y Terminal row.
 * @param x Terminal column.
 * @param batch Buffer holding the UTF-8 bytes.
 * @param length Pointer to the current byte count. Reset to zero on return.
 */
static void flushRenderBatch(int y, int x, char* batch, size_t* length) {
  if (*length == 0) return;
  batch[*length] = '\0';
  mvprintw(y, x, "%s", batch);
  *length = 0;
}

/**
 * Sum durations from the current frame through the end of the cycle.
 * @param rollfilm Pointer to the rollfilm.
 * @param cycle_duration_ms Cached total cycle duration.
 * @return Time before playback wraps to frame zero.
 */
static double timeToWrap(const Rollfilm* rollfilm, double cycle_duration_ms) {
  uint32_t count = frameCount(rollfilm);
  if (count == 0) return 0.0;

  double remaining = 0.0;
  for (uint32_t index = rollfilm->current_frame; index < count; index++) {
    const SpriteFrame* frame =
      SpriteAnimationFrame(rollfilm->animation, (size_t)index);
    double duration = SpriteFrameDurationMS(frame);
    if (!isfinite(duration) || duration <= 0.0 ||
        remaining > cycle_duration_ms - duration)
      return 0.0;
    remaining += duration;
  }
  return remaining;
}

/**
 * Create mutable playback state for a loaded sprite animation.
 * @param library Owning sprite library.
 * @param name Animation name.
 * @param icon_type Icon type index (0=nerd-icons, 1=emojis, 2=ascii).
 * @return New rollfilm, or NULL when not found or allocation fails.
 */
Rollfilm* CreateRollfilm(const SpriteLibrary* library, const char* name,
                         int icon_type) {
  const SpriteAnimation* animation =
    SpriteLibraryFind(library, name, icon_type);
  if (animation == NULL) return NULL;

  Rollfilm* rollfilm = (Rollfilm*)malloc(sizeof(*rollfilm));
  if (rollfilm == NULL) {
    LogError("CreateRollfilm", MALLOC_ERROR);
    return NULL;
  }

  rollfilm->animation = animation;
  rollfilm->current_frame = 0;
  rollfilm->default_frame = 0;
  rollfilm->loop = SpriteAnimationLoop(animation);
  rollfilm->last_tick_ms = GetCurrentTimeMS();
  return rollfilm;
}

/**
 * Free playback state. Sprite data is owned by the library.
 * @param rollfilm Pointer to the rollfilm to free.
 */
void FreeRollfilm(Rollfilm* rollfilm) { free(rollfilm); }

/**
 * Create several rollfilms in one allocation.
 * @param library Owning sprite library.
 * @param definitions Definitions for each requested rollfilm.
 * @param count Number of definitions.
 * @param out Output array receiving count rollfilm pointers.
 * @return Owning storage handle, or NULL on failure.
 */
Rollfilm* CreateRollfilms(const SpriteLibrary* library,
                          const RollfilmDefinition* definitions, size_t count,
                          Rollfilm** out) {
  if (definitions == NULL || out == NULL || count == 0) return NULL;
  for (size_t i = 0; i < count; i++) out[i] = NULL;

  if (count > SIZE_MAX / sizeof(Rollfilm)) {
    LogError("CreateRollfilms", MALLOC_ERROR);
    return NULL;
  }

  Rollfilm* storage = (Rollfilm*)malloc(count * sizeof(*storage));
  if (storage == NULL) {
    LogError("CreateRollfilms", MALLOC_ERROR);
    return NULL;
  }

  for (size_t i = 0; i < count; i++) {
    const SpriteAnimation* animation =
      SpriteLibraryFind(library, definitions[i].name, definitions[i].icon_type);
    if (animation == NULL) {
      free(storage);
      for (size_t j = 0; j < count; j++) out[j] = NULL;
      return NULL;
    }

    storage[i].animation = animation;
    storage[i].current_frame = 0;
    storage[i].default_frame = 0;
    storage[i].loop = SpriteAnimationLoop(animation);
    storage[i].last_tick_ms = GetCurrentTimeMS();
    out[i] = &storage[i];
  }
  return storage;
}

/**
 * Free one rollfilm batch and clear its pointer array.
 * @param storage Storage handle returned by CreateRollfilms.
 * @param out Pointer array filled by CreateRollfilms.
 * @param count Number of pointers in out.
 */
void FreeRollfilms(Rollfilm* storage, Rollfilm** out, size_t count) {
  if (out != NULL)
    for (size_t i = 0; i < count; i++) out[i] = NULL;
  free(storage);
}

/**
 * Set the loop override for selected rollfilms.
 * @param film Array of rollfilm pointers.
 * @param indices Indices to update.
 * @param count Number of indices.
 * @param loop true to enable looping.
 */
void SetRollfilmLoop(Rollfilm** film, const int* indices, size_t count,
                     bool loop) {
  if (film == NULL || indices == NULL) return;
  for (size_t i = 0; i < count; i++) {
    int32_t index = indices[i];
    if (index < 0 || film[index] == NULL) continue;
    film[index]->loop = loop;
  }
}

/**
 * Advance a rollfilm using accumulated wall-clock time.
 * Complete looping cycles are skipped without visiting every frame.
 * @param rollfilm Pointer to the rollfilm to update.
 */
void UpdateRollfilm(Rollfilm* rollfilm) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  if (frame == NULL) return;

  uint32_t count = frameCount(rollfilm);
  if (count == 0) return;

  double now = GetCurrentTimeMS();
  double elapsed = now - rollfilm->last_tick_ms;
  if (elapsed <= 0.0) return;

  double cycle_duration_ms =
    SpriteAnimationCycleDurationMS(rollfilm->animation);
  double until_wrap =
    rollfilm->loop ? timeToWrap(rollfilm, cycle_duration_ms) : 0.0;

  while (elapsed > 0.0) {
    frame = currentFrame(rollfilm);
    if (frame == NULL) return;

    if (!rollfilm->loop && rollfilm->current_frame >= count - 1) {
      rollfilm->last_tick_ms = now;
      return;
    }

    double duration = SpriteFrameDurationMS(frame);
    if (duration <= 0.0 || elapsed < duration) return;

    if (rollfilm->loop && until_wrap > 0.0 && cycle_duration_ms > 0.0 &&
        elapsed >= until_wrap) {
      elapsed -= until_wrap;
      rollfilm->last_tick_ms += until_wrap;
      rollfilm->current_frame = 0;
      until_wrap = cycle_duration_ms;

      if (elapsed >= cycle_duration_ms) {
        double skipped_cycles = floor(elapsed / cycle_duration_ms);
        double skipped_ms = skipped_cycles * cycle_duration_ms;
        elapsed -= skipped_ms;
        rollfilm->last_tick_ms += skipped_ms;
      }
      continue;
    }

    elapsed -= duration;
    rollfilm->last_tick_ms += duration;

    if (!rollfilm->loop) {
      rollfilm->current_frame++;
      continue;
    }

    if (rollfilm->current_frame + 1 >= count) {
      rollfilm->current_frame = 0;
      until_wrap = cycle_duration_ms;
    } else {
      rollfilm->current_frame++;
      until_wrap -= duration;
    }
  }
}

/**
 * Advance one frame when the current frame duration has elapsed.
 * Movement is clamped to the animation bounds.
 * @param rollfilm Pointer to the rollfilm.
 * @param direction Positive for forward, negative for backward.
 * @return true when the frame changed.
 */
bool RollfilmTryAdvance(Rollfilm* rollfilm, int direction) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  if (frame == NULL) return false;

  double now = GetCurrentTimeMS();
  double duration = SpriteFrameDurationMS(frame);
  if (duration <= 0.0 || now - rollfilm->last_tick_ms < duration) return false;

  uint32_t count = frameCount(rollfilm);
  if (count == 0) return false;

  int32_t step = direction < 0 ? -1 : 1;
  int32_t target = (int32_t)rollfilm->current_frame + step;

  if (target < 0 || (uint32_t)target >= count) {
    if (!rollfilm->loop) {
      rollfilm->last_tick_ms = now;
      return false;
    }
    target = (target + (int32_t)count) % (int32_t)count;
  }

  RollfilmSeekFrame(rollfilm, target);
  return true;
}

/**
 * Seek to a frame and reset the frame clock.
 * @param rollfilm Pointer to the rollfilm.
 * @param frame_index Target frame index.
 */
void RollfilmSeekFrame(Rollfilm* rollfilm, int frame_index) {
  if (rollfilm == NULL || rollfilm->animation == NULL) return;
  uint32_t count = frameCount(rollfilm);
  if (count == 0 || frame_index < 0 || (uint32_t)frame_index >= count) return;
  rollfilm->current_frame = (uint32_t)frame_index;
  rollfilm->last_tick_ms = GetCurrentTimeMS();
}

/**
 * Set the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 * @param frame_index Target frame index.
 */
void RollfilmSetDefaultFrame(Rollfilm* rollfilm, int frame_index) {
  if (rollfilm == NULL) return;
  uint32_t count = frameCount(rollfilm);
  if (count == 0) {
    rollfilm->default_frame = 0;
    return;
  }
  if (frame_index < 0)
    rollfilm->default_frame = 0;
  else if ((uint32_t)frame_index >= count)
    rollfilm->default_frame = count - 1;
  else
    rollfilm->default_frame = (uint32_t)frame_index;
}

/**
 * Seek to the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 */
void RollfilmSeekDefault(Rollfilm* rollfilm) {
  if (rollfilm == NULL) return;
  RollfilmSeekFrame(rollfilm, (int)rollfilm->default_frame);
}

/**
 * Render the current frame at the given screen position.
 * Consecutive nonblank cells with equal attributes are written as one run.
 * @param rollfilm Pointer to the rollfilm to render.
 * @param start_y Starting terminal row.
 * @param start_x Starting terminal column.
 */
void RenderRollfilm(const Rollfilm* rollfilm, int start_y, int start_x) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  if (frame == NULL) return;

  int16_t active_foreground = SPRITE_NO_COLOR;
  int16_t active_background = SPRITE_NO_COLOR;
  int32_t y = start_y;
  int32_t batch_x = start_x;
  int32_t batch_y = start_y;
  char batch[RENDER_BATCH_SIZE + 4u];
  size_t batch_length = 0;

  for (size_t row_index = 0; row_index < SpriteFrameRowCount(frame);
       row_index++) {
    const SpriteRow* row = SpriteFrameRow(frame, row_index);
    if (row == NULL) {
      flushRenderBatch((int)batch_y, (int)batch_x, batch, &batch_length);
      y++;
      batch_x = start_x;
      batch_y = y;
      continue;
    }

    int32_t x = start_x;
    batch_x = x;
    batch_y = y;
    for (size_t cell_index = 0; cell_index < SpriteRowCellCount(row);
         cell_index++) {
      const SpriteCell* cell = SpriteRowCell(row, cell_index);
      if (cell == NULL) continue;

      int16_t foreground =
        resolveForeground(rollfilm, (int16_t)SpriteCellForeground(cell));
      int16_t background = (int16_t)SpriteCellBackground(cell);
      if (foreground != active_foreground || background != active_background) {
        flushRenderBatch((int)batch_y, (int)batch_x, batch, &batch_length);
        SetColor((short)foreground, (short)background, A_BOLD);
        active_foreground = foreground;
        active_background = background;
        batch_x = x;
        batch_y = y;
      }

      if (SpriteCellIsBlank(cell)) {
        flushRenderBatch((int)batch_y, (int)batch_x, batch, &batch_length);
        x += SpriteCellWidth(cell);
        batch_x = x;
        continue;
      }

      char utf8[5];
      size_t length = encodeUtf8(SpriteCellCodepoint(cell), utf8);
      if (batch_length + length > RENDER_BATCH_SIZE) {
        flushRenderBatch((int)batch_y, (int)batch_x, batch, &batch_length);
        batch_x = x;
      }
      memcpy(batch + batch_length, utf8, length);
      batch_length += length;
      x += SpriteCellWidth(cell);
    }
    flushRenderBatch((int)batch_y, (int)batch_x, batch, &batch_length);
    y++;
    batch_x = start_x;
    batch_y = y;
  }
}

/**
 * Get the number of frames.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame count.
 */
int RollfilmFrameCount(const Rollfilm* rollfilm) {
  return (int)frameCount(rollfilm);
}

/**
 * Get the current frame index.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame index.
 */
int RollfilmFrameIndex(const Rollfilm* rollfilm) {
  return rollfilm == NULL ? 0 : (int)rollfilm->current_frame;
}

/**
 * Get the cached animation width in terminal columns.
 * @param rollfilm Pointer to the rollfilm.
 * @return Maximum frame width.
 */
int RollfilmWidth(const Rollfilm* rollfilm) {
  return rollfilm == NULL ? 0 : SpriteAnimationWidth(rollfilm->animation);
}

/**
 * Get the cached animation height in rows.
 * @param rollfilm Pointer to the rollfilm.
 * @return Maximum frame height.
 */
int RollfilmHeight(const Rollfilm* rollfilm) {
  return rollfilm == NULL ? 0 : SpriteAnimationHeight(rollfilm->animation);
}

/**
 * Get the current frame width in terminal columns.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame width.
 */
int RollfilmFrameWidth(const Rollfilm* rollfilm) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  return SpriteFrameWidth(frame);
}

/**
 * Get the current frame height in rows.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame height.
 */
int RollfilmFrameHeight(const Rollfilm* rollfilm) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  return SpriteFrameHeight(frame);
}

/**
 * Get the current frame duration in milliseconds.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame duration.
 */
double RollfilmFrameDurationMS(const Rollfilm* rollfilm) {
  return SpriteFrameDurationMS(currentFrame(rollfilm));
}

/**
 * Find the first blank cell in the last frame.
 * @param rollfilm Pointer to the rollfilm.
 * @param out_x Output: terminal column relative to the frame.
 * @param out_y Output: row relative to the frame.
 * @return true when found.
 */
bool RollfilmFirstBlank(const Rollfilm* rollfilm, int* out_x, int* out_y) {
  return findBlank(rollfilm, false, out_x, out_y);
}

/**
 * Find the end of the last blank cell run in the last frame.
 * @param rollfilm Pointer to the rollfilm.
 * @param out_x Output: column one past the blank run.
 * @param out_y Output: row containing the blank run.
 * @return true when found.
 */
bool RollfilmLastBlank(const Rollfilm* rollfilm, int* out_x, int* out_y) {
  return findBlank(rollfilm, true, out_x, out_y);
}

/**
 * Find the largest rollfilm among selected indices.
 * Height takes priority over width.
 * @param animations Array of rollfilm pointers.
 * @param indices Indices to compare.
 * @param indices_count Number of indices.
 * @return Index of the largest rollfilm, or -1 when none is valid.
 */
int RollfilmLargest(Rollfilm* const animations[], const int* indices,
                    int indices_count) {
  if (animations == NULL || indices == NULL || indices_count <= 0) return -1;

  int32_t best_index = -1;
  int32_t best_width = -1;
  int32_t best_height = -1;

  for (int32_t i = 0; i < indices_count; i++) {
    int32_t index = indices[i];
    if (index < 0) continue;
    const Rollfilm* rollfilm = animations[index];
    if (rollfilm == NULL) continue;

    int32_t width = RollfilmWidth(rollfilm);
    int32_t height = RollfilmHeight(rollfilm);
    if (height > best_height || (height == best_height && width > best_width)) {
      best_index = index;
      best_width = width;
      best_height = height;
    }
  }
  return (int)best_index;
}
