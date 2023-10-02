build: clean packet-lib build-client

packet-lib: ../lib/tw_packet.c
	gcc -c ../lib/tw_packet.c -o ../lib/tw_packet.o

build-client: client.c
	gcc client.c ../lib/tw_packet.o -o client -lreadline -Werror -Wextra -Wall

clean:
	rm -rf client server
