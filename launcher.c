/*
                             888888888               888888888     
                           88:::::::::88           88:::::::::88   
                         88:::::::::::::88       88:::::::::::::88 
                        8::::::88888::::::8     8::::::88888::::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                         8:::::88888:::::8       8:::::88888:::::8 
                          8:::::::::::::8         8:::::::::::::8  
                         8:::::88888:::::8       8:::::88888:::::8 
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8:::::8     8:::::8     8:::::8     8:::::8
                        8::::::88888::::::8     8::::::88888::::::8
                         88:::::::::::::88       88:::::::::::::88 
                           88:::::::::88           88:::::::::88   
                             888888888               888888888     

------------------------------------------------------------------------------------------
                            CAB403 Assignment 2 - Launcher
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au
*/

#include <stdio.h>
#include <stdlib.h>

int main() {
    if (system("which gnome-terminal") == 0) {
        system("gnome-terminal -e \"./simulator\"");
        system("gnome-terminal -e \"./firealarm\"");
        system("gnome-terminal -e \"./manager\"");
    } else if (system("which xfce4-terminal") == 0) {
        system("xfce4-terminal -e \"./simulator\"");
        system("xfce4-terminal -e \"./firealarm\"");
        system("xfce4-terminal -e \"./manager\"");
    } else if (system("which konsole") == 0) {
        system("konsole -e \"./simulator\"");
        system("konsole -e \"./firealarm\"");
        system("konsole -e \"./manager\"");
    } else if (system("which xterm") == 0) {
        system("xterm -e \"./simulator\"");
        system("xterm -e \"./firealarm\"");
        system("xterm -e \"./manager\"");
    } else {
        printf("No terminal emulator found. Please run the programs manually.\n");
    }
};