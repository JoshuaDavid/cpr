#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "compare_sets.h"
#include "shame.h"
#include "bit_vector.h"
#include <limits.h>
#include "filter_reads.h"
#include "extract_reads.h"

// Find the reads in one file which are also found in at least one other file
// in that set.
//
// For file SET1_A, this would be done by finding all reads in A which appear
// in any file in SET1 which is not SET1_A. This is most easily done by or-ing
// SET1_B, SET1_C, etc... with each other, and then and-ing the result with
// SET1_A.

BITVEC *reads_elsewhere_in_set(CJOB *settings, READSET *set, int i) {
    char *sbvfname = get_bvfname_from_one_fafname(settings, set->filenames[i]);
    BITVEC *sbv = bv_read_from_file(sbvfname);
    BITVEC *others = bv_create(sbv->num_bits);

    int j = 0;
    for(j = 0; j < set->num_files; j++) {
        if(j != i) {
            char *isbvfname = get_bvfname_of_index_and_search(settings, set->filenames[j], set->filenames[i]);
            BITVEC *isbv = bv_copy(bv_read_from_file(isbvfname));
            bv_ior(others, isbv);
        }
    }

    bv_iand(others, sbv);
    bv_destroy(sbv);
    free(sbvfname);
    return others;
}

// Find reads that are in the search fasta file (filtered by the filter bv)
// and then filter those reads further by excluding any that occur in the
// excludeset

BITVEC *reads_not_in_set(CJOB *settings, char *sfafname, char *fbvfname, READSET *set) {
    char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
    int i = 0;

    BITVEC *sbv = bv_read_from_file(sbvfname);
    BITVEC *diff = bv_create(sbv->num_bits);
    int j = 0;
    for(j = 0; j < set->num_files; j++) {
        if(j != i) {
            char *isbvfname = get_bvfname_of_index_and_search(settings, set->filenames[j], set->filenames[i]);
            BITVEC *isbv = bv_copy(bv_read_from_file(isbvfname));
            bv_ior(diff, isbv);
        }
    }
    bv_inot(diff);
    bv_iand(diff, sbv);
    return diff;
}

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    if(DEBUG_LEVEL >= 1) print_commet_job(settings);
    int i = 0, j = 0;
    int n = settings->num_sets;
    char *sfafname = settings->sets[0]->filenames[0];
    char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
    READSET *set_b = settings->sets[1];
    BITVEC *uniq = reads_not_in_set(settings, sfafname, sbvfname, set_b);
    for(i = 2; i < settings->num_sets; i++) {
        set_b = settings->sets[i];
        BITVEC *uniq2 = reads_not_in_set(settings, sfafname, sbvfname, set_b);
        bv_iand(uniq, uniq2);
    }
    bv_save_to_file(uniq, "uniq.bv");
    for(i = 0; i < uniq->num_bits / CHAR_BIT; i++) {
        printf("%i", bv_get(uniq, i));
        if(i % 100 == 0) puts("");
    }
    puts("");
    extract_reads(sfafname, "uniq.bv", "uniq.fasta");
    cjob_destroy(settings);
    return 0;
}
