all: manager.c manager.h simulator.c simulator.h common.h
	gcc -o manager manager.c -s
	gcc -o simulator simulator.c -s

debug: manager.c manager.h simulator.c simulator.h common.h
	gcc -o manager manager.c -g
	gcc -o simulator simulator.c -g

manager: manager.c manager.h common.h
	gcc -o manager manager.c

simulator: simulator.c simulator.h common.h
	gcc -o simulator simulator.c

testing: testing.c
	gcc -o testing testing.c