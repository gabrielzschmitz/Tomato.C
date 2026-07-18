#ifndef ANIM_H_
#define ANIM_H_

#include <stdbool.h>
#include <stdio.h>

#define MAX_FRAME_WIDTH 120

typedef struct AppData AppData;
typedef struct Rollfilm Rollfilm;
typedef void (*AnimationUpdate)(struct Rollfilm*);
typedef void (*AnimationRender)(struct Rollfilm*, int start_y, int start_x);

/**
 * Structure for storing frame tokens with color information.
 * Tokens are linked in a list for a single row of text output.
 */
struct FrameToken {
  struct FrameToken* next; /**< Pointer to next token in list (NULL if last) */
  char* token;             /**< String representing the token content */
  int length;              /**< Length of the token string in bytes */
  int color;               /**< Color for the token */
  bool is_blank;           /**< True if this is a blank (non-printing) token */
};

/**
 * Structure for storing a row of frame tokens.
 * A frame consists of multiple rows forming the visual output.
 */
struct FrameRow {
  struct FrameRow* next;     /**< Pointer to next row in list (NULL if last) */
  struct FrameToken* tokens; /**< Pointer to list of tokens in this row */
};

/**
 * Structure for storing an animation frame.
 * A frame is a single image in the animation sequence.
 */
struct Frame {
  struct Frame* next;        /**< Pointer to the next frame (circular list) */
  struct FrameRow* rows;     /**< Pointer to the linked list of rows */
  double seconds_multiplier; /**< Time multiplier for frame duration */
  int width;                 /**< Width of the widest row in the frame */
  int id;                    /**< Unique identifier for this frame */
};

/**
 * Structure for storing an animation (Rollfilm).
 * Contains a circular linked list of frames and playback state.
 */
struct Rollfilm {
  struct Frame* frames; /**< Pointer to the circular linked list of frames */
  bool loop;            /**< true to loop the animation, false to stop at end */
  double delta_frame_ms;  /**< Elapsed time in milliseconds since last frame */
  int current_frame;      /**< Index of the currently displayed frame */
  int frame_count;        /**< Total number of frames in the rollfilm */
  int frame_height;       /**< Height of each frame in lines */
  int frame_width;        /**< Width of the widest frame */
  int default_frame;      /**< Default frame index for static (non-animated) display */
  AnimationUpdate update; /**< Function pointer for custom update logic */
  AnimationRender render; /**< Function pointer for custom render logic */
};

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new Rollfilm with N frames of height M.
 * @param N Number of frames to allocate
 * @param M Height of each frame in lines
 * @return Pointer to the created Rollfilm, or NULL on allocation failure
 */
Rollfilm* CreateRollfilm(int N, int M);

/**
 * Set the loop variable for a list of Rollfilms.
 * @param app Pointer to application data (used for SetError on alloc failure)
 * @param film Pointer to array of rollfilm pointers
 * @param list_to_update Array of indices to update
 * @param list_size Number of indices in list_to_update
 * @param loop true to enable looping, false to disable
 */
void SetRollfilmLoop(AppData* app, Rollfilm** film, const int* list_to_update,
                     size_t list_size, bool loop);

/**
 * Free all memory associated with a Rollfilm.
 * @param rollfilm Pointer to the Rollfilm to free
 */
void FreeRollfilm(Rollfilm* rollfilm);

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Deserialization
 * ---------------------------------------------------------------------------
 */

/**
 * Deserialize sprites from a file into a Rollfilm structure.
 * Parses sprite file format with frame size, timing, and color codes.
 * @param filename Path to the sprite file
 * @return Pointer to the created Rollfilm, or NULL on failure
 */
Rollfilm* DeserializeSprites(const char* filename);

/**
 * ---------------------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------------------
 */

/**
 * Find the rollfilm with the largest width among specified indices.
 * Used to size panels to fit the largest animation.
 * @param animations Pointer to array of rollfilm pointers
 * @param indices Array of indices to check
 * @param indices_count Number of indices
 * @return Index of the largest rollfilm, or -1 if none found
 */
int RollfilmLargest(Rollfilm* animations[], int* indices, int indices_count);

/**
 * Find the position of the first blank token in the last frame.
 * Used for cursor positioning in text input.
 * @param rollfilm Pointer to the rollfilm
 * @param out_x Output: x coordinate of the blank
 * @param out_y Output: y coordinate of the blank
 * @return true if found, false if not found
 */
bool RollfilmFirstBlank(Rollfilm* rollfilm, int* out_x, int* out_y);

/**
 * Find the position of the last blank token in the last frame.
 * @param rollfilm Pointer to the rollfilm
 * @param out_x Output: x coordinate of the blank
 * @param out_y Output: y coordinate of the blank
 * @return true if found, false if not found
 */
bool RollfilmLastBlank(Rollfilm* rollfilm, int* out_x, int* out_y);

/**
 * Seek the rollfilm to a specific frame by index.
 * Properly updates both the frames pointer and current_frame index,
 * and resets the frame timer so the next update doesn't skip frames.
 * @param rollfilm Pointer to the rollfilm
 * @param frame_index Target frame index (0 <= frame_index < frame_count)
 */
void RollfilmSeekFrame(Rollfilm* rollfilm, int frame_index);

#endif /* ANIM_H */
