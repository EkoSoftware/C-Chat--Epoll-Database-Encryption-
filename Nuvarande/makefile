# Compiler and Flags
CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c11
RUNFILE = epollserver.bin
SERVERFILE = epollserver.c
HEADER = epollserver.h 
LIBRARIES = -lsqlite3 -lsodium
DBFILES = DataBase/database.c DataBase/hashing.c

all: 

server: $(RUNFILE)
	$(CC) $(CFLAGS) $(SERVERFILE) $(HEADER) $(DBFILES:c.=.o) $(LIBRARIES) -o $(RUNFILE) 

test:
	$(CC) test.c $(DBFILES) $(LIBRARIES) -o test.bin
runtest:
	clear
	./test.bin
run:
	./$(RUNFILE)
input:
	clear
	python "./ClientTesting/manualinput.py"
one:
	clear
	python "./ClientTesting/oneclient.py"
many:
	clear
	python "./ClientTesting/manyclients.py"
deldb:
	rm -f DataBase/userdata.db
clean:
	clear
	rm -f $(RUNFILE)
	$(CC) $(CFLAGS) $(SERVERFILE) $(HEADER) $(DBFILES:c.=.o) $(LIBRARIES) -o $(RUNFILE) 
