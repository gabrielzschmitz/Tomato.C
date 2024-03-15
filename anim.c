/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// anim.c
*/
#include "anim.h"

#include <ncurses.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "draw.h"
#include "tomato.h"
#include "util.h"

/* Time the animations frames */
void frameTimer(appData* app) {
  const clock_t sec = 60 * CLOCKS_PER_SEC;
  clock_t currentTime = clock();
  const clock_t time = currentTime + sec;
  if (currentTime < time && !app->pausedTimer) {
    app->framems++;
    if (app->framems >= app->sfps) {
      app->framems = 0;
      app->frameTimer = app->frameTimer + 1;
    }
  }
}

/* Print the logo frames */
void printLogo(appData* app) {
  const char* logoFramesNerdIcons[] = {
    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |          |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",

    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |╱    \\    ", "    |          |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |      ─   |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |          |     ",
    "     \\     ╲   /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |          |     ",
    "     \\    |    /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |          |     ",
    "     \\   ╱     /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |   ─      |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /    ╲|     \\    ", "    |          |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_"};
  const char* logoFramesIconsOn[] = {
    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     o     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |╱    \\    ", "    |     o     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     o ─   |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     o     |     ",
    "     \\     ╲   /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     o     |     ",
    "     \\    |    /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     o     |     ",
    "     \\   ╱     /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |   ─ o     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /    ╲|     \\    ", "    |     o     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_"};
  const char* logoFramesIconsOff[] = {
    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     0     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |/    \\    ", "    |     0     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     0 -   |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     0     |     ",
    "     \\     \\   /    ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     0     |     ",
    "     \\    |    /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |     0     |     ",
    "     \\   /     /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /     |     \\    ", "    |   - 0     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_",

    "       __\\W/__       ", "     .\'.-\'_\'-.\'.  ",
    "    /    \\|     \\   ", "    |     0     |     ",
    "     \\         /     ", "      \'-.___.-\'     ",
    "___                 _ ", " | _  _  _ |_ _    /  ",
    " |(_)|||(_||_(_) . \\_"};
  char** logoFrames;
  if (strcmp(ICONS, "nerdicons") == 0)
    logoFrames = (char**)logoFramesNerdIcons;
  else if (strcmp(ICONS, "iconson") == 0)
    logoFrames = (char**)logoFramesIconsOn;
  else
    logoFrames = (char**)logoFramesIconsOff;
  int startx = app->middlex - 10;
  int starty = app->middley - 6;
  int frameIndex = app->logoFrame * 9;
  int lineColor[9] = {COLOR_GREEN,   COLOR_GREEN,   COLOR_RED,
                      COLOR_RED,     COLOR_RED,     COLOR_RED,
                      COLOR_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA};

  for (int i = 0; i < 9; i++) {
    setColor(lineColor[i], COLOR_BLACK, A_BOLD);
    mvprintw(starty + i, startx, "%s", logoFrames[frameIndex + i]);
  }
}

/* Print the coffee frames */
void printCoffee(appData* app) {
  const char* coffeeFrames[] = {
    "   ) )  ", "  ( (   ", "....... ", "|     |]", "\\     /", " `---\' ",

    " ( (    ", "  ) )   ", "....... ", "|     |]", "\\     /", " `---\' "};
  int startx = app->middlex - 3;
  int starty = app->middley - 2;
  int frameIndex = app->coffeeFrame * 6;
  int lineColor[6] = {COLOR_BLACK, COLOR_BLACK, COLOR_WHITE,
                      COLOR_WHITE, COLOR_WHITE, COLOR_WHITE};

  for (int i = 0; i < 6; i++) {
    setColor(lineColor[i], COLOR_BLACK, A_BOLD);
    mvprintw(starty + i, startx, "%s", coffeeFrames[frameIndex + i]);
  }
}

