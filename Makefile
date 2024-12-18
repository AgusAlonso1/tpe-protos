include ./Makefile.inc

SOURCES_SERVER=$(wildcard src/*.c src/lib/*.c src/server/*.c src/manager/server/*.c src/pop3/*.c src/mail_manager/*.c ) 

MANAGER_CLIENT_SOURCES=$(wildcard src/manager/client/*.c src/lib/args.c)

SERVER_OUTPUT_FILE=./serverx

MANAGER_CLIENT_OUTPUT_FILE=./mng

all:
	$(CC) $(CFLAGS) $(SOURCES_SERVER) -I./src/include -o $(SERVER_OUTPUT_FILE)
	$(CC) $(CFLAGS) $(MANAGER_CLIENT_SOURCES) -I./src/include -o $(MANAGER_CLIENT_OUTPUT_FILE) 	

clean:
	rm -rf $(SERVER_OUTPUT_FILE) $(MANAGER_CLIENT_OUTPUT_FILE)

