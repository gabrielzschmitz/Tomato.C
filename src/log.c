#define _XOPEN_SOURCE 700

#include "log.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "gap_buffer.h"
#include "notes.h"
#include "tomato.h"

/* PRIVATE LOG FUNCTIONS */
/* Retry configuration for socket connections */
#define SOCKET_RETRY_COUNT 3
#define SOCKET_RETRY_DELAY_NS 1000000000L /* 1000ms */

/**
 * Summary of a single recent pomodoro session for history display.
 */
typedef struct {
  int sessionIndex; /**< Session index from the binary log */
  int cycle;        /**< Current cycle number (1-based) */
  int totalCycles;  /**< Total pomodoro cycles configured */
  int workTime;     /**< Work time duration in minutes */
  int shortPause;   /**< Short pause duration in minutes */
  int longPause;    /**< Long pause duration in minutes */
  int totalElapsed; /**< Total elapsed time across steps (seconds) */
  int
    step; /**< Current step at last record (0=work, 1=short pause, 2=long pause) */
  int status;      /**< 0 = completed, 1 = uncompleted */
  int sessionMins; /**< Total session time in minutes (completed only) */
} recentSessionInfo;

/**
 * Aggregated pomodoro history statistics for the CLI report.
 */
typedef struct {
  int totalSessions;       /**< Total unique sessions */
  int completedSessions;   /**< Count of completed sessions */
  int totalWorkMinutes;    /**< Total work time in minutes */
  int workHours;           /**< Work hours (totalWorkMinutes / 60) */
  int workMins;            /**< Work minutes remainder */
  int totalSessionMinutes; /**< Total session time including pauses (minutes) */
  int sessionHours;        /**< Session hours (totalSessionMinutes / 60) */
  int sessionMins;         /**< Session minutes remainder */
  int numRecent;           /**< Number of recent session entries populated */
  recentSessionInfo recentSessions
    [MAX_RECENT_SESSIONS]; /**< Array of recent session summaries */
} pomodoroHistoryStats;

/* Notes */
static NoteState charToNoteState(char c);
/* Pomodoro */
static pomodoroHistoryStats createPomodoroHistoryStats(const char* path,
                                                       int maxRecent);
/* Socket */
static void socketRetryDelay(void) {
  struct timespec ts = {0, SOCKET_RETRY_DELAY_NS};
  nanosleep(&ts, NULL);
}

/**
 * ---------------------------------------------------------------------------
 * Timer
 * ---------------------------------------------------------------------------
 */

/**
 * Create a timer log server using Unix domain sockets.
 * Server listens for connections and serves timer data.
 * @param path File path for the Unix socket
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType CreateTimerLog(const char* path) {
  int server_sock, client_sock;
  struct sockaddr_un addr;
  fd_set master_set, working_set;
  int max_sd;
  const int buffer_size = 32;
  char buffer[buffer_size];
  char last_message[buffer_size];
  last_message[0] = '\0';

  /* Create Unix socket */
  server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_sock == -1) {
    LogError("CreateTimerLog", SOCKET_CREATION_ERROR);
    return SOCKET_CREATION_ERROR;
  }

  /* Set up address structure */
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  /* Unlink if the path exists */
  if (unlink(path) == -1 && errno != ENOENT) {
    LogError("CreateTimerLog", UNLINK_ERROR);
    close(server_sock);
    return UNLINK_ERROR;
  }

  /* Bind and listen */
  if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    LogError("CreateTimerLog", SOCKET_BIND_ERROR);
    close(server_sock);
    return SOCKET_BIND_ERROR;
  }
  chmod(path, 0700); /* restrict socket to owner only */
  if (listen(server_sock, 5) == -1) {
    LogError("CreateTimerLog", SOCKET_LISTEN_ERROR);
    close(server_sock);
    return SOCKET_LISTEN_ERROR;
  }

  FD_ZERO(&master_set);
  FD_SET(server_sock, &master_set);
  max_sd = server_sock;

  while (1) {
    working_set = master_set;

    if (select(max_sd + 1, &working_set, NULL, NULL, NULL) < 0) {
      if (errno == EINTR) continue;
      LogError("CreateTimerLog", SOCKET_READ_ERROR);
      break;
    }

    for (int i = 0; i <= max_sd; i++) {
      if (FD_ISSET(i, &working_set)) {
        if (i == server_sock) {
          /* Accept new connection */
          client_sock = accept(server_sock, NULL, NULL);
          if (client_sock == -1) {
            if (errno != EINTR && errno != ECONNABORTED)
              LogError("CreateTimerLog", SOCKET_ACCEPT_ERROR);
            continue;
          }
          FD_SET(client_sock, &master_set);
          if (client_sock > max_sd) max_sd = client_sock;
        } else {
          /* Read from client */
          ssize_t n = recv(i, buffer, sizeof(buffer) - 1, 0);
          if (n > 0) {
            buffer[n] = '\0';

            /* Sanitize input and store */
            char* newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            strncpy(last_message, buffer, buffer_size - 1);
            last_message[buffer_size - 1] = '\0';

            /* Broadcast last message */
            for (int j = 0; j <= max_sd; j++) {
              if (FD_ISSET(j, &master_set) && j != server_sock && j != i) {
                if (send(j, last_message, strlen(last_message), 0) == -1) {
                  if (errno != EINTR && errno != ECONNRESET && errno != EPIPE)
                    LogError("CreateTimerLog", SOCKET_WRITE_ERROR);
                  close(j);
                  FD_CLR(j, &master_set);
                }
              }
            }
          } else if (n == 0) {
            /* Connection closed */
            close(i);
            FD_CLR(i, &master_set);
          } else {
            if (errno != EINTR && errno != ECONNRESET)
              LogError("CreateTimerLog", SOCKET_READ_ERROR);
            close(i);
            FD_CLR(i, &master_set);
          }
        }
      }
    }
  }

  close(server_sock);
  unlink(path);
  return NO_ERROR;
}

