/**
 * @file anim.c
 * @brief Mutable rollfilm playback and ncurses rendering.
 */

#include "features/animation/anim.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/error.h"
#include "core/util.h"
#include "features/animation/sprite.h"

/** Mutable playback state for one sprite animation. */
struct Rollfilm {
  const SpriteAnimation* animation; /**< Immutable shared sprite data. */
  int current_frame;                /**< Index of the displayed frame. */
  int default_frame;                /**< Frame used for static display. */
  bool loop;                        /**< Mutable playback override. */
  double last_tick_ms;              /**< Time of the last frame transition. */
};

/* PRIVATE ANIM FUNCTIONS */
static const SpriteFrame* currentFrame(const Rollfilm* rollfilm);
static int resolveForeground(const Rollfilm* rollfilm, int foreground);
static size_t encodeUtf8(uint32_t codepoint, char output[5]);
static bool findBlank(const Rollfilm* rollfilm, bool last, int* out_x,
                      int* out_y);

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
 * @param foreground Cell foreground color.
 * @return Effective foreground color.
 */
static int resolveForeground(const Rollfilm* rollfilm, int foreground) {
  if (foreground != SPRITE_NO_COLOR) return foreground;
  int configured = SpriteAnimationDefaultColor(rollfilm->animation);
  return configured == SPRITE_NO_COLOR ? 7 : configured;
}

/**
 * Encode a Unicode codepoint as UTF-8.
 * @param codepoint Codepoint to encode.
 * @param output Output buffer of at least five bytes.
 * @return Number of bytes written.
 */
