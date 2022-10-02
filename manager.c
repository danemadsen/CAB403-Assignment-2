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
