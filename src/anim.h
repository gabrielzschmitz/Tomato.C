#ifndef ANIM_H_
#define ANIM_H_

#include <stdbool.h>
#include <stdio.h>

#define MAX_FRAME_WIDTH 120

/* Animation specific structs */
typedef struct FrameToken FrameToken;
typedef struct FrameRow FrameRow;
typedef struct Frame Frame;
typedef struct Rollfilm Rollfilm;

/* Animation specific functions */
typedef void (*AnimationUpdate)(struct Rollfilm*);
typedef void (*AnimationRender)(struct Rollfilm*, int start_y, int start_x);

/* Structure for storing frame tokens with color information */
struct FrameToken {
  FrameToken* next; /* Pointer to the next token in the list */
  char* token;      /* String representing the token */
  int length;       /* Length of the token */
  int color;        /* Color code for the token */
};

/* Structure for storing a row of frame tokens */
struct FrameRow {
  FrameRow* next;     /* Pointer to the next row in the list */
  FrameToken* tokens; /* Pointer to the linked list of tokens in the row */
};

/* Structure for storing an animation frame */
struct Frame {
  Frame* next;    /* Pointer to the next frame in the circular list */
  FrameRow* rows; /* Pointer to the linked list of rows in the frame */
  double seconds_multiplier; /* Frame time multiplier */
  int width;                 /* Width of the largest row in the frame */
  int id;                    /* Identifier for the frame */
};

/* Structure for storing an animation (Rollfilm) */
struct Rollfilm {
  Frame* frames;          /* Pointer to the circular linked list of frames */
  double delta_frame_ms;  /* Elapsed time in milliseconds since last frame */
  int current_frame;      /* Index of the current frame */
  bool loop;              /* Flag to loop or not the animation */
  int frame_count;        /* Total number of frames */
  int frame_height;       /* Height of each frame */
  int frame_width;        /* Width of the largest frame */
  AnimationUpdate update; /* Function pointer for animation update logic */
  AnimationRender render; /* Function pointer for animation render logic */
};

/* Function to get the current time in milliseconds */
double GetCurrentTimeMS(void);

/* Increments animation frames based on real-life seconds */
void FrameTimer(int* frame_second, double* milliseconds);

/* Creates a new Rollfilm with N frames of height M */
Rollfilm* CreateRollfilm(int N, int M);

/* Frees all memory associated with a Rollfilm */
void FreeRollfilm(Rollfilm* rollfilm);

/* Creates a new frame */
Frame* CreateFrame(void);

/* Frees all frames in a circular linked list */
void FreeFrames(Frame* frames);

/* Frees all memory associated with a single frame */
void FreeFrame(Frame* frame);

/* Creates a new frame row */
FrameRow* CreateRow(void);

/* Frees all rows in a frame */
void FreeRows(FrameRow* rows);

/* Creates a new frame token */
FrameToken* CreateToken(void);

/* Frees all tokens in a linked list */
void FreeTokens(FrameToken* tokens);

/* Frees all memory associated with a single frame token */
void FreeToken(FrameToken* token);

/* Deserializes sprites from a file into a Rollfilm structure */
Rollfilm* DeserializeSprites(const char* filename);

/* Process the line containing icons */
int ProcessIconsLine(const char* line, int* read, Rollfilm** rollfilm,
                     Frame** head_frame, Frame** current_frame,
                     FrameRow** head_row, FrameRow** current_row,
                     int* current_frame_id, FILE* file);

/* Cleanup and finalize the Rollfilm structure */
Rollfilm* CleanupAndReturn(FILE* file, Rollfilm* rollfilm, Frame* head_frame,
                           Frame* current_frame, FrameRow* head_row,
                           int line_width);

/* Parses the frame height from a line. */
int ParseFrameSize(const char* line, int* frame_count, int* frame_height);

/* Parses the frame height from a line. */
int ParseFrameTime(const char* line, double* frame_time);

/* Checks if the line contains icons. */
int IsIconsLine(const char* line);

/* Checks if the line is a separator. */
int IsSeparatorLine(const char* line);

/* Processes a line of frame data and updates the current row. */
int ProcessFrameLine(const char* line, FrameRow** current_row, int* line_color,
                     int* line_width);

/* Links a new frame to the current frame and updates the frame ID, maintaining
 * a circular linked list. */
void LinkNewFrame(Frame** current_frame, FrameRow* head_row, int frame_id);

/* Returns the widest frame from a rollfilm */
int GetWidestFrame(Rollfilm* rollfilm);

/* Deserializes a single line of frame data */
FrameToken* DeserializeFrameLine(const char* src, int* color);

/* Handles a UTF-8 character in a line */
int HandleUnicode(const char* src, char** dest);

/* Handles a color code in a line */
int HandleColor(const char* src, int* dest);

/* Render the current frame of the Rollfilm at the specified coordinates */
void RenderCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x);

/* Updates the current frame of the Rollfilm to be next frame */
void UpdateAnimation(Rollfilm* rollfilm);

/* Updates the loop variable of a given list of Rollfilms */
void SetAnimationsLoop(Rollfilm** film, const int* list_to_update,
                       size_t list_size, bool loop);

#endif /* ANIM_H */