/**
 * Get the timer log from the socket (client-side).
 * Connects to the timer log server and reads current status.
 * @param path File path for the Unix socket
 * @param loop Whether to continuously poll the socket
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType GetTimerLog(const char* path, bool loop) {
  const int buffer_size = 32;
  char buffer[buffer_size];
  ssize_t n;

  /* Create socket */
  int sock;
  struct sockaddr_un addr;
  int retries = 0;

  do {
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
      LogError("GetTimerLog", SOCKET_CREATION_ERROR);
      return SOCKET_CREATION_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) break;

    close(sock);
    sock = -1;
    retries++;
    if (retries < SOCKET_RETRY_COUNT) socketRetryDelay();
  } while (retries < SOCKET_RETRY_COUNT);

  if (sock == -1) {
    LogError("GetTimerLog", SOCKET_CONNECTION_ERROR);
    return SOCKET_CONNECTION_ERROR;
  }

  do {
    /* Drain the socket buffer to skip old logs */
    do {
      n = recv(sock, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    } while (n > 0);

    if (n == -1 && errno != EAGAIN) {
      LogError("GetTimerLog", SOCKET_READ_ERROR);
      close(sock);
      return SOCKET_READ_ERROR;
    }

    /* Wait for the latest log */
    n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
      buffer[n] = '\0';       /* Null-terminate the received message */
      printf("%s\n", buffer); /* Print the latest log */
    } else if (n == 0) {
      /* Server closed the connection */
      break;
    } else {
      LogError("GetTimerLog", SOCKET_READ_ERROR);
      close(sock);
      return SOCKET_READ_ERROR;
    }

    if (!loop) break;
    sleep(1); /* Wait before checking again */
  } while (loop);

  /* Close the socket */
  if (close(sock) == -1) {
    LogError("GetTimerLog", SOCKET_CLOSE_ERROR);
    return SOCKET_CLOSE_ERROR;
  }

  return NO_ERROR;
}

/**
 * Set a timer log by writing to the socket.
 * Sends timer data to the Unix socket server.
 * @param path File path for the Unix socket
 * @param log Formatted log string to write
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType SetTimerLog(const char* path, const char* log) {
  static char last_log[32] = "";

  /* Skip sending if the log is identical to the last one */
  if (strcmp(last_log, log) == 0) return NO_ERROR;

  int sock;
  struct sockaddr_un addr;
  int retries = 0;

  do {
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
      LogError("SetTimerLog", SOCKET_CREATION_ERROR);
      return SOCKET_CREATION_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) break;

    close(sock);
    sock = -1;
    retries++;
    if (retries < SOCKET_RETRY_COUNT) socketRetryDelay();
  } while (retries < SOCKET_RETRY_COUNT);

  if (sock == -1) {
    LogError("SetTimerLog", SOCKET_CONNECTION_ERROR);
    return SOCKET_CONNECTION_ERROR;
  }

  /* Truncate log to buffer size */
  const int max_length = 31;
  char message[max_length + 2];
  snprintf(message, sizeof(message), "%.*s\n", max_length, log);
  message[max_length + 1] = '\0'; /* Ensure null termination */

  if (send(sock, message, strlen(message), 0) == -1) {
    LogError("SetTimerLog", SOCKET_WRITE_ERROR);
    close(sock);
    return SOCKET_WRITE_ERROR;
  }
  close(sock);

  /* Update the last log */
  strncpy(last_log, log, sizeof(last_log) - 1);
  last_log[sizeof(last_log) - 1] = '\0';

  return NO_ERROR;
}

