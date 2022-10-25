all: src/manager.c src/manager.h src/simulator.c src/simulator.h src/firealarm.c src/firealarm.h src/launcher.c src/common.h 
	gcc -Wall -Wextra -Werror -lrt -pthread -o  build/manager src/manager.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/simulator src/simulator.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/firealarm src/firealarm.c -s
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/launcher src/launcher.c -s

debug: src/manager.c src/manager.h src/simulator.c src/simulator.h src/firealarm.c src/firealarm.h src/launcher.c src/common.h
	gcc -lrt -pthread -o build/manager src/manager.c -g
	gcc -lrt -pthread -o build/simulator src/simulator.c -g
	gcc -lrt -pthread -o build/firealarm src/firealarm.c -g
	gcc -lrt -pthread -o build/launcher src/launcher.c -g

manager: src/manager.c src/manager.h src/common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -o  build/manager src/manager.c -s

simulator: src/simulator.c src/simulator.h src/common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/simulator src/simulator.c -s

firealarm: src/firealarm.c src/firealarm.h src/common.h
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/firealarm src/firealarm.c -s

launcher: src/launcher.c
	gcc -Wall -Wextra -Werror -lrt -pthread -o build/launcher src/launcher.c -s