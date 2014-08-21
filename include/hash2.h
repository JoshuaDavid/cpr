#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

// The 10-trillionth prime -- it seems unlikely that a hash will be over 320Tb
const uint64_t prime_1e13 = 0x0001267a06388f9b;

struct hash *hash_create (uint64_t size);
void         hash_destroy(struct hash *h);

void hash_insert (struct hash *h, uint64_t value);
int  hash_lookup (struct hash *h, uint64_t value);

struct hash {
    uint64_t capacity;
    uint64_t count;
    uint64_t *values;
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

struct hash *hash_create(uint64_t size) {
    struct hash *h = shared_calloc(1, sizeof(struct hash));
    h->capacity = size * 3;
    h->count = 0;
    h->values = shared_calloc(h->capacity, sizeof(uint64_t));
    if(!h->values) {
        fprintf(stderr, "%s", "Cannot create hash: Out of memory\n");
        exit(ENOMEM);
    }
    return h;
}

void hash_destroy(struct hash *h) {
    if(h && h->values) shared_free_size(h->values, h->capacity, sizeof(uint64_t));
    if(h) shared_free_size(h, 1, sizeof(struct hash));
}

void hash_insert(struct hash *h, uint64_t value) {
    uint64_t key = (value % prime_1e13) % h->capacity;
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

int  hash_lookup(struct hash *h, uint64_t value) {
    uint64_t key = (value % prime_1e13) % h->capacity;
    while(h->values[key]) {
        if(h->values[key] == value) {
            return 1;
        } else {
            key = (key + 1) % h->capacity;
        }
    }
    return 0;
}