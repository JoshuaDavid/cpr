#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include "commet_job.h"
#include "filter_reads.h"
#include "shame.h"

BITVEC **get_filter_bvs(CJOB *settings, READSET *set) {
    BITVEC **bvs = calloc(set->num_files, sizeof(BITVEC *));
    uint64_t i = 0;
    for(i = 0; i < set->num_files; i++) {
        char *fafname = set->filenames[i];
        char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
        bvs[i] = bv_read_from_file(bvfname);
    }
    return bvs;
}

BITVEC ***get_comparison_bvs(CJOB *settings, READSET *set_a, READSET *set_b) {
    BITVEC ***comparisons = calloc(set_a->num_files, sizeof(BITVEC **));
    uint64_t i = 0, j = 0;
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
    uint64_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {
        char *sfafname = set_a->filenames[i];
        char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
        BITVEC *sbv = bv_read_from_file(sbvfname);
        printf("Counting reads in \"%s\" that occur in any of [\n", sfafname);
        BITVEC *acc = bv_read_from_file(sfafname);
        uint64_t sbv_count = bv_count_bits(sbv);
        BITVEC *next;
        for(j = 0; j < set_b->num_files; j++) {
            char *ifafname = set_a->filenames[j];
            char *ibvfname = get_bvfname_from_one_fafname(settings, ifafname);
            BITVEC *ibv = bv_read_from_file(ibvfname);
            uint64_t ibv_count = bv_count_bits(ibv);
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *intersection = bv_read_from_file(bvfname);
            next = bv_or(acc, intersection);
            printf("\n\t\"%s\", ", sfafname);
            fflush(NULL);
            uint64_t acc_count = bv_count_bits(acc);
            uint64_t int_count = bv_count_bits(intersection);
            uint64_t next_count = bv_count_bits(next);
            acc = next;
            bv_destroy(acc); // Fix memory leak
            printf("\n%s(%lli) IN %s(%lli): %lli\n", ibvfname, ibv_count, sbvfname, sbv_count, int_count);
        }
        printf("\n]\n");
    }
}

void compare_files_within_set(CJOB *settings, READSET *set) {
    int i = 0, j = 0;
    // Bit vectors for each file;
    // Easily parallelizeable
    BITVEC **bvs = get_filter_bvs(settings, set);

    // Get comparison bit vectors
    // Easily parallelizeable
    BITVEC **comparisons[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        BITVEC **row = calloc(set->num_files, sizeof(BITVEC **));
        comparisons[i] = row;
        for(j = 0; j < set->num_files; j++) {
            char *ifafname = set->filenames[i];
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            comparisons[i][j] = bv_read_from_file(bvfname);
        }
    }

    // Calculate similarity between each pair of files
    // Easily parallelizeable
    float *similarity[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        float *row = NULL;
        similarity[i] = calloc(set->num_files, sizeof(float));
        char *ifafname = set->filenames[i];
        uint64_t num_reads_indexed = bv_count_bits(bvs[i]);
        printf("%s: %lli\n", ifafname, num_reads_indexed);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            // Segfaults here for some reason
            uint64_t num_index_in_search = bv_count_bits(comparisons[i][j]);
            similarity[i][j] = (float) num_index_in_search ;// (float) num_reads_indexed;
        }
    }

    // Print similarity percents: 
    // Not parallelizeable (and why would you bother?)
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf(";%s", ifafname);
    }
    printf("\n");
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf("%s", ifafname);
        uint64_t num_reads_indexed = bv_count_bits(bvs[i]);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            printf(";%7.0f", similarity[i][j]);
        }
        printf("\n");
    }
}

void compare_two_sets(CJOB *settings, READSET *set_a, READSET *set_b) {
    printf("Comparing sets %s and %s.\n", set_a->name, set_b->name);
    BITVEC **bvs_a = get_filter_bvs(settings, set_a);
    BITVEC **bvs_b = get_filter_bvs(settings, set_b);
    VERIFY_NONEMPTY(bvs_a);
    uint64_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {

        char *sfafname  = set_a->filenames[i];
        char *sbvfname  = get_bvfname_from_one_fafname(settings, sfafname);
        BITVEC *sbv  = bv_read_from_file(sbvfname);
        uint64_t sbvcount  = bv_count_bits(sbv);

        for(j = 0; j < set_b->num_files; j++) {

            char *ifafname  = set_b->filenames[j];
            char *ibvfname  = get_bvfname_from_one_fafname(settings, ifafname);
            BITVEC *ibv  = bv_read_from_file(ibvfname);
            uint64_t ibvcount  = bv_count_bits(ibv);

            char *isbvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *isbv = bv_read_from_file(isbvfname);
            uint64_t isbvcount = bv_count_bits(isbv);

            printf("%s(%lli) in %s(%lli): %s(%lli)\n", sbvfname, sbvcount, ibvfname, ibvcount, isbvfname, isbvcount);
        }
    }
}

void compare_all_sets(CJOB *settings) {
    int i = 0, j = 0;
    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        for(j = 0; j < settings->num_sets; j++) {
            compare_two_sets(settings, sets[i], sets[j]);
        }
    }
}

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    compare_all_sets(settings);
    return EXIT_SUCCESS;
}
