listener: listener.c
	mkdir -p ./bin
	gcc -pedantic -Os -o ./bin/gb-listener listener.c -lwiringPi -lncurses -std=c99

clean:
	rm -r ./bin
