#include "update.h"

#include <ncurses.h>

#include "anim.h"
#include "tomato.h"
#include "ui.h"

/* Update variables */
ErrorType UpdateApp(AppData *app) {
  ErrorType status = NO_ERROR;

  UpdateScreen(app->screen);

  Panel *current_panel = &app->screen->panels[app->screen->current_panel];

  UpdateStatusBar(app, app->status_bar, current_panel);
  for (int i = 0; i < MAX_PANELS; i++) {
    current_panel = &app->screen->panels[i];

    switch (current_panel->scene_history->present) {
      case MAIN_MENU:
        UpdateMainMenu(app);
        break;
      case WORK_TIME:
        UpdateWorkTime(app);
        break;
      case SHORT_PAUSE:
        UpdateShortPause(app);
        break;
      case LONG_PAUSE:
        UpdateLongPause(app);
        break;
      case NOTES:
        UpdateNotes(app);
        break;
      case HELP:
        UpdateHelp(app);
        break;
      case CONTINUE:
        UpdateContinue(app);
        break;
      default:
        break;
    }
  }

  return status;
}

/* Update MAIN_MENU */
void UpdateMainMenu(AppData *app) {
  /* Tomato Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[MAIN_MENU];
    animation->update(animation);
  }
}

/* Update WORK_TIME */
void UpdateWorkTime(AppData *app) {
  /* Coffee Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[WORK_TIME];
    animation->update(animation);
  }
}

/* Update SHORT_PAUSE */
void UpdateShortPause(AppData *app) {
  /* Coffee Machine Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[SHORT_PAUSE];
    animation->update(animation);
  }
}

/* Update LONG_PAUSE */
void UpdateLongPause(AppData *app) {
  /* Beach Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[LONG_PAUSE];
    animation->update(animation);
  }
}

/* Update NOTES */
void UpdateNotes(AppData *app) {
  /* Notepad Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[NOTES];
    animation->update(animation);
  }
}

/* Update HELP */
void UpdateHelp(AppData *app) {
  /* Scroll Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[HELP];
    animation->update(animation);
  }
}

/* Update CONTINUE */
void UpdateContinue(AppData *app) {
  /* Banner Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm *animation = app->animations[CONTINUE];
    animation->update(animation);
  }
}
