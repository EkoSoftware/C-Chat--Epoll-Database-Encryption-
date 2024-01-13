/*		To Do
X	1. Sql Database
X	2. Hash Passwords
X	3. Login Authorization
X	4. Register Accounts
	5. Login debugging
		- Bugs found so far.
		> Database corruption !??
		> When pass invalid wont get back to main screen
		> Couldnt fetch data when username or password doesnt exist
	6. Register debugging
		- Bugs found so far.
	7. Admin Commands
	8. User Commands
*/
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
#include <sodium.h>
#include <sqlite3.h>
#include "epollserver.h"
#include "DataBase/database.h"



// Global Variables
serverData *Server;
//char buffer[BUFFER_SIZE];



	
int main() {
	system("clear");
	printf("Client Struct size: %zu\n",sizeof(clientData));
	printf("SA_IN Struct Size: %zu \n",sizeof(SA_IN));
	
	
	
	// Signal Handler
	signal(SIGPIPE, sigpipe_handler);
	signal(SIGINT, ExitGracefully);
	
	// Setup Server
	Server = malloc(sizeof(serverData));
	Server->Clients = malloc(sizeof(clientData *) * Server->size);
	SetupServer(Server);
    ASSERT(Server->fd == 3,"Assertion!");
	
	// Initialize Database
	CreateDataBase(Server);
	
	// Buffer vars
	char buffer[BUFFER_SIZE];
	char msg[1024];
	

    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error creating epoll instance");
        ExitGracefully(EXIT_FAILURE);
    } else {
		Server->epoll_fd = epoll_fd;
	}

	// Create Epoll Set
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = Server->fd;
	
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, Server->fd, &event) == -1) {
        perror("Error adding server socket to epoll set");
		ExitGracefully(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    while (1)
	{	
		signal(SIGPIPE, sigpipe_handler);
		// Check if Client List needs more memory
		if (Server->used == Server->size) 
			ServerClients_realloc(Server);

        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (num_events < 0) { perror("Epoll"); }

        for (int i = 0; i < num_events; i++) 
		{
			int Check = WhichClient(4);
			if (Check)
				fprintf(stderr, "Client socket %d belongs to: %s\n",4 , Server->Clients[Check]->Username);
			// Current event
			switch(events[i].data.fd) 
			{
				// Server Socket
				case ((int) 3 ): 
				{

					// New connection
					clientData *C;
					C = (clientData*) malloc(sizeof(clientData));
					C->db = Server->db;
					int try;
					if (C == NULL) {
						perror("Client Malloc Error"); 
						continue; }
					
					if ((try = AcceptConnection(C)) == FAILURE)
						continue;
					
					try = AuthenticateUser(C);
					switch (try) {
						case (USER_VALID):
							if((AddClient(C, &event)) == SUCCESS) 
							puts("Adding Client to Broadcast list");
							continue;
						case (USER_REGISTERED):
							Login(C);
							if((AddClient(C, &event)) == SUCCESS) 
							puts("Adding Client to Broadcast list");
							continue;
						case (USER_INVALID):
							puts("Couldnt Validate User");
							continue;
						case (USER_QUIT):
							puts("User Closed connection");
							continue;
						default:
							break;
					}
				{ break; }
				}					

					
					
					

				// Handle data from clients
				default: 
				{
					int clientIndex = WhichClient(events[i].data.fd);
					clientData *C =  Server->Clients[clientIndex];
                	

					ssize_t nbytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
					if (nbytes <= 0) { 
						RemoveClient(events[i].data.fd);
						continue;
					} 

					
					size_t len = MAX_USERNAME + strlen(buffer);
					snprintf(msg, len, "%s: %s",C->Username, buffer);


					size_t sendbytes = strlen(msg);
					for (int j = 4; j < Server->fd_max ;j++) {
						Write(events[i].data.fd, msg, &sendbytes);
					}


					// Debug Messages
					if (buffer[0] > 0) {
						PrintDebug(msg, sendbytes);
					}
					memset(buffer, 0, nbytes);

				{ continue; }
				}
            }
        }
    }
	return EXIT_FAILURE; 
}


/*	Gets Client index in Server List
*/
int WhichClient(int eventID) {
	for (int i = 0; i < Server->used;i++ ) {
		if (Server->Clients[i]->clientFD == eventID) {
			return i;
		}
	}
	return 0;
}



void RemoveClient(int eventID) {
	char tempUsername[MAX_USERNAME];
	for (int i = 0; i < Server->used;i++ ) {
		if (Server->Clients[i]->clientFD == eventID) 
		{
			clientData *C = Server->Clients[i];
			strcpy(tempUsername, C->Username);
			if (C->clientFD == Server->fd_max) 
				Server->fd_max -= 1;
			if (C->clientFD == Server->fd_min) 
				Server->fd_min -= 1;

			
			
			epoll_ctl(Server->epoll_fd, EPOLL_CTL_DEL, eventID, NULL);
			close(eventID);
			free(Server->Clients[i]);
			Server->used -= 1;
			Server->size -= 1;
			
			fprintf(stderr, "%-74cClient Removed: %s:%-5d [FD%3d]\n",' ', C->IP, C->Port, eventID);
			fprintf(stderr, "%-74cEvent id: %d\n", ' ', eventID);
			fprintf(stderr, "%-74cSrvIndex: %d\n", ' ', i);
			fprintf(stderr, "%-74cUsername: %s\n", ' ', C->Username);
		}
	}
}




int AddClient(clientData *C, struct epoll_event *event) {
	if (Server->used == 0) 
		Server->fd_min = C->clientFD;

	if( C->clientFD > Server->fd_max ) 
		Server->fd_max = C->clientFD;
	
	if (C->clientFD < Server->fd_min)
		Server->fd_min = C->clientFD;

	if (C->clientFD == 5 && Server->fd_min < 5)
		Server->fd_min = 5;

	if (Server->fd_min > Server->fd_max)
		Server->fd_max = Server->fd_max;
	
	Server->Clients[Server->used] = C;
	Server->used += 1;

	// Add the new client socket to the epoll set
	event->events = EPOLLIN;
	event->data.fd = C->clientFD;
	if (epoll_ctl(Server->epoll_fd, EPOLL_CTL_ADD, C->clientFD, event) == -1) {
			close(C->clientFD);
			puts("Error adding client socket to epoll set\n");
			return FAILURE;
	}
	return SUCCESS;
}




void PrintDebug(char *msg, size_t nbytes) {
	static int count = 12;

	/* [Broadcasting] " Message " */
	write(STDERR_FILENO, "[Broadcasting] ", 16);
	write(STDERR_FILENO, msg, nbytes);
	write(STDERR_FILENO, "\n",1);
	
	/* Advertises resource use occassionally*/
	if (count == 12) {
		fprintf(stderr, "%73cConnected Clients :[%d] Sockets\n", ' ', Server->used);
		fprintf(stderr, "%73cClientMemoryUsed  :[%zu Bytes]\n",  ' ', sizeof(clientData) * Server->used);
		fprintf(stderr, "%73cMax FileDescriptor:[%d] \n",        ' ', Server->fd_max);
		count = 0;
	}
	
	count += 1;
}




/*	Creates Server Socket
*/
void SetupServer(serverData *Server) {
	// Create Server File descriptor
	check(Server->fd = socket(AF_INET, SOCK_STREAM, 0),
		"Failed to create Socket");
	
	int reuse = 1;
	if (setsockopt(Server->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    perror("Setsockopt failed");
    ExitGracefully(EXIT_FAILURE);
	}
	
	Server->fd_max = 4; // Because im testing on the same computer
	Server->fd_min = 4; // Because im testing on the same computer
	Server->used = 0;
	Server->size = 2;

	// Init Server Connection Details
	memset(&Server->Addr, 0, sizeof(Server->Addr));
    Server->Addr.sin_family = AF_INET;
    Server->Addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "192.168.1.20", &Server->Addr.sin_addr);
	
	// Try to Bind Server fd
    check(bind(Server->fd, (SA*)&Server->Addr, sizeof(Server->Addr)),
        "Socket bind failed: ");

	// Try opening Server fd to listening 
	check(listen(Server->fd, BACKLOG),
		"Listening Error:");

	// Intro Message If Successfull Start
	fprintf(stderr, "Server Started!\n");
	fprintf(stderr, "IP-Address:%s\n", 		SERVER_IP);
	fprintf(stderr, "Port:%d\n", 			PORT);
	fprintf(stderr, "ServerSocket: %d\n",	Server->fd);
	fprintf(stderr, "Waiting for connections...\n");
}




/* Safe realloc
*/
void ServerClients_realloc(serverData *Server) {
	fprintf(stderr, "%-72c Reallocating Server Memory.\n",' ');
	fprintf(stderr, "%-72c Current Memory used: %zu bytes\n",' ',
		sizeof(clientData) * Server->used);

	Server->size *= 2;
	clientData **temp;
	temp = realloc(Server->Clients, sizeof(clientData) * Server->size);
	if (temp == NULL) {
		fprintf(stderr, "Client List Realloc Failed:");
		ExitGracefully(EXIT_FAILURE);
	} else {
		Server->Clients = temp;
	}
	
	fprintf(stderr, "%-72c Client List Realloc Success\n",' ');
	fprintf(stderr, "%-72c New Memory Available %zu bytes\n",' ', 
		sizeof(clientData) * Server->size);
}

void check(int exp, char *msg) {
	if (exp < 0) {
		perror(msg);
		ExitGracefully(EXIT_FAILURE);
	}
}




/*	Accepts connection as s File Descriptor
	Saves IP and Port info to Client struct;
	returns SUCCESS 1
	returns FAILURE -1
*/
int AcceptConnection(clientData *C) {
	// Attempt to accept connection 
	socklen_t addr_size = sizeof(SA_IN);
	if ((C->clientFD = accept(
				Server->fd,
				(SA*)&C->Addr,
				(socklen_t*)&addr_size))
				 < 0) {
		fprintf(stderr,"Accepting client failed.");
		return FAILURE;
	}
	// Save Address to Client Struct
	getpeername(C->clientFD, (SA*)&C->Addr, &addr_size);
	inet_ntop(AF_INET, &C->Addr.sin_addr, C->IP, INET_ADDRSTRLEN);
	C->Port = C->Addr.sin_port;	


	// Notify Connection
	fprintf(stdout,"%-74cNew Connection: %s:%d[FD:%d]\n",' ',C->IP, C->Port, C->clientFD);

	return SUCCESS;
}





/*	This Exists because the write function 
	may or may not write every byte from the buffer.
*/
int Write(int s, char *buf, size_t *len) {
	size_t total = 0;
	size_t bytesleft = *len;
	int n;
	while(total < *len) {
		n = write(s, buf+total, bytesleft);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return ( n == -1 ) 
				? -1 : 0;
}




/* Frees Memory if there is any
*/
void FreeAll(void) {
	if (Server->used > 0) {

		fprintf(stderr,   "%-71c Freeing allocated Memory!\n",' ');

		for (int i = 0; i < Server->used;i++) {
			close(Server->Clients[i]->clientFD);
			free(Server->Clients[i]);
			Server->Clients[i] = NULL;
		}
		fprintf(stderr,   "%-73c Client memory Freed\n",' ');
	}

		free(Server->Clients);
		free(Server);
		fprintf(stderr,   "%-73c Server memory Freed\n",' ');

}




// Shouldnt do anything
void sigpipe_handler(){}




// Exits Gracefully when Ctrl+c or Signicant errors occur.
void ExitGracefully() {
	fprintf(stderr, "\n%-73c ! Exiting Program !\n",' ');
	
	// Cleanup
	FreeAll();
	close(Server->epoll_fd);
    close(Server->fd);

	exit(EXIT_SUCCESS);
}