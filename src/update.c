#include "update.h"

#include <ncurses.h>
#include <stdlib.h>

#include "bar.h"
#include "log.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* Update variables */
ErrorType UpdateApp(AppData* app) {
  ErrorType status = NO_ERROR;

  UpdateScreen(app->screen);

  Panel* current_panel = &app->screen->panels[app->screen->current_panel];

  UpdateStatusBar(app, app->status_bar, current_panel);
  for (int i = 0; i < MAX_PANELS; i++) {
    current_panel = &app->screen->panels[i];

    switch (current_panel->scene_history->present) {
      case MAIN_MENU: UpdateMainMenu(app); break;
      case WORK_TIME: UpdateWorkTime(app); break;
      case SHORT_PAUSE: UpdateShortPause(app); break;
      case LONG_PAUSE: UpdateLongPause(app); break;
      case NOTES: UpdateNotes(app); break;
      case HELP: UpdateHelp(app); break;
      case CONTINUE: UpdateContinue(app); break;
      default: break;
    }
  }

  const int steps[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
  const size_t steps_count = sizeof(steps) / sizeof(steps[0]);
  UpdateTimerLog(app, steps, steps_count);

  return status;
}

/* Update MAIN_MENU */
void UpdateMainMenu(AppData* app) {
  /* Tomato Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[MAIN_MENU];
    animation->update(animation);
  }
}

/* Update WORK_TIME */
void UpdateWorkTime(AppData* app) {
  /* Coffee Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[WORK_TIME];
    animation->update(animation);
  }
  if (!app->is_paused) UpdatePomodoroTime(app);
  if (StepEnded(app->pomodoro_data.current_step_time,
                app->pomodoro_data.work_time)) {
    if (app->pomodoro_data.current_cycle >=
        app->pomodoro_data.total_cycles - 1) {
      ExecuteHistory(app->screen->panels[0].scene_history, LONG_PAUSE);
      app->screen->panels[0].menu_index = -1;
      app->pomodoro_data.current_step = LONG_PAUSE;
      app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
      app->pomodoro_data.current_step_time = 0;
      Notification notification = {
        .title = " Long Pause Break",
        .description = "You have some time to chill",
        .audio_path = "./sounds/pausenotify.mp3",
      };
      Notify(&notification);
    } else {
      ExecuteHistory(app->screen->panels[0].scene_history, SHORT_PAUSE);
      app->screen->panels[0].menu_index = -1;
      app->pomodoro_data.current_step = SHORT_PAUSE;
      app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
      app->pomodoro_data.current_step_time = 0;
      Notification notification = {
        .title = " Pause Break",
        .description = "You have some time to chill",
        .audio_path = "./sounds/pausenotify.mp3",
      };
      Notify(&notification);
    }
  }
}

/* Update SHORT_PAUSE */
void UpdateShortPause(AppData* app) {
  /* Coffee Machine Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[SHORT_PAUSE];
    animation->update(animation);
  }
  if (!app->is_paused) UpdatePomodoroTime(app);
  if (StepEnded(app->pomodoro_data.current_step_time,
                app->pomodoro_data.short_pause_time)) {
    ExecuteHistory(app->screen->panels[0].scene_history, WORK_TIME);
    app->screen->panels[0].menu_index = -1;
    app->pomodoro_data.current_step = WORK_TIME;
    app->pomodoro_data.current_cycle += 1;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    app->pomodoro_data.current_step_time = 0;
    Notification notification = {
      .title = " Work!",
      .description = "You need to focus",
      .audio_path = "./sounds/dfltnotify.mp3",
    };
    Notify(&notification);
  }
}

/* Update LONG_PAUSE */
void UpdateLongPause(AppData* app) {
  /* Beach Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[LONG_PAUSE];
    animation->update(animation);
  }
  if (!app->is_paused) UpdatePomodoroTime(app);
  if (StepEnded(app->pomodoro_data.current_step_time,
                app->pomodoro_data.long_pause_time)) {
    ExecuteHistory(app->screen->panels[0].scene_history, MAIN_MENU);
    app->screen->panels[0].menu_index = MAIN_MENU_MENU;
    app->pomodoro_data.current_step = MAIN_MENU;
    app->pomodoro_data.current_cycle = 0;
    app->pomodoro_data.current_step_time = 0;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    Notification notification = {
      .title = " End of Pomodoro Cycle",
      .description = "Feel free to start another!",
      .audio_path = "./sounds/endnotify.mp3",
    };
    Notify(&notification);
  }
}

/* Update NOTES */
void UpdateNotes(AppData* app) {
  /* Notepad Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[NOTES];
    animation->update(animation);
  }
}

/* Update HELP */
void UpdateHelp(AppData* app) {
  /* Scroll Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[HELP];
    animation->update(animation);
  }
}

/* Update CONTINUE */
void UpdateContinue(AppData* app) {
  /* Banner Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[CONTINUE];
    animation->update(animation);
  }
}

/* Update pomodoro data time */
void UpdatePomodoroTime(AppData* app) {
  double current_time = GetCurrentTimeMS();
  double delta_time = current_time - app->pomodoro_data.delta_time_ms;

  int speed_multiplier = 1;
  if (DEBUG) speed_multiplier = 100;
  if (delta_time >= 1000.0 / speed_multiplier) {
    app->pomodoro_data.delta_time_ms = current_time;
    app->pomodoro_data.current_step_time += 1;
    if (app->pomodoro_data.current_step_time ==
        app->pomodoro_data.last_step_time + 10)
      app->pomodoro_data.last_step_time = app->pomodoro_data.current_step_time;
  }
}

/* Update timer log socket */
void UpdateTimerLog(AppData* app, const int* steps, const size_t steps_count) {
  if (IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step)) {
    char* time_string = FormatTimerLog(app->pomodoro_data, app->is_paused);
    SetTimerLog(TIMER_FILE, time_string);
    free(time_string);
  }
}
