/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// notify.c
*/
#include "tomato.h"
#include "anim.h"
#include "draw.h"
#include "input.h"
#include "notify.h"
#include "update.h"
#include "util.h"
#include "config.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>

/* Send a notification with sound */
void notify(const char * message){
    /* Work notification */
    if(strcmp(message, "worktime") == 0){
    #ifdef __APPLE__
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0)
                system("osascript -e \'display notification \"華 Work!\" with title \"You need to focus\"\'");
            else if(strcmp(ICONS, "iconson") == 0)
                system("osascript -e \'display notification \"👷 Work!\" with title \"You need to focus\"\'");
            else
                system("osascript -e \'display notification \"Work!\" with title \"You need to focus\"\'");
        }
    #else
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \'華 Work!\' \'You need to focus\'");
            else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \'👷 Work!\' \'You need to focus\'");
            else
                system("notify-send -t 5000 -c Tomato.C \'Work! You need to focus\'");
        }
    #endif
        if(SOUND == 1 && WSL == 0)
            system("mpv --no-vid --no-input-terminal --volume=50 /usr/local/share/tomato/sounds/dfltnotify.mp3 --really-quiet &");
    }
    /* Short Pause notification */
    else if(strcmp(message, "shortpause") == 0){
    #ifdef __APPLE__
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0)
                system("osascript -e \'display notification \" Pause Break\" with title \"You have some time to chill\"\'");
            else if(strcmp(ICONS, "iconson") == 0)
                system("osascript -e \'display notification \"☕ Pause Break\" with title \"You have some time to chill\"\'");
            else
                system("osascript -e \'display notification \"Pause Break\" with title \"You have some time to chill\"\'");
        }
    #else
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \' Pause Break\' \'You have some time to chill\'");
            else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \'☕ Pause Break\' \'You have some time to chill\'");
            else
                system("notify-send -t 5000 -c Tomato.C \'Pause Break. You have some time to chill\'");
        }
    #endif
        if(SOUND == 1 && WSL == 0)
            system("mpv --no-vid --no-input-terminal --volume=50 /usr/local/share/tomato/sounds/pausenotify.mp3 --really-quiet &");
    }
    /* Long Pause notification */
    else if(strcmp(message, "longpause") == 0){
    #ifdef __APPLE__
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0)
                system("osascript -e \'display notification \" Pause Break\" with title \"You have some time to chill\"\'");
            else if(strcmp(ICONS, "iconson") == 0)
                system("osascript -e \'display notification \"🌴 Pause Break\" with title \"You have some time to chill\"\'");
            else
                system("osascript -e \'display notification \"Long Pause Break\" with title \"You have some time to chill\"\'");
        }
    #else
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \' Pause Break\' \'You have some time to chill\'");
            else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \'🌴 Pause Break\' \'You have some time to chill\'");
            else
                system("notify-send -t 5000 -c Tomato.C \'Long Pause Break. You have some time to chill\'");
        }
    #endif
        if(SOUND == 1 && WSL == 0)
            system("mpv --no-vid --no-input-terminal --volume=50 /usr/local/share/tomato/sounds/pausenotify.mp3 --really-quiet &");
    }
    /* End of cycle notification */
    else{
    #ifdef __APPLE__
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0)
                system("osascript -e \'display notification \" End of Pomodoro Cycle\" with title \"Feel free to start another!\"\'");
            else if(strcmp(ICONS, "iconson") == 0)
                system("osascript -e \'display notification \"🍅 End of Pomodoro Cycle\" with title \"Feel free to start another!\"\'");
            else
                system("osascript -e \'display notification \"End of a Pomodoro Cycle\" with title \"Feel free to start another!\"\'");
        }
    #else
        if(NOTIFY == 1){
            if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \' End of Pomodoro Cycle\' \'Feel free to start another!\'");
            else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                system("notify-send -t 5000 -c Tomato.C \'🍅 End of Pomodoro Cycle\' \'Feel free to start another!\'");
            else
                system("notify-send -t 5000 -c Tomato.C \'End of Pomodoro Cycle. Feel free to start another!\'");
        }
    #endif
        if(SOUND == 1 && WSL == 0)
            system("mpv --no-vid --no-input-terminal --volume=50 /usr/local/share/tomato/sounds/endnotify.mp3 --really-quiet &");
    }
}
