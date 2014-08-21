#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

struct bit_vector {
    char    *values;
    uint64_t _size; // length including crap at front
    uint64_t num_bits;
    uint64_t offset;
};

struct bit_vector *read_bit_vector(char *bvfname) {
    FILE *bvfp; // Bit vector file pointer
    bvfp = fopen(bvfname, "r");
    int bvfd = fileno(bvfp);
    struct stat st_bv;
    assert(0 == fstat(bvfd, &st_bv));
    off_t size_bv = st_bv.st_size;
    char *values = mmap(NULL, size_bv, PROT_READ, MAP_PRIVATE, bvfd, 0);
    struct bit_vector *bv = calloc(1, sizeof(struct bit_vector));
    bv->values = values;
    bv->_size = size_bv * CHAR_BIT;
    bv->offset = 0;
    int i;
    if(0 == strncmp(bv->values, "--------", 8)) {
        // This bit vector was created by Commet
        int hashhit = 0;
        for(i = 0; i < 512; i++) {
            bv->offset++;
            if(bv->values[i] == '#') {
                hashhit = 1;
            }
            if(hashhit && bv->values[i] == '\n') {
                break;
            }
        }
    }
    bv->num_bits = (bv->_size - bv->offset) * CHAR_BIT;
    return bv;
}

int bv_get(struct bit_vector *bv, uint64_t index) {
    index += (CHAR_BIT * bv->offset);
    uint64_t byte_index = (index / CHAR_BIT);
    char c = bv->values[byte_index];
    char bit_mask = (char)(1 << (index % CHAR_BIT));
    c &= bit_mask;
    return c && 1;
}
