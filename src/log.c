#include "log.h"

#include <errno.h>
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
#include "tomato.h"

/* Function to create the timer log server */
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
          if (client_sock > max_sd) {
            max_sd = client_sock;
          }
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

/* Function to get the last timer log from the socket */
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

/* Function to set a timer log by writing to the socket */
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

/* Format pomdoro data to log in timer log */
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
