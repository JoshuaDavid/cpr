#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#define HASH struct hash

HASH {
    uintmax_t capacity;
    uintmax_t count;
    uintmax_t *values;
};

HASH *hash_create (uintmax_t size);
void hash_destroy(HASH *h);

void hash_insert (HASH *h, uintmax_t value);
int  hash_lookup (HASH *h, uintmax_t value);

void *shared_calloc(size_t nmemb, size_t size);
void shared_free_size(void *mem, size_t nmemb, size_t size);

#endif
