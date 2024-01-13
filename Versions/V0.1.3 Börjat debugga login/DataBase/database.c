#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <sqlite3.h>
#include <sodium.h>
#include <time.h>
#include "../epollserver.h"

/*	Authenticates username/password against database
	< Client Options >
		1. Login
		2. Register
		3. Exit
*/
int AuthenticateUser(clientData *C) {
	// Struct to check against database
	C->Account = malloc(sizeof(HashStorage));
	
	// Login or Register User
	char *options = "\n1. Login\n2. Register\n3. Exit";
	size_t optlen = strlen(options) + 1; // 30
	
	//Write(C->clientFD, options, &optlen);
	write(C->clientFD, options, optlen);
	char choice[2];
	read(C->clientFD, choice, 2);
	puts("Client wrote "); puts(choice);
	int try;
	switch (choice[0])
	{
	case ('1'):
		while(1) {
			fprintf(stderr, "Client login in\n");
			
			if ((try = Login(C)  == USER_INVALID)) {
				puts("Client Couldnt Validate Details");
				close(C->clientFD);
				free(C);
				return USER_INVALID;
			}
			else {
				puts("Client has Authenticated Login");
				size_t nbytes = strlen("Welcome ") + strlen(C->Username) + 1;
				char *msg = malloc(nbytes);
				snprintf(msg, nbytes, "Welcome %s", C->Username);
				Write(C->clientFD, msg, &nbytes);
				free(msg);
				return USER_VALID;
			}
		}
		break;
	case ('2'):
			fprintf(stderr, "Client Registrating\n");
			if ((try = Register(C)) == USER_REGISTERED) {
				puts("Client has Registered Succesfully");
				write(C->clientFD, "Registered Succesfully! Please Login", 37);
				return USER_REGISTERED;
			}
			else {
				
			}
		break;
	case ('3'):
			puts("Client Disconnected before Validation");
			close(C->clientFD);
			free(C);
			return USER_QUIT;
		break;
	default:
		break;
	}
	
	
	
	return 0;
}


/* 	Gets Username input from Client
	Returns  1 for Success
	Returns -1 for Failure
*/
int GetUsername(clientData *C) {
	// Prompt Username
	char username[MAX_USERNAME];
	size_t len = strlen("Enter Username") + 1;
	//Write(C->clientFD, "Enter Username", &len);
	write(C->clientFD, "Enter Username", len);
	
	// Store Input for Authentication
	size_t nbytes = recv(C->clientFD, username, MAX_USERNAME, 0);
	if (nbytes <= 0) {
			fprintf(stderr, "GetUsername Error: Didnt get client input from %s\n",C->IP);
			close(C->clientFD);
			free(C);
			return USERNAME_NOT_EXISTS;
	}

	len = strlen(username) + 1;
	username[len] = '\0';
	snprintf(C->Username, len, "%s", username);
	fprintf(stderr,"Client wrote %s\n", C->Username);
	return SUCCESS;
}




/* Gets Password input from Client
*/
int GetPassword(clientData *C) {
	// Prompt Password
	char password[MAX_PASSWORD];
	size_t len = strlen("Enter Password") + 1;
	//Write(C->clientFD, "Enter Password", &len);
	write(C->clientFD, "Enter Password", len);
	
	// Store Input for Authentication
	size_t nbytes = read(C->clientFD, password, MAX_PASSWORD);
	if (nbytes <= 0) {
			fprintf(stderr, "GetUsername Error: Didnt get client input from %s\n",C->IP);
			return FAILURE;
	}

	len = strlen(password) + 1;
	password[len] = '\0';
	snprintf((char * )C->Password, len, "%s", password);
	fprintf(stderr,"Client wrote %s\n", C->Password);

	return SUCCESS;
}





