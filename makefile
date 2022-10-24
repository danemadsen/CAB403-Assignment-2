all: manager.c manager.h simulator.c simulator.h firealarm.c firealarm.h launcher.c common.h 
	gcc -Wall -Wextra -Werror -lrt -pthread -o  manager manager.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -o simulator simulator.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -Wpedantic -o firealarm firealarm.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -o launcher launcher.c -s

debug: manager.c manager.h simulator.c simulator.h firealarm.c firealarm.h launcher.c common.h 
	gcc -lrt -pthread -o manager manager.c -g
	gcc -lrt -pthread -o simulator simulator.c -g
	gcc -lrt -pthread -o firealarm firealarm.c -g
	gcc -lrt -pthread -o launcher launcher.c -g

manager: manager.c manager.h common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -o  manager manager.c -s

simulator: simulator.c simulator.h common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -o simulator simulator.c -s

firealarm: firealarm.c firealarm.h common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -Wpedantic -o firealarm firealarm.c -s

launcher: launcher.c
	gcc -Wall -Wextra -Werror -lrt -pthread -o launcher launcher.c -s