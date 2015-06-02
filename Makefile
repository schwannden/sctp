CFLAGS=-g -L/usr/lib/ -Wl,-rpath=/usr/local/lib 
CLIBS=-lsctp -lsnp 
ifeq ($(LOG_LEVEL),)
  CFLAGS += -D LOG_LEVEL=3
  $(info setting log level to 3)
else
  CFLAGS += -D LOG_LEVEL=$(LOG_LEVEL)
  $(info setting log level to $(LOG_LEVEL))
endif

all: client server

client: sctp_client.o
	gcc -o client ${CFLAGS} sctp_client.o ${CLIBS}

sctp_client.o: sctp_client.c
	gcc -c ${CFLAGS} sctp_client.c

server: sctp_server.o
	gcc -o server ${CFLAGS} sctp_server.o ${CLIBS}

sctp_server.o: sctp_server.c
	gcc -c ${CFLAGS} sctp_server.c

clean:
	rm *.o client server
