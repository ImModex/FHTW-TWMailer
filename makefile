build: clean queue-lib packet-lib utility-lib build-client build-server

debug: clean queue-lib packet-lib utility-lib
	gcc client/client.c lib/tw_packet.o lib/tw_utility.o lib/queue.o -o client/client -lreadline -Wextra -Wall -g
	gcc server/server.c lib/tw_packet.o lib/tw_utility.o lib/queue.o -o server/server -lreadline -Wextra -Wall -g

queue-lib: lib/queue.c
	gcc -c lib/queue.c -o lib/queue.o

packet-lib: lib/tw_packet.c
	gcc -c lib/tw_packet.c -o lib/tw_packet.o

utility-lib: lib/tw_utility.c
	gcc -c lib/tw_utility.c -o lib/tw_utility.o

build-client: client/client.c
	gcc client/client.c lib/tw_packet.o lib/tw_utility.o lib/queue.o -o client/client -lreadline -Werror -Wextra -Wall

build-server: server/server.c
	gcc server/server.c lib/tw_packet.o lib/tw_utility.o lib/queue.o -o server/server -lreadline -Werror -Wextra -Wall

clean:
	rm -rf server/server client/client
