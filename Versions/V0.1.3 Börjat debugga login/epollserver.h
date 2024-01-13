#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include <sqlite3.h>
#include <sodium.h>
#include "DataBase/database.h"

// Constants
#define MAX_USERNAME (size_t)24
#define MAX_PASSWORD crypto_pwhash_STRBYTES
#define MAX_EVENTS 128
#define SERVER_IP "192.168.1.20"
#define PORT 7890
#define BACKLOG 128
#define BUFFER_SIZE 1024
#define SUCCESS 1
#define FAILURE -1

#define USER_REGISTERED INT_MAX
#define USER_VALID 1
#define USER_INVALID 0
#define USER_QUIT INT_MIN
#define USERNAME_EXISTS 1
#define USERNAME_NOT_EXISTS 0

#define ASSERT(c, m) \
			do { if (!(c)) { fprintf(stderr, __FILE__ ":%d: Assertion ' %s ' Failed: %s\n", __LINE__, #c, m); \
				FreeAll();exit(1);}\
				} while(0)

// Types And Structs
typedef struct sockaddr SA;
typedef struct sockaddr_in SA_IN;
struct clientData;
struct serverData;

typedef struct HashStorage {
	unsigned char Username[MAX_USERNAME];
	unsigned char Out[crypto_pwhash_STRBYTES];
	const char Password[MAX_PASSWORD];
	char Created[20];
	unsigned long long opslimit;
	size_t memlimit;
	sqlite3 *db;
} HashStorage;

typedef struct serverData {
	int fd;
	int fd_min;
	int fd_max;
	int used;
	int size;
	int epoll_fd;
	SA_IN Addr;
	struct clientData ** Clients;
	sqlite3 * db;
} serverData;

typedef struct clientData {
	char Username[MAX_USERNAME];
	const char Password[MAX_PASSWORD];
	int Port;
	int clientFD;
	int eventID;
	char IP[INET_ADDRSTRLEN];
	struct sockaddr_in Addr;
	HashStorage * Account;
	sqlite3 * db;
} clientData;




// Prototypes
int WhichClient(int eventID);
void RemoveNull(char *string);
void RemoveClient(int eventID);
int AddClient(clientData *C, struct epoll_event *event);
void PrintDebug(char *msg, size_t nbytes);
void ServerClients_realloc(serverData *Server);
void SetupServer(serverData *Server);
void check(int exp, char *msg);
int AcceptConnection(clientData *C);
int Write(int s, char *buffer, size_t *len);
void FreeAll();
void sigpipe_handler();
void ExitGracefully();

// Database
int AuthenticateUser(clientData *C);
int GetUsername(clientData *C);
int GetPassword(clientData *C);
int Login(clientData *C);
int Register(clientData *C);
int UsernameExists(HashStorage *A);
void HashPassword(HashStorage *A);
void CheckSQL(int exp, char *fail_msg, char *errMsg, sqlite3 *db, char *success_msg);
int CreateDataBase(serverData *S);
int AddAccount(HashStorage *A);
int CheckLogin(HashStorage *A);

#endif // EPOLLSERVER_H