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
                            CAB403 Assignment 2 - Manager
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au

The roles of the manager:
● Monitor the status of the LPR sensors and keep track of where each car is in the car
park

● Tell the boom gates when to open and when to close (the boom gates are a simple
piece of hardware that can only be told to open or close, so the job of automatically
closing the boom gates after they have been open for a little while is up to the
manager)

● Control what is displayed on the information signs at each entrance

● As the manager knows where each car is, it is the manager’s job to ensure that there
is room in the car park before allowing new vehicles in (number of cars < number of
levels * the number of cars per level). The manager also needs to keep track of how
full the individual levels are and direct new cars to a level that is not fully occupied

● Keep track of how long each car has been in the parking lot and produce a bill once
the car leaves.

● Display the current status of the parking lot on a frequently-updating screen, showing
how full each level is, the current status of the boom gates, signs, temperature
sensors and alarms, as well as how much revenue the car park has brought in so far.

Manager timings:
● After a boom gate has been fully opened, it will start to close 20ms later. Cars
entering the car park will just drive in if the boom gate is fully open after they have
been directed to a level (however, if the car arrives just as the boom gate starts to
close, it will have to wait for the boom gate to fully close, then fully open again.)

● Cars are billed based on how long they spend in the car park (see the Billing section
for more information.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "manager.h"
#include "common.h"

int main() {
  // Setup the shared memory segement
  shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666);
  Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  return 0;
}

bool check_plate(char* plate) {
    FILE* file = fopen("plates.txt", "r");
    char c;
    char* file_plate = malloc(7);
    while ((c = fgetc(file)) != EOF) {
        fseek(file, -1, SEEK_CUR);
        fgets(file_plate, 7, file);
        if (strcmp(plate, file_plate) == 0) {
            free(file_plate);
            fclose(file);
            return true;
        }
        memset(file_plate, 0, 7);
    }
    free(file_plate);
    fclose(file);
    return false;
}
