#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 2930
#define LEVELS 5
#define LEVEL_CAPACITY 20
#define ENTRANCES 5
#define EXITS 5

struct Car{
    char plate[6];
    int arrival_time;
    int departure_time;
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