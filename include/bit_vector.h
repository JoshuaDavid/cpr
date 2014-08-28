#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#define BITVEC struct bit_vector

uint8_t bits_in_byte[256] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,
    2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,
    5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,
    3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,
    4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,
    4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,
    4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,
    6,7,5,6,6,7,6,7,7,8 };

BITVEC {
    char    *values;
    uint64_t _size; // length IN BYTES including crap at front
    uint64_t num_bits;
    uint64_t offset; // Offset in BYTES
};

// Consistent naming scheme
BITVEC *bv_create(uint64_t num_bits) {
    BITVEC *bv = calloc(1, sizeof(BITVEC));
    uint64_t num_bytes = 1 + (num_bits / CHAR_BIT);
    char *values = mmap(NULL, num_bytes, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint64_t i = 0;
    for(i = 0; i < num_bytes; i++) {
        values[i] = 0;
    }
    bv->num_bits = num_bits;
    bv->_size = num_bytes;
    bv->offset = 0;
    bv->values = values;
    return bv;
}

BITVEC *bv_read_from_file(char *bvfname) {
    FILE *bvfp; // Bit vector file pointer
    bvfp = fopen(bvfname, "r");
    int bvfd = fileno(bvfp);
    struct stat st_bv;
    assert(0 == fstat(bvfd, &st_bv));
    off_t size_bv = st_bv.st_size;
    char *values = mmap(NULL, size_bv, PROT_READ, MAP_PRIVATE, bvfd, 0);
    BITVEC *bv = calloc(1, sizeof(BITVEC));
    bv->values = values;
    bv->_size = size_bv + 1;
    bv->offset = 0;
    int i;
    if(0 == strncmp(bv->values, "--------", 8)) {
        // This bit vector was created by Commet
        int poundsignhit = 0;
        for(i = 0; i < 512; i++) {
            bv->offset++;
            if(bv->values[i] == '#') {
                poundsignhit = 1;
            }
            if(poundsignhit && bv->values[i] == '\n') {
                break;
            }
        }
    }
    bv->num_bits = (bv->_size - bv->offset) * CHAR_BIT;
    return bv;
}

void bv_save_to_file(BITVEC *bv, char *fname) {
    FILE *fp;
    fp = fopen(fname, "wb");
    fwrite(bv->values, 1, bv->_size, fp);
}

void bv_destroy(BITVEC *bv) {
    if(bv->values) {
        munmap(bv->values, bv->num_bits / CHAR_BIT);
    }
    free(bv);
}

int bv_get(BITVEC *bv, uint64_t index) {
    index += (CHAR_BIT * bv->offset);
    uint64_t byte_index = (index / CHAR_BIT);
    if(byte_index >= bv->_size) {
        // Just return 0 if we're looking past the end, not a big deal
        return 0;
    }
    char c = bv->values[byte_index];
    char bit_mask = (char)(1 << (index % CHAR_BIT));
    c &= bit_mask;
    return c && 1;
}

void bv_set(BITVEC *bv, uint64_t index, int val) {
    index += (CHAR_BIT * bv->offset);
    uint64_t byte_index = (index / CHAR_BIT);
    char c = bv->values[byte_index];
    char bit_mask = (char)(1 << (index % CHAR_BIT));
    if(val) c |= (char) bit_mask;
    else    c &= (char) ~(1 << bit_mask);
    bv->values[byte_index] = c;
}

uint64_t bv_count_bits(BITVEC *bv) {
    uint64_t i;
    uint64_t count = 0;
    for(i = 0; i < bv->num_bits; i++) {
        // There is a better way of doing this, but unless it's a performance
        // issue, this is the clearest way
        count += bv_get(bv, i);
    }
    return count;
}

BITVEC *bv_copy(BITVEC *bv) {
    BITVEC *ret = bv_create(bv->num_bits);
    uint64_t i;
    for(i = 0; i < bv->num_bits; i++) {
        bv_set(ret, i, bv_get(bv, i));
    }
    return ret;
}

BITVEC *bv_and(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uint64_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uint64_t i = 0;
    for(i = 0;  i < num_bits; i++) {
        bv_set(ret, i, bv_get(bva, i) & bv_get(bvb, i));
    }
    return ret;
}

BITVEC *bv_or(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uint64_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uint64_t i = 0;
    for(i = 0;  i < num_bits; i++) {
        bv_set(ret, i, bv_get(bva, i) | bv_get(bvb, i));
    }
    return ret;
}

BITVEC *bv_xor(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uint64_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uint64_t i = 0;
    for(i = 0;  i < num_bits; i++) {
        bv_set(ret, i, bv_get(bva, i) ^ bv_get(bvb, i));
    }
    return ret;
}

#endif
