#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
        uintmax_t num_reads_indexed = bv_count_bits(bvs[i]);
        printf("%s: %ju\n", ifafname, num_reads_indexed);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            // Segfaults here for some reason
            uintmax_t num_index_in_search = bv_count_bits(comparisons[i][j]);
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
        uintmax_t num_reads_indexed = bv_count_bits(bvs[i]);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            printf(";%7.0f", similarity[i][j]);
        }
        printf("\n");
    }
}

COUNTER *compare_two_sets(CJOB *settings, READSET *set_a, READSET *set_b) {
    if(DEBUG_LEVEL >= 2) printf("Comparing sets %s and %s.\n", set_a->name, set_b->name);
    BITVEC **bvs_a = get_filter_bvs(settings, set_a);
    BITVEC **bvs_b = get_filter_bvs(settings, set_b);
    VERIFY_NONEMPTY(bvs_a);
    uintmax_t i = 0, j = 0;
    COUNTER *shared = calloc(1, sizeof(COUNTER));
    for(i = 0; i < set_a->num_files; i++) {

        char *sfafname  = set_a->filenames[i];
        char *sbvfname  = get_bvfname_from_one_fafname(settings, sfafname);
        BITVEC *sbv  = bv_read_from_file(sbvfname);
        uintmax_t sbvcount  = bv_count_bits(sbv);

        BITVEC *acc = bv_create(sbv->num_bits);

        for(j = 0; j < set_b->num_files; j++) {

            char *ifafname  = set_b->filenames[j];
            char *ibvfname  = get_bvfname_from_one_fafname(settings, ifafname);
            BITVEC *ibv  = bv_read_from_file(ibvfname);
            uintmax_t ibvcount  = bv_count_bits(ibv);

            char *isbvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *isbv = bv_read_from_file(isbvfname);
            uintmax_t isbvcount = bv_count_bits(isbv);

            acc = bv_or(acc, isbv);

        }
        uintmax_t shared_count = bv_count_bits(acc);
        if(DEBUG_LEVEL >= 2) printf("%s(%ju) in %s: %ju\n", sfafname, sbvcount, set_b->name, shared_count);
        shared->t += shared_count;
        shared->f += sbvcount - shared_count;

        bv_destroy(acc);
        bv_destroy(sbv);
    }
    if(DEBUG_LEVEL >= 1) {
        printf("%s     in %s: %ju\n", set_a->name, set_b->name, shared->t);
        printf("%s NOT in %s: %ju\n", set_a->name, set_b->name, shared->f);
        puts("");
    }
    return shared;
}

COUNTER ***get_raw_comparison_matrix(CJOB *settings) {
    int i = 0, j = 0;
    READSET **sets = settings->sets;
    COUNTER ***shared = calloc(settings->num_sets, sizeof(COUNTER **));
    for(i = 0; i < settings->num_sets; i++) {
        shared[i] = calloc(settings->num_sets, sizeof(COUNTER *));
        for(j = 0; j < settings->num_sets; j++) {
            shared[i][j] = compare_two_sets(settings, sets[i], sets[j]);
        }
    }
    return shared;
}

void print_comparison_percents(CJOB *settings, COUNTER ***shared) {
    int i = 0, j = 0;

    if(DEBUG_LEVEL >= 1) {
        printf("Percentage of set on left which appears in set on top\n");
    }
    printf(" ");
    for(i = 0; i < settings->num_sets; i++) {
        printf(";%s", settings->sets[i]->name);
    }
    printf("\n");
    for(i = 0; i < settings->num_sets; i++) {
        printf("%s", settings->sets[i]->name);
        for(j = 0; j < settings->num_sets; j++) {
            printf(";%7f%%", 100.0 * (float) shared[i][j]->t / (float)(shared[i][j]->t + shared[i][j] ->f));
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    COUNTER ***shared = get_raw_comparison_matrix(settings);
    print_comparison_percents(settings, shared);
    return EXIT_SUCCESS;
}
