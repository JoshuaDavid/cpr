#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// The 10-trillionth prime -- it seems unlikely that a hash will be over 320Tb
const uint64_t prime_1e13 = 0x0001267a06388f9b;

struct hash *hash_create (void);
void         hash_destroy(struct hash *h);
void         hash_expand (struct hash *h);

void hash_insert (struct hash *h, uint64_t value);
int  hash_lookup (struct hash *h, uint64_t value);

struct array *array_create(void);
void          array_destroy(struct array *a);
void          array_expand (struct array *a);

void array_insert (struct array *a, uint64_t value);

struct array *hash_serialize  (struct hash *h);
struct hash  *hash_unserialize(struct array *a);

struct hash {
    uint64_t capacity;
    uint64_t count;
    uint64_t *values;
};

struct array {
    uint64_t capacity;
    uint64_t size;
    uint64_t *values;
};

struct hash *hash_create(void) {
    struct hash *h = calloc(1, sizeof(struct hash));
    h->capacity = 16;
    h->count = 0;
    h->values = calloc(h->capacity, sizeof(uint64_t));
    if(!h->values) {
        fprintf(stderr, "%s", "Cannot create hash: Out of memory\n");
        exit(ENOMEM);
    }
    return h;
}

void hash_destroy(struct hash *h) {
    if(h && h->values) free(h->values);
    if(h) free(h);
}

void hash_expand(struct hash *h) {
    // Double the size of *values
    uint64_t *old_values = h->values;
    uint64_t old_capacity = h->capacity;
    h->capacity *= 2;
    h->values = calloc(h->capacity, sizeof(uint64_t));
    if(!h->values) {
        fprintf(stderr, "%s", "Cannot add more reads to hash: Out of memory\n");
        exit(ENOMEM);
    }
    int i;
    for(i = 0; i < old_capacity; i++) {
        if(old_values[i]) {
            hash_insert(h, old_values[i]);
        }
    }
    free(old_values);
}

void hash_insert(struct hash *h, uint64_t value) {
    uint64_t key = (value % prime_1e13) % h->capacity;
    while(h->values[key] && h->values[key] != value) {
        key = (key + 1) % h->capacity;
    }
    h->values[key] = value;
    h->count++;
    if(h->count * 3 > h->capacity) {
        hash_expand(h);
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

struct array *hash_serialize(struct hash *h) {
    struct array *a = calloc(1, sizeof(struct array));
    a->size = h->count;
    a->values = calloc(a->size + 1, sizeof(uint64_t));
    if(!a->values) {
        fprintf(stderr, "%s", "Cannot create array from hash: Out of memory\n");
        exit(ENOMEM);
    }
    uint64_t i;
    uint64_t cap = h->capacity;
    uint64_t *p = a->values;
    for(i = 0; i < cap; i++) {
        if(h->values[i]) {
            *p = h->values[i];
            p++;
        }
    }
    return a;
}

struct hash *hash_unserialize(struct array *a) {
    struct hash *h = calloc(1, sizeof(struct hash));
    uint64_t count = a->size;
    uint64_t i;
    for(i = 0; i < count; i++) {
        hash_insert(h, a->values[i]);
    }
    return h;
}

struct array *array_create(void) {
    struct array *a = calloc(1, sizeof(struct array));
    a->capacity = 16;
    a->size = 0;
    a->values = calloc(a->capacity, sizeof(uint64_t));
    if(!a->values) {
        fprintf(stderr, "%s", "Cannot create array: Out of memory\n");
        exit(ENOMEM);
    }
    return a;
}
