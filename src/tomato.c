#include "tomato.h"

#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
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

#ifndef TOMATO_VERSION
#define TOMATO_VERSION "unknown"
#endif

/* Helper function to handle errors and cleanup */
static void handleErrorAndExit(const char* context, ErrorType error,
                               AppData* app, pid_t pid);

/**
 * Print the command-line usage.
 * @param stream Output stream, stdout for --help and stderr for errors.
 */
static void printUsage(FILE* stream) {
  fprintf(stream,
          "usage: tomato [--help] [--version]\n"
          "       tomato [-loop true|false] -t [iconset]\n"
          "       tomato -h [overview|today]\n"
          "\n"
          "  -t                stream the timer status\n"
          "  -loop true|false  stream the timer status (default: true);\n"
          "                    false prints once and exits\n"
          "  -h                show pomodoro history\n"
          "  --help            show this help\n"
          "  --version         show the version\n"
          "  iconset           nerd-icons, emojis, ascii, or no-icons\n");
}

/**
 * Parse a true/false option value.
 * @param arg Option value.
 * @param value Receives the parsed boolean.
 * @return true on success, false on an invalid value.
 */
static bool parseBool(const char* arg, bool* value) {
  if (strcmp(arg, "true") == 0)
    *value = true;
  else if (strcmp(arg, "false") == 0)
    *value = false;
  else
    return false;
  return true;
}

/**
 * Parse the optional icon-set argument of a timer command.
 * @param arg Icon-set name.
 * @param icon_type Receives the selected icon set.
 * @return true on success, false on an unknown icon set.
 */
static bool parseIconSet(const char* arg, IconType* icon_type) {
  if (strcmp(arg, "nerd-icons") == 0)
    *icon_type = NERD_ICONS;
  else if (strcmp(arg, "emojis") == 0)
    *icon_type = EMOJIS;
  else if (strcmp(arg, "ascii") == 0)
    *icon_type = ASCII;
  else if (strcmp(arg, "no-icons") == 0)
    *icon_type = NO_ICONS;
  else
    return false;
  return true;
}

