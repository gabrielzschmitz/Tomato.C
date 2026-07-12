#include "update.h"

#include <ncurses.h>
#include <stdlib.h>

#include "anim.h"
#include "bar.h"
#include "config.h"
#include "log.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* PRIVATE UPDATE FUNCTIONS */
/* Pomodoro */
static void updatePomodoroTime(AppData* app);
static void updatePomodoroLog(AppData* app);
static void updateTimerLog(AppData* app, const int* steps,
                           const size_t steps_count);
static bool skip_auto_save = false;

/**
 * ---------------------------------------------------------------------------
 * App / Screen
 * ---------------------------------------------------------------------------
 */

/**
 * Update all application variables and state.
 * Called each frame to refresh app state.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType UpdateApp(AppData* app) {
  ErrorType status = NO_ERROR;

  /* Sync status bar position from config (allows runtime changes) */
  if (app->status_bar)
    app->status_bar->position = STATUS_BAR_POSITION ? TOP : BOTTOM;

  UpdateScreen(app->screen, HasErrors());

  Panel* current_panel = &app->screen->panels[app->screen->current_panel];

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
      case NOTES_TRANSITION:
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

  const int steps[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
  const size_t steps_count = sizeof(steps) / sizeof(steps[0]);
  updateTimerLog(app, steps, steps_count);

  updatePomodoroLog(app);

  return status;
}

/**
 * ---------------------------------------------------------------------------
 * Scene Updates
 * ---------------------------------------------------------------------------
 */

/**
 * Update MAIN_MENU scene state and menu selection.
 * @param app Pointer to the application data
 */