/**
 * Format pomodoro data into a timer log string.
 * Creates a human-readable string for timer status display.
 * @param data Pomodoro timer data to format
 * @param is_paused Whether the timer is currently paused
 * @return Newly allocated formatted string (caller must free), or NULL on failure
 */
char* FormatTimerLog(PomodoroData data, bool is_paused) {
  const char *step_icon = NULL, *pause_icon = NULL;
  int icon_type = GetConfigIconType();
  int duration = 0;

  if (is_paused) pause_icon = PLAY_ICONS[icon_type];

  switch (data.current_step) {
    case WORK_TIME:
      step_icon = WORK_ICONS[icon_type];
      duration = data.work_time;
      break;
    case SHORT_PAUSE:
      step_icon = SHORT_PAUSE_ICONS[icon_type];
      duration = data.short_pause_time;
      break;
    case LONG_PAUSE:
      step_icon = LONG_PAUSE_ICONS[icon_type];
      duration = data.long_pause_time;
      break;
    default:
      return NULL;
  }

  char* remaining_time = FormatRemainingTime(data.current_step_time, duration);
  if (!remaining_time) return NULL;

  size_t buffer_size = (pause_icon ? sizeof(pause_icon) : 0) +
                       sizeof(step_icon) + strlen(remaining_time) + 1;
  char* log_string = (char*)malloc(buffer_size);
  if (!log_string) {
    free(remaining_time);
    return NULL;
  }

  if (pause_icon)
    snprintf(log_string, buffer_size, "%s %s %s", pause_icon, step_icon,
             remaining_time);
  else
    snprintf(log_string, buffer_size, "%s %s", step_icon, remaining_time);

  free(remaining_time);
  return log_string;
}

/**
 * ---------------------------------------------------------------------------
 * Notes
 * ---------------------------------------------------------------------------
 */

/**
 * Save notes to a file for persistence.
 * Writes the current notes data structure to disk.
 * @param path File path for the notes log
 * @param notes Pointer to the notes data to save
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType SaveNotes(const char* path, const NotesData* notes) {
  if (!path || !notes) return NO_ERROR;

  FILE* file = FOpenNoFollow(path, "w");
  if (!file) return FILE_ERROR;

  for (int i = 0; i < notes->count; i++) {
    NoteItem* item = notes->items[i];
    char* text = GapBufferToString(item->text);
    if (!text) continue;

    char state_char;
    switch (item->state) {
      case NOTE_DONE:
        state_char = 'X';
        break;
      case NOTE_UNDONE:
        state_char = ' ';
        break;
      case NOTE_PLAIN:
      default:
        state_char = '-';
        break;
    }

    fprintf(file, "%d|%d|%d|%c|", item->id, item->depth, item->parent_id,
            state_char);

    for (size_t j = 0; j < strlen(text); j++) {
      if (text[j] == '|')
        fprintf(file, "\\|");
      else
        fputc(text[j], file);
    }
    fputc('\n', file);

    free(text);
  }

  fclose(file);
  return NO_ERROR;
}

/**
 * Load notes from a file for persistence.
 * Reads notes data from disk and populates the notes structure.
 * @param path File path for the notes log
 * @param notes Pointer to the notes data to populate
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType LoadNotes(const char* path, NotesData* notes) {
  if (!path || !notes) return NO_ERROR;

  FILE* file = fopen(path, "r");
  if (!file) return NO_ERROR;

  char line[1024];
  while (fgets(line, sizeof(line), file)) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

    char* p = line;
    char* end;
    long id = strtol(p, &end, 10);
    if (end == p || *end != '|') continue;
    p = end + 1;

    long depth = strtol(p, &end, 10);
    if (end == p || *end != '|') continue;
    p = end + 1;

    long parent_id = strtol(p, &end, 10);
    if (end == p || *end != '|') continue;
    p = end + 1;

    if (*p == '\0') continue;
    NoteState state = charToNoteState(*p);
    p++;
    if (*p != '|') continue;
    p++;

    char text[1024] = {0};
    int ti = 0;
    while (*p && ti < (int)sizeof(text) - 1) {
      if (*p == '\\' && *(p + 1) == '|') {
        text[ti++] = '|';
        p += 2;
      } else
        text[ti++] = *p++;
    }
    text[ti] = '\0';

    if (notes->count >= notes->capacity) {
      notes->capacity *= 2;
      NoteItem** new_items =
        (NoteItem**)realloc(notes->items, sizeof(NoteItem*) * notes->capacity);
      if (new_items) notes->items = new_items;
    }

    NoteItem* item = (NoteItem*)malloc(sizeof(NoteItem));
    if (!item) continue;

    item->id = (int)id;
    item->depth = (int)depth;
    item->parent_id = (int)parent_id;
    item->state = state;
    item->text = GapBufferCreate();
    if (!item->text) {
      free(item);
      continue;
    }
    GapBufferSetText(item->text, text);

    notes->items[notes->count++] = item;
  }

  fclose(file);
  return NO_ERROR;
}

/**
 * Convert a character to its corresponding NoteState.
 * @param c Character representing note state ('X', ' ', '-')
 * @return NoteState corresponding to the character
 */
