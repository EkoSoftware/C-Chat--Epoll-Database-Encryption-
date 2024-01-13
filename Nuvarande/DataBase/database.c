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
	// LOGIN
	case ('1'):
		while(1) {
			fprintf(stderr, "Client login in\n");
			try = Login(C);
			if (try == USER_INVALID ) {
				puts("Client Couldnt Validate Details");
				close(C->clientFD);
				free(C);
				return USER_INVALID;
			}
			if (try == USER_QUIT) {
				close(C->clientFD);
				free(C);
				return USER_QUIT;
			}
			if (try == USER_VALID) {
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
	// Registration
	case ('2'):
			fprintf(stderr, "Client Registrating\n");
			int clientregistered = Register(C);
			if (clientregistered == USER_REGISTERED) {
				fprintf(stderr,"%s has Registered Succesfully\n", C->Username);
				write(C->clientFD, "Registered Succesfully! Please Login\n", 38);
				while(1){
					int login = Login(C);
					if (login == USER_VALID) {
						return USER_REGISTERED;
					}
					}
			}
			else {
				puts("Error at login after registration");
				return USER_QUIT;
			}
		break;
	// Quit
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
	Write(C->clientFD, "Enter Username", &len);
	//write(C->clientFD, "Enter Username", len);
	
	// Store Input for Authentication
	size_t nbytes = read(C->clientFD, username, MAX_USERNAME);
	if (nbytes <= 0) {
			fprintf(stderr, "GetUsername Error: Didnt get client input from %s\n",C->IP);
			return USER_QUIT;
	}
	else
	if (strncmp(username, "quit", 4) == 0) {
		return USER_QUIT;
	}
	
	else
	len = strlen(username) + 1;
	username[len] = '\0';
	snprintf(C->Username, len, "%s", username);
	fprintf(stderr,"Client wrote %s\n", C->Username);
	return SUCCESS;
}




/* Gets Password input from Client
*/
int GetPassword(clientData *C) {
	char password[MAX_PASSWORD];
	size_t len = strlen("Enter Password") + 1;
	write(C->clientFD, "Enter Password", len);
	
	
	// Store Input for Authentication
	size_t nbytes = read(C->clientFD, password, MAX_PASSWORD);
	if (nbytes <= 0) {
			fprintf(stderr, "GetUsername Error: Didnt get client input from %s\n",C->IP);
			return FAILURE;
	}
	if (strncmp(password, "quit", 4) == 0) {
		return USER_QUIT;
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

		// Get Username And Password
		int try = GetUsername(C);
	{	if (try == FAILURE)   return FAILURE; }
	{	if (try == USER_QUIT) return USER_QUIT; }

		try = GetPassword(C);
	{	if (try == FAILURE)   return FAILURE; }
	{	if (try == USER_QUIT) return USER_QUIT; }
		
	
		char msg[64];
		snprintf(msg, 64, "Authorizing: %s", C->Username);
		size_t nbytes = strlen(msg) + 1;
		Write(C->clientFD, msg, &nbytes);
		//write(C->clientFD, msg, nbytes);
	
		HashStorage *Account = malloc(sizeof(HashStorage));
		strncpy((char *)Account->Username, C->Username, strlen(C->Username) + 1);
		strncpy((char *)Account->Password, C->Password, strlen(C->Password) + 1);
	
	/*	
	Try Authentication
	*/ 

		// If invalid
		try = CheckLogin(Account);
		if(try == USER_INVALID) {
			nbytes = strlen("Invalid Username or Password try again.\n") + 1;
			Write(C->clientFD, "Invalid Username or Password try again.\n", &nbytes);
			continue;
		}
		else
		if(try == USER_VALID) {
		// if Valid
		// Overwrite Users cached password with zero.
		memset((char * )C->Password, 0, MAX_PASSWORD);
		}
		return USER_VALID;
	}
}
	

/* 	Returns -1 for FAILURe
	Returns -INT MIN for USER_QUIT
	return  INT_MAX for USER_REGISTERED
*/
int Register(clientData *C) {	
	int try;
	C->Account = malloc(sizeof(HashStorage));
	
	while (1) {
	int try = GetUsername(C);
	{if (try == FAILURE )  return FAILURE; }
	{if (try == USER_QUIT) return USER_QUIT; }


		try = UsernameExists(C->Account);
		if (try == USER_QUIT) return USER_QUIT;
		if (try == USERNAME_EXISTS) {
			write(C->clientFD, "Username taken!", 16);
			continue;
		}
	{ break; }
	}

	try = GetPassword(C);
	{ if (try == FAILURE)   return FAILURE; }
	{ if (try == USER_QUIT) return USER_QUIT; }

	memcpy(C->Account->Username, (char *)C->Username, MAX_USERNAME);
	memcpy((char * )C->Account->Password, (char * )C->Password, MAX_PASSWORD);
	try = AddAccount(C->Account);
	if (try == USER_REGISTERED) {
		return USER_REGISTERED;
	}
	return FAILURE;
}


/*	Function for sqlite3_exec parameter 3.
*/
int FetchUsernameCallback(void *used, int __attribute__((unused)) count, char **data, char __attribute__((unused)) **columns) {
    HashStorage *Temp = (HashStorage *)used;
    // Store the retrieved hash in the structure
    strcpy((char *)Temp->Username, data[0]);

    return 0;
}
/*	Checks database for username 
*/
int UsernameExists(HashStorage *A) {
	sqlite3 *db;
	// Local Vars
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;

	// Make sure we can open database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", db,
		"");

	
	const char *sqlTemplate = "SELECT Username FROM Accounts WHERE Username  = '%q'";
	char *sql = sqlite3_mprintf(sqlTemplate, A->Username);
	
	HashStorage *Temp = malloc(sizeof(HashStorage));
	CheckSQL(rc = sqlite3_exec(db, sql, FetchUsernameCallback, Temp, &errMsg),
			"Couldnt Fetch data: %s\n", db,
			"");
	fprintf(stderr,"Username strings:\nA:%s\nTemp:%s\n",A->Username, Temp->Username);
	int is_equal = strcmp((const char*) A->Username,(const char *) Temp->Username);
	if (is_equal == 0){
			MyCloseSQL(db, errMsg);
			puts("Username in database");
			return USERNAME_EXISTS;
	}
	puts("Username not in database");
	MyCloseSQL(db, errMsg);
	return USERNAME_NOT_EXISTS;
}





