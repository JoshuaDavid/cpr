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
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>
#include "debug.h"

#define PI 3.14159265358979

#define VERIFY_NONEMPTY(arr) if(arr[0] == NULL) {\
    fprintf(stderr, "Array is empty\n");\
    exit(EXIT_FAILURE);\
}

int file_exists(char *fname) {
    struct stat st;
    return 0 == stat(fname, &st);
}

// Because I prefer working with basename / dirname functions that do not
// modify my original strings.
//
// Yes, I know there's a memory leak. I'll fix it eventually, but the amount
// leaked is only a few hundred bytes so it's not a huge priority.
char *basenameof(char *path) {
    char *p = calloc(strlen(path) + 1, sizeof(char));
    strcpy(p, path);
    char *tmp = basename(p);
    char *bname = calloc(strlen(tmp) + 1, sizeof(char));
    strcpy(bname, tmp);
    free(p);
    return bname;
}

char *dirnameof(char *path) {
    char *p = calloc(strlen(path) + 1, sizeof(char));
    strcpy(p, path);
    char *tmp = dirname(p);
    char *dname = calloc(strlen(tmp) + 1, sizeof(char));
    strcpy(dname, tmp);
    free(p);
    return dname;
}

int mkdirp(char *dname) {
    struct stat st;
    int err = stat(dname, &st);
    if(err == -1) {
        if(errno == ENOENT) {
            if(DEBUG_LEVEL >= 2) {
                printf("Directory %s does not exist, creating it.\n", dname);
            }
            char *ddname = dirnameof(dname);
            mkdirp(ddname);
            free(ddname);
            err = mkdir(dname, 0777);
            if(err == -1) {
                printf("%s Exists\n", dname);
                perror("mkdir");
                return 0;
            } else {
                return 0;
            }
        } else {
            perror("stat");
            return -1;
        }
    } else if(err == 0) {
        if(S_ISDIR(st.st_mode)) {
            if(DEBUG_LEVEL >= 3) {
                printf("Directory %s exists. Do nothing.\n", dname);
            }
            return 0;
        } else {
            if(DEBUG_LEVEL >= 3) {
                printf("%s exists, but is not a directory\n", dname);
            }
            return -1;
        }
    } else {
        perror("stat:");
        exit(EXIT_FAILURE);
    }
}

#endif
