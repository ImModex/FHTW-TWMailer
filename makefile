CC=gcc
CFLAGS=-Werror -Wextra -Wall
DEBUG_FLAGS=${CFLAGS} -g

SERVER_LIBS=-pthread -lldap -llber

rebuild: clean all
all: queue-lib packet-lib utility-lib ldap-lib pw-lib build-client build-server 

debug: clean queue-lib packet-lib utility-lib ldap-lib pw-lib
	${CC} client/client.c lib/tw_packet.o lib/tw_utility.o lib/queue.o lib/hide_pw.o -o client/client ${DEBUG_FLAGS}
	${CC} server/server.c lib/tw_packet.o lib/tw_utility.o lib/queue.o lib/myldap.o lib/hide_pw.o -o server/server ${DEBUG_FLAGS} ${SERVER_LIBS}

queue-lib: lib/queue.c
	${CC} -c lib/queue.c -o lib/queue.o

packet-lib: lib/tw_packet.c
	${CC} -c lib/tw_packet.c -o lib/tw_packet.o

utility-lib: lib/tw_utility.c
	${CC} -c lib/tw_utility.c -o lib/tw_utility.o

ldap-lib: lib/myldap.c
	${CC} -c lib/myldap.c -o lib/myldap.o -lldap -llber

pw-lib: lib/hide_pw.c
	${CC} -c lib/hide_pw.c -o lib/hide_pw.o 

build-client: client/client.c
	${CC} client/client.c lib/tw_packet.o lib/tw_utility.o lib/queue.o lib/hide_pw.o -o client/client ${CFLAGS}

build-server: server/server.c
	${CC} server/server.c lib/tw_packet.o lib/tw_utility.o lib/queue.o lib/myldap.o lib/hide_pw.o -o server/server ${CFLAGS} ${SERVER_LIBS}

clean:
	rm -rf server/server client/client lib/tw_packet.o lib/tw_utility.o lib/queue.o lib/myldap.o lib/hide_pw.o
