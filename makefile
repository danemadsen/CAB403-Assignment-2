all: manager.c manager.h simulator.c simulator.h firealarm.c firealarm.h launcher.c common.h 
	gcc -Wall -Wextra -Werror -o  manager manager.c -s
	gcc -Wall -Wextra -Werror -o simulator simulator.c -s
	gcc -Wall -Wextra -Werror -o firealarm firealarm.c -s
	gcc -Wall -Wextra -Werror -o launcher launcher.c -s

debug: manager.c manager.h simulator.c simulator.h firealarm.c firealarm.h launcher.c common.h 
	gcc -o manager manager.c -g
	gcc -o simulator simulator.c -g
	gcc -o firealarm firealarm.c -g
	gcc -o launcher launcher.c -g

manager: manager.c manager.h common.h
	gcc -o manager manager.c

simulator: simulator.c simulator.h common.h
	gcc -o simulator simulator.c

testing: testing.c
	gcc -o testing testing.c