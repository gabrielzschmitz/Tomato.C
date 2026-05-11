#ifndef ANIM_H_
#define ANIM_H_

#include <stdbool.h>
#include <stdio.h>

#define MAX_FRAME_WIDTH 120

/* Animation structs */
typedef struct FrameToken FrameToken;
typedef struct FrameRow FrameRow;
typedef struct Frame Frame;
typedef struct Rollfilm Rollfilm;

/* Animation callback functions */
typedef void (*AnimationUpdate)(struct Rollfilm*);
typedef void (*AnimationRender)(struct Rollfilm*, int start_y, int start_x);

/**
 * Structure for storing frame tokens with color information.
 * Tokens are linked in a list for a single row of text output.
 */
struct FrameToken {
  FrameToken* next; /* Pointer to the next token in the list (NULL if last) */
  char* token;      /* String representing the token content */
  int length;       /* Length of the token string in bytes */
  int color;        /* Color for the token */
  bool is_blank;    /* True if this is a blank (non-printing) token */
};

/**
 * Structure for storing a row of frame tokens.
 * A frame consists of multiple rows forming the visual output.
 */
struct FrameRow {
  FrameRow* next;     /* Pointer to the next row in the list (NULL if last) */
  FrameToken* tokens; /* Pointer to the linked list of tokens in this row */
};

/**
 * Structure for storing an animation frame.
 * A frame is a single image in the animation sequence.
 */
struct Frame {
  Frame* next;               /* Pointer to the next frame (circular list) */
  FrameRow* rows;            /* Pointer to the linked list of rows */
  double seconds_multiplier; /* Time multiplier for frame duration */
  int width;                 /* Width of the widest row in the frame */
  int id;                    /* Unique identifier for this frame */
};

/**
 * Structure for storing an animation (Rollfilm).
 * Contains a circular linked list of frames and playback state.
 */
