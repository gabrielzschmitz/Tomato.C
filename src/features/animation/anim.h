/**
 * @file anim.h
 * @brief Public rollfilm playback API.
 */

#ifndef ANIM_H_
#define ANIM_H_

#include <stdbool.h>
#include <stddef.h>

/** Opaque mutable playback state for one sprite animation. */
typedef struct Rollfilm Rollfilm;

/** Owning collection of immutable sprite animation data. */
typedef struct SpriteLibrary SpriteLibrary;

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create mutable playback state for a loaded sprite animation.
 * The sprite library must outlive the returned rollfilm.
 * @param library Owning sprite library.
 * @param name Animation name.
 * @param icon_type Icon type index (0=nerd-icons, 1=emojis, 2=ascii).
 * @return New rollfilm, or NULL when not found or allocation fails.
 */
Rollfilm* CreateRollfilm(const SpriteLibrary* library, const char* name,
                         int icon_type);

/**
 * Free playback state. Sprite data is owned by the library.
 * @param rollfilm Pointer to the rollfilm to free.
 */
void FreeRollfilm(Rollfilm* rollfilm);

/**
 * Set the loop override for selected rollfilms.
 * @param film Array of rollfilm pointers.
 * @param indices Indices to update.
 * @param count Number of indices.
 * @param loop true to enable looping.
 */
void SetRollfilmLoop(Rollfilm** film, const int* indices, size_t count,
                     bool loop);

/**
 * ---------------------------------------------------------------------------
 * Playback
 * ---------------------------------------------------------------------------
 */

/**
 * Advance a rollfilm using accumulated wall-clock time.
 * Multiple frames are consumed when enough time has elapsed.
 * @param rollfilm Pointer to the rollfilm to update.
 */
void UpdateRollfilm(Rollfilm* rollfilm);

/**
 * Advance one frame when the current frame duration has elapsed.
 * Movement is clamped to the animation bounds.
 * @param rollfilm Pointer to the rollfilm to update.
 * @param direction Positive for forward, negative for backward.
 * @return true when the frame changed.
 */
bool RollfilmTryAdvance(Rollfilm* rollfilm, int direction);

/**
 * Seek to a frame and reset the frame clock.
 * @param rollfilm Pointer to the rollfilm.
 * @param frame_index Target frame index.
 */
void RollfilmSeekFrame(Rollfilm* rollfilm, int frame_index);

/**
 * Set the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 * @param frame_index Target frame index.
 */
void RollfilmSetDefaultFrame(Rollfilm* rollfilm, int frame_index);

/**
 * Seek to the frame used for static display.
 * @param rollfilm Pointer to the rollfilm.
 */
void RollfilmSeekDefault(Rollfilm* rollfilm);

/**
 * ---------------------------------------------------------------------------
 * Rendering
 * ---------------------------------------------------------------------------
 */

/**
 * Render the current frame at the given screen position.
 * @param rollfilm Pointer to the rollfilm to render.
 * @param start_y Starting terminal row.
 * @param start_x Starting terminal column.
 */
void RenderRollfilm(const Rollfilm* rollfilm, int start_y, int start_x);

/**
 * ---------------------------------------------------------------------------
 * Queries
 * ---------------------------------------------------------------------------
 */

/**
 * Get the number of frames.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame count.
 */
int RollfilmFrameCount(const Rollfilm* rollfilm);

/**
 * Get the current frame index.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame index.
 */
int RollfilmFrameIndex(const Rollfilm* rollfilm);

/**
 * Get the cached animation width in terminal columns.
 * @param rollfilm Pointer to the rollfilm.
 * @return Maximum frame width.
 */
int RollfilmWidth(const Rollfilm* rollfilm);

/**
 * Get the cached animation height in rows.
 * @param rollfilm Pointer to the rollfilm.
 * @return Maximum frame height.
 */
int RollfilmHeight(const Rollfilm* rollfilm);

/**
 * Get the current frame width in terminal columns.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame width.
 */
int RollfilmFrameWidth(const Rollfilm* rollfilm);

/**
 * Get the current frame height in rows.
 * @param rollfilm Pointer to the rollfilm.
 * @return Current frame height.
 */
int RollfilmFrameHeight(const Rollfilm* rollfilm);

/**
 * Get the current frame duration in milliseconds.
 * @param rollfilm Pointer to the rollfilm.
 * @return Frame duration.
 */
double RollfilmFrameDurationMS(const Rollfilm* rollfilm);

/**
 * Find the first blank cell in the last frame.
 * @param rollfilm Pointer to the rollfilm.
 * @param out_x Output: terminal column relative to the frame.
 * @param out_y Output: row relative to the frame.
 * @return true when found.
 */
bool RollfilmFirstBlank(const Rollfilm* rollfilm, int* out_x, int* out_y);

/**
 * Find the end of the last blank cell run in the last frame.
 * @param rollfilm Pointer to the rollfilm.
 * @param out_x Output: column one past the blank run.
 * @param out_y Output: row containing the blank run.
 * @return true when found.
 */
bool RollfilmLastBlank(const Rollfilm* rollfilm, int* out_x, int* out_y);

/**
 * Find the largest rollfilm among selected indices.
 * Height takes priority over width.
 * @param animations Array of rollfilm pointers.
 * @param indices Indices to compare.
 * @param indices_count Number of indices.
 * @return Index of the largest rollfilm, or -1 when none is valid.
 */
int RollfilmLargest(Rollfilm* const animations[], const int* indices,
                    int indices_count);

#endif /* ANIM_H_ */
