include ./Makefile.inc

SOURCES=$(wildcard src/*.c)

OUTPUT_FILE=./server

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT_FILE) 	

clean:
	rm -rf $(OUTPUT_FILE)