static NoteState charToNoteState(char c) {
  switch (c) {
    case 'X':
      return NOTE_DONE;
    case ' ':
      return NOTE_UNDONE;
    case '-':
    default:
      return NOTE_PLAIN;
  }
}

/**
 * ---------------------------------------------------------------------------
 * Pomodoro
 * ---------------------------------------------------------------------------
 */

/**
 * Read the binary pomodoro log and return the next session index.
 * @param path File path for the pomodoro log
 * @return The next session index (last index + 1), or FILE_ERROR on failure
 */
static int GetLastLogIndex(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) return FILE_ERROR;

  pomodoroLogRecord record;
  int last_index = 0;
  while (fread(&record, sizeof(record), 1, file) == 1)
    if (record.session_index > last_index) last_index = record.session_index;
  fclose(file);
  return last_index + 1;
}

/**
 * Get the last used index from the pomodoro log.
 * @param path File path for the pomodoro log
 * @return Last used index (0 if file is empty), or FILE_ERROR on failure
 */
int GetLastLogIndexOnly(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) return FILE_ERROR;

  pomodoroLogRecord record;
  int last_index = 0;
  while (fread(&record, sizeof(record), 1, file) == 1)
    if (record.session_index > last_index) last_index = record.session_index;
  fclose(file);
  return last_index;
}

/**
 * Remove completed entries matching the given session index.
 * Writes all non-matching records to a temp file, then renames.
 * @param path  File path for the pomodoro log
 * @param index Session index whose completed entries to remove
 * @return ErrorType NO_ERROR on success, or TIMER_LOG_ERROR / FILE_ERROR on failure
 */
ErrorType RemoveUncompletedEntries(const char* path, int index) {
  FILE* read_file = fopen(path, "rb");
  if (!read_file) return TIMER_LOG_ERROR;

  char temp_path[256];
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
  FILE* write_file = FOpenNoFollow(temp_path, "wb");
  if (!write_file) return FILE_ERROR;

  pomodoroLogRecord record;
  while (fread(&record, sizeof(record), 1, read_file) == 1) {
    if (record.session_index == index && record.status == 0) continue;
    fwrite(&record, sizeof(record), 1, write_file);
  }

  fclose(read_file);
  fclose(write_file);
  if (rename(temp_path, path) != 0) {
    remove(temp_path);
    return TIMER_LOG_ERROR;
  }
  return NO_ERROR;
}

/**
 * Save current pomodoro state to a log file.
 * Format: timestamp|step|current/total|work|short|long|current_step_time|last_step|elapsed/total
 * @param path   File path for the pomodoro log
 * @param data   Pointer to the pomodoro data to save
 * @param append If true, append new line; if false, update last line (rewrite file)
 * @return ErrorType NO_ERROR on success, or TIMER_LOG_ERROR on failure
 */
ErrorType SavePomodoro(const char* path, const PomodoroData* data,
                       bool append) {
  if (!path || !data) return NO_ERROR;

  int index =
    data->session_index > 0 ? data->session_index : GetLastLogIndex(path);

  pomodoroLogRecord record = {
    .session_index = (uint16_t)index,
    .current_step = (uint8_t)data->current_step,
    .current_cycle = (uint8_t)(data->current_cycle + 1),
    .total_cycles = (uint8_t)data->total_cycles,
    .work_time = (uint8_t)data->work_time,
    .short_pause_time = (uint8_t)data->short_pause_time,
    .long_pause_time = (uint8_t)data->long_pause_time,
    .total_elapsed = (uint32_t)data->total_elapsed,
    .current_step_time = (uint32_t)data->current_step_time,
    .status = (uint8_t)data->status,
    .session_start_time = (uint32_t)data->session_start_time};

  FILE* file = FOpenNoFollow(path, append ? "ab" : "wb");
  if (!file) return TIMER_LOG_ERROR;

  if (fwrite(&record, sizeof(record), 1, file) != 1) {
    fclose(file);
    return TIMER_LOG_ERROR;
  }

  fclose(file);
  return NO_ERROR;
}

