#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "harness.h"

int main(int argc, char ** argv) {
	/* this will setup the signal handler to take care of seg fault */
    init_infiniti();

	infiniti_dump();
    return 0;
}

/* vim: set ts=4: */
