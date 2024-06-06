#include "util.h"

#include <ncurses.h>

#include "tomato.h"

/* Set text foreground and background colors */
void SetColor(short int fg, short int bg, chtype attr) {
  chtype color;

  // Handle background color when BGTRANSPARENCY is disabled
  if (!BGTRANSPARENCY && bg == NO_COLOR) bg = COLOR_BLACK;

  if (bg == NO_COLOR && BGTRANSPARENCY)
    color = COLOR_PAIR((fg + 1) + (PALETTE_SIZE * PALETTE_SIZE));
  else
    color = COLOR_PAIR((bg * PALETTE_SIZE) + fg + 1);

  color |= attr;
  attrset(color);
}
