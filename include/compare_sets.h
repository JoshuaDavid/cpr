#ifndef SIMILARITY_MATRIX_H
#define SIMILARITY_MATRIX_H

#include "counter.h"
#include "bit_vector.h"
#include "commet_job.h"
#include "filter_reads.h"
#include "shame.h"

BITVEC **get_filter_bvs(CJOB *settings, READSET *set) {
    BITVEC **bvs = calloc(set->num_files, sizeof(BITVEC *));
    uintmax_t i = 0;
    for(i = 0; i < set->num_files; i++) {
        char *fafname = set->filenames[i];
        char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
        bvs[i] = bv_read_from_file(bvfname);
    }
    return bvs;
}

BITVEC ***get_comparison_bvs(CJOB *settings, READSET *set_a, READSET *set_b) {
    BITVEC ***comparisons = calloc(set_a->num_files, sizeof(BITVEC **));
    uintmax_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {
        comparisons[i] = calloc(set_b->num_files, sizeof(BITVEC *));
        for(j = 0; j < set_b->num_files; j++) {
            char *ifafname = set_a->filenames[i];
            char *sfafname = set_a->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            comparisons[i][j] = bv_read_from_file(bvfname);
        }
    }
    return comparisons;
}

void print_comparison(CJOB *settings, READSET *set_a, READSET *set_b) {
    BITVEC ***comparisons = get_comparison_bvs(settings, set_a, set_b);
    BITVEC **bvs_a = get_filter_bvs(settings, set_a);
    BITVEC **bvs_b = get_filter_bvs(settings, set_b);
    uintmax_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {
        char *sfafname = set_a->filenames[i];
        char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
        BITVEC *sbv = bv_read_from_file(sbvfname);
        printf("Counting reads in \"%s\" that occur in any of [\n", sfafname);
        BITVEC *acc = bv_read_from_file(sfafname);
        uintmax_t sbv_count = bv_count_bits(sbv);
        BITVEC *next;
        for(j = 0; j < set_b->num_files; j++) {
            char *ifafname = set_a->filenames[j];
            char *ibvfname = get_bvfname_from_one_fafname(settings, ifafname);
            BITVEC *ibv = bv_read_from_file(ibvfname);
            uintmax_t ibv_count = bv_count_bits(ibv);
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *intersection = bv_read_from_file(bvfname);
            next = bv_or(acc, intersection);
            printf("\n\t\"%s\", ", sfafname);
            fflush(NULL);
            uintmax_t acc_count = bv_count_bits(acc);
            uintmax_t int_count = bv_count_bits(intersection);
            uintmax_t next_count = bv_count_bits(next);
            acc = next;
            bv_destroy(acc); // Fix memory leak
            printf("\n%s(%ju) IN %s(%ju): %ju\n", ibvfname, ibv_count, sbvfname, sbv_count, int_count);
        }
        printf("\n]\n");
    }
}

#endif
