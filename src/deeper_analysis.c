#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "compare_sets.h"
#include "shame.h"
#include "filter_reads.h"

// Find the reads in one file which are also found in at least one other file
// in that set.
//
// For file SET1_A, this would be done by finding all reads in A which appear
// in any file in SET1 which is not SET1_A. This is most easily done by or-ing
// SET1_B, SET1_C, etc... with each other, and then and-ing the result with
// SET1_A.

BITVEC *reads_elsewhere_in_set(CJOB *settings, READSET *set, int i) {
    char *sbvfname = get_bvfname_from_one_fafname(settings, set->filenames[i]);
    BITVEC *sbv = bv_copy(bv_read_from_file(sbvfname));
    BITVEC *others = bv_create(sbv->num_bits);

    int j = 0;
    for(j = 0; j < set->num_files; j++) {
        if(j != i) {
            char *isbvfname = get_bvfname_of_index_and_search(settings, set->filenames[j], set->filenames[i]);
            BITVEC *isbv = bv_copy(bv_read_from_file(isbvfname));
            bv_ior(others, isbv);
        }
    }


    bv_destroy(sbv);
    free(sbvfname);
}
