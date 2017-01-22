listener: listener.c
	mkdir -p ./bin
	gcc -o ./bin/gb-listener listener.c -lwiringPi -lncurses

listener-int: listener.c
	mkdir -p ./bin
	gcc -o ./bin/gb-listener-int listener-interrupts.c -lwiringPi -lncurses

clean:
	rm -r ./bin
