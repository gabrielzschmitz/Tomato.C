#ifndef LOG_H_
#define LOG_H_

#include <ncurses.h>
#include "error.h"
#include "tomato.h"

/* Function to create the timer log server */
ErrorType CreateTimerLog(const char* path);

/* Function to get the timer log from the socket */
ErrorType GetTimerLog(const char* path, bool loop);

/* Function to set a timer log by writing to the socket */
ErrorType SetTimerLog(const char* path, const char* log);

/* Format pomdoro data to log in timer log */
char* FormatTimerLog(PomodoroData data, bool is_paused);

#endif /* LOG_H_ */
