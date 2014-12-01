#include "bit_vector.h"
#include <stdio.h>
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
#include "shame.h"

// Handle MacOS mman providing MAP_ANON instead of MAP_ANONYMOUS
// And the fact that Ubuntu seems to define MAP_ANON
#ifdef MAP_ANON
#ifndef MAP_ANONYMOUS 
#define MAP_ANONYMOUS (MAP_ANON)
#endif
#endif

// Consistent naming scheme
BITVEC *bv_create(uintmax_t num_bits) {
    BITVEC *bv = calloc(1, sizeof(BITVEC));
    uintmax_t num_bytes = (num_bits / CHAR_BIT);
    if(num_bits % CHAR_BIT > 0) {
        num_bytes += 1;
    }
    char *values = mmap(NULL, num_bytes, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uintmax_t i = 0;
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
    bvfp = fopen(bvfname, "rw+");
    if(NULL == bvfp) {
        DBG(0) printf("Could not read from file \"%s\". \n", bvfname);
        perror("fopen:");
        exit(EXIT_FAILURE);
    }
    int bvfd = fileno(bvfp);
    struct stat st_bv;
    assert(0 == fstat(bvfd, &st_bv));
    // Align with page boundary
    uintmax_t size_bv = st_bv.st_size;
    uintmax_t fsize = size_bv;
    off_t pagesize = getpagesize();
    if(fsize % pagesize != 0) {
        fsize += pagesize - fsize % pagesize;
        if(fsize != size_bv) {
            DBG(1) printf("Expanding %s(%i) from %jx bytes to %jx bytes.\n", bvfname, bvfd, size_bv, fsize);
            if(0 != ftruncate(bvfd, fsize)) {
                perror("Could not resize file. ftruncate");
                exit(EXIT_FAILURE);
            }
        }
    }
    char *values = mmap(NULL, size_bv, PROT_READ, MAP_PRIVATE, bvfd, 0);
    fclose(bvfp);
    BITVEC *bv = calloc(1, sizeof(BITVEC));
    bv->values = values;
    bv->_size = size_bv;
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
    DBG(1) { printf("Saving bit vector to %s\n", fname); }
    if(bv == NULL) {
        perror("Bit vector is null: bv_save_to_file");
        exit(EXIT_FAILURE);
    }
    char *savedir = dirnameof(fname);
    assert(0 == mkdirp(savedir));
    free(savedir);
    DBG(3) { printf("Saving bit vector to %s\n", fname); }
    FILE *fp = fopen(fname, "wb");
    uintmax_t fsize = bv->_size;
    // Align with page boundary
    off_t pagesize = getpagesize();
    if(fsize % pagesize != 0) {
        fsize += pagesize - fsize % pagesize;
    }
    fwrite(bv->values, 1, fsize, fp);
    fclose(fp);
}

void bv_destroy(BITVEC *bv) {
    if(bv->values) {
        munmap(bv->values, bv->num_bits / CHAR_BIT);
    }
    free(bv);
}

int bv_get(BITVEC *bv, uintmax_t index) {
    index += (CHAR_BIT * bv->offset);
    uintmax_t byte_index = (index / CHAR_BIT);
    if(byte_index >= bv->_size) {
        // Just return 0 if we're looking past the end, not a big deal
        return 0;
    }
    char c = bv->values[byte_index];
    char bit_mask = (char)(1 << (index % CHAR_BIT));
    c &= bit_mask;
    return c && 1;
}

void bv_set(BITVEC *bv, uintmax_t index, int val) {
    index += (CHAR_BIT * bv->offset);
    uintmax_t byte_index = (index / CHAR_BIT);
    char c = bv->values[byte_index];
    char bit_mask = (char)(1 << (index % CHAR_BIT));
    if(val) c |= (char) bit_mask;
    else    c &= (char) ~(1 << bit_mask);
    bv->values[byte_index] = c;
}

uintmax_t bv_count_bits(BITVEC *bv) {
    uintmax_t i;
    uintmax_t count = 0;
    for(i = 0; i < bv->_size; i++) {
        // There is a better way of doing this, but unless it's a performance
        // issue, this is the clearest way
        int j = 0;
        for(j = 0; j < 8;  j++) {
            if(1 & (bv->values[i] >> j)) {
                count++;
            }
        }
    }
    return count;
}

BITVEC *bv_copy(BITVEC *bv) {
    BITVEC *ret = bv_create(bv->num_bits);
    uintmax_t i;
    for(i = 0; i < bv->num_bits; i++) {
        bv_set(ret, i, bv_get(bv, i));
    }
    return ret;
}

BITVEC *bv_and(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uintmax_t i = 0;
    for(i = 0; i < ret->_size; i++) {
        ret->values[i] = bva->values[i] & bvb->values[i];
    }
    return ret;
}

BITVEC *bv_or(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uintmax_t i = 0;
    for(i = 0; i < ret->_size; i++) {
        ret->values[i] = bva->values[i] | bvb->values[i];
    }
    return ret;
}

BITVEC *bv_xor(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    BITVEC *ret = bv_create(num_bits);
    uintmax_t i = 0;
    for(i = 0; i < ret->_size; i++) {
        ret->values[i] = bva->values[i] ^ bvb->values[i];
    }
    return ret;
}

BITVEC *bv_not(BITVEC *bv) {
    // Minimum of the lengths, as the rest is zeroed anyway
    BITVEC *ret = bv_create(bv->num_bits);
    uintmax_t i = 0;
    for(i = 0; i < ret->_size; i++) {
        ret->values[i] = ~bv->values[i];
    }
    return ret;
}

void bv_ior(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    uintmax_t i = 0;
    for(i = 0; i < num_bits / CHAR_BIT; i++) {
        bva->values[i] = bva->values[i] | bvb->values[i];
    }
}

void bv_iand(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    uintmax_t i = 0;
    for(i = 0; i < num_bits / CHAR_BIT; i++) {
        bva->values[i] = bva->values[i] & bvb->values[i];
    }
}

void bv_ixor(BITVEC *bva, BITVEC *bvb) {
    // Minimum of the lengths, as the rest is zeroed anyway
    uintmax_t num_bits = bva->num_bits < bvb->num_bits ? 
                        bva->num_bits : bvb->num_bits;
    uintmax_t i = 0;
    for(i = 0; i < num_bits / CHAR_BIT; i++) {
        bva->values[i] = bva->values[i] ^ bvb->values[i];
    }
}

void bv_inot(BITVEC *bva) {
    uintmax_t i = 0;
    for(i = 0; i < bva->num_bits / CHAR_BIT; i++) {
        bva->values[i] = ~(bva->values[i]);
    }
}
