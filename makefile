all: manager.c manager.h simulator.c simulator.h common.h
	gcc -o manager manager.c
	gcc -o simulator simulator.c

manager: manager.c manager.h common.h
	gcc -o manager manager.c

simulator: simulator.c simulator.h common.h
	gcc -o simulator simulator.c
