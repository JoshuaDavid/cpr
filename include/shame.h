#ifndef SHAME_H
#define SHAME_H
// This is the header file for the things I'm not yet sure where to put.
// Also, all of the includes.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

#define VERIFY_NONEMPTY(arr) if(arr[0] == NULL) {\
    fprintf(stderr, "Array is empty\n");\
    exit(EXIT_FAILURE);\
}

int file_exists(char *fname) {
    struct stat st;
    return 0 == stat(fname, &st);
}
#endif