/*	Checks Username and Password against database
	Returns 1 for Succesful login
	Returns 0 for invalid login
*/
int Login(clientData *C) {
	while (1) {

		int try;
		// Send Authorization Message
		if ((try = GetUsername(C) == FAILURE))
				return FAILURE;
	
		if ((try = GetPassword(C) == FAILURE))
				return FAILURE;
		
	
		char msg[64];
		snprintf(msg, 64, "Authorizing: %s", C->Username);
		size_t nbytes = strlen(msg) + 1;
		//Write(C->clientFD, msg, &nbytes);
		write(C->clientFD, msg, nbytes);
	
		HashStorage *Account = malloc(sizeof(HashStorage));
		memcpy(Account->Username, C->Username, MAX_USERNAME);
		memcpy((char * )Account->Password, C->Password, MAX_PASSWORD);
	
		// Try Authentication

		// If invalid
		if((try = CheckLogin(Account)) == USER_INVALID) {
			nbytes = strlen("Invalid Username or Password try again.\n") + 1;
			//Write(C->clientFD, "Invalid Password", &nbytes);
			write(C->clientFD, "Invalid Username or Password try again.\n", nbytes);
			continue;
			//return USER_INVALID;
		}

		// if Valid
		// Overwrite Users cached password with zero.
		memset((char * )C->Password, 0, MAX_PASSWORD);
		return USER_VALID;
	}
}
	


int Register(clientData *C) {	
	int try;
	HashStorage A;
	C->Account = malloc(sizeof(HashStorage));
	C->Account = &A;
	C->Account->db = C->db;
	
	while (1) {
		if ((try = GetUsername(C) == FAILURE))
				return FAILURE;
		
		memcpy(C->Account->Username, (char *)C->Username, MAX_USERNAME);
		if ((try = UsernameExists(C->Account)) == USERNAME_EXISTS) {
				write(C->clientFD, "Username taken!", 16);
				continue;
		}
		{ break; }
	}
	
	if ((try = GetPassword(C) == FAILURE))
			return FAILURE;
	

	memcpy((char * )C->Account->Password, (char * )C->Password, MAX_PASSWORD);
	if ((try = AddAccount(C->Account)) == USER_REGISTERED) {
			C->Account->db = NULL;
			return USER_REGISTERED;
	}
	ExitGracefully(EXIT_FAILURE);
	return -1;
}


/*	Checks database for username 
*/
int UsernameExists(HashStorage *A) {
	sqlite3 *db = A->db;
	// Local Vars
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;

	// Make sure we can open database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", errMsg, db,
		"");

	
	const char *sqlTemplate = "SELECT Username FROM Accounts WHERE Username  = '%q'";
	char *sql = sqlite3_mprintf(sqlTemplate, A->Username);

	sqlite3_stmt *res;
	CheckSQL(rc = sqlite3_prepare_v2(db, sql, -1, &res, 0),
			"Couldnt Fetch data: %s\n", errMsg, db,
			"");
		
	
	const unsigned char *hash;
	if(sqlite3_step(res) == SQLITE_ROW) {
			hash = sqlite3_column_text(res, 0);
			fprintf(stderr,"Username in database: %s\n",hash);
			if (errMsg) {sqlite3_free(errMsg);}
			return USERNAME_EXISTS;
	} else {
			puts("Username not in database");
			if (errMsg) {sqlite3_free(errMsg);}
			return USERNAME_NOT_EXISTS;
	}

}
/*	Checks password against hash in storage.
	Returns 1 for Valid Password
	Returns 0 for Invalid Password
*/
int CheckLogin(HashStorage *A) {
	sqlite3 *db = A->db;
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;
	

	// Open database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Couldnt open Database", errMsg, db,
		"");

	// Prepare Sqlite Statement
	const char *sqlTemplate = "SELECT Password FROM Accounts WHERE Username  = '%q'";
	char *sql = sqlite3_mprintf(sqlTemplate, A->Username);

	sqlite3_stmt *res;
	CheckSQL(rc = sqlite3_prepare_v2(db, sql, -1, &res, 0),
		"Couldnt Fetch data: %s\n", errMsg, db,
		"");
	
	// Get Database info from Table
	const char *hash;
	if(sqlite3_step(res) == SQLITE_ROW) {
		hash = (const char *)sqlite3_column_text(res, 0);
		//printf("Hash: %s\n",hash);
	}


	// Sodium function which returns -1 for invalid password
	if(crypto_pwhash_str_verify(hash, A->Password, strlen(A->Password)) == -1) {
		puts("Pass Invalid");
		if (errMsg) {sqlite3_free(errMsg);}
		sqlite3_close(db);
		return USER_INVALID;
	} 
	else {
		puts("Pass Valid");
		sqlite3_close(db);
		if (errMsg) {sqlite3_free(errMsg);}
		return USER_VALID;
	}

}




