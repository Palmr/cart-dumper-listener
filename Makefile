listener: listener.c
	mkdir -p ./bin
	gcc -pedantic -Os -o ./bin/gb-listener listener.c -lwiringPi -lncurses -std=c99

listener-int: listener.c
	mkdir -p ./bin
	gcc -pedantic -Os -o ./bin/gb-listener-int listener-interrupts.c -lwiringPi -lncurses -std=c99

clean:
	rm -r ./bin
