#include "draw.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "notes.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  /* Only render quit popup if in DEFAULT mode */
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  bool is_default_mode =
    (current_mode == DEFAULT) || (input && input->len == 0);
  if (is_default_mode && IsKeyAssignedToAction(app->user_input, QuitApp))
    RenderQuitConfirmation(app);

  int steps[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
  size_t steps_count = sizeof(steps) / sizeof(steps[0]);
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (IsKeyAssignedToAction(app->last_input, SkipPomodoroStep) &&
      IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step) &&
      (POMODORO_SCENES & (1 << current_scene)))
    RenderSkipConfirmation(app);
  if ((IsKeyAssignedToAction(app->last_input, ResetPomodoroStep) ||
       IsKeyAssignedToAction(app->last_input, ResetPomodoroCycle)) &&
      IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step) &&
      (POMODORO_SCENES & (1 << current_scene)))
    RenderResetMenu(app);

  if (!CheckScreenSize(app)) return status;

  Border border = InitBorder();
  for (int i = 0; i < MAX_PANELS; i++) {
    Panel* current_panel = &app->screen->panels[i];
    if (!current_panel->visible) continue;

    Rollfilm* animation = NULL;

    int indices[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
    int largest_index = FindLargestRollfilm(app->animations, indices, 3);
    if (largest_index == -1) return ANIMATION_EQUAL_NULL;
    Dimensions size = {.width = app->animations[largest_index]->frame_width,
                       .height = app->animations[largest_index]->frame_height};
    Vector2D position = {.x = current_panel->position.x +
                              (current_panel->size.width -
                               app->animations[largest_index]->frame_width) /
                                2,
                         .y = current_panel->position.y +
                              (current_panel->size.height -
                               app->animations[largest_index]->frame_height) /
                                2};

    switch (current_panel->scene_history->present) {
      case MAIN_MENU:
        if (ANIMATIONS) {
          animation = app->animations[MAIN_MENU];
          if (DEBUG)
            DebugAnimation(current_panel, animation, (Vector2D){0, 0});
          else
            RenderAnimationAtPanelCenter(current_panel, animation,
                                         (Vector2D){0, 0});
        }
        if (DEBUG)
          PrintMenuAtCenter(current_panel, app->menus[0],
                            (Vector2D){0, animation->frame_height / 2 + 3}, 0);
        else
          PrintMenuAtCenter(current_panel, app->menus[0],
                            (Vector2D){0, animation->frame_height / 2 + 2}, 0);
        break;
      case WORK_TIME:
        if (ANIMATIONS) {
          animation = app->animations[WORK_TIME];
          if (DEBUG)
            DebugAnimation(current_panel, animation, (Vector2D){0, 0});
          else
            RenderAnimationAtPanelCenter(current_panel, animation,
                                         (Vector2D){0, 0});
        }
        RenderPomodoroStatus(app, size, position);
        break;
      case SHORT_PAUSE:
        if (ANIMATIONS) {
          animation = app->animations[SHORT_PAUSE];
          if (DEBUG)
            DebugAnimation(current_panel, animation, (Vector2D){0, 0});
          else
            RenderAnimationAtPanelCenter(current_panel, animation,
                                         (Vector2D){0, 0});
        }
        if (DEBUG)
          RenderPomodoroStatus(app, size,
                               (Vector2D){position.x, position.y + 2});
        else
          RenderPomodoroStatus(app, size, position);
        break;
      case LONG_PAUSE:
        if (ANIMATIONS) {
          animation = app->animations[LONG_PAUSE];
          if (DEBUG)
            DebugAnimation(current_panel, animation, (Vector2D){0, 0});
          else
            RenderAnimationAtPanelCenter(current_panel, animation,
                                         (Vector2D){0, 0});
        }
        if (DEBUG)
          RenderPomodoroStatus(app, size,
                               (Vector2D){position.x, position.y + 1});
        else
          RenderPomodoroStatus(app, size, position);
        break;
      case NOTES:
        if (ANIMATIONS) animation = app->animations[NOTES];
        /* Render notes below the animation */
        if (app->notes != NULL) {
          int start_x, start_y, end_x, end_y;
          if (animation != NULL) {
            int panel_center_x =
              current_panel->position.x + current_panel->size.width / 2;
            int panel_center_y =
              current_panel->position.y + current_panel->size.height / 2;
            int anim_x = panel_center_x - animation->frame_width / 2;
            int anim_y = panel_center_y - animation->frame_height / 2;
            int blank_x, blank_y;
            if (FindFirstBlankInLastFrame(animation, &blank_x, &blank_y)) {
              start_x = anim_x + blank_x;
              start_y = anim_y + blank_y;
            } else {
              start_x = anim_x;
              start_y = anim_y + animation->frame_height;
            }
            if (FindLastBlankInLastFrame(animation, &blank_x, &blank_y)) {
              end_x = anim_x + blank_x;
              end_y = anim_y + blank_y + 1;
            } else {
              end_x = anim_x + animation->frame_width;
              end_y = anim_y + animation->frame_height;
            }
          } else {
            start_x = current_panel->position.x + 1;
            start_y = current_panel->position.y + 1;
            end_x = current_panel->position.x + current_panel->size.width - 1;
            end_y = current_panel->position.y + current_panel->size.height - 1;
          }
          /* Ensure valid bounds */
          if (end_x <= start_x) end_x = start_x + 1;
          if (end_y <= start_y) end_y = start_y + 1;
          /* Pass input state and mode if in INSERT/NORMAL/VISUAL mode */
          int current_mode =
            app->screen->panels[app->screen->current_panel].mode;
          InputState* input = current_panel->input;

          RenderNotes(app->notes, start_x, start_y, end_x, end_y, input,
                      current_mode);
        }
        if (ANIMATIONS) {
          if (DEBUG)
            DebugAnimation(current_panel, animation, (Vector2D){0, 0});
          else
            RenderAnimationAtPanelCenter(current_panel, animation,
                                         (Vector2D){0, 0});
        }
        break;
      case HELP:
        if (ANIMATIONS) animation = app->animations[HELP];
        break;
      case CONTINUE:
        if (ANIMATIONS) animation = app->animations[CONTINUE];
        break;
      default:
        break;
    }
    if (DEBUG && current_panel->visible)
      mvprintw(1, 2, "CS%d - %c/%c - %dcm", app->pomodoro_data.current_step,
               app->user_input, app->last_input, app->current_menu);

    SetColor((app->screen->current_panel == i) ? FOCUSED_PANEL_COLOR
                                               : UNFOCUSED_PANEL_COLOR,
             NO_COLOR, A_NORMAL);
    RenderPanelBorder(*current_panel, border);
  }

  RenderStatusBar(app->status_bar, app->screen);

  if (app->popup_dialog != NULL &&
      (current_mode == NORMAL || current_mode == DEFAULT)) {
    int current_scene =
      app->screen->panels[app->screen->current_panel].scene_history->present;
    if (app->popup_dialog->menu.items[0].action == ForcefullyQuitApp)
      RenderQuitConfirmation(app);
    else if (app->popup_dialog->menu.items[0].action ==
               ForcefullySkipPomodoroStep &&
             (POMODORO_SCENES & (1 << current_scene)))
      RenderSkipConfirmation(app);
    else if ((app->popup_dialog->menu.items[0].action == ResetPomodoroStep ||
              app->popup_dialog->menu.items[0].action == ResetPomodoroCycle) &&
             (POMODORO_SCENES & (1 << current_scene)))
      RenderResetMenu(app);
  }

  if (DEBUG) {
    SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
    Panel* current_panel = &app->screen->panels[app->screen->current_panel];
    mvprintw(current_panel->position.y, current_panel->position.x,
             "%02dM - %dP - %02dWx%02dH", current_panel->scene_history->present,
             app->is_paused, current_panel->size.width,
             current_panel->size.height);
    mvprintw(current_panel->position.y, current_panel->position.x + 20, "%dx%d",
             app->screen->min_panel_size.width,
             app->screen->min_panel_size.height);
  }

  refresh();
  return status;
}

