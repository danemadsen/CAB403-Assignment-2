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
  // wait until a shared memory segment named PARKING is created
  while((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
    printf("Waiting for shared memory segment to be created...\n");
    sleep(1);
    printf("\033[2J\033[1;1H");
  }
  Parking = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
  revenue = 0;

  //Initialise the mutexes and conditions
  pthread_mutex_init(&parked_cars_mlock, NULL);
  pthread_cond_init(&parked_cars_condition, NULL);
  pthread_mutex_init(&revenue_lock, NULL);

  // Initialise the threads
  for (int i = 0; i < ENTRANCES; i++) {
    pthread_create(&entrance_threads[i], NULL, entrance_loop, &Parking->entrances[i]);
  }
  for (int i = 0; i < LEVELS; i++) {
    pthread_create(&level_threads[i], NULL, level_loop, &Parking->levels[i]);
  }
  for (int i = 0; i < EXITS; i++) {
    pthread_create(&exit_threads[i], NULL, exit_loop, &Parking->exits[i]);
  }
  //pthread_create(&entrance_threads[0], NULL, entrance_loop, &Parking->entrances[0]);
  //pthread_create(&level_threads[0], NULL, level_loop, &Parking->levels[0]);
  //pthread_create(&exit_threads[0], NULL, exit_loop, &Parking->exits[0]);
  display_loop();
  while(1);
  return 0;
}

void charge_car(Car_t *Auto) {
  // Calculate the time the car has been in the car park
  int time_in_parking_lot = Auto->departure_time - Auto->arrival_time;
  // Calculate the cost of the car's stay
  double cost = time_in_parking_lot * RATE;
  //Increment the total revenue
  pthread_mutex_lock(&revenue_lock);
  revenue += cost;
  pthread_mutex_unlock(&revenue_lock);
};

void add_car(Car_t Auto){
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVEL_CAPACITY; i++) {
      if (parked_cars[Auto.level][i].plate[0] == 0) {
        parked_cars_count[Auto.level]++;
        parked_cars[Auto.level][i] = Auto;
        break;
      }
  }
  pthread_cond_signal(&parked_cars_condition);
  pthread_mutex_unlock(&parked_cars_mlock);
};

void remove_car(Car_t Auto) {
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVELS; i++) {
    for (int j = 0; j < LEVEL_CAPACITY; j++) {
      if (strcmp(parked_cars[i][j].plate, Auto.plate) == 0) {
        parked_cars_count[i]--;
        parked_cars[i][j] = (Car_t) {0};
        break;
      }
    }
  }
  pthread_cond_signal(&parked_cars_condition);
  pthread_mutex_unlock(&parked_cars_mlock);
};

void get_car(Car_t *Auto) {
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVELS; i++) {
    for (int j = 0; j < LEVEL_CAPACITY; j++) {
      if (strcmp(parked_cars[i][j].plate, Auto->plate) == 0) {
        *Auto = parked_cars[i][j];
        break;
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
};

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
        return check_unique(plate);
      }
      memset(file_plate, 0, 7);
  }
  free(file_plate);
  fclose(file);
  return false;
};

