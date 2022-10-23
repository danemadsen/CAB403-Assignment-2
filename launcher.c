#include <stdio.h>
#include <stdlib.h>

int main() {
    if (system("which gnome-terminal") == 0) {
        system("gnome-terminal -e \"./simulator\"");
        system("gnome-terminal -e \"./firealarm\"");
        system("gnome-terminal -e \"./manager\"");
    } else if (system("which konsole") == 0) {
        system("konsole -e \"./simulator\"");
        system("konsole -e \"./firealarm\"");
        system("konsole -e \"./manager\"");
    } else if (system("which xterm") == 0) {
        system("xterm -e \"./simulator\"");
        system("xterm -e \"./firealarm\"");
        system("xterm -e \"./manager\"");
    } else {
        printf("No terminal emulator found. Please run the firealarm program manually.\n");
    }
};