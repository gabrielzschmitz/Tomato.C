#include "tomato.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "draw.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "log.h"
#include "ui.h"
#include "update.h"
#include "util.h"

/* Helper function to handle errors and cleanup */
static void handleErrorAndExit(const char* context, ErrorType error,
                               AppData* app, pid_t pid);

int main(int argc, char* argv[]) {
  if (argc == 2 && !strcmp("-t", argv[1])) {
    if (TIMER_LOG == 1) return GetTimerLog(TIMER_FILE, true);
    printf("enable timer log to use [-t]\n");
    return 1;
  } else if (argc == 2 && !strcmp("-h", argv[1])) {
    if (WORK_LOG == 1) {
      GetPomodoroHistory(POMODORO_LOG);
      return 0;
    }
    printf("enable work log to use [-h]\n");
    return 1;
  } else if (argc != 1) {
    printf("usage: tomato [-t] [-h]\n");
    return 0;
  }

  /* Enable emojis */
  setlocale(LC_CTYPE, "");

  /* Init everything */
  AppData app;
  InitScreen();
  if (InitApp(&app) != NO_ERROR)
    handleErrorAndExit("Initializing app data", INIT_ERROR, NULL, 0);

  /* Log process */
  pid_t pid = fork();
  if (pid == 0) {
    /* Timer log process */
    if (CreateTimerLog(TIMER_FILE) != NO_ERROR)
      LogError("Timer log server failure", TIMER_LOG_ERROR);
    exit(0);
  } else if (pid < 0)
    handleErrorAndExit("Forking the main process", FORK_ERROR, NULL, 0);

  if (DEBUG) {
    /* TEST ERROR LINE - uncomment to test */
    // TestErrorLine("Test INFO message", ERROR_LEVEL_INFO);
    // TestErrorLine("Test WARNING message", ERROR_LEVEL_WARNING);
    // TestErrorLine("Test ERROR message", ERROR_LEVEL_ERROR);
    // TestErrorLine("Test DEBUG message", ERROR_LEVEL_DEBUG);
    // TestErrorLine("Test CRITICAL message", ERROR_LEVEL_CRITICAL);
  }

  /* Main application loop */
  while (app.running) {
    if (DEBUG) {
      /* TEST ERROR LINE - uncomment to test */
      // app.frozen = true;
      // RenderCriticalQuitConfirmation(&app);
    }

    if (UpdateApp(&app) != NO_ERROR)
      handleErrorAndExit("Updating app", UPDATE_ERROR, &app, pid);
    if (HandleInputs(&app) != NO_ERROR)
      handleErrorAndExit("Handling input", INPUT_ERROR, &app, pid);
    if (DrawScreen(&app) != NO_ERROR)
      handleErrorAndExit("Drawing screen", DRAW_ERROR, &app, pid);
    napms(FPMS);
  }

  /* Cleanup */
  kill(pid, SIGTERM);
  waitpid(pid, NULL, 0);

  if (EndScreen() != NO_ERROR) LogError("Ending app screen", END_SCREEN_ERROR);
  if (EndApp(&app) != NO_ERROR) LogError("Ending app", END_APP_ERROR);

  printf("Goodbye!\n");

  return 0;
}

/**
 * Handles fatal errors, performs cleanup, and terminates the process.
 * @param context Human-readable description of the error context.
 * @param error Error code describing the failure condition.
 * @param app Pointer to the application state structure. May be NULL.
 * @param pid Process ID of a child process to terminate. Ignored if <= 0.
 */
static void handleErrorAndExit(const char* context, ErrorType error,
                               AppData* app, pid_t pid) {
  if (app) EndApp(app);
  if (pid > 0) {
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
  }
  endwin();
  LogError(context, error);
  exit(error);
}