/* Print the coffee machine frames */
void printMachine(appData* app) {
  const char* machineFramesIconsOff[] = {
    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    -      ", "|   -   |   ___     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| ",

    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    I      ", "|   -   |   ___     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| ",

    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    -      ", "|   -   |   _|_     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| "

  };
  const char* machineFramesIconsOn[] = {
    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    ¯      ", "|   ¯   |   ___     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| ",

    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    †      ", "|   ¯   |   ___     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| ",

    "________._________  ", "|   _   |\\       / ", "|  |.|  | \\     /  ",
    "|  |.|  |__\\___/   ", "|  |.|  |    ¯      ", "|   ¯   |   _|_     ",
    "|_______|  \\___/_  ", "| _____ |  /~~~\\ \\", "||     ||__\\___/__ ",
    "||_____|          | ", "|_________________| "};
  char** machineFrames;
  if (strcmp(ICONS, "iconsoff") == 0)
    machineFrames = (char**)machineFramesIconsOff;
  else
    machineFrames = (char**)machineFramesIconsOn;
  int startx = app->middlex - 9;
  int starty = app->middley - 5;
  int frameIndex = app->machineFrame * 11;

  setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
  for (int i = 0; i < 11; i++) {
    mvprintw(starty + i, startx, "%s", machineFrames[frameIndex + i]);
  }
}

/* Print the beach frames */
void printBeach(appData* app) {
  if (app->beachFrame == 0) {
    setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 5), (app->middlex - 7), "|");
    mvprintw((app->middley - 4), (app->middlex - 9), "\\ _ /");
    mvprintw((app->middley - 3), (app->middlex - 11), "-= (_) =-");
    mvprintw((app->middley - 2), (app->middlex - 9), "/   \\");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 2), (app->middlex + 3), "_\\/_");
    setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 1), (app->middlex - 7), "|");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 1), (app->middlex + 3), "//o\\  _\\/_");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex - 12), "_ _ __ __ ____ _");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 5), "|");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 6), "__");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 9), "/o\\\\");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 1), (app->middlex - 12), "__=_-= _=_=-=");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 1), (app->middlex + 1), "_,-\'|\"\'\"\"-|-");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 2), (app->middlex - 12), "-=- -_=-=");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 2), (app->middlex - 3), "_,-\"          |");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 3), (app->middlex - 12), "=- -=");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 3), (app->middlex - 7), ".--\"");
  } else {
    setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 4), (app->middlex - 9), "\\ | /");
    mvprintw((app->middley - 3), (app->middlex - 10), "- (_) -");
    mvprintw((app->middley - 2), (app->middlex - 9), "/ | \\");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 2), (app->middlex + 2), "_\\/_");
    setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 1), (app->middlex - 8), "     ");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 1), (app->middlex + 2), "//o\\  _\\/_");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex - 12), "__ ____ __  ____");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 5), "|");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 6), "_ ");
    setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 8), "//o\\");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 0), (app->middlex + 12), "_");
    mvprintw((app->middley + 1), (app->middlex - 12), "--_=-__=  _-=");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 1), (app->middlex + 1), "_,-\'|\"\'\"\"-|-");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 2), (app->middlex - 12), "_-= _=-_=");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 2), (app->middlex - 3), "_,-\"          |");
    setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 3), (app->middlex - 12), "-= _-");
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 3), (app->middlex - 7), ".--\"");
  }
}

/* Print the gear frames */
void printWrench(appData* app, int flip) {
  const char* wrenchFramesDefault[] = {
    "  .---.                 .----. ", ".`  _  '._____________.`  ,---'",
    ":  >_<   _____________   )     ", "'.     .`             '.  '---.",
    "  '---`                 '----` "};
  const char* wrenchFramesFlipped[] = {
    " .----.                 .---.  ", "'---,  `._____________.'  _  `.",
    "     )   _____________   <_>  :", ".---'  .'             `.     .'",
    " `----'                 `---'  "};
  char** wrenchFrames;
  int startx, starty;
  if (flip == 1) {
    wrenchFrames = (char**)wrenchFramesFlipped;
    startx = app->middlex - 15;
    starty = app->middley + 5;
  } else {
    wrenchFrames = (char**)wrenchFramesDefault;
    startx = app->middlex - 15;
    starty = app->middley - 9;
  }

  setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
  for (int i = 0; i < 5; i++) {
    mvprintw(starty + i, startx, "%s", wrenchFrames[i]);
  }
}

