CFLAGS=-g -lsctp


all: client server

client: sctp_client.o nplib/np_lib.o nplib/error_functions.o nplib/np_lib.h nplib/np_header.h
	gcc -o client ${CFLAGS} sctp_client.o  nplib/np_lib.o nplib/error_functions.o

sctp_client.o: sctp_client.c
	gcc -c ${CFLAGS} sctp_client.c

server: sctp_server.o nplib/np_lib.o nplib/error_functions.o nplib/np_lib.h nplib/np_header.h
	gcc -o server ${CFLAGS} sctp_server.o  nplib/np_lib.o nplib/error_functions.o

sctp_server.o: sctp_server.c
	gcc -c ${CFLAGS} sctp_server.c

clean:
	rm *.o client server
