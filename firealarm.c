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
                            CAB403 Assignment 2 - Fire Alarm
------------------------------------------------------------------------------------------

Group: 88
Team Member: Dane Madsen
Student ID: n10983864
Student Email: n10983864@qut.edu.au

The roles of the fire alarm system:
● Monitor the status of temperature sensors on each car park level

● When a fire is detected, activate alarms on every car park level, open all boom gates
and display an evacuation message on the information signs
*/

#include "firealarm.h"

int main(void)
{
	shm_fd = shm_open(SHM_NAME, O_RDWR, SHM_MODE);
	// if shm_open fails, exit
	if(shm_fd != -1) {
		Parking = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		assert(Parking != NULL);

		alarm_active = 0;

		//initialise mutex and condition variables
		pthread_mutex_init(&alarm_lock, NULL);
		pthread_cond_init(&alarm_cond, NULL);

		for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
			Parking->levels[i].alarm = 0;
			pthread_create(&level_threads[i], NULL, temperature_monitor, (void *) (uintptr_t) i);
		}

		while(!alarm_active) {
			pthread_mutex_lock(&alarm_lock);
			pthread_cond_wait(&alarm_cond, &alarm_lock);
			pthread_mutex_unlock(&alarm_lock);
		}
		assert(alarm_active);
		emergency_mode();
		for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
			pthread_join(level_threads[i], NULL);
		}
		pthread_mutex_destroy(&alarm_lock);
		pthread_cond_destroy(&alarm_cond);
  	}
	else {
		assert(shm_fd != -1);
	}
	return 0;
}

void emergency_mode(void) {
	assert(alarm_active);
	
	// Handle the alarm system and open boom gates
	// Activate alarms on all levels
	for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
		Parking->levels[i].alarm = 1;
	}
	// Open all boom gates
	open_all_boom_gates();
	
	// Show evacuation message while alarm is active
	while(alarm_active != 0) {
		// Display evacuation message on information signs
		evacuation_message();

		// check if the alarm is still active
		check_alarm();
	}
	assert(!alarm_active);
}

void check_alarm(void) {
	assert(alarm_active);
	for (uint8_t i = 0; i < (uint8_t) LEVELS; i++) {
		if (Parking->levels[i].alarm == (uint8_t) 0) {
			pthread_mutex_lock(&alarm_lock);
			alarm_active = 0;
			pthread_cond_broadcast(&alarm_cond);
			pthread_mutex_unlock(&alarm_lock);
			assert(alarm_active == (uint8_t) 0);
			
		}
	}
	return;
}

void evacuation_message(void) {
	assert(alarm_active);
	const char evacmessage[9] = "EVACUATE";
	for (uint8_t i = 0; i != (uint8_t) 9; i++) {
		for (uint8_t j = 0; j < (uint8_t) ENTRANCES; j++) {
			pthread_mutex_lock(&Parking->entrances[j].information_sign.mlock);
			Parking->entrances[j].information_sign.display = evacmessage[i];
			assert(Parking->entrances[j].information_sign.display == (char) evacmessage[i]);
			pthread_cond_broadcast(&Parking->entrances[j].information_sign.condition);
			pthread_mutex_unlock(&Parking->entrances[j].information_sign.mlock);
		}
		usleep(20000*TIMESCALE);
	}
}

void open_all_boom_gates(void) {
	assert(alarm_active);
	for(uint8_t i = 0; i < (uint8_t) ENTRANCES; i++) {
		pthread_mutex_lock(&Parking->entrances[i].boom_gate.mlock);
		Parking->entrances[i].boom_gate.status = 'O';
		assert(Parking->entrances[i].boom_gate.status == (char) 'O');
		pthread_cond_broadcast(&Parking->entrances[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->entrances[i].boom_gate.mlock);
	}
	for(uint8_t i = 0; i < (uint8_t) EXITS; i++) {
		pthread_mutex_lock(&Parking->exits[i].boom_gate.mlock);
		Parking->exits[i].boom_gate.status = 'O';
		assert(Parking->exits[i].boom_gate.status == (char) 'O');
		pthread_cond_broadcast(&Parking->exits[i].boom_gate.condition);
		pthread_mutex_unlock(&Parking->exits[i].boom_gate.mlock);
	}
}

void *temperature_monitor(void *arg) {
	uint8_t level = (uint8_t) (uintptr_t) arg;
	assert(level < (uint8_t) LEVELS);
	volatile uint16_t temperatures[MEDIAN_SAMPLES];
	uint16_t smoothed_temperatures[SMOOTHED_SAMPLES];
	uint8_t under_samples = (uint8_t) SMOOTHED_SAMPLES * (uint8_t) MEDIAN_SAMPLES;
	
	while(!alarm_active) {		
		// Add temperature to beginning of temperatures array
		for(uint8_t i = MEDIAN_SAMPLES - 1; i > (uint8_t) 0; i--) {
			temperatures[i] = temperatures[i - (uint8_t) 1];
		}
		temperatures[0] = Parking->levels[level].temperature;
		
		// Add median temp beginning of smoothed_temperatures array
		for(uint8_t i = SMOOTHED_SAMPLES - 1; i > (uint8_t) 0; i--) {
			smoothed_temperatures[i] = smoothed_temperatures[i - (uint8_t) 1];
		}
		smoothed_temperatures[0] = median_temperature(temperatures);

		if(!under_samples) {
			assert(under_samples == (uint8_t) 0);
			pthread_mutex_lock(&alarm_lock);
			assert(alarm_active == (uint8_t) 0);
			alarm_active = check_fire(smoothed_temperatures);
			pthread_cond_broadcast(&alarm_cond);
			pthread_mutex_unlock(&alarm_lock);
		} 
		else {
			assert(under_samples != (uint8_t) 0);
			under_samples--;
		}

		usleep(2000*TIMESCALE);
	}
	return NULL;
}

uint8_t check_fire(uint16_t smoothed_temperatures[SMOOTHED_SAMPLES]) {
	uint8_t hightemps = 0;
	uint8_t return_val = 0;
	for(uint8_t i = 0; i < (uint8_t) SMOOTHED_SAMPLES; i++) {
		if (smoothed_temperatures[i] >= (uint16_t) FIRE_THRESHOLD) {
			hightemps++;
		}
		else {
			continue;
		}
	}

	if (hightemps >= (uint8_t) (SMOOTHED_SAMPLES * 0.9) || (smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) >= (uint16_t) 8) {
		if(hightemps >= (uint8_t) (SMOOTHED_SAMPLES * 0.9)) {
			return_val = (uint8_t) 1;
		}
		else if((smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) >= (uint16_t) 8) {
			return_val = (uint8_t) 2;
		}
		else {
			assert(hightemps < (uint8_t) (SMOOTHED_SAMPLES * 0.9) || (smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) < (uint16_t) 8);
		}
	}
	else {
		assert(hightemps < (uint8_t) (SMOOTHED_SAMPLES * 0.9) || (smoothed_temperatures[0] - smoothed_temperatures[SMOOTHED_SAMPLES - 1]) < (uint16_t) 8);
	}
	return return_val;
}

uint16_t median_temperature(volatile uint16_t temperatures[MEDIAN_SAMPLES])
{
	uint16_t median = 0;
	for(uint8_t i = 0; i < (uint8_t) MEDIAN_SAMPLES; i++) {
		assert(temperatures[i] <= (uint16_t) MAX_TEMP);
		median += temperatures[i];
	}
	if(median > (uint16_t) 0) {
		median = median / (uint16_t) MEDIAN_SAMPLES;
	}
	else {
		assert(!median);
	}
	return median;
}