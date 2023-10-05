build: clean packet-lib build-client build-server

debug: clean packet-lib
	gcc client/client.c lib/tw_packet.o -o client/client -lreadline -Werror -Wextra -Wall -g
	gcc server/server.c lib/tw_packet.o -o server/server -lreadline -Werror -Wextra -Wall -g

packet-lib: lib/tw_packet.c
	gcc -c lib/tw_packet.c -o lib/tw_packet.o

build-client: client/client.c
	gcc client/client.c lib/tw_packet.o -o client/client -lreadline -Werror -Wextra -Wall

build-server: server/server.c
	gcc server/server.c lib/tw_packet.o -o server/server -lreadline -Werror -Wextra -Wall

clean:
	rm -rf server/server client/client
