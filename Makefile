CFLAGS=-g -lsctp -lsnp


all: client server

client: sctp_client.o
	gcc -o client ${CFLAGS} sctp_client.o

sctp_client.o: sctp_client.c
	gcc -c ${CFLAGS} sctp_client.c

server: sctp_server.o
	gcc -o server ${CFLAGS} sctp_server.o

sctp_server.o: sctp_server.c
	gcc -c ${CFLAGS} sctp_server.c

clean:
	rm *.o client server
