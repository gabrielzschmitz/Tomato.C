#include "util.h"

#include <ncurses.h>

#include "tomato.h"

/* Set text foreground and background colors */
void SetColor(short int fg, short int bg, chtype attr) {
  chtype color = COLOR_PAIR(bg * PALLETE_SIZE + fg + 1);
  color |= attr;
  attrset(color);
}