/* Check and Render Screen Size */
bool CheckScreenSize(AppData* app) {
  if (app->screen->size.width < app->screen->min_panel_size.width ||
      app->screen->size.height < app->screen->min_panel_size.height) {
    app->block_input = true;
    RenderScreenSizeError(app->screen,
                          &app->screen->panels[app->screen->current_panel]);
    refresh();
    return false;
  }
  app->block_input = false;
  return true;
}

/* Show debug info and render a animation */
void DebugAnimation(Panel* panel, Rollfilm* animation, Vector2D offset) {
  const int DEBUG_INFO_WIDTH = 23;
  if (animation) {
    RenderAnimationAtPanelCenter(panel, animation, offset);

    int panel_center_x = panel->position.x + panel->size.width / 2;
    int panel_center_y = panel->position.y + panel->size.height / 2;

    int frame_x = panel_center_x - animation->frame_width / 2 + offset.x;
    int frame_y = panel_center_y - animation->frame_height / 2 + offset.y;

    int debug_y = frame_y + animation->frame_height;
    int debug_x = frame_x + (animation->frame_width - DEBUG_INFO_WIDTH) / 2;

    SetColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(debug_y, debug_x, "%dC - %dW - %1.3lfS - %dID",
             animation->frame_count, animation->frame_width,
             animation->frames->seconds_multiplier, animation->frames->id);
    mvprintw(debug_y + 1, debug_x + 5, "%lf", animation->delta_frame_ms);
  }
}
