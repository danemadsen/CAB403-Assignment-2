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
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "manager.h"
#include "common.h"

int main() {
  // wait until a shared memory segment named PARKING is created
  if((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
    printf("Shared memory segment doesnt exist\n");
    return(1);
  }
  Parking = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
  revenue = 0;

  //Initialise the mutexes
  pthread_mutex_init(&parked_cars_mlock, NULL);
  pthread_mutex_init(&arriving_cars_mlock, NULL);
  pthread_mutex_init(&departing_cars_mlock, NULL);
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
  display_loop();
  //while(1);
  return 0;
};

void charge_car(Car_t *Auto) {
  // Calculate the cost of the car's stay
  if(Auto->plate[0] == '\0') return;
  double cost = (clock() - Auto->arrival_time) * RATE;
  
  // Create a new text file for the bill
  FILE *bill = fopen("billing.txt", "a");
  fprintf(bill, "%s $%0.01lf\n", Auto->plate, cost);
  fclose(bill);

  //Increment the total revenue
  pthread_mutex_lock(&revenue_lock);
  revenue += cost;
  pthread_mutex_unlock(&revenue_lock);
};

void add_parked_car(Car_t Auto){
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVEL_CAPACITY; i++) {
    if (parked_cars[Auto.level][i].plate[0] == 0) {
      parked_cars[Auto.level][i] = Auto;
      break;
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
};

void add_arriving_car(Car_t Auto){
  pthread_mutex_lock(&arriving_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (arriving_cars[i].plate[0] == 0) {
      arriving_cars[i] = Auto;
      break;
    }
  }
  pthread_mutex_unlock(&arriving_cars_mlock);
};

void add_departing_car(Car_t Auto){
  pthread_mutex_lock(&departing_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (departing_cars[i].plate[0] == 0) {
      departing_cars[i] = Auto;
      break;
    }
  }
  pthread_mutex_unlock(&departing_cars_mlock);
};

void get_car(Car_t *Auto) {
  pthread_mutex_lock(&arriving_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (strcmp(arriving_cars[i].plate, (char*) &Auto->plate[0]) == 0) {
      *Auto = arriving_cars[i];
      arriving_cars[i] = (Car_t) {0};
      pthread_mutex_unlock(&arriving_cars_mlock);
    }
  }
  pthread_mutex_unlock(&arriving_cars_mlock);
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVELS; i++) {
    for (int j = 0; j < LEVEL_CAPACITY; j++) {
      if (strcmp(parked_cars[i][j].plate, (char*) &Auto->plate[0]) == 0) {
        *Auto = parked_cars[i][j];
        parked_cars[i][j] = (Car_t) {0};
        pthread_mutex_unlock(&parked_cars_mlock);
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  pthread_mutex_lock(&departing_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (strcmp(departing_cars[i].plate, (char*) &Auto->plate[0]) == 0) {
      *Auto = departing_cars[i];
      departing_cars[i] = (Car_t) {0};
      pthread_mutex_unlock(&departing_cars_mlock);
    }
  }
  pthread_mutex_unlock(&departing_cars_mlock);
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
  pthread_mutex_lock(&arriving_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (strcmp(arriving_cars[i].plate, plate) == 0) {
      pthread_mutex_unlock(&arriving_cars_mlock);
      return false;
    }
  }
  pthread_mutex_unlock(&arriving_cars_mlock);
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVELS; i++) {
    for (int j = 0; j < LEVEL_CAPACITY; j++) {
      if (strcmp(parked_cars[i][j].plate, plate) == 0) {
        pthread_mutex_unlock(&parked_cars_mlock);
        return false;
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  pthread_mutex_lock(&departing_cars_mlock);
  for (int i = 0; i < LEVELS* LEVEL_CAPACITY; i++) {
    if (strcmp(departing_cars[i].plate, plate) == 0) {
      pthread_mutex_unlock(&departing_cars_mlock);
      return false;
    }
  }
  pthread_mutex_unlock(&departing_cars_mlock);
  return true;
};

bool check_space() {
  pthread_mutex_lock(&parked_cars_mlock);
  for (int i = 0; i < LEVEL_CAPACITY; i++) {
    for (int j = 0; j < LEVELS; j++) {
      if (parked_cars[j][i].plate[0] == '\0') {
        pthread_mutex_unlock(&parked_cars_mlock);
        return true;
      }
    }
  }
  pthread_mutex_unlock(&parked_cars_mlock);
  return false;
};

bool check_alarm() {
  for (int i = 0; i < LEVELS; i++) {
    if(Parking->levels[i].alarm) {
      return true;
    }
  }
  return false;
};

char get_level() {
  pthread_mutex_lock(&parked_cars_mlock);
  while(1) {
    int lvl = rand() % LEVELS;
    for (int j = 0; j < LEVEL_CAPACITY; j++) {
      if (parked_cars[lvl][j].plate[0] == '\0') {
        pthread_mutex_unlock(&parked_cars_mlock);
        return lvl;
      }
    }
  }
};

void raise_boom_gate(BoomGate_t *boom_gate) {
  pthread_mutex_lock(&boom_gate->mlock);
  while (boom_gate->status != 'C') {
    pthread_cond_wait(&boom_gate->condition, &boom_gate->mlock);
  }
  boom_gate->status = 'R';
  pthread_mutex_unlock(&boom_gate->mlock);
  pthread_cond_broadcast(&boom_gate->condition);
};

void lower_boom_gate(BoomGate_t *boom_gate) {
  pthread_mutex_lock(&boom_gate->mlock);
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
  while(sign->display != ' ') {
    pthread_cond_wait(&sign->condition, &sign->mlock);
  }
  sign->display = signal + 49; // Converts the level number to a char
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

int get_total_count() {
  int count = 0;
  for (int i = 0; i < LEVELS; i++) {
    count += get_level_count(i);
  }
  return count;
};

void *entrance_loop(void *arg) {
  Entrance_t *entrance = (Entrance_t *)arg;
  Car_t Auto;
  char lvl ;
  while(!check_alarm()) {
    char plate[6];
    pthread_mutex_lock(&entrance->LPR.mlock);
    while(entrance->LPR.plate[0] == 0) {
      pthread_cond_wait(&entrance->LPR.condition, &entrance->LPR.mlock);
    }
    strcpy(plate, entrance->LPR.plate);
    entrance->LPR.plate[0] = '\0';
    pthread_mutex_unlock(&entrance->LPR.mlock);
    if (check_plate(plate) && check_space()) {
      lvl = get_level();
      set_sign(&entrance->information_sign, lvl);
      raise_boom_gate(&entrance->boom_gate);
      // wait 20ms
      usleep(20000*TIMESCALE);
      lower_boom_gate(&entrance->boom_gate);
      // Using strcmp makes plate overflow so a for loop is using instead
      for (int i = 0; i < 6; i++) {
        Auto.plate[i] = plate[i];
      }
      Auto.arrival_time = clock();
      Auto.level = 254;
      add_arriving_car(Auto);
    }
    else{
      if (!check_space()) set_sign(&entrance->information_sign, 21); // Converts to char 'F'
      else if (!check_plate(plate)) set_sign(&entrance->information_sign, 39); // Converts to char 'X'
    }
  }
  pthread_exit(NULL);
};

void *level_loop(void *arg) {
  Level_t *level = (Level_t *)arg;
  while(!check_alarm()) { 
    Car_t Auto;
    pthread_mutex_lock(&level->LPR.mlock);
    pthread_cond_wait(&level->LPR.condition, &level->LPR.mlock);
    strcpy(Auto.plate, level->LPR.plate);
    *level->LPR.plate = '\0';
    pthread_mutex_unlock(&level->LPR.mlock);
    get_car(&Auto);
    if(Auto.level != get_level_index(level)) {
      Auto.level = (char) get_level_index(level);
      add_parked_car(Auto);
    }
    else {
      add_departing_car(Auto);
    }
  }
  pthread_exit(NULL);
};

void *exit_loop(void *arg) {
  Exit_t *exit = (Exit_t *)arg;
  Car_t Auto;
  while(!check_alarm()) {
    pthread_mutex_lock(&exit->LPR.mlock);
    pthread_cond_wait(&exit->LPR.condition, &exit->LPR.mlock);
    strcpy(Auto.plate, exit->LPR.plate);
    pthread_mutex_unlock(&exit->LPR.mlock);
    get_car(&Auto);
    charge_car(&Auto);
    raise_boom_gate(&exit->boom_gate);
    // wait 20ms
    usleep(20000*TIMESCALE);
    lower_boom_gate(&exit->boom_gate);
    *exit->LPR.plate = '\0';
  }
  while(get_total_count() > 0){
    pthread_mutex_lock(&exit->LPR.mlock);
    pthread_cond_wait(&exit->LPR.condition, &exit->LPR.mlock);
    strcpy(Auto.plate, exit->LPR.plate);
    pthread_mutex_unlock(&exit->LPR.mlock);
    get_car(&Auto);
  }
  pthread_exit(NULL);
};

void display_loop() {
  while(1) {
    system("clear");
    printf("████████████████████████████████████████████████████████████████████████\n"
           "█                              PARKING LOT                             █\n"
           "████████████████████████████████████████████████████████████████████████\n"
           "█       Index        |    1    |    2    |    3    |    4    |    5    █\n"
           "█----------------------------------------------------------------------█\n"
           "█ Entrance Boom Gate |    %c    |    %c    |    %c    |    %c    |    %c    █\n"
           "█----------------------------------------------------------------------█\n"
           "█   Exit Boom Gate   |    %c    |    %c    |    %c    |    %c    |    %c    █\n"
           "█----------------------------------------------------------------------█\n"
           "█   Entrance Sign    |    %c    |    %c    |    %c    |    %c    |    %c    █\n"
           "████████████████████████████████████████████████████████████████████████\n\n"
           "Level 1 Count: %d\n"
           "Level 2 Count: %d\n"
           "Level 3 Count: %d\n"
           "Level 4 Count: %d\n"
           "Level 5 Count: %d\n\n"           
           "Current Revenue: %0.01f\n",
           get_boom_gate_status(&Parking->entrances[0].boom_gate),
           get_boom_gate_status(&Parking->entrances[1].boom_gate),
           get_boom_gate_status(&Parking->entrances[2].boom_gate),
           get_boom_gate_status(&Parking->entrances[3].boom_gate),
           get_boom_gate_status(&Parking->entrances[4].boom_gate),
           get_boom_gate_status(&Parking->exits[0].boom_gate),
           get_boom_gate_status(&Parking->exits[1].boom_gate),
           get_boom_gate_status(&Parking->exits[2].boom_gate),
           get_boom_gate_status(&Parking->exits[3].boom_gate),
           get_boom_gate_status(&Parking->exits[4].boom_gate),
           Parking->entrances[0].information_sign.display,
           Parking->entrances[1].information_sign.display,
           Parking->entrances[2].information_sign.display,
           Parking->entrances[3].information_sign.display,
           Parking->entrances[4].information_sign.display,
           get_level_count(0),
           get_level_count(1),
           get_level_count(2),
           get_level_count(3),
           get_level_count(4),
           revenue);
    usleep(10000);
  }
};