/* Adds Following Values to Database:
	Username
	Password
	Created 	(Time of Creation)
	Banned  	(Ban status)
	Days Banned (Ban length)
	Returns USER_REGISTERED (INT_MAX) for success
	Exit Failure else
*/
int AddAccount(HashStorage *A) {
	sqlite3 *db = A->db;
	// Local Vars
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;
	
	// Check Database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", errMsg, db,
		"");
	
	// Prepare Hash Details
	A->opslimit = crypto_pwhash_OPSLIMIT_MIN;
	A->memlimit = crypto_pwhash_MEMLIMIT_MIN;
	HashPassword(A);
    
	// Convert current time to struct tm in UTC
	time_t current;
    time(&current);
    struct tm *date = gmtime(&current);
	strftime(A->Created, sizeof(A->Created), "%Y-%m-%d %H:%M:%S", date);

	// Insert Details
	const char *sqlTemplate = "INSERT INTO Accounts (Username, Password, Created, Banned, 'Days Banned') VALUES ('%s', '%s', '%s', '%d', '%d');";
	char *sql = sqlite3_mprintf(sqlTemplate, A->Username, A->Out, A->Created, 0, -1);
	//fprintf(stderr,"Debug: %s\n",sql);
	CheckSQL(rc = sqlite3_exec(db, sql, 0, 0, &errMsg),
		"SQL error:", errMsg, db,
		"");


	// Cleaning Up
	fprintf(stderr,"%-74cDebug: \"%s\" was regístered succesfully!\n",' ', A->Username);
	fprintf(stderr,"%-74cAt: %s",' ',A->Created);
	if (errMsg) {sqlite3_free(errMsg);}
	puts("Got to add acc cleanup");
	return USER_REGISTERED;
}
/*
Generic Test for SQL functions
Exits Gracefully if functions fail.
Prints Success message if functions succeed.
*/
void CheckSQL(int exp, char *fail_msg, char *errMsg, sqlite3 *db, char *success_msg) {
	if (exp != SQLITE_OK) 
	{
		fprintf(stderr, "%s: %s\n", fail_msg, sqlite3_errmsg(db));
		sqlite3_free(db);
		if(errMsg) {
			sqlite3_free(errMsg);
		}
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, success_msg);
}

/*
Returns 0 if Database files exist
Returns 1 if Database file was created
*/
int CreateDataBase(serverData *Server) {
	const char *dbpath = "DataBase/userdata.db";
	sqlite3 *db = Server->db;
	char *errMsg = 0;
	int rc;

	// Return if Database exists.
	FILE *exists = fopen(dbpath, "r");
	if(exists == NULL) {
		fprintf(stdout,"Creating Database File %s\n",dbpath);
	} else {
		fclose(exists);
		return 0;
	}
	
	// Create Database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", errMsg, db,
		"");

	// Create SQL table
	char *createsql =  "CREATE TABLE IF NOT EXISTS Accounts("
					   "Username 		TEXT PRIMARY KEY	NOT NULL,"
					   "Password 		TEXT NOT NULL,"
					   "Created 			TEXT NOT NULL,"
					   "Banned   		INTEGER NOT NULL,"
					  "'Days Banned' 	INTEGER NOT NULL);";

	CheckSQL(rc = sqlite3_exec(db, createsql, 0, 0, &errMsg),
		"SQL error: %s", errMsg, db,
		"");


	// Create Admin Account
	HashStorage Admin = {.Username = "admin",
						 .Password = "adminpass",};
	if (AddAccount(&Admin) == USER_REGISTERED)
		fprintf(stderr, "Database");

	if (errMsg) {sqlite3_free(errMsg);}
	sqlite3_close(db);
	return 1;
}


