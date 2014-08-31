#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#define HASH struct hash

// The 10-trillionth prime -- it seems unlikely that a hash will be over 320Tb
const uintmax_t prime_1e13 = 0x0001267a06388f9b;

HASH *hash_create (uintmax_t size);
void         hash_destroy(HASH *h);

void hash_insert (HASH *h, uintmax_t value);
int  hash_lookup (HASH *h, uintmax_t value);

HASH {
    uintmax_t capacity;
    uintmax_t count;
    uintmax_t *values;
};

void *shared_calloc(size_t nmemb, size_t size) {
    void *mem = mmap(NULL, nmemb * size, PROT_READ | PROT_WRITE, 
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(mem, 0, nmemb * size);
    return mem;
}

void shared_free_size(void *mem, size_t nmemb, size_t size) {
    if(mem) {
        munmap(mem, nmemb * size);
    }
}

HASH *hash_create(uintmax_t size) {
    HASH *h = shared_calloc(1, sizeof(HASH));
    // Handle attempts to allocate extremely small hashes
    if(size < 1024) {
        size = 1024;
    }
    h->capacity = size * 3;
    h->count = 0;
    h->values = shared_calloc(h->capacity, sizeof(uintmax_t));
    if(!h->values) {
        fprintf(stderr, "%s", "Cannot create hash: Out of memory\n");
        exit(ENOMEM);
    }
    return h;
}

void hash_destroy(HASH *h) {
    if(h && h->values) shared_free_size(h->values, h->capacity, sizeof(uintmax_t));
    if(h) shared_free_size(h, 1, sizeof(HASH));
}

void hash_insert(HASH *h, uintmax_t value) {
    uintmax_t key = (value % prime_1e13) % h->capacity;
    while(h->values[key] && h->values[key] != value) {
        key = (key + 1) % h->capacity;
    }
    h->values[key] = value;
    h->count++;
    if(h->count * 2 > h->capacity) {
        fprintf(stderr, "Hash too full. Aborting.\n");
        exit(ENOMEM);
    }
}

int  hash_lookup(HASH *h, uintmax_t value) {
    if(h == NULL) {
        perror("hash_lookup was passed an empty hash");
        exit(EXIT_FAILURE);
    }
    if(h->capacity == 0) {
        perror("hash_lookup was passed a hash with capacity 0");
        exit(EXIT_FAILURE);
    }
    uintmax_t key = (value % prime_1e13) % h->capacity;
    while(h->values[key]) {
        if(h->values[key] == value) {
            return 1;
        } else {
            key = (key + 1) % h->capacity;
        }
    }
    return 0;
}
