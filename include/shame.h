#ifndef SHAME_H
#define SHAME_H

#include <assert.h>
#include <math.h>

int file_exists(char *fname);
char *basenameof(char *path);
char *dirnameof(char *path);
int mkdirp(char *dname);
int is_base(char b);

#endif
