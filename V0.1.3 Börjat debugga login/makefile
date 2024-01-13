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
db:
	clear
	$(CC) $(CFLAGS) $(DBFILES) -o DataBase/db.bin -lsqlite3 -lsodium
rundb:
	clear
	chmod 777 DataBase/db.bin
	./DataBase/db.bin
deldb:
	rm -f DataBase/userdata.db
hash:
	clear
	$(CC) $(CFLAGS) DataBase/hashing.c -o DataBase/hash.bin -lsodium
runhash:
	clear
	./DataBase/hash.bin
clean:
	clear
	rm -f $(RUNFILE)
	$(CC) $(CFLAGS) $(SERVERFILE) $(HEADER) $(DBFILES:c.=.o) $(LIBRARIES) -o $(RUNFILE) 