/*	Function for sqlite3_exec parameter 3.
*/
int FetchPasswordHashCallback(void *used, int __attribute__((unused)) count, char **data, char __attribute__((unused)) **columns) {
    HashStorage *A = (HashStorage *)used;
    // Store the retrieved hash in the structure
    strncpy((char *)A->Out, data[0], strlen(data[0]));
  	A->Out[127] = '\0'; // Ensure null-termination

    return 0;
}



/*	Checks password against hash in storage.
	Returns 1 for Valid Password
	Returns 0 for Invalid Password
*/
int CheckLogin(HashStorage *A) {
	sqlite3 *db;
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;
	

	// Open database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Couldnt open Database", db,
		"");

	// Prepare and Execute Sqlite query
	const char *sqlTemplate = "SELECT Password FROM Accounts WHERE Username  = '%s'";
	char *sql = sqlite3_mprintf(sqlTemplate, A->Username);
	sqlite3_exec(db, sql, FetchPasswordHashCallback, A, &errMsg);
	
	
	// crypto_pwhash function actually returns 0 for valid and -1 for invalid
	int is_user_valid = crypto_pwhash_str_verify((const char *) A->Out, A->Password, strlen(A->Password));
	MyCloseSQL(db, errMsg);
	return (is_user_valid == 0) ? USER_VALID : USER_INVALID;

}




/* Adds Following Values to Database:
	Returns USER_REGISTERED (INT_MAX) for success
	Exit Failure else
*/
int AddAccount(HashStorage *A) {
	sqlite3 *db;
	// Local Vars
	const char *dbpath = "DataBase/userdata.db";
	char *errMsg = 0;
	int rc;
	
	// Check Database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", db,
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
	size_t len = strlen(sqlTemplate) + strlen((char *) A->Username) + strlen((char *) A->Out) + strlen(A->Created) + 1 + 1 + 1;
	char sql[len+1];
	snprintf(sql, len,
	"INSERT INTO Accounts (Username, Password, Created, Banned, 'Days Banned') VALUES ('%s', '%s', '%s', '%d', '%d');",
	 A->Username, A->Out, A->Created, 0, -1);


	CheckSQL(rc = sqlite3_exec(db, sql, 0, 0, &errMsg),
		"AddAccount Insert Error:", db,
		"");
	if (rc != SQLITE_OK) {
		return FAILURE;
	}

	// Cleaning Up
	fprintf(stderr,"%-74cDebug: \"%s\" was regístered succesfully!\n",' ', A->Username);
	fprintf(stderr,"%-74cAt: %s\n",' ',A->Created);
	MyCloseSQL(db, errMsg);

	return USER_REGISTERED;
}

int MyCloseSQL(sqlite3 *db, char * errMsg) {
	if(errMsg) { sqlite3_free(errMsg); }
	int rc;
	rc = sqlite3_close(db);
	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL database object closed with error :%d\n", rc);
	}
	return rc;
}


/*
Generic Test for SQL functions
Exits Gracefully if functions fail.
Prints Success message if functions succeed.
*/
void CheckSQL(int exp, char *fail_msg, sqlite3 *db, char *success_msg) {
	if (exp != SQLITE_OK) 
	{
		fprintf(stderr, "%s: %s\n", fail_msg, sqlite3_errmsg(db));
		
	}
	fprintf(stderr, success_msg);
}

/*
Returns 0 if Database files exist
Returns 1 if Database file was created
*/
int CreateDataBase() {
	const char *dbpath = "DataBase/userdata.db";
	sqlite3 *db;
	char *errMsg = 0;
	int rc;

	// Create Database
	CheckSQL(rc = sqlite3_open(dbpath, &db),
		"Can't open database: %s\n", db,
		"");

	// Create SQL table
	char *createsql =  "CREATE TABLE IF NOT EXISTS Accounts("
					   "Username 		TEXT PRIMARY KEY	NOT NULL,"
					   "Password 		TEXT NOT NULL,"
					   "Created 			TEXT NOT NULL,"
					   "Banned   		INTEGER NOT NULL,"
					  "'Days Banned' 	INTEGER NOT NULL);";

	CheckSQL(rc = sqlite3_exec(db, createsql, 0, 0, &errMsg),
		"SQL error: %s", db,
		"");
	
	// Close Database for this function
	MyCloseSQL(db, errMsg);


	// Create Admin Account
	HashStorage Admin = {.Username = "admin",
						 .Password = "adminpass",};
	if (UsernameExists(&Admin) == USERNAME_EXISTS) {
		fprintf(stderr, "Database Exists\n");
		return DATABASE_EXISTS;
	}

	if (AddAccount(&Admin) == USER_REGISTERED) {
		fprintf(stderr, "Database Created\n");
	}
		return DATABASE_CREATED;
}


