#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H

#include <stdlib.h>
#include <stdint.h> /* for uintmax_t */

struct bit_vector {
    char    *values;
    uintmax_t _size; // length IN BYTES including crap at front
    uintmax_t num_bits;
    uintmax_t offset; // Offset in BYTES
};

#define BITVEC struct bit_vector

BITVEC *bv_create(uintmax_t num_bits);
BITVEC *bv_read_from_file(char *bvfname);
void bv_save_to_file(BITVEC *bv, char *fname);
void bv_destroy(BITVEC *bv);
int bv_get(BITVEC *bv, uintmax_t index);
void bv_set(BITVEC *bv, uintmax_t index, int val);
uintmax_t bv_count_bits(BITVEC *bv);
BITVEC *bv_copy(BITVEC *bv);
BITVEC *bv_and(BITVEC *bva, BITVEC *bvb);
BITVEC *bv_or(BITVEC *bva, BITVEC *bvb);
BITVEC *bv_xor(BITVEC *bva, BITVEC *bvb);
BITVEC *bv_not(BITVEC *bv);
// Changes bv_a, so be careful with these.
void bv_iand(BITVEC *bva, BITVEC *bvb);
void bv_ior(BITVEC *bva, BITVEC *bvb);
void bv_ixor(BITVEC *bva, BITVEC *bvb);

#endif