int main(int argc, char* argv[]) {
  LoadConfig();

  bool loop = true;
  int timer_arg = 0;

  if (argc >= 2 && !strcmp("-loop", argv[1])) {
    if (argc < 3 || !parseBool(argv[2], &loop)) {
      printUsage(stderr);
      return 1;
    }
    timer_arg = 3;
    if (timer_arg >= argc || strcmp("-t", argv[timer_arg]) != 0) {
      printUsage(stderr);
      return 1;
    }
  } else if (argc >= 2 && !strcmp("-t", argv[1])) {
    timer_arg = 1;
  }

  if (timer_arg != 0) {
    if (TIMER_LOG != 1) {
      printf("enable timer log to use [-t]\n");
      return 1;
    }
    int next = timer_arg + 1;
    if (next < argc && !strcmp("-loop", argv[next])) {
      if (next + 1 >= argc || !parseBool(argv[next + 1], &loop)) {
        printUsage(stderr);
        return 1;
      }
      next += 2;
    }
    IconType icon_type = GetConfigIconType();
    if (next < argc) {
      if (!parseIconSet(argv[next], &icon_type)) {
        printUsage(stderr);
        return 1;
      }
      next++;
    }
    if (next != argc) {
      printUsage(stderr);
      return 1;
    }
    return GetTimerLog(TIMER_FILE, loop, icon_type);
  } else if (argc >= 2 && !strcmp("-h", argv[1])) {
    if (HISTORY_LOG != 1) {
      printf("enable history log to use [-h]\n");
      return 1;
    }
    if (argc == 2 || (argc == 3 && !strcmp("overview", argv[2]))) {
      GetPomodoroHistory(POMODORO_LOG);
      return 0;
    }
    if (argc == 3 && !strcmp("today", argv[2])) {
      GetPomodoroHistoryDay(POMODORO_LOG);
      return 0;
    }
    printUsage(stderr);
    return 1;
  } else if (argc >= 2 && !strcmp("--help", argv[1])) {
    if (argc != 2) {
      printUsage(stderr);
      return 1;
    }
    printUsage(stdout);
    return 0;
  } else if (argc >= 2 && !strcmp("--version", argv[1])) {
    if (argc != 2) {
      printUsage(stderr);
      return 1;
    }
    printf("tomato %s\n", TOMATO_VERSION);
    return 0;
  } else if (argc != 1) {
    printUsage(stderr);
    return 1;
  }

  /* Enable emojis */
  setlocale(LC_CTYPE, "");

  /* Init everything */
  AppData app = {0};
  if (InitScreen() != NO_ERROR)
    handleErrorAndExit("Initializing screen", WINDOW_CREATION_ERROR, NULL, 0);
  ErrorType init_status = InitApp(&app);
  if (init_status != NO_ERROR)
    handleErrorAndExit("Initializing app data", init_status, NULL, 0);
  for (int i = 0; i < GetConfigWarningCount(); i++) {
    ErrorLevel level = ERROR_LEVEL_WARNING;
    const char* message = GetConfigWarning(i, &level);
    SetErrorMessage(level, message);
  }

  /* Start timer server and wait until it listens before the UI sends updates. */
  int ready_pipe[2];
  if (pipe(ready_pipe) != 0)
    handleErrorAndExit("Creating timer log readiness pipe", FILE_ERROR, &app,
                       0);

  pid_t pid = fork();
  if (pid == 0) {
    close(ready_pipe[0]);
    ErrorType timer_status =
      CreateTimerLogWithReadyFD(TIMER_FILE, ready_pipe[1]);
    if (timer_status != NO_ERROR) {
      close(ready_pipe[1]);
      LogError("Timer log server failure", TIMER_LOG_ERROR);
    }
    _exit(0);
  } else if (pid < 0) {
    close(ready_pipe[0]);
    close(ready_pipe[1]);
    handleErrorAndExit("Forking the main process", FORK_ERROR, NULL, 0);
  }

  close(ready_pipe[1]);
  char server_ready = '\0';
  ssize_t ready_size;
  do {
    ready_size = read(ready_pipe[0], &server_ready, sizeof(server_ready));
  } while (ready_size < 0 && errno == EINTR);
  close(ready_pipe[0]);
  if (ready_size != sizeof(server_ready) || server_ready != 'R')
    SetError(&app, "Starting timer log server", TIMER_LOG_ERROR);

  if (DEBUG) {
    /* TEST ERROR LINE - uncomment to test */
    // TestErrorLine(&app, "Test INFO message", ERROR_LEVEL_INFO, true);
    // TestErrorLine(&app, "Test WARNING message", ERROR_LEVEL_WARNING, true);
    // TestErrorLine(&app, "Test ERROR message", ERROR_LEVEL_ERROR, true);
    // TestErrorLine(&app, "Test DEBUG message", ERROR_LEVEL_DEBUG, true);
    // TestErrorLine(&app, "Test CRITICAL message", ERROR_LEVEL_CRITICAL, true);
  }

  /* Main application loop */
  while (app.running) {
    if (UpdateApp(&app) != NO_ERROR)
      handleErrorAndExit("Updating app", UPDATE_ERROR, &app, pid);
    if (HandleInputs(&app) != NO_ERROR)
      handleErrorAndExit("Handling input", INPUT_ERROR, &app, pid);
    if (DrawScreen(&app) != NO_ERROR)
      handleErrorAndExit("Drawing screen", DRAW_ERROR, &app, pid);
    napms(FPMS);
  }

  /* Cleanup */
  if (kill(pid, SIGTERM) != 0)
    SetError(&app, "Killing timer log process", END_APP_ERROR);
  if (waitpid(pid, NULL, 0) < 0)
    SetError(&app, "Waiting for timer log process", END_APP_ERROR);

  if (EndScreen() != NO_ERROR)
    SetError(&app, "Ending app screen", END_SCREEN_ERROR);
  if (EndApp(&app) != NO_ERROR) SetError(&app, "Ending app", END_APP_ERROR);

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
    if (kill(pid, SIGTERM) != 0)
      LogError("Killing child process on exit", END_APP_ERROR);
    if (waitpid(pid, NULL, 0) < 0)
      LogError("Waiting for child process on exit", END_APP_ERROR);
  }
  endwin();
  LogError(context, error);
  fprintf(stderr, "Tomato.C [ErrorCode %d]: %s\n", error,
          GetErrorMessage(error));
  exit(error);
}