void printBanner(appData* app) {
  const char* bannerFrames[] = {
    "    _________________________________     ",
    "___|                                 |___ ",
    "\\  |                                 |  /",
    " \\ |                                 | / ",
    " < |                                 | >  ",
    " / |_________________________________| \\ ",
    "/______)                         (______\\",

    "     _________________________________      ",
    "____|                                 |____ ",
    "\\   |                                 |   /",
    " \\  |                                 |  / ",
    " <  |                                 |  >  ",
    " /  |_________________________________|  \\ ",
    "/_______)                         (_______\\",

    "      ___________________________________       ",
    "_____|                                   |_____ ",
    "\\    |                                   |    /",
    " \\   |                                   |   / ",
    " <   |                                   |   >  ",
    " /   |___________________________________|   \\ ",
    "/_______)                             (_______\\",

    "       ___________________________________        ",
    "______|                                   |______ ",
    "\\     |                                   |     /",
    " \\    |                                   |    / ",
    " <    |                                   |    >  ",
    " /    |___________________________________|    \\ ",
    "/________)                             (________\\"};
  int startx;
  if (app->bannerFrame == 0)
    startx = app->middlex - 20;
  else if (app->bannerFrame == 1)
    startx = app->middlex - 21;
  else if (app->bannerFrame == 2)
    startx = app->middlex - 23;
  else
    startx = app->middlex - 24;
  int starty = app->middley - 3;
  int frameIndex = app->bannerFrame * 7;

  setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
  for (int i = 0; i < 7; i++) {
    mvprintw(starty + i, startx, "%s", bannerFrames[frameIndex + i]);
  }
}

void printPergament(appData* app) {
  const char* pergamentFrames[] = {
    "  ________________________________________________      ",
    " / \\                                              \\   ",
    "|   |                                             |     ",
    " \\__|                                             /    ",
    "    \\     _______________________________________|_____",
    "     \\   /                                            /",
    "      \\_/____________________________________________/ ",

    "  ________________________________________________      ",
    " / \\                                              \\   ",
    "|   |                                             |     ",
    " \\__|                                             /    ",
    "    |                                            /      ",
    "    |                                           |       ",
    "    \\     _______________________________________|_____",
    "     \\   /                                            /",
    "      \\_/____________________________________________/ ",

    "  ________________________________________________      ",
    " / \\                                              \\   ",
    "|   |                                             |     ",
    " \\__|                                             /    ",
    "    |                                            /      ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                          |        ",
    "    \\     ______________________________________|______",
    "     \\   /                                            /",
    "      \\_/____________________________________________/ ",

    "  ________________________________________________      ",
    " / \\                                              \\   ",
    "|   |                                             |     ",
    " \\__|                                             /    ",
    "    |                                            /      ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                          |        ",
    "    |                                          |        ",
    "    |                                          |        ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    \\     _______________________________________|_____",
    "     \\   /                                            /",
    "      \\_/____________________________________________/ ",

    "  ________________________________________________      ",
    " / \\                                              \\   ",
    "|   |                  HELP PAGE                  |     ",
    " \\__|                                             /    ",
    "    |                                            /      ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                          |        ",
    "    |                                          |        ",
    "    |                                          |        ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                           |       ",
    "    |                                            |      ",
    "    |                                            |      ",
    "    |                                             |     ",
    "    |                                             |     ",
    "    \\     _________________________________________|___",
    "     \\   /                                            /",
    "      \\_/____________________________________________/ "};
  int startx = app->middlex - 27;
  int starty = app->middley - 11;
  int frameIndex;
  setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
  if (app->helpFrame == 0) {
    frameIndex = 0;
    for (int i = 0; i < 7; i++) {
      mvprintw(starty + i, startx, "%s", pergamentFrames[frameIndex + i]);
    }
    printKeybinds(app, 0);
  } else if (app->helpFrame == 1) {
    frameIndex = 7;
    for (int i = 0; i < 9; i++) {
      mvprintw(starty + i, startx, "%s", pergamentFrames[frameIndex + i]);
    }
    printKeybinds(app, 1);
  } else if (app->helpFrame == 2) {
    frameIndex = 7 + 9;
    for (int i = 0; i < 13; i++) {
      mvprintw(starty + i, startx, "%s", pergamentFrames[frameIndex + i]);
    }
    printKeybinds(app, 2);
  } else if (app->helpFrame == 3) {
    frameIndex = 7 + 9 + 13;
    for (int i = 0; i < 17; i++) {
      mvprintw(starty + i, startx, "%s", pergamentFrames[frameIndex + i]);
    }
    printKeybinds(app, 3);
  } else {
    frameIndex = 7 + 9 + 13 + 17;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", pergamentFrames[frameIndex + i]);
    }
    printKeybinds(app, 4);
  }
}

