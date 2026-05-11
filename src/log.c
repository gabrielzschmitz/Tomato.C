#include "log.h"

#include <errno.h>
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

/* PRIVATE ANIM FUNCTIONS */
/* Notes */
static NoteState charToNoteState(char c);

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
