#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sodium.h>
#include "../epollserver.h"


void HashPassword(HashStorage *A) {
	if (crypto_pwhash_str(
			(char *)A->Out,
			A->Password, strlen(A->Password),
			A->opslimit, A->memlimit) < 0) {
		fprintf(stderr, "Couldnt Hash!\n");
		// free(A);
		return;
	}
	//printf("Before:%s\nAfter:%s\n", A->Password, A->Out);
}
