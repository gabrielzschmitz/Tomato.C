/*
//         .             .              .		  
//         |             |              |           .	  
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
//  ,|							  
//  `'							  
// GITHUB:https://github.com/gabrielzschmitz		  
// INSTAGRAM:https://www.instagram.com/gabrielz.schmitz/   
// DOTFILES:https://github.com/gabrielzschmitz/dotfiles/
*/

/* Include Librarys */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Colors */
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

/* True and False */
#define true 1
#define false 0

/* Time Display */
void display_time(int x){
    int div = x / 60;
    int mod = x % 60;
    char minutes[3] = {(div / 10) + '0', (div % 10) + '0', '\0'};
    char seconds[3] = {(mod / 10) + '0', (mod % 10) + '0', '\0'};

    printf(BOLDBLACK "          %s:%s\n" RESET, minutes, seconds);
}

void machine_display(int x){
    int div = x / 60;
    int mod = x % 60;
    char minutes[3] = {(div / 10) + '0', (div % 10) + '0', '\0'};
    char seconds[3] = {(mod / 10) + '0', (mod % 10) + '0', '\0'};

    printf(BOLDBLACK "%s:%s" RESET, minutes, seconds);
}

/* Printing Funtions */
void print_tomato(int frame){
    if(frame == 0){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\         / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }              
    else if(frame == 1){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |╱    \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\         / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 2){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |     ⬤ ─   |\n" RESET);
        printf(BOLDRED "       \\         / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 3){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\     ╲   / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 4){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\    |    / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 5){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\   ╱     / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 6){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /     |     \\\n" RESET);
        printf(BOLDRED "      |   ─ ⬤     |\n" RESET);
        printf(BOLDRED "       \\         / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
    else if(frame == 7){
        printf(BOLDGREEN "\n\n\n         __\\W/__   \n" RESET);
        printf(BOLDGREEN "       .\'.-\'_\'-.\'. \n" RESET);
        printf(BOLDRED "      /    ╲|     \\\n" RESET);
        printf(BOLDRED "      |     ⬤     |\n" RESET);
        printf(BOLDRED "       \\         / \n" RESET);
        printf(BOLDRED "        \'-.___.-\'  \n" RESET);
        printf(BOLDMAGENTA "  ___                 _\n" RESET);
        printf(BOLDMAGENTA "   | _  _  _ |_ _    / \n" RESET);
        printf(BOLDMAGENTA "   |(_)|||(_||_(_) . \\_\n" RESET);
    }
}

void print_coffee(int frame){
    if(frame == 0){
        printf("\n\n\n\n\n");
        printf(BOLDBLACK "            ) )  \n" RESET);
        printf(BOLDBLACK "           ( (   \n" RESET);
        printf(BOLDWHITE "         ....... \n" RESET);
        printf(BOLDWHITE "         |     |]\n" RESET);
        printf(BOLDWHITE "         \\     / \n" RESET);
        printf(BOLDWHITE "          `---\'  \n" RESET);
    }              
    else if(frame == 1){
        printf("\n\n\n\n\n");
        printf(BOLDBLACK "          ( (    \n" RESET);
        printf(BOLDBLACK "           ) )   \n" RESET);
        printf(BOLDWHITE "         ....... \n" RESET);
        printf(BOLDWHITE "         |     |]\n" RESET);
        printf(BOLDWHITE "         \\     / \n" RESET);
        printf(BOLDWHITE "          `---\'  \n" RESET);
    }
}

void print_coffee_machine(int x, int frame){
    if(frame == 0){
        printf(BOLDWHITE "\n   ________._________ \n" RESET);
        printf(BOLDWHITE "   |   _   |\\       / \n" RESET);
        printf(BOLDWHITE "   |  |.|  | \\     /  \n" RESET);
        printf(BOLDWHITE "   |  |.|  |__\\___/   \n" RESET);
        printf(BOLDWHITE "   |  |.|  |    ¯     \n" RESET);
        printf(BOLDWHITE "   |   ¯   |   ___    \n" RESET);
        printf(BOLDWHITE "   |_______|  \\___/_  \n" RESET);
        printf(BOLDWHITE "   | _____ |  /~~~\\ \\ \n" RESET);
        printf(BOLDWHITE "   ||" RESET);
        machine_display(x);
        printf(BOLDWHITE "||__\\___/__ \n" RESET);
        printf(BOLDWHITE "   ||_____|          |\n" RESET);
        printf(BOLDWHITE "   |_________________|\n" RESET);
    }
    else if(frame == 1){
        printf(BOLDWHITE "\n   ________._________ \n" RESET);
        printf(BOLDWHITE "   |   _   |\\       / \n" RESET);
        printf(BOLDWHITE "   |  |.|  | \\     /  \n" RESET);
        printf(BOLDWHITE "   |  |.|  |__\\___/   \n" RESET);
        printf(BOLDWHITE "   |  |.|  |    †     \n" RESET);
        printf(BOLDWHITE "   |   ¯   |   ___    \n" RESET);
        printf(BOLDWHITE "   |_______|  \\___/_  \n" RESET);
        printf(BOLDWHITE "   | _____ |  /~~~\\ \\ \n" RESET);
        printf(BOLDWHITE "   ||" RESET);
        machine_display(x);
        printf(BOLDWHITE "||__\\___/__ \n" RESET);
        printf(BOLDWHITE "   ||_____|          |\n" RESET);
        printf(BOLDWHITE "   |_________________|\n" RESET);
    }
    else if(frame == 2){
        printf(BOLDWHITE "\n   ________._________ \n" RESET);
        printf(BOLDWHITE "   |   _   |\\       / \n" RESET);
        printf(BOLDWHITE "   |  |.|  | \\     /  \n" RESET);
        printf(BOLDWHITE "   |  |.|  |__\\___/   \n" RESET);
        printf(BOLDWHITE "   |  |.|  |    ¯     \n" RESET);
        printf(BOLDWHITE "   |   ¯   |   _|_    \n" RESET);
        printf(BOLDWHITE "   |_______|  \\___/_  \n" RESET);
        printf(BOLDWHITE "   | _____ |  /~~~\\ \\ \n" RESET);
        printf(BOLDWHITE "   ||" RESET);
        machine_display(x);
        printf(BOLDWHITE "||__\\___/__ \n" RESET);
        printf(BOLDWHITE "   ||_____|          |\n" RESET);
        printf(BOLDWHITE "   |_________________|\n" RESET);
    }
}

void print_beach(int frame){
    if(frame == 0){
        printf("\n\n");
        printf(BOLDYELLOW "     |                   \n" RESET);
        printf(BOLDYELLOW "   ╲ _ ╱                 \n" RESET);
        printf(BOLDYELLOW " -= (_) =-               \n" RESET);
        printf(BOLDYELLOW "   ╱   ╲" RESET);
        printf(BOLDGREEN "       _\\/_      \n" RESET);
        printf(BOLDYELLOW "     |" RESET);
        printf(BOLDGREEN "         //o\\  _\\/_\n" RESET);
        printf(BOLDBLUE "_ _ __ __ ____ _" RESET);
        printf(BOLDWHITE " |" RESET);
        printf(BOLDBLUE " __" RESET);
        printf(BOLDGREEN "/o\\\\\n" RESET);
        printf(BOLDBLUE "__=_-= _=_=-=" RESET);
        printf(BOLDWHITE "_,-\'|\"\'\"\"-|-\n" RESET);
        printf(BOLDBLUE "-=- -_=-=" RESET);
        printf(BOLDWHITE "_,-\"          | \n" RESET);
        printf(BOLDBLUE "=- -=" RESET);
        printf(BOLDWHITE ".--\"                \n" RESET);
    }
    else if(frame == 1){
        printf("\n\n");
        printf(BOLDYELLOW "                        \n" RESET);
        printf(BOLDYELLOW "   \\ | /                 \n" RESET);
        printf(BOLDYELLOW "  - (_) -                \n" RESET);
        printf(BOLDYELLOW "   / | \\" RESET);
        printf(BOLDGREEN "      _\\/_      \n" RESET);
        printf(BOLDYELLOW "     " RESET);
        printf(BOLDGREEN "         //o\\  _\\/_\n" RESET);
        printf(BOLDBLUE "__ ____ __  ____" RESET);
        printf(BOLDWHITE " |" RESET);
        printf(BOLDBLUE "_ /" RESET);
        printf(BOLDGREEN "/o\\" RESET);
        printf(BOLDBLUE "_\n" RESET);
        printf(BOLDBLUE "--_=-__=  _-=" RESET);
        printf(BOLDWHITE "_,-\'|\"\'\"\"-|-\n" RESET);
        printf(BOLDBLUE "_-= _=-_=" RESET);
        printf(BOLDWHITE "_,-\"          | \n" RESET);
        printf(BOLDBLUE "-= _-" RESET);
        printf(BOLDWHITE ".--\"                \n" RESET);
    }
}

/* Just Cleanig the Terminal */
void clear_terminal(void){
    system("clear");
}

/* Scenes */
void welcome(int duration){
    int w = 0;
    int t = 10;
    int f0 = 0, f1 = 1, f2 = 2, f3 = 3, f4 = 4, f5 = 5, f6 = 6, f7 = 7;

    while (w < duration) {
        clear_terminal();
        print_tomato(f0);
        // swapping f0 and f1
        f0 = f0 + f1;
        f1 = f0 - f1;
        f0 = f0 - f1;
        // swapping f1 and f2
        f1 = f1 + f2;
        f2 = f1 - f2;
        f1 = f1 - f2;
        // swapping f2 and f3
        f2 = f2 + f3;
        f3 = f2 - f3;
        f2 = f2 - f3;
        // swapping f3 and f4
        f3 = f3 + f4;
        f4 = f3 - f4;
        f3 = f3 - f4;
        // swapping f4 and f5
        f4 = f4 + f5;
        f5 = f4 - f5;
        f4 = f4 - f5;
        // swapping f5 and f6
        f5 = f5 + f6;
        f6 = f5 - f6;
        f5 = f5 - f6;
        // swapping f6 and f7
        f6 = f6 + f7;
        f7 = f6 - f7;
        f6 = f6 - f7;
        printf(BOLDWHITE "\n        Start in:\n" RESET);
        display_time(t);
        sleep(1);
        t -= 1;
        w += 1;
    }
}

void run(int duration){
    int x = 0;
    int f0 = 0, f1 = 0, f2 = 0, f3 = 1, f4 = 1, f5 = 1;

    while (x < (duration * 60)) {
        clear_terminal();
        print_coffee(f0);
        // swapping f0 and f1
        f0 = f0 + f1;
        f1 = f0 - f1;
        f0 = f0 - f1;
        // swapping f1 and f2
        f1 = f1 + f2;
        f2 = f1 - f2;
        f1 = f1 - f2;
        // swapping f2 and f3
        f2 = f2 + f3;
        f3 = f2 - f3;
        f2 = f2 - f3;
        // swapping f3 and f4
        f3 = f3 + f4;
        f4 = f3 - f4;
        f3 = f3 - f4;
        // swapping f4 and f5
        f4 = f4 + f5;
        f5 = f4 - f5;
        f4 = f4 - f5;
        printf(BOLDMAGENTA "  Pomodoro" BOLDWHITE " [%d minutes]\n" RESET, duration);
        display_time(x);
        sleep(1);
        x += 1;
    }
}

void short_pause(int duration){
    int x = 0;
    int f0 = 0, f1 = 0, f2 = 1, f3 = 2;

    while (x < (duration * 60)) {
        clear_terminal();
        print_coffee_machine(x, f0);
        // swapping f0 and f1
        f0 = f0 + f1;
        f1 = f0 - f1;
        f0 = f0 - f1;
        // swapping f1 and f2
        f1 = f1 + f2;
        f2 = f1 - f2;
        f1 = f1 - f2;
        // swapping f2 and f3
        f2 = f2 + f3;
        f3 = f2 - f3;
        f2 = f2 - f3;
        printf(BOLDCYAN "    Pause" BOLDWHITE " [%d minutes]\n" RESET, duration);
        sleep(1);
        x += 1;
    }
}

void long_pause(int duration){
    int x = 0;
    int f0 = 0, f1 = 0, f2 = 0, f3 = 1, f4 = 1, f5 = 1;

    while (x < (duration * 60)) {
        clear_terminal();
        print_beach(f0);
        // swapping f0 and f1
        f0 = f0 + f1;
        f1 = f0 - f1;
        f0 = f0 - f1;
        // swapping f1 and f2
        f1 = f1 + f2;
        f2 = f1 - f2;
        f1 = f1 - f2;
        // swapping f2 and f3
        f2 = f2 + f3;
        f3 = f2 - f3;
        f2 = f2 - f3;
        // swapping f3 and f4
        f3 = f3 + f4;
        f4 = f3 - f4;
        f3 = f3 - f4;
        // swapping f4 and f5
        f4 = f4 + f5;
        f5 = f4 - f5;
        f4 = f4 - f5;
        printf(BOLDCYAN " Long pause" BOLDWHITE " [%d minutes]\n" RESET, duration);
        display_time(x);
        sleep(1);
        x += 1;
    }
}

/* Main Function */
int main(void){
    // hide cursor
    printf("\e[?25l");
    welcome(10);

    int count_run = 0;
    while (true) {
        system("notify-send -t 5750 -c cpomo '華 Work!' 'You need to focus 25 minutes'");
        run(25);
        count_run += 1;
        if ((count_run > 0) && (count_run % 4 == 0)) {
            system("notify-send -t 5750 -c cpomo ' Pause Break' 'You have 30 minutes to chill'");
            long_pause(30);
        } else {
            system("notify-send -t 5750 -c cpomo ' Pause Break' 'You have 5 minutes to chill'");
            short_pause(5);
        }
    }
    return (0);
}