/* Print the Notepad */
void printNotepad(appData* app) {
  const char* notepadFrames[] = {
    " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ",
    "(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "|-----------------------------------|||",
    "(___________________________________)|/",

    "           _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ",
    "          (-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "          /------------------------------------/|",
    "         /------------------------------------/||",
    "        /------------------------------------/ ||",
    "       /------------------------------------/  ||",
    "      /------------------------------------/   ||",
    "     /------------------------------------/    ||",
    "    /------------------------------------/     ||",
    "   /------------------------------------/      ||",
    "  /------------------------------------/       ||",
    " /------------------------------------/        ||",
    "(____________________________________)         ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |                                    ||",
    "          |____________________________________|/",

    "        _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ",
    "       (-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "   __/¯----------------------------------__/||",
    " _/------------------------------------_/   ||",
    "(_____________________________________)     ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |                                    ||",
    "       |____________________________________|/",

    "          (¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯)",
    "         /-------------------------------------/",
    "        /-------------------------------------/",
    "       /-------------------------------------/",
    "      /-------------------------------------/",
    "     /-------------------------------------/",
    "    /-------------------------------------/",
    "   /-------------------------------------/",
    "  /-------------------------------------/",
    " /-------------------------------------/",
    "/-------------------------------------/",
    "(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|____________________________________|/",

    "      (¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯)",
    "   _/¯-----------------------------------/¯",
    " /¯-----------------------------------/¯¯",
    "(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|____________________________________|/",

    " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ",
    "(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()",
    "|                                    ||\\",
    "|                                    ||-\\",
    "|                                    ||--\\",
    "|                                    ||---\\",
    "|                                    ||----\\",
    "|                                    ||-----\\",
    "|                                    ||------\\",
    "|                                    ||-------\\",
    "|                                    ||--------\\",
    "|                                    ||---------\\",
    "|                                    ||__________)",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|                                    ||",
    "|____________________________________|/",

    " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  ",
    "(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(().",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|                                    |||",
    "|____________________________________|/ ",
  };
  int startx;
  int starty;
  int frameIndex;
  setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
  if (app->notepadFrame == 0) {
    startx = app->middlex - 18;
    starty = app->middley - 11;
    frameIndex = 0;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
  } else if (app->notepadFrame == 1) {
    startx = app->middlex - 28;
    starty = app->middley - 11;
    frameIndex = 23;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printPartialNotes(app, 11);
  } else if (app->notepadFrame == 2) {
    startx = app->middlex - 25;
    starty = app->middley - 11;
    frameIndex = 23 + 23;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printPartialNotes(app, 3);
  } else if (app->notepadFrame == 3) {
    startx = app->middlex - 18;
    starty = app->middley - 21;
    frameIndex = 23 + 23 + 23;
    for (int i = 0; i < 33; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printNotes(app);
  } else if (app->notepadFrame == 4) {
    startx = app->middlex - 18;
    starty = app->middley - 13;
    frameIndex = 23 + 23 + 23 + 33;
    for (int i = 0; i < 25; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printNotes(app);
  } else if (app->notepadFrame == 5) {
    startx = app->middlex - 18;
    starty = app->middley - 11;
    frameIndex = 23 + 23 + 23 + 33 + 25;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printNotes(app);
  } else {
    startx = app->middlex - 18;
    starty = app->middley - 11;
    frameIndex = 23 + 23 + 23 + 33 + 25 + 23;
    for (int i = 0; i < 23; i++) {
      mvprintw(starty + i, startx, "%s", notepadFrames[frameIndex + i]);
    }
    printNotes(app);
  }
}
