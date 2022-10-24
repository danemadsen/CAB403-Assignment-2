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
                            CAB403 Assignment 2 - Common Header
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au
*/

#pragma once
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// -------------------------------- Settings --------------------------------
#define LEVELS              5       // number of levels in the car park
#define LEVEL_CAPACITY      20      // number of spaces on each level
#define ENTRANCES           5       // number of entrances to the car park
#define EXITS               5       // number of exits from the car park
#define RATE                0.05    // amount cars are charged per ms
#define FIRE_CHANCE         5000    // chance of fire occuring per temperature cycle
#define FIRE_THRESHOLD      58      // temperature at which a fire is detected
#define MAX_TEMP_CHANGE     6       // max temperature change
#define BASE_TEMP           20      // base temperature
#define MAX_TEMP            60      // maximum temperature
#define MEDIAN_SAMPLES      5       // number of samples to take for median
#define SMOOTHED_SAMPLES    30      // number of samples to take for smoothed
#define TIMESCALE           1       // timescale of simulation
// ---------------------------------------------------------------------------

typedef struct Car Car_t;
typedef struct LicencePlateRecognition LPR_t;
typedef struct BoomGate BoomGate_t;
typedef struct InformationSign Sign_t;
typedef struct Entrance Entrance_t;
typedef struct Exit Exit_t;
typedef struct Level Level_t;
typedef struct CarPark CarPark_t;

#define SHM_NAME "PARKING"
#define SHM_SIZE sizeof(CarPark_t) // Default 2930
#define SHM_MODE 438

struct Car{
    char plate[6];
    clock_t arrival_time;
    char level;
};

struct LicencePlateRecognition{
    pthread_mutex_t mlock;
    pthread_cond_t condition;
    char plate[6];
    char padding[2];
};

struct BoomGate{
    pthread_mutex_t mlock;
    pthread_cond_t condition;
    char status;
    char padding[7];
};

struct InformationSign{
    pthread_mutex_t mlock;
    pthread_cond_t condition;
    char display;
    char padding[7];
};

struct Entrance{
    struct LicencePlateRecognition LPR;
    struct BoomGate boom_gate;
    struct InformationSign information_sign;
};

struct Exit{
    struct LicencePlateRecognition LPR;
    struct BoomGate boom_gate;
};

struct Level{
    struct LicencePlateRecognition LPR;
    uint16_t temperature;
    uint8_t alarm;
    char padding[5];
};

struct CarPark{
    struct Entrance entrances[ENTRANCES];
    struct Level levels[LEVELS];
    struct Exit exits[EXITS];
};

int shm_fd = -1;
CarPark_t* Parking;