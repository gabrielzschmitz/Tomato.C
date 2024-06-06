#ifndef ANIM_H_
#define ANIM_H_

#define MAX_FRAME_WIDTH 120

/* Structure for storing frame tokens with color information */
typedef struct FrameToken FrameToken;
struct FrameToken {
  FrameToken* next; /* Pointer to the next token in the list */
  FrameToken* prev; /* Pointer to the previous token in the list */
  char* token;      /* String representing the token */
  int length;       /* Length of the token */
  int color;        /* Color code for the token */
};

/* Structure for storing a row of frame tokens */
typedef struct FrameRow FrameRow;
struct FrameRow {
  FrameRow* next;     /* Pointer to the next row in the list */
  FrameRow* prev;     /* Pointer to the previous row in the list */
  FrameToken* tokens; /* Pointer to the linked list of tokens in the row */
};

/* Structure for storing an animation frame */
typedef struct Frame Frame;
struct Frame {
  Frame* next;    /* Pointer to the next frame in the circular list */
  Frame* prev;    /* Pointer to the previous frame in the circular list */
  FrameRow* rows; /* Pointer to the linked list of rows in the frame */
  int width;      /* Width of the largest row in the frame */
  int id;         /* Identifier for the frame */
};

/* Structure for storing an animation (Rollfilm) */
typedef struct Rollfilm Rollfilm;
struct Rollfilm {
  Frame* frames;     /* Pointer to the circular linked list of frames */
  int current_frame; /* Index of the current frame */
  int frame_count;   /* Total number of frames */
  int frame_height;  /* Height of each frame */
  int frame_width;   /* Width of the largest frame */
};

/* Increments animation frames based on real-life seconds */
void FrameTimer(int* frame_second, double* milliseconds);

/* Creates a new Rollfilm with N frames of height M */
Rollfilm* CreateRollfilm(int N, int M);

/* Frees all memory associated with a Rollfilm */
void FreeRollfilm(Rollfilm* rollfilm);

/* Creates a new frame */
Frame* CreateFrame();

/* Frees all frames in a circular linked list */
void FreeFrames(Frame* frames);

/* Frees all memory associated with a single frame */
void FreeFrame(Frame* frame);

/* Creates a new frame row */
FrameRow* CreateRow();

/* Frees all rows in a frame */
void FreeRows(FrameRow* rows);

/* Creates a new frame token */
FrameToken* CreateToken();

/* Frees all tokens in a linked list */
void FreeTokens(FrameToken* tokens);

/* Frees all memory associated with a single frame token */
void FreeToken(FrameToken* token);

/* Deserializes sprites from a file into a Rollfilm structure */
Rollfilm* DeserializeSprites(const char* filename);

/* Parses the frame height from a line. */
int ParseFrameSize(const char* line, int* frame_count, int* frame_height);

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

/* Prints all frames from a Rollfilm */
void PrintAllFrames(Rollfilm* rollfilm);

/* Draws the current frame of the Rollfilm at the specified coordinates */
void DrawCurrentFrame(Rollfilm* rollfilm, int start_y, int start_x);

#endif /* ANIM_H */