void UpdateMainMenu(AppData* app) {
  /* Tomato Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[MAIN_MENU];
    if (animation == NULL)
      SetError(app, "UpdateMainMenu", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
}

/**
 * Update WORK_TIME scene - timer countdown and animations.
 * @param app Pointer to the application data
 */
void UpdateWorkTime(AppData* app) {
  /* Coffee Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[WORK_TIME];
    if (animation == NULL)
      SetError(app, "UpdateWorkTime", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
  if (!app->is_paused) updatePomodoroTime(app);
  if (IsStepEnded(app->pomodoro_data.current_step_time,
                  app->pomodoro_data.work_time)) {
    if (WORK_LOG) {
      skip_auto_save = true;
      if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
        SetError(app, "Saving pomodoro on work end", TIMER_LOG_ERROR);
      skip_auto_save = false;
    }
    if (app->pomodoro_data.current_cycle >=
        app->pomodoro_data.total_cycles - 1) {
      ExecuteHistory(app->screen->panels[0].scene_history, LONG_PAUSE);
      app->screen->panels[0].menu_index = -1;
      app->pomodoro_data.current_step = LONG_PAUSE;
      app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
      app->pomodoro_data.current_step_time = 0;
      app->pomodoro_data.step_start_time = time(NULL);
      if (!AUTOSTART_PAUSE) app->is_paused = true;
      Notification notification = {
        .title = "Long Pause Break",
        .description = "You have some time to chill",
        .audio_path = DATADIR "/sounds/pausenotify.mp3",
      };
      if (Notify(&notification) != NO_ERROR)
        SetError(app, "Sending long pause notification",
                 NOTIFICATION_SEND_ERROR);
    } else {
      ExecuteHistory(app->screen->panels[0].scene_history, SHORT_PAUSE);
      app->screen->panels[0].menu_index = -1;
      app->pomodoro_data.current_step = SHORT_PAUSE;
      app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
      app->pomodoro_data.current_step_time = 0;
      app->pomodoro_data.step_start_time = time(NULL);
      if (!AUTOSTART_PAUSE) app->is_paused = true;
      Notification notification = {
        .title = "Pause Break",
        .description = "You have some time to chill",
        .audio_path = DATADIR "/sounds/pausenotify.mp3",
      };
      if (Notify(&notification) != NO_ERROR)
        SetError(app, "Sending short pause notification",
                 NOTIFICATION_SEND_ERROR);
    }
  }
}

/**
 * Update LONG_PAUSE scene - timer countdown and animations.
 * @param app Pointer to the application data
 */
void UpdateShortPause(AppData* app) {
  /* Coffee Machine Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[SHORT_PAUSE];
    if (animation == NULL)
      SetError(app, "UpdateShortPause", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
  if (!app->is_paused) updatePomodoroTime(app);
  if (IsStepEnded(app->pomodoro_data.current_step_time,
                  app->pomodoro_data.short_pause_time)) {
    if (WORK_LOG) {
      skip_auto_save = true;
      if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
        SetError(app, "Saving pomodoro on pause end", TIMER_LOG_ERROR);
      skip_auto_save = false;
    }
    ExecuteHistory(app->screen->panels[0].scene_history, WORK_TIME);
    app->screen->panels[0].menu_index = -1;
    app->pomodoro_data.current_step = WORK_TIME;
    app->pomodoro_data.current_cycle += 1;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    app->pomodoro_data.current_step_time = 0;
    app->pomodoro_data.step_start_time = time(NULL);
    if (!AUTOSTART_WORK) app->is_paused = true;
    Notification notification = {
      .title = "Work!",
      .description = "You need to focus",
      .audio_path = DATADIR "/sounds/dfltnotify.mp3",
    };
    if (Notify(&notification) != NO_ERROR)
      SetError(app, "Sending work notification", NOTIFICATION_SEND_ERROR);
  }
}

/**
 * Update LONG_PAUSE scene - timer countdown and animations.
 * @param app Pointer to the application data
 */
void UpdateLongPause(AppData* app) {
  /* Beach Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[LONG_PAUSE];
    if (animation == NULL)
      SetError(app, "UpdateLongPause", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
  if (!app->is_paused) updatePomodoroTime(app);
  if (IsStepEnded(app->pomodoro_data.current_step_time,
                  app->pomodoro_data.long_pause_time)) {
    if (WORK_LOG) {
      skip_auto_save = true;
      app->pomodoro_data.status = 0;
      if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
        SetError(app, "Saving pomodoro on long pause end", TIMER_LOG_ERROR);
      skip_auto_save = false;
    }
    ExecuteHistory(app->screen->panels[0].scene_history, MAIN_MENU);
    app->screen->panels[0].menu_index = MAIN_MENU_MENU;
    app->pomodoro_data.current_step = MAIN_MENU;
    app->pomodoro_data.current_cycle = 0;
    app->pomodoro_data.current_step_time = 0;
    app->pomodoro_data.total_elapsed = 0;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    Notification notification = {
      .title = "End of Pomodoro Cycle",
      .description = "Feel free to start another!",
      .audio_path = DATADIR "/sounds/endnotify.mp3",
    };
    if (Notify(&notification) != NO_ERROR)
      SetError(app, "Sending end notification", NOTIFICATION_SEND_ERROR);
  }
}

/**
 * Update NOTES scene - note selection and editing state.
 * Handles both normal notes display and page transition animation.
 * @param app Pointer to the application data
 */
void UpdateNotes(AppData* app) {
  if (!ANIMATIONS || app->is_paused) return;

  if (app->notes && app->notes->transitioning) {
    Rollfilm* transition = app->animations[NOTES_TRANSITION];
    if (transition == NULL) {
      SetError(app, "UpdateNotes", ANIMATION_EQUAL_NULL);
      return;
    }

    bool forward = app->notes->transition_target > app->notes->current_page;

    double current_time = GetCurrentTimeMS();
    double delta_time = current_time - transition->delta_frame_ms;

    if (delta_time >= 1000.0 * transition->frames->seconds_multiplier) {
      transition->delta_frame_ms = current_time;
      if (forward) {
        if (transition->current_frame < transition->frame_count - 1)
          RollfilmSeekFrame(transition, transition->current_frame + 1);
      } else {
        if (transition->current_frame > 0)
          RollfilmSeekFrame(transition, transition->current_frame - 1);
      }
    }

    int end_frame = forward ? transition->frame_count - 1 : 0;
    if (transition->current_frame == end_frame) {
      int old_page = app->notes->current_page;
      int old_id = app->notes->current_id;

      app->notes->current_page = app->notes->transition_target;
      app->notes->transitioning = false;

      /* Reset NOTES animation to last frame */
      RollfilmSeekFrame(app->animations[NOTES],
                        app->animations[NOTES]->frame_count - 1);

      /* Calculate index of previously selected note within old page */
      int old_idx = 0;
      if (old_id >= 0) {
        for (int i = 0; i < app->notes->count; i++) {
          if (app->notes->items[i]->page_id == old_page) {
            if (app->notes->items[i]->id == old_id) break;
            old_idx++;
          }
        }
      }

      /* Select note at same index on new page, or last if not enough items */
      app->notes->current_id = -1;
      int new_idx = 0;
      int last_id = -1;
      for (int i = 0; i < app->notes->count; i++) {
        if (app->notes->items[i]->page_id == app->notes->current_page) {
          last_id = app->notes->items[i]->id;
          if (new_idx == old_idx)
            app->notes->current_id = app->notes->items[i]->id;
          new_idx++;
        }
      }
      if (app->notes->current_id < 0) app->notes->current_id = last_id;
    }
  } else {
    Rollfilm* animation = app->animations[NOTES];
    if (animation == NULL)
      SetError(app, "UpdateNotes", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
}

/**
 * Update HELP scene - help content display.
 * @param app Pointer to the application data
 */
void UpdateHelp(AppData* app) {
  /* Scroll Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[HELP];
    if (animation == NULL)
      SetError(app, "UpdateHelp", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
}

/**
 * Update CONTINUE scene - pause/resume confirmation.
 * @param app Pointer to the application data
 */
void UpdateContinue(AppData* app) {
  /* Banner Animation */
  if (ANIMATIONS && !app->is_paused) {
    Rollfilm* animation = app->animations[CONTINUE];
    if (animation == NULL)
      SetError(app, "UpdateContinue", ANIMATION_EQUAL_NULL);
    else
      animation->update(animation);
  }
}

/**
 * ---------------------------------------------------------------------------
 * Pomodoro
 * ---------------------------------------------------------------------------
 */

/**
 * Update pomodoro timer data - decrement time, check for step end.
 * @param app Pointer to the application data
 */
static void updatePomodoroTime(AppData* app) {
  double current_time = GetCurrentTimeMS();
  double delta_time = current_time - app->pomodoro_data.delta_time_ms;

  int speed_multiplier = 1;
  if (DEBUG) speed_multiplier = 100;
  if (delta_time >= 1000.0 / speed_multiplier) {
    app->pomodoro_data.delta_time_ms = current_time;
    app->pomodoro_data.current_step_time += 1;
    app->pomodoro_data.total_elapsed += 1;
    if (app->pomodoro_data.current_step_time ==
        app->pomodoro_data.last_step_time + 10)
      app->pomodoro_data.last_step_time = app->pomodoro_data.current_step_time;
  }
}

/**
 * Update pomodoro log file with current state.
 * Triggers on step change and every 60 seconds (or 60 timer seconds in DEBUG mode).
 * @param app Pointer to the application data
 */
static void updatePomodoroLog(AppData* app) {
  if (!WORK_LOG || skip_auto_save) return;

  static int last_logged_step = -1;
  static int last_logged_minute = -1;

  int speed_multiplier = 1;
  if (DEBUG) speed_multiplier = 100;

  int current_step = app->pomodoro_data.current_step;
  int step_time = app->pomodoro_data.current_step_time;
  int current_minute = step_time / (60 * speed_multiplier);

  if (current_step == MAIN_MENU) {
    last_logged_step = -1;
    last_logged_minute = -1;
    return;
  }

  if (current_step != WORK_TIME && current_step != SHORT_PAUSE &&
      current_step != LONG_PAUSE) {
    return;
  }

  if (last_logged_step == -1 || last_logged_step != current_step) {
    if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
      SetError(app, "Logging pomodoro step change", TIMER_LOG_ERROR);
    last_logged_step = current_step;
    last_logged_minute = current_minute;
  } else if (current_minute != last_logged_minute && current_minute > 0) {
    if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, false) != NO_ERROR)
      SetError(app, "Logging pomodoro minute", TIMER_LOG_ERROR);
    last_logged_minute = current_minute;
  }
}

/**
 * Update the timer log socket with current step information.
 * @param app Pointer to the application data
 * @param steps Array of step types to log
 * @param steps_count Number of steps in the array
 */
static void updateTimerLog(AppData* app, const int* steps,
                           const size_t steps_count) {
  if (IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step)) {
    char* time_string = FormatTimerLog(app->pomodoro_data, app->is_paused);
    if (time_string != NULL) {
      SetTimerLog(TIMER_FILE, time_string);
      free(time_string);
    } else {
      SetError(app, "updateTimerLog", TIMER_LOG_ERROR);
    }
  }
}
