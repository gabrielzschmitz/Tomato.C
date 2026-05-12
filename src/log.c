#define _XOPEN_SOURCE 700

#include "log.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
typedef struct __attribute__((packed)) {
  uint16_t session_index;
  uint8_t current_step;
  uint8_t current_cycle;
  uint8_t total_cycles;
  uint8_t work_time;
  uint8_t short_pause_time;
  uint8_t long_pause_time;
  uint32_t total_elapsed;
  uint32_t current_step_time;
  uint8_t status;
  uint8_t padding[3];
} PomodoroLogRecord;

/* Notes */
static NoteState charToNoteState(char c);
/* Pomodoro */
static void printRow(int w1, int w2, const char* label, const char* value);
static int strwidth(const char* s);
static void printLine(int w1, int w2, int is_top);
static void printLineBottom(int w1, int w2);
static void formatTime(char* buf, int size, int hours, int minutes);

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
      LogError("CreateTimerLog", SOCKET_READ_ERROR);
      break;
    }

    for (int i = 0; i <= max_sd; i++) {
      if (FD_ISSET(i, &working_set)) {
        if (i == server_sock) {
          /* Accept new connection */
          client_sock = accept(server_sock, NULL, NULL);
          if (client_sock == -1) {
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

            /* Broadcast last message */
            for (int j = 0; j <= max_sd; j++) {
              if (FD_ISSET(j, &master_set) && j != server_sock && j != i) {
                if (send(j, last_message, strlen(last_message), 0) == -1) {
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
  int sock;
  struct sockaddr_un addr;
  const int buffer_size = 32;
  char buffer[buffer_size];
  ssize_t n;

  /* Create socket */
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    LogError("GetTimerLog", SOCKET_CREATION_ERROR);
    return SOCKET_CREATION_ERROR;
  }

  /* Set up the address structure */
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  /* Connect to the server */
  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    LogError("GetTimerLog", SOCKET_CONNECTION_ERROR);
    close(sock);
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

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    LogError("SetTimerLog", SOCKET_CREATION_ERROR);
    return SOCKET_CREATION_ERROR;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    LogError("SetTimerLog", SOCKET_CONNECTION_ERROR);
    close(sock);
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

  if (is_paused) pause_icon = PAUSE_ICONS[icon_type];

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

  FILE* file = fopen(path, "w");
  if (!file) return NO_ERROR;

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
 * Save current pomodoro state to a log file.
 * Format: timestamp|step|current/total|work|short|long|current_step_time|last_step|elapsed/total
 * @param path File path for the pomodoro log
 * @param data Pointer to the pomodoro data to save
 * @param append If true, append new line; if false, update last line (rewrite file)
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static int GetLastLogIndex(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) return 1;

  PomodoroLogRecord record;
  int last_index = 0;
  while (fread(&record, sizeof(record), 1, file) == 1) {
    if (record.session_index > last_index) last_index = record.session_index;
  }
  fclose(file);
  return last_index + 1;
}

int GetLastLogIndexOnly(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) return 0;

  PomodoroLogRecord record;
  int last_index = 0;
  while (fread(&record, sizeof(record), 1, file) == 1) {
    if (record.session_index > last_index) last_index = record.session_index;
  }
  fclose(file);
  return last_index;
}

ErrorType RemoveUncompletedEntries(const char* path, int index) {
  FILE* read_file = fopen(path, "rb");
  if (!read_file) return NO_ERROR;

  char temp_path[256];
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
  FILE* write_file = fopen(temp_path, "wb");
  if (!write_file) {
    fclose(read_file);
    return NO_ERROR;
  }

  PomodoroLogRecord record;
  while (fread(&record, sizeof(record), 1, read_file) == 1) {
    if (record.session_index == index && record.status == 0) {
      continue;
    }
    fwrite(&record, sizeof(record), 1, write_file);
  }

  fclose(read_file);
  fclose(write_file);
  rename(temp_path, path);
  return NO_ERROR;
}

ErrorType SavePomodoro(const char* path, const PomodoroData* data,
                       bool append) {
  if (!path || !data) return NO_ERROR;

  int index =
    data->session_index > 0 ? data->session_index : GetLastLogIndex(path);

  PomodoroLogRecord record = {
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
    .padding = {0, 0, 0}};

  FILE* file = fopen(path, append ? "ab" : "wb");
  if (!file) return NO_ERROR;

  fwrite(&record, sizeof(record), 1, file);

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

  PomodoroLogRecord record;
  PomodoroLogRecord last_record = {0};

  while (fread(&record, sizeof(record), 1, file) == 1) {
    last_record = record;
  }
  fclose(file);

  if (last_record.session_index == 0) return NO_ERROR;

  data->session_index = last_record.session_index;
  data->step_start_time = time(NULL);
  data->current_step = last_record.current_step;
  data->current_cycle = last_record.current_cycle - 1;
  data->total_cycles = last_record.total_cycles;

  if (data->total_cycles < 1 || data->total_cycles > 8) {
    return NO_ERROR;
  }

  if (data->current_step != WORK_TIME && data->current_step != SHORT_PAUSE &&
      data->current_step != LONG_PAUSE) {
    return NO_ERROR;
  }

  data->work_time = last_record.work_time;
  if (data->work_time < 1 || data->work_time > 120) {
    return NO_ERROR;
  }

  data->short_pause_time = last_record.short_pause_time;
  data->long_pause_time = last_record.long_pause_time;
  data->total_elapsed = last_record.total_elapsed;
  data->current_step_time = last_record.current_step_time;
  data->status = last_record.status;

  data->delta_time_ms = GetCurrentTimeMS();

  return NO_ERROR;
}

void GetPomodoroHistory(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) {
    printf("No pomodoro history found.\n");
    return;
  }

  PomodoroLogRecord records[1000];
  int count = 0;

  while (fread(&records[count], sizeof(PomodoroLogRecord), 1, file) == 1) {
    count++;
    if (count >= 1000) break;
  }
  fclose(file);

  if (count == 0) {
    printf("No pomodoro history found.\n");
    return;
  }

  int total_sessions = 0;
  int completed_sessions = 0;
  int total_work_seconds = 0;
  int total_session_seconds = 0;
  int last_session = -1;

  for (int i = 0; i < count; i++) {
    if (records[i].session_index > 0 &&
        records[i].session_index != (unsigned int)last_session) {
      total_sessions++;
      last_session = records[i].session_index;
    }
    if (records[i].status == 0) {
      completed_sessions++;
      int cycles = records[i].total_cycles;
      int work = records[i].work_time * 60;
      int short_p = records[i].short_pause_time * 60;
      int long_p = records[i].long_pause_time * 60;
      total_session_seconds +=
        (cycles * work) + ((cycles - 1) * short_p) + long_p;
    }
    if (records[i].current_step == WORK_TIME && records[i].current_cycle > 0)
      total_work_seconds += records[i].current_step_time;
  }

  int unique_sessions = 0;
  int session_indices[1000];
  for (int i = 0; i < count; i++) {
    if (records[i].session_index > 0) {
      int already_counted = 0;
      for (int j = 0; j < unique_sessions; j++) {
        if (session_indices[j] == records[i].session_index) {
          already_counted = 1;
          break;
        }
      }
      if (!already_counted)
        session_indices[unique_sessions++] = records[i].session_index;
    }
  }

  int num_recent = unique_sessions > 3 ? 3 : unique_sessions;
  int start_idx = unique_sessions > 3 ? unique_sessions - 3 : 0;

  int total_work_minutes = total_work_seconds / 60;
  int work_hours = total_work_minutes / 60;
  int work_mins = total_work_minutes % 60;

  int total_session_minutes = total_session_seconds / 60;
  int session_hours = total_session_minutes / 60;
  int session_mins = total_session_minutes % 60;

  int max_label_w = 16;
  int max_value_w = 7;

  if (total_sessions > 0 && max_value_w < 1) max_value_w = 1;
  if (completed_sessions > 0 && max_value_w < 1) max_value_w = 1;
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%d min", total_work_minutes);
  int len = strwidth(tmp);
  if (len > max_value_w) max_value_w = len;
  formatTime(tmp, sizeof(tmp), work_hours, work_mins);
  len = strwidth(tmp);
  if (len > max_value_w) max_value_w = len;
  snprintf(tmp, sizeof(tmp), "%d min", total_session_minutes);
  len = strwidth(tmp);
  if (len > max_value_w) max_value_w = len;
  formatTime(tmp, sizeof(tmp), session_hours, session_mins);
  len = strwidth(tmp);
  if (len > max_value_w) max_value_w = len;

  if (num_recent > 0) {
    char tmpbuf[64];
    for (int idx = start_idx; idx < unique_sessions; idx++) {
      int sess = session_indices[idx];
      int len = snprintf(tmpbuf, sizeof(tmpbuf), "#%d", sess);
      uint8_t cycle = 0, total_cycles = 0, status = 1;
      for (int i = 0; i < count; i++) {
        if (records[i].session_index == sess) {
          cycle = records[i].current_cycle;
          total_cycles = records[i].total_cycles;
          status = records[i].status;
        }
      }
      if (status == 0)
        len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, " %d/%d done 99m",
                        cycle, total_cycles);
      else
        len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len,
                        " %d/%d cycle 99m left", cycle, total_cycles);
      if (len > max_label_w) max_label_w = len;
    }
  }

  int W1 = max_label_w;
  int W2 = max_value_w;
  char buf[64];

  printLine(W1, W2, 1);
  printRow(W1, W2, "POMODORO HISTORY", "");
  printLine(W1, W2, 0);

  snprintf(buf, sizeof(buf), "%d", total_sessions);
  printRow(W1, W2, "Total sessions", buf);

  snprintf(buf, sizeof(buf), "%d", completed_sessions);
  printRow(W1, W2, "Completed sessions", buf);

  snprintf(buf, sizeof(buf), "%d min", total_work_minutes);
  printRow(W1, W2, "Total work time", buf);

  formatTime(buf, sizeof(buf), work_hours, work_mins);
  printRow(W1, W2, "", buf);

  snprintf(buf, sizeof(buf), "%d min", total_session_minutes);
  printRow(W1, W2, "Total session time", buf);

  formatTime(buf, sizeof(buf), session_hours, session_mins);
  printRow(W1, W2, "", buf);

  if (num_recent > 0) {
    printLine(W1, W2, 0);
    snprintf(buf, sizeof(buf), "Last %d session%s", num_recent,
             num_recent > 1 ? "s" : "");
    printRow(W1, W2, buf, "");
    for (int idx = start_idx; idx < unique_sessions; idx++) {
      int sess = session_indices[idx];
      uint32_t total_elapsed = 0;
      uint8_t step = 0, cycle = 0, total_cycles = 0, work_time = 0, status = 1;
      uint8_t short_pause = 0, long_pause = 0;
      for (int i = 0; i < count; i++) {
        if (records[i].session_index == sess) {
          total_elapsed += records[i].current_step_time;
          step = records[i].current_step;
          cycle = records[i].current_cycle;
          total_cycles = records[i].total_cycles;
          work_time = records[i].work_time;
          status = records[i].status;
          short_pause = records[i].short_pause_time;
          long_pause = records[i].long_pause_time;
        }
      }
      int mins = total_elapsed / 60;
      if (status == 0) {
        int work = work_time * 60;
        int session_mins =
          (cycle * work + (cycle - 1) * short_pause * 60 + long_pause * 60) /
          60;
        snprintf(buf, sizeof(buf), "#%d %d/%d done %dm", sess, cycle,
                 total_cycles, session_mins);
      } else if (step == WORK_TIME) {
        int remaining = (work_time * 60) - (total_elapsed % (work_time * 60));
        if (remaining > 0) {
          int rem_mins = remaining / 60;
          snprintf(buf, sizeof(buf), "#%d %d/%d cycle %dm left", sess, cycle,
                   total_cycles, rem_mins);
        } else
          snprintf(buf, sizeof(buf), "#%d %d/%d cycle %dm", sess, cycle,
                   total_cycles, mins);
      } else
        snprintf(buf, sizeof(buf), "#%d %d/%d cycle %dm", sess, cycle,
                 total_cycles, mins);
      printRow(W1, W2, buf, "");
    }
  }

  printLineBottom(W1, W2);
}

static int strwidth(const char* s) {
  int w = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (c < 128 || c >= 192) w++;
    s++;
  }
  return w;
}

static void printLine(int w1, int w2, int is_top) {
  const char* tl = is_top ? "┌" : "├";
  const char* tr = is_top ? "┐" : "┤";
  const char* ml = is_top ? "┬" : "┼";
  const char* mr = "┤";
  const char* hz = "─";

  printf("%s", tl);
  for (int i = 0; i < w1 + 2; i++) printf("%s", hz);
  printf("%s", ml);
  for (int i = 0; i < w2 + 2; i++) printf("%s", hz);
  printf("%s\n", tr);
}

static void printLineBottom(int w1, int w2) {
  printf("└");
  for (int i = 0; i < w1 + 2; i++) printf("─");
  printf("┴");
  for (int i = 0; i < w2 + 2; i++) printf("─");
  printf("┘\n");
}

static void printRow(int w1, int w2, const char* label, const char* value) {
  int label_w = strwidth(label);
  int value_w = strwidth(value);
  printf("│ %s%*s │ %s%*s │\n", label, w1 - label_w, "", value, w2 - value_w,
         "");
}

static void formatTime(char* buf, int size, int hours, int minutes) {
  if (hours > 0)
    snprintf(buf, size, "%dh %2dm", hours, minutes);
  else
    snprintf(buf, size, "%d min", minutes);
}