bool check_unique(char* plate) {
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < (LEVELS); i++) {
    for (int j = 0; j < (LEVEL_CAPACITY); j++) {
      if (strcmp(parked_cars[i][j].plate, plate) == 0) {
        pthread_mutex_unlock(&parked_cars_mlock);
        return false;
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  return true;
};

bool check_space(char *lvl) {
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < (LEVELS); i++) {
    for (int j = 0; j < (LEVEL_CAPACITY); j++) {
      if (parked_cars[i][j].plate[0] == '\0') {
      *lvl = i + 49; // Converts the level number to a char
      pthread_mutex_unlock(&parked_cars_mlock);
      return true;
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  return false;
};

void raise_boom_gate(BoomGate_t *boom_gate) {
  pthread_mutex_lock(&boom_gate->mlock);
  if (boom_gate->status == 'O') boom_gate->status = 'L';
  while (boom_gate->status != 'C') {
    pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
  }
  boom_gate->status = 'R';
  pthread_mutex_unlock(&boom_gate->mlock);
  pthread_cond_broadcast(&boom_gate->condition);
};

void lower_boom_gate(BoomGate_t *boom_gate) {
  pthread_mutex_lock(&boom_gate->mlock);
  if (boom_gate->status == 'C') boom_gate->status = 'R';
  while (boom_gate->status != 'O') {
    pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
  }
  boom_gate->status = 'L';
  pthread_mutex_unlock(&boom_gate->mlock);
  pthread_cond_broadcast(&boom_gate->condition);
};

char get_boom_gate_status(BoomGate_t *boom_gate) {
  pthread_mutex_lock(&boom_gate->mlock);
  char status = boom_gate->status;
  pthread_mutex_unlock(&boom_gate->mlock);
  return status;
};

void set_sign(Sign_t *sign, char signal) {
  pthread_mutex_lock(&sign->mlock);
  sign->display = signal;
  pthread_mutex_unlock(&sign->mlock);
  pthread_cond_signal(&sign->condition);
};

int get_level_index(Level_t *lvl) {
  for (int i = 0; i < LEVELS; i++) {
    if (lvl == &Parking->levels[i]) {
      return i;
    }
  }
  return -1;
};

int get_level_count(int level) {
  int count = 0;
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVEL_CAPACITY; i++) {
    if (parked_cars[level][i].plate[0] != 0) {
      count++;
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  return count;
};

void *entrance_loop(void *arg) {
  Entrance_t *entrance = (Entrance_t *)arg;
  char lvl;
  while(1) {
    char plate[6];
    pthread_mutex_lock(&entrance->LPR.mlock);
    while(entrance->LPR.plate[0] == 0) {
      pthread_cond_wait(&entrance->LPR.condition, &entrance->LPR.mlock);
    }
    strcpy(plate, entrance->LPR.plate);
    strcpy(entrance->LPR.plate, (char[6]) {0});
    pthread_mutex_unlock(&entrance->LPR.mlock);
    if (check_plate(plate) && check_space(&lvl)) {
      set_sign(&entrance->information_sign, lvl);
      raise_boom_gate(&entrance->boom_gate);
      // wait 20ms
      usleep(20000);
      lower_boom_gate(&entrance->boom_gate);
    }
    else {
      set_sign(&entrance->information_sign, 'F');
    }
  }
};

void *level_loop(void *arg) {
  Level_t *level = (Level_t *)arg;
  while(1) {
    Car_t Auto;
    pthread_mutex_lock(&level->LPR.mlock);
    pthread_cond_wait(&level->LPR.condition, &level->LPR.mlock);
    strcpy(Auto.plate, level->LPR.plate);
    *level->LPR.plate = '\0';
    pthread_mutex_unlock(&level->LPR.mlock);
    get_car(&Auto);
    Auto.level = (char) get_level_index(level);
    remove_car(Auto);
    add_car(Auto);
  }
};

void *exit_loop(void *arg) {
  Exit_t *exit = (Exit_t *)arg;
  while(1) {
    Car_t Auto;
    pthread_mutex_lock(&exit->LPR.mlock);
    pthread_cond_wait(&exit->LPR.condition, &exit->LPR.mlock);
    strcpy(Auto.plate, exit->LPR.plate);
    pthread_mutex_unlock(&exit->LPR.mlock);
    get_car(&Auto);
    remove_car(Auto);
    charge_car(&Auto);
    raise_boom_gate(&exit->boom_gate);
    // wait 20ms
    usleep(20000);
    lower_boom_gate(&exit->boom_gate);
    *exit->LPR.plate = '\0';
  }
};

void display_loop() {
  revenue = 0;
  while(1) {
    printf("Current Revenue: %lf\n\n", revenue);
    for (int i = 0; i < LEVELS; i++) {
      printf("Level %d Vehicle Count: %d\n", i + 1, get_level_count(i));
    }
    printf("\n");
    // Display the current status of all boom gates
    for (int i = 0; i < ENTRANCES; i++) {
      printf("Entrance %d Boom Gate Status: %c\n", i + 1, get_boom_gate_status(&Parking->entrances[i].boom_gate));
    }
    for (int i = 0; i < EXITS; i++) {
      printf("Exit %d Boom Gate Status: %c\n", i + 1, get_boom_gate_status(&Parking->exits[i].boom_gate));
    }
    //wait 1s
    sleep(1);
    //clear the console
    printf("\033[2J\033[1;1H");
  }
};
