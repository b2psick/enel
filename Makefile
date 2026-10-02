all:
	gcc -Wall -Wextra -Wpedantic -std=c17 src/server.c -o server
	gcc -Wall -Wextra -Wpedantic -std=c17 src/client.c -o client

clean:
	rm -f server client