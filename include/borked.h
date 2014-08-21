#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

// Expandable array, doubles memory allocation when it runs out of space.
struct earr {
    int *vals;
    int capacity;
    int length;
};

void *shared_calloc(size_t nmemb, size_t size) {
    void *mem = mmap(NULL, nmemb * size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    return memset(mem, 0, nmemb * size);
}

void shared_free_size(void *mem, size_t nmemb, size_t size) {
    if(mem) munmap(mem, nmemb * size);
}

struct earr *create_earr() {
    struct earr *a = shared_calloc(1, sizeof(struct earr));
    a->length = 0;
    a->capacity = 16;
    a->vals = shared_calloc(a->capacity, sizeof(int));
    return a;
}

void earr_expand(struct earr *a) {
    int *new_vals = shared_calloc(a->capacity * 2, sizeof(int));
    memcpy(new_vals, a->vals, a->capacity * sizeof(int));
    a->vals = new_vals;
    a->capacity *= 2;
}

void earr_insert(struct earr *a, int val) {
    if(a->length >= a->capacity) earr_expand(a);
    a->vals[a->length] = val;
    a->length++;
}

int earr_lookup(struct earr *a, int index) {
    return a->vals[index];
}