static size_t encodeUtf8(uint32_t codepoint, char output[5]) {
  if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
    codepoint = '?';

  if (codepoint < 0x80) {
    output[0] = (char)codepoint;
    return 1;
  }
  if (codepoint < 0x800) {
    output[0] = (char)(0xC0 | (codepoint >> 6));
    output[1] = (char)(0x80 | (codepoint & 0x3F));
    return 2;
  }
  if (codepoint < 0x10000) {
    output[0] = (char)(0xE0 | (codepoint >> 12));
    output[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    output[2] = (char)(0x80 | (codepoint & 0x3F));
    return 3;
  }

  output[0] = (char)(0xF0 | (codepoint >> 18));
  output[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
  output[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
  output[3] = (char)(0x80 | (codepoint & 0x3F));
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
  int frame_count = RollfilmFrameCount(rollfilm);
  if (frame_count <= 0) return false;

  const SpriteFrame* frame =
    SpriteAnimationFrame(rollfilm->animation, (size_t)(frame_count - 1));
  if (frame == NULL) return false;

  bool found = false;

  for (size_t row_index = 0; row_index < SpriteFrameRowCount(frame);
       row_index++) {
    const SpriteRow* row = SpriteFrameRow(frame, row_index);
    if (row == NULL) continue;

    int x = 0;
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
    int index = indices[i];
    if (index < 0 || film[index] == NULL) continue;
    film[index]->loop = loop;
  }
}

/**
 * Advance a rollfilm using accumulated wall-clock time.
 * Multiple frames are consumed when enough time has elapsed.
 * @param rollfilm Pointer to the rollfilm to update.
 */
void UpdateRollfilm(Rollfilm* rollfilm) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  if (frame == NULL) return;

  int frame_count = RollfilmFrameCount(rollfilm);
  double now = GetCurrentTimeMS();
  double elapsed = now - rollfilm->last_tick_ms;
  if (elapsed <= 0.0) return;

  while (elapsed > 0.0) {
    frame = currentFrame(rollfilm);
    if (frame == NULL) return;

    if (!rollfilm->loop && rollfilm->current_frame >= frame_count - 1) {
      rollfilm->last_tick_ms = now;
      return;
    }

    double duration = SpriteFrameDurationMS(frame);
    if (duration <= 0.0 || elapsed < duration) return;

    elapsed -= duration;
    rollfilm->current_frame = rollfilm->loop
                                ? (rollfilm->current_frame + 1) % frame_count
                                : rollfilm->current_frame + 1;
    rollfilm->last_tick_ms += duration;
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

  int frame_count = RollfilmFrameCount(rollfilm);
  int step = direction < 0 ? -1 : 1;
  int target = rollfilm->current_frame + step;

  if (target < 0 || target >= frame_count) {
    if (!rollfilm->loop) {
      rollfilm->last_tick_ms = now;
      return false;
    }
    target = (target + frame_count) % frame_count;
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
  int frame_count = RollfilmFrameCount(rollfilm);
  if (frame_count <= 0 || frame_index < 0 || frame_index >= frame_count) return;
  rollfilm->current_frame = frame_index;
  rollfilm->last_tick_ms = GetCurrentTimeMS();
}

/**
 * Set the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 * @param frame_index Target frame index.
 */
void RollfilmSetDefaultFrame(Rollfilm* rollfilm, int frame_index) {
  if (rollfilm == NULL) return;
  int frame_count = RollfilmFrameCount(rollfilm);
  if (frame_count <= 0) {
    rollfilm->default_frame = 0;
    return;
  }
  if (frame_index < 0)
    rollfilm->default_frame = 0;
  else if (frame_index >= frame_count)
    rollfilm->default_frame = frame_count - 1;
  else
    rollfilm->default_frame = frame_index;
}

/**
 * Seek to the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 */
void RollfilmSeekDefault(Rollfilm* rollfilm) {
  if (rollfilm == NULL) return;
  RollfilmSeekFrame(rollfilm, rollfilm->default_frame);
}

/**
 * Render the current frame at the given screen position.
 * @param rollfilm Pointer to the rollfilm to render.
 * @param start_y Starting terminal row.
 * @param start_x Starting terminal column.
 */
void RenderRollfilm(const Rollfilm* rollfilm, int start_y, int start_x) {
  const SpriteFrame* frame = currentFrame(rollfilm);
  if (frame == NULL) return;

  int active_foreground = SPRITE_NO_COLOR;
  int active_background = SPRITE_NO_COLOR;
  int y = start_y;

  for (size_t row_index = 0; row_index < SpriteFrameRowCount(frame);
       row_index++) {
    const SpriteRow* row = SpriteFrameRow(frame, row_index);
    if (row == NULL) {
      y++;
      continue;
    }

    int x = start_x;
    for (size_t cell_index = 0; cell_index < SpriteRowCellCount(row);
         cell_index++) {
      const SpriteCell* cell = SpriteRowCell(row, cell_index);
      if (cell == NULL) continue;

      int foreground = resolveForeground(rollfilm, SpriteCellForeground(cell));
      int background = SpriteCellBackground(cell);
      if (foreground != active_foreground || background != active_background) {
        SetColor((short)foreground, (short)background, A_BOLD);
        active_foreground = foreground;
        active_background = background;
      }

      if (SpriteCellIsBlank(cell)) {
        mvprintw(y, x, " ");
      } else {
        char utf8[5];
        size_t length = encodeUtf8(SpriteCellCodepoint(cell), utf8);
        utf8[length] = '\0';
        mvprintw(y, x, "%s", utf8);
      }
      x += SpriteCellWidth(cell);
    }
    y++;
  }
}

/**
 * Get the number of frames.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame count.
 */
int RollfilmFrameCount(const Rollfilm* rollfilm) {
  if (rollfilm == NULL) return 0;
  return (int)SpriteAnimationFrameCount(rollfilm->animation);
}

/**
 * Get the current frame index.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame index.
 */
int RollfilmFrameIndex(const Rollfilm* rollfilm) {
  return rollfilm == NULL ? 0 : rollfilm->current_frame;
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

  int best_index = -1;
  int best_width = -1;
  int best_height = -1;

  for (int i = 0; i < indices_count; i++) {
    int index = indices[i];
    if (index < 0) continue;
    const Rollfilm* rollfilm = animations[index];
    if (rollfilm == NULL) continue;

    int width = RollfilmWidth(rollfilm);
    int height = RollfilmHeight(rollfilm);
    if (height > best_height || (height == best_height && width > best_width)) {
      best_index = index;
      best_width = width;
      best_height = height;
    }
  }
  return best_index;
}