struct Rollfilm {
  Frame* frames;          /* Pointer to the circular linked list of frames */
  bool loop;              /* true to loop the animation, false to stop at end */
  double delta_frame_ms;  /* Elapsed time in milliseconds since last frame */
  int current_frame;      /* Index of the currently displayed frame */
  int frame_count;        /* Total number of frames in the rollfilm */
  int frame_height;       /* Height of each frame in lines */
  int frame_width;        /* Width of the widest frame */
  AnimationUpdate update; /* Function pointer for custom update logic */
  AnimationRender render; /* Function pointer for custom render logic */
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
 * @param film Pointer to array of rollfilm pointers
 * @param list_to_update Array of indices to update
 * @param list_size Number of indices in list_to_update
 * @param loop true to enable looping, false to disable
 */
void SetRollfilmLoop(Rollfilm** film, const int* list_to_update,
                     size_t list_size, bool loop);

/**
 * Free all memory associated with a Rollfilm.
 * @param rollfilm Pointer to the Rollfilm to free
 */
void FreeRollfilm(Rollfilm* rollfilm);

/**
 * Create a new empty Frame.
 * @return Pointer to the created Frame, or NULL on allocation failure
 */
Frame* CreateFrame(void);

/**
 * Link a new frame to the current frame, maintaining circular list.
 * Initializes the new frame and connects it to the existing chain.
 * @param current_frame Pointer to current frame pointer (updated)
 * @param head_row Row to attach as the first row of the new frame
 * @param frame_id ID to assign to the new frame
 */
void LinkNewFrame(Frame** current_frame, FrameRow* head_row, int frame_id);

/**
 * Free all memory associated with a single Frame.
 * @param frame Pointer to the Frame to free
 */
void FreeFrame(Frame* frame);

/**
 * Free all frames in a circular linked list.
 * @param frames Pointer to the first frame in the circular list
 */
void FreeFrames(Frame* frames);

/**
 * Create a new empty FrameRow.
 * @return Pointer to the created FrameRow, or NULL on allocation failure
 */
FrameRow* CreateRow(void);

/**
 * Free all rows in a linked list starting from the given row.
 * @param rows Pointer to the first row to free
 */
void FreeRows(FrameRow* rows);

/**
 * Create a new empty FrameToken.
 * @return Pointer to the created FrameToken, or NULL on allocation failure
 */
FrameToken* CreateToken(void);

/**
 * Free all memory associated with a single FrameToken.
 * @param token Pointer to the FrameToken to free
 */
void FreeToken(FrameToken* token);

/**
 * Free all tokens in a linked list starting from the given token.
 * @param tokens Pointer to the first token to free
 */
void FreeTokens(FrameToken* tokens);

/**
 * ---------------------------------------------------------------------------
 * Rollfilm Serialization / Deserialization
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
 * Deserialize a single line of frame data into tokens.
 * Handles color codes, UTF-8 characters, and text segments.
 * @param src Source line to deserialize
 * @param color Output: color code parsed from line
 * @return Pointer to the first token, or NULL on error
 */
FrameToken* DeserializeFrameLine(const char* src, int* color);

/**
 * Cleanup and finalize the Rollfilm structure.
 * Closes file and returns the completed rollfilm.
 * @param file File handle to close
 * @param rollfilm Pointer to the rollfilm to finalize
 * @param head_frame Pointer to the head frame
 * @param current_frame Pointer to the current frame
 * @param head_row Pointer to the head row
 * @param line_width Width of the largest line processed
 * @return Pointer to the finalized Rollfilm
 */
Rollfilm* CleanupAndReturn(FILE* file, Rollfilm* rollfilm, Frame* head_frame,
                           Frame* current_frame, FrameRow* head_row,
                           int line_width);

/**
 * Process a line containing icons in the sprite file.
 * @param line Line of text to parse
 * @param read Pointer to track bytes read
 * @param rollfilm Pointer to the rollfilm pointer
 * @param head_frame Pointer to the head frame pointer
 * @param current_frame Pointer to the current frame pointer
 * @param head_row Pointer to the head row pointer
 * @param current_row Pointer to the current row pointer
 * @param current_frame_id Pointer to current frame ID
 * @param file File handle for reading
 * @return 0 on success, non-zero on error
 */
int ProcessIconsLine(const char* line, int* read, Rollfilm** rollfilm,
                     Frame** head_frame, Frame** current_frame,
                     FrameRow** head_row, FrameRow** current_row,
                     int* current_frame_id, FILE* file);

/**
 * ---------------------------------------------------------------------------
 * Parsing Helpers
 * ---------------------------------------------------------------------------
 */

/**
 * Process a line of frame data and update the current row.
 * Parses color codes and text tokens from the line.
 * @param line Line to process
 * @param current_row Pointer to current row pointer
 * @param line_color Output: color code for the line
 * @param line_width Output: width of the line
 * @return 0 on success, non-zero on error
 */
int ProcessFrameLine(const char* line, FrameRow** current_row, int* line_color,
                     int* line_width);

/**
 * Parse the frame height and count from a line.
 * Format: "SIZE: NxM" where N is count and M is height.
 * @param line Line to parse
 * @param frame_count Output: number of frames
 * @param frame_height Output: height of each frame
 * @return 0 on success, non-zero on error
 */
int ParseFrameSize(const char* line, int* frame_count, int* frame_height);

/**
 * Parse the frame time multiplier from a line.
 * Format: "TIME: X.X" where X.X is the multiplier value.
 * @param line Line to parse
 * @param frame_time Output: time multiplier value
 * @return 0 on success, non-zero on error
 */
int ParseFrameTime(const char* line, double* frame_time);

/**
 * Check if a line contains icon data (not a separator or metadata).
 * @param line Line to check
 * @return true if icons, false if not
 */
bool IsIconsLine(const char* line);

/**
 * Check if a line is a separator between sprite sections.
 * @param line Line to check
 * @return true if separator, false if not
 */
bool IsSeparatorLine(const char* line);

/**
 * Handle a UTF-8 character in a line.
 * Copies the multi-byte character to the destination buffer.
 * @param src Source position in the line
 * @param dest Pointer to destination pointer (updated)
 * @return Number of bytes consumed, or -1 on error
 */
int HandleUnicode(const char* src, char** dest);

/**
 * Handle a color code in a line.
 * Parses escape sequence and sets the color value.
 * @param src Source position in the line
 * @param dest Output: color code
 * @return Number of bytes consumed, or -1 on error
 */
int HandleColor(const char* src, int* dest);

/**
 * ---------------------------------------------------------------------------
 * Util functions
 * ---------------------------------------------------------------------------
 */

/**
 * Get the width of the widest frame in a rollfilm.
 * @param rollfilm Pointer to the rollfilm
 * @return Width of the widest frame, or 0 if rollfilm is NULL
 */
int GetWidestFrame(Rollfilm* rollfilm);

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
 * ---------------------------------------------------------------------------
 * Animation callbacks
 * ---------------------------------------------------------------------------
 */

/**
 * Render the current frame of the Rollfilm at the specified coordinates.
 * @param rollfilm Pointer to the rollfilm to render
 * @param start_y Starting y coordinate on screen
 * @param start_x Starting x coordinate on screen
 */
void RenderCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x);

/**
 * Advance the Rollfilm to the next frame.
 * Handles timing, looping, and frame index wrapping.
 * @param rollfilm Pointer to the rollfilm to update
 */
void UpdateAnimation(Rollfilm* rollfilm);

#endif /* ANIM_H */
