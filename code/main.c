#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libvirt/libvirt.h>			// INCLUDING THE LIBVIRT LIBRARY
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "definitions.h"


/* -MAIN- */
int main (int argc, char *argv[]) {
	char input [100];
	int inputNum;
	while (1) {
		printf ("VManager # ");
		scanf("%s", input);
		if (!strcmp (input, "quit")) 
			return 0;
		inputNum = assignNum (input);
		handleInput (inputNum);
	}
}

