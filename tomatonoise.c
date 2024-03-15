#include <mpv/client.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tomato.h"

void mpvCheckError(int status) {
  if (status < 0) {
    printf("mpv API error: %s\n", mpv_error_string(status));
    exit(1);
  }
}

int play(char *file, char *volume, char *title) {
  mpv_handle *mpvCtx = mpv_create();
  if (!mpvCtx) {
    printf("failed creating context\n");
    return 1;
  }

  /* Set loop */
  char loop[4] = "inf";
  char quit[2] = "1";
  mpvCheckError(mpv_set_option(mpvCtx, "loop", MPV_FORMAT_FLAG, &loop));
  mpvCheckError(
    mpv_set_option(mpvCtx, "save-position-on-quit", MPV_FORMAT_FLAG, &quit));
  mpvCheckError(mpv_set_property_string(mpvCtx, "title", title));

  char *tmppath = NULL;
  tmppath = malloc(strlen("/tmp/") + strlen(title) + 1);
  if (strcmp(title, "tomato noise rain") == 0)
    strcpy(tmppath, "/tmp/tomato_rain_state");
  else if (strcmp(title, "tomato noise fire") == 0)
    strcpy(tmppath, "/tmp/tomato_fire_state");
  else if (strcmp(title, "tomato noise wind") == 0)
    strcpy(tmppath, "/tmp/tomato_wind_state");
  else if (strcmp(title, "tomato noise thunder") == 0)
    strcpy(tmppath, "/tmp/tomato_thunder_state");
  else
    strcpy(tmppath, "/tmp/tomato_noise_state");

  char line[16];
  int save;

  FILE *tmpfile;
  tmpfile = fopen(tmppath, "w");
  fprintf(tmpfile, "%s ON", volume);
  fclose(tmpfile);

  // Done setting up options.
  mpvCheckError(mpv_initialize(mpvCtx));

  // Play this file.
  const char *load[] = {"loadfile", file, NULL};
  mpvCheckError(mpv_command(mpvCtx, load));
  const char *volumecmd[] = {"set", "volume", volume, NULL};
  mpvCheckError(mpv_command(mpvCtx, volumecmd));

  int toggle = 1;
  // Let it play, and wait until the user quits.
  while (toggle == 1) {
    mpv_event *mpvEvent = mpv_wait_event(mpvCtx, 0);
    tmpfile = fopen(tmppath, "r");
    if (tmpfile) {
      fgets(line, sizeof line, tmpfile);
      if (strstr(line, "100"))
        save = 100;
      else if (strstr(line, "90"))
        save = 90;
      else if (strstr(line, "80"))
        save = 80;
      else if (strstr(line, "70"))
        save = 70;
      else if (strstr(line, "60"))
        save = 60;
      else if (strstr(line, "50"))
        save = 50;
      else if (strstr(line, "40"))
        save = 40;
      else if (strstr(line, "30"))
        save = 30;
      else if (strstr(line, "20"))
        save = 20;
      else if (strstr(line, "10"))
        save = 10;
      else
        save = 0;
      if (strstr(line, "ON")) toggle = 1;
      if (strstr(line, "OFF")) toggle = 0;
      fclose(tmpfile);
      sprintf(volume, "%d", save);
      const char *volumecmd[] = {"set", "volume", volume, NULL};
      mpvCheckError(mpv_command(mpvCtx, volumecmd));
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 0) return 1;

  play(argv[0], argv[1], argv[2]);

  return 0;
}