/**
 * Load pomodoro state from a log file.
 * Reads the last record and restores pomodoro data.
 * @param path File path for the pomodoro log
 * @param data Pointer to the pomodoro data to populate
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType LoadPomodoro(const char* path, PomodoroData* data) {
  if (!path || !data) return NO_ERROR;

  FILE* file = fopen(path, "rb");
  if (!file) return NO_ERROR;

  pomodoroLogRecord record;
  pomodoroLogRecord last_record = {0};

  while (fread(&record, sizeof(record), 1, file) == 1) last_record = record;
  fclose(file);

  if (last_record.session_index == 0) return NO_ERROR;

  data->session_index = last_record.session_index;
  data->session_start_time = last_record.session_start_time;
  data->step_start_time = time(NULL);
  data->current_step = last_record.current_step;
  data->current_cycle = last_record.current_cycle - 1;
  data->total_cycles = last_record.total_cycles;

  if (data->total_cycles < 1 || data->total_cycles > 8) return NO_ERROR;

  if (data->current_step != WORK_TIME && data->current_step != SHORT_PAUSE &&
      data->current_step != LONG_PAUSE) {
    return NO_ERROR;
  }

  data->work_time = last_record.work_time;
  if (data->work_time < 1 || data->work_time > 120) return NO_ERROR;

  data->short_pause_time = last_record.short_pause_time;
  data->long_pause_time = last_record.long_pause_time;
  data->total_elapsed = last_record.total_elapsed;
  data->current_step_time = last_record.current_step_time;
  data->status = last_record.status;

  data->delta_time_ms = GetCurrentTimeMS();

  return NO_ERROR;
}

/**
 * Print pomodoro history statistics from the log file.
 * Displays a contribution graph with monthly activity, totals, and streaks.
 * @param path File path for the pomodoro log
 */
