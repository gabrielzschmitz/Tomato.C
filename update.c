/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// update.c
*/
#include "update.h"

#include <ncurses.h>

#include "anim.h"
#include "config.h"
#include "notify.h"
#include "tomato.h"
#include "util.h"

/* Mode -3 (Help Page) */
void updateHelpPage(appData* app) {
  frameTimer(app);
  if (app->runHelpOnce == 1) {
    /* Pergament Animation */
    app->helpFrame = app->frameTimer;
    if (app->frameTimer >= 4) {
      app->helpFrame = 4;
      app->frameTimer = 0;
      app->runHelpOnce = 0;
    }
  }
}

/* Mode -2 (Notepad) */
void updateNotepad(appData* app) {
  if (app->lastMode > 0) timer(app);
  frameTimer(app);
  if (app->runNotepadOnce == 1) {
    /* Notepad Animation */
    app->notepadFrame = app->frameTimer;
    if (app->frameTimer >= 6) {
      app->notepadFrame = 6;
      app->frameTimer = 0;
      app->runNotepadOnce = 0;
    }
  }

  if (app->timer <= 0) {
    switch (app->lastMode) {
      case 1:
        if (app->autostartWork == 0 && app->pausedTimer == 0) {
          app->pausedTimer = 1;
          notify("autostartwork");
          break;
        }
        if (app->pomodoroCounter == app->pomodoros) {
          app->timer = app->longPause;
          app->lastMode = 3;
          notify("longpause");
        } else {
          app->timer = app->shortPause;
          app->lastMode = 2;
          notify("shortpause");
        }
        app->frameTimer = 0;
        break;
      case 2:
        if (app->autostartPause == 0 && app->pausedTimer == 0) {
          app->pausedTimer = 1;
          notify("autostartpause");
          break;
        }
        notify("worktime");
        app->timer = app->workTime;
        app->frameTimer = 0;
        app->lastMode = 1;
        app->pomodoroCounter += 1;
        break;
      case 3:
        app->lastMode = 0;
        app->cycles += 1;
        app->needToLog = 1;
        if (WORKLOG == 1) writeToLog(app);
        app->needToLog = 0;
        app->pomodoroCounter = 0;
        if (TIMERLOG == 1) endTimerLog(app);
        notify("end");
        break;
      default:
        break;
    }
  }
}

/* Mode 0 (Main Menu) */
void updateMainMenu(appData* app) {
  app->pausedTimer = 0;
  frameTimer(app);

  if (app->needResume == 0) {
    /* Tomato Animation */
    app->logoFrame = (app->frameTimer / 8) % 8;
    if (app->frameTimer == (8 * 8)) {
      app->logoFrame = 0;
      app->frameTimer = 0;
    }
  } else if (app->runOnce == 1) {
    /* Banner Animation */
    app->bannerFrame = app->frameTimer % 4;
    if (app->frameTimer == (3 * 1)) {
      app->bannerFrame = 3;
      app->frameTimer = 0;
      app->runOnce = 0;
    }
  }
}

/* Mode 1 (Work Time) */
void updateWorkTime(appData* app) {
  if (app->runOnce == 1 && app->pausedTimer == 0) {
    notify("worktime");
    app->runOnce = 0;
  }

  timer(app);
  frameTimer(app);
  if (app->timer <= 0) {
    completeWorkHistory(app);
    if (app->autostartWork == 0 && app->pausedTimer == 0) {
      app->pausedTimer = 1;
      notify("autostartwork");
    } else {
      if (app->pomodoroCounter == app->pomodoros) {
        app->timer = app->longPause;
        app->currentMode = 3;
      } else {
        app->timer = app->shortPause;
        app->currentMode = 2;
      }
      app->frameTimer = 0;
      app->lastMode = app->currentMode;
      app->runOnce = 1;
    }
  }

  /* Coffee Animation */
  if (app->frameTimer == (3 * 8))
    app->coffeeFrame = 1;
  else if (app->frameTimer == (6 * 8)) {
    app->coffeeFrame = 0;
    app->frameTimer = 0;
  }
}

/* Mode 2 (Short Pause) */
void updateShortPause(appData* app) {
  if (app->runOnce == 1 && app->pausedTimer == 0) {
    notify("shortpause");
    app->runOnce = 0;
  }

  timer(app);
  frameTimer(app);
  if (app->timer <= 0) {
    if (app->autostartPause == 0 && app->pausedTimer == 0) {
      app->pausedTimer = 1;
      notify("autostartpause");
    } else {
      app->timer = app->workTime;
      app->frameTimer = 0;
      app->lastMode = app->currentMode;
      app->currentMode = 1;
      beginWorkHistory(app, 0);
      app->pomodoroCounter += 1;
      app->runOnce = 1;
    }
  }

  /* Machine Animation */
  if (app->frameTimer == (2 * 8))
    app->machineFrame = 1;
  else if (app->frameTimer == (4 * 8))
    app->machineFrame = 2;
  else if (app->frameTimer == (6 * 8)) {
    app->machineFrame = 0;
    app->frameTimer = 0;
  }
}

/* Mode 3 (Long Pause) */
void updateLongPause(appData* app) {
  if (app->runOnce == 1 && app->pausedTimer == 0) {
    notify("longpause");
    app->runOnce = 0;
  }

  timer(app);
  frameTimer(app);
  if (app->timer <= 0 && app->pausedTimer == 0) {
    app->lastMode = app->currentMode;
    app->currentMode = 0;
    app->cycles += 1;
    app->needToLog = 1;
    if (WORKLOG == 1) writeToLog(app);
    app->needToLog = 0;
    app->pomodoroCounter = 0;
    if (TIMERLOG == 1) endTimerLog(app);
    notify("end");
    app->timer = 0;
  }

  /* Beach Animation */
  if (app->frameTimer == (3 * 8))
    app->beachFrame = 1;
  else if (app->frameTimer == (6 * 8)) {
    app->beachFrame = 0;
    app->frameTimer = 0;
  }
}