void GetPomodoroHistory(const char* path) {
  pomodoroHistoryStats stats =
    createPomodoroHistoryStats(path, MAX_RECENT_SESSIONS);

  if (stats.totalSessions == 0) {
    printf("No pomodoro history found.\n");
    return;
  }

  /* Current date */
  time_t now = time(NULL);
  struct tm* lt = localtime(&now);
  int curYear = lt->tm_year + 1900;
  int curMonth = lt->tm_mon + 1;
  int curDay = lt->tm_mday;

  /* Compute 5-month window ending with current month */
  int firstYear = curYear, firstMonth = curMonth - 4;
  while (firstMonth < 1) { firstMonth += 12; firstYear--; }

  /* Graph layout arrays */
  int monthWeeks[5], monthStartWeek[5], monthDays[5], monthStartDow[5];
  int dailyCounts[5][31];
  int totalWeeks = 0;

  int y = firstYear, m = firstMonth;
  for (int i = 0; i < 5; i++) {
    monthDays[i] = HistDaysInMonth(y, m);
    monthStartDow[i] = HistDayOfWeek(y, m, 1);
    monthWeeks[i] = (monthStartDow[i] + monthDays[i] + 6) / 7;
    monthStartWeek[i] = totalWeeks;
    totalWeeks += monthWeeks[i];
    HistDailyCounts(path, y, m, dailyCounts[i]);
    if (++m > 12) { m = 1; y++; }
  }

  /* Streaks for today */
  int currentStreak = 0, longestStreak = 0;
  HistStreak(path, curYear, curMonth, curDay, &currentStreak, &longestStreak);

  /* Layout dimensions */
  int cellW = 3;
  int monthGap = 3;
  int gapBefore[5] = {0};
  for (int i = 1; i < 5; i++)
    gapBefore[i] = gapBefore[i-1] + monthGap;
  int gridW = totalWeeks * cellW + gapBefore[4];
  int innerW = 4 + gridW;
  int boxW = innerW + 2;

  static const char* dowNames[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  static const char* monNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  /* Top border */
  printf("┌");
  for (int i = 0; i < boxW - 2; i++) printf("─");
  printf("┐\n");

  /* Title */
  {
    int tl = (int)strlen("Pomodoro History");
    int pad = (innerW - tl) / 2;
    printf("│%*sPomodoro History%*s│\n", pad, "", innerW - tl - pad, "");
  }

  /* Blank separator */
  printf("│%*s│\n", innerW, "");

  /* Month headers row */
  {
    char row[4096];
    memset(row, ' ', innerW);
    int em = firstMonth;
    for (int i = 0; i < 5; i++) {
      if (monthWeeks[i] >= 2) {
        const char* name = monNames[em - 1];
        int nl = (int)strlen(name);
        int bw = monthWeeks[i] * cellW;
        int p = 4 + monthStartWeek[i] * cellW + gapBefore[i] + (bw - nl) / 2;
        if (p >= 0 && p + nl <= innerW)
          memcpy(row + p, name, nl);
      }
      if (++em > 12) em = 1;
    }
    row[innerW] = '\0';
    printf("│%s│\n", row);
  }

  /* Grid rows: 7 days of week */
  for (int dow = 0; dow < 7; dow++) {
    printf("│ %s ", dowNames[dow]);
    for (int wc = 0; wc < totalWeeks; wc++) {
      int found = 0;
      for (int mi = 0; mi < 5; mi++) {
        if (wc >= monthStartWeek[mi] && wc < monthStartWeek[mi] + monthWeeks[mi]) {
          int dn = (wc - monthStartWeek[mi]) * 7 + dow - monthStartDow[mi] + 1;
          if (dn >= 1 && dn <= monthDays[mi]) {
            int level = HistLevelForCount(dailyCounts[mi][dn - 1]);
            printf("%s ", HISTORY_ICONS[level]);
          } else {
            printf("   ");
          }
          found = 1;
          break;
        }
      }
      if (!found) printf("   ");
      for (int mi = 0; mi < 4; mi++) {
        if (wc + 1 == monthStartWeek[mi] + monthWeeks[mi]) {
          for (int g = 0; g < monthGap; g++) putchar(' ');
          break;
        }
      }
    }
    printf("│\n");
  }

  /* Blank separator */
  printf("│%*s│\n", innerW, "");

  /* Summary and streak lines with column-aligned layout */
  {
    char c1s[64], c2s[64], c3s[64];
    char c1c[64], c2c[64];

    snprintf(c1s, sizeof(c1s), "Total sessions: %d", stats.totalSessions);
    snprintf(c2s, sizeof(c2s), "Completed: %d", stats.completedSessions);
    if (stats.workHours > 0)
      snprintf(c3s, sizeof(c3s), "Work: %dh %dm", stats.workHours, stats.workMins);
    else
      snprintf(c3s, sizeof(c3s), "Work: %d min", stats.totalWorkMinutes);

    snprintf(c1c, sizeof(c1c), "Current streak: %dd", currentStreak);
    snprintf(c2c, sizeof(c2c), "Longest streak: %dd", longestStreak);

    int l1a = (int)strlen(c1s), l1b = (int)strlen(c1c);
    int l2a = (int)strlen(c2s), l2b = (int)strlen(c2c);
    int cw1 = (l1a > l1b ? l1a : l1b) + 3;
    int cw2 = (l2a > l2b ? l2a : l2b) + 3;

    char line[512];
    snprintf(line, sizeof(line), "%-*s%-*s%s", cw1, c1s, cw2, c2s, c3s);
    printf("│ %-*s│\n", innerW - 1, line);

    snprintf(line, sizeof(line), "%-*s%-*s", cw1, c1c, cw2, c2c);
    printf("│ %-*s│\n", innerW - 1, line);
  }

  /* Bottom border */
  printf("└");
  for (int i = 0; i < boxW - 2; i++) printf("─");
  printf("┘\n");
}

/**
 * Read the binary log and compute aggregated history statistics.
 * @param path     Binary log path (POMODORO_LOG)
 * @param maxRecent Maximum number of recent sessions to include
 * @return Populated pomodoroHistoryStats struct
 */
static pomodoroHistoryStats createPomodoroHistoryStats(const char* path,
                                                       int maxRecent) {
  pomodoroHistoryStats stats = {0};

  FILE* file = fopen(path, "rb");
  if (!file) return stats;

  pomodoroLogRecord records[1000];
  int count = 0;

  while (fread(&records[count], sizeof(pomodoroLogRecord), 1, file) == 1) {
    count++;
    if (count >= 1000) break;
  }
  fclose(file);

  if (count == 0) return stats;

  int totalWorkSeconds = 0;
  int totalSessionSeconds = 0;
  int lastSession = -1;

  for (int i = 0; i < count; i++) {
    if (records[i].session_index > 0 &&
        records[i].session_index != (unsigned int)lastSession) {
      stats.totalSessions++;
      lastSession = records[i].session_index;
    }
    if (records[i].status == 0) {
      stats.completedSessions++;
      int cycles = records[i].total_cycles;
      int work = records[i].work_time * 60;
      int shortP = records[i].short_pause_time * 60;
      int longP = records[i].long_pause_time * 60;
      totalSessionSeconds += (cycles * work) + ((cycles - 1) * shortP) + longP;
    }
    if (records[i].current_step == WORK_TIME && records[i].current_cycle > 0)
      totalWorkSeconds += records[i].current_step_time;
  }

  int uniqueSessions = 0;
  int sessionIndices[1000];
  for (int i = 0; i < count; i++) {
    if (records[i].session_index > 0) {
      int alreadyCounted = 0;
      for (int j = 0; j < uniqueSessions; j++) {
        if (sessionIndices[j] == records[i].session_index) {
          alreadyCounted = 1;
          break;
        }
      }
      if (!alreadyCounted)
        sessionIndices[uniqueSessions++] = records[i].session_index;
    }
  }

  stats.numRecent = uniqueSessions > maxRecent ? maxRecent : uniqueSessions;
  int startIdx = uniqueSessions > maxRecent ? uniqueSessions - maxRecent : 0;

  stats.totalWorkMinutes = totalWorkSeconds / 60;
  stats.workHours = stats.totalWorkMinutes / 60;
  stats.workMins = stats.totalWorkMinutes % 60;

  totalSessionSeconds /= 60;
  stats.totalSessionMinutes = totalSessionSeconds;
  stats.sessionHours = totalSessionSeconds / 60;
  stats.sessionMins = totalSessionSeconds % 60;

  for (int i = 0; i < stats.numRecent; i++) {
    int idx = startIdx + i;
    int sess = sessionIndices[idx];
    recentSessionInfo* rs = &stats.recentSessions[i];
    rs->sessionIndex = sess;

    for (int j = 0; j < count; j++) {
      if (records[j].session_index == sess) {
        rs->totalElapsed += records[j].current_step_time;
        rs->step = records[j].current_step;
        rs->cycle = records[j].current_cycle;
        rs->totalCycles = records[j].total_cycles;
        rs->workTime = records[j].work_time;
        rs->status = records[j].status;
        rs->shortPause = records[j].short_pause_time;
        rs->longPause = records[j].long_pause_time;
      }
    }

    if (rs->status == 0) {
      rs->sessionMins =
        (rs->cycle * rs->workTime * 60 + (rs->cycle - 1) * rs->shortPause * 60 +
         rs->longPause * 60) /
        60;
    }
  }

  return stats;
}

/**
 * ---------------------------------------------------------------------------
 * History
 * ---------------------------------------------------------------------------
 */

/**
 * Returns the number of days in a given month.
 * @param year  Gregorian year (e.g. 2026)
 * @param month Month (1-12)
 * @return Days in month (28-31)
 */
int HistDaysInMonth(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  int d = days[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
    d++;
  return d;
}

/**
 * Returns the day-of-week for a date.
 * @param year  Gregorian year
 * @param month Month (1-12)
 * @param day   Day (1-31)
 * @return 0=Sunday .. 6=Saturday
 */
int HistDayOfWeek(int year, int month, int day) {
  /* Tomohiko Sakamoto's algorithm */
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) year--;
  return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

/**
 * Check if a unix timestamp falls on a given calendar date.
 * @param ts    Unix timestamp
 * @param year  Gregorian year
 * @param month Month (1-12)
 * @param day   Day (1-31)
 * @return true if ts is on (year, month, day)
 */
static bool histIsSameDay(time_t ts, int year, int month, int day) {
  struct tm* tm = localtime(&ts);
  if (!tm) return false;
  return (tm->tm_year + 1900 == year && tm->tm_mon + 1 == month &&
          tm->tm_mday == day);
}

/**
 * Check if a unix timestamp falls within a given year/month.
 * @param ts    Unix timestamp
 * @param year  Gregorian year
 * @param month Month (1-12)
 * @return true if ts is in (year, month)
 */
static bool histIsSameMonth(time_t ts, int year, int month) {
  struct tm* tm = localtime(&ts);
  if (!tm) return false;
  return (tm->tm_year + 1900 == year && tm->tm_mon + 1 == month);
}

/**
 * Fills daily session-count array for a given month from the binary log.
 * @param path   Binary log path (POMODORO_LOG)
 * @param year   Year
 * @param month  Month (1-12)
 * @param counts Output array[31] — count per day (0 for days outside month)
 * @return Days in month (same as length of valid entries in counts)
 */
int HistDailyCounts(const char* path, int year, int month, int* counts) {
  int dim = HistDaysInMonth(year, month);
  for (int i = 0; i < 31 && i < dim; i++) counts[i] = 0;

  FILE* file = fopen(path, "rb");
  if (!file) return dim;

  pomodoroLogRecord record;
  while (fread(&record, sizeof(record), 1, file) == 1) {
    if (record.session_index == 0) continue;
    time_t ts = (time_t)record.session_start_time;
    if (!histIsSameMonth(ts, year, month)) continue;
    struct tm* tm = localtime(&ts);
    int d = tm->tm_mday - 1;
    if (d >= 0 && d < dim) counts[d]++;
  }
  fclose(file);
  return dim;
}

/**
 * Returns session records for a specific day from the binary log.
 * @param path       Binary log path (POMODORO_LOG)
 * @param year       Year
 * @param month      Month (1-12)
 * @param day        Day (1-31)
 * @param indices    Output array of session indices
 * @param startTimes Output array of unix timestamps
 * @param durations  Output array of durations in seconds
 * @param statuses   Output array (0=completed, 1=uncompleted)
 * @param maxCount   Capacity of output arrays
 * @return Number of sessions found (capped at maxCount)
 */
int HistSessionsForDay(const char* path, int year, int month, int day,
                       int* indices, time_t* startTimes, int* durations,
                       int* statuses, int maxCount) {
  int count = 0;

  FILE* file = fopen(path, "rb");
  if (!file) return 0;

  pomodoroLogRecord record;
  while (fread(&record, sizeof(record), 1, file) == 1) {
    if (record.session_index == 0) continue;
    time_t ts = (time_t)record.session_start_time;
    if (!histIsSameDay(ts, year, month, day)) continue;
    if (count >= maxCount) break;

    /* Each unique session_index is one session */
    /* Check if we already have this session index */
    int dup = 0;
    for (int i = 0; i < count; i++) {
      if (indices[i] == record.session_index) {
        dup = 1;
        break;
      }
    }
    if (dup) continue;

    indices[count] = record.session_index;
    startTimes[count] = ts;
    durations[count] = (int)record.current_step_time;
    statuses[count] = record.status;
    count++;
  }
  fclose(file);
  return count;
}

/**
 * Computes current and longest streak ending at the given date.
 * A streak is consecutive calendar days (past to present) with at
 * least one completed session.
 * @param path    Binary log path (POMODORO_LOG)
 * @param year    Year of streak endpoint
 * @param month   Month of streak endpoint
 * @param day     Day of streak endpoint
 * @param current Output — current streak length
 * @param longest Output — longest streak ever
 */
void HistStreak(const char* path, int year, int month, int day, int* current,
                int* longest) {
  *current = 0;
  *longest = 0;

  /* Read all unique session dates */
  time_t sessionDates[2000];
  int nDates = 0;

  FILE* file = fopen(path, "rb");
  if (file) {
    pomodoroLogRecord record;
    while (fread(&record, sizeof(record), 1, file) == 1) {
      if (record.session_index == 0) continue;
      time_t ts = (time_t)record.session_start_time;
      struct tm* tm = localtime(&ts);
      if (!tm) continue;
      int y = tm->tm_year + 1900;
      int m = tm->tm_mon + 1;
      int d = tm->tm_mday;

      /* Check for duplicates */
      int dup = 0;
      for (int i = 0; i < nDates; i++) {
        struct tm* dtm = localtime(&sessionDates[i]);
        if (dtm && dtm->tm_year + 1900 == y && dtm->tm_mon + 1 == m &&
            dtm->tm_mday == d) {
          dup = 1;
          break;
        }
      }
      if (!dup) {
        /* Store normalized timestamp (midnight) */
        struct tm nd = {0};
        nd.tm_year = y - 1900;
        nd.tm_mon = m - 1;
        nd.tm_mday = d;
        time_t nt = mktime(&nd);
        if (nt != (time_t)-1) sessionDates[nDates++] = nt;
        if (nDates >= 2000) break;
      }
    }
    fclose(file);
  }

  if (nDates == 0) return;

  /* Sort dates */
  for (int i = 0; i < nDates - 1; i++) {
    for (int j = i + 1; j < nDates; j++) {
      if (sessionDates[j] < sessionDates[i]) {
        time_t tmp = sessionDates[i];
        sessionDates[i] = sessionDates[j];
        sessionDates[j] = tmp;
      }
    }
  }

  /* Compute end-of-day timestamp for the given date */
  struct tm endTm = {0};
  endTm.tm_year = year - 1900;
  endTm.tm_mon = month - 1;
  endTm.tm_mday = day;
  endTm.tm_hour = 23;
  endTm.tm_min = 59;
  endTm.tm_sec = 59;
  time_t endTs = mktime(&endTm);
  if (endTs == (time_t)-1) return;

  /* Current streak: count backwards from end date */
  *current = 0;
  time_t cursor = endTs;
  int daySecs = 86400;
  for (int i = nDates - 1; i >= 0; i--) {
    if (sessionDates[i] > endTs) continue;
    /* Check if this date is within range of our streak */
    if (sessionDates[i] <= cursor && sessionDates[i] > cursor - daySecs) {
      (*current)++;
      cursor = sessionDates[i] - daySecs; /* move to previous day */
    } else if (sessionDates[i] < cursor - daySecs) {
      break; /* gap found */
    }
  }

  /* Longest streak: scan all dates */
  *longest = 0;
  if (nDates > 0) {
    int run = 1;
    for (int i = 1; i < nDates; i++) {
      if (sessionDates[i] - sessionDates[i - 1] <= daySecs + 3600) {
        run++;
      } else {
        if (run > *longest) *longest = run;
        run = 1;
      }
    }
    if (run > *longest) *longest = run;
  }
}

/**
 * Maps session count to contribution-icon index.
 *
 * Boundaries:
 *   0   → 0 ("░░")
 *   1-2 → 1 ("▒▒")
 *   3-5 → 2 ("▓▓")
 *   6+  → 3 ("██")
 *
 * @param count Number of sessions
 * @return Icon index 0-3
 */
int HistLevelForCount(int count) {
  if (count == 0) return 0;
  if (count <= 2) return 1;
  if (count <= 5) return 2;
  return 3;
}
