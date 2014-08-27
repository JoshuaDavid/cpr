#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include "commet_job.h"
#include "shame.h"


void compare_files_within_set(struct commet_job *settings, struct readset *set) {
    int i = 0, j = 0;
    // Bit vectors for each file;
    // Easily parallelizeable
    struct bit_vector *bvs[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        char *fafname = set->filenames[i];
        char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
        bvs[i] = bv_read_from_file(bvfname);
    }

    // Get comparison bit vectors
    // Easily parallelizeable
    struct bit_vector **comparisons[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        struct bit_vector *comparisons[i][set->num_files];
        for(j = 0; j < set->num_files; j++) {
            char *ifafname = set->filenames[i];
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            comparisons[i][j] = bv_read_from_file(bvfname);
        }
    }

    // Calculate similarity between each pair of files
    // Easily parallelizeable
    float **similarity[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        float similarity[i][set->num_files];
        char *ifafname = set->filenames[i];
        uint64_t num_reads_indexed = 1;//bv_count_bits(bvs[i]);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            // Segfaults here for some reason
            printf("%p\n", comparisons);
            printf("%p\n", comparisons[i]);
            printf("%p\n", comparisons[i][j]);
            uint64_t num_index_in_search = bv_count_bits(comparisons[i][j]);
            similarity[i][j] = (float) num_index_in_search / (float) num_reads_indexed;
        }
    }

    // Print similarity percents: 
    // Not parallelizeable (and why would you bother?)
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf(";%s\t", ifafname);
    }
    printf("\n");
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf("%s\t", ifafname);
        uint64_t num_reads_indexed = bv_count_bits(bvs[i]);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
        }
    }
}

void compare_all_sets(struct commet_job *settings) {
    int i = 0, j = 0;
    struct readset **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        printf("COMPARING SETS: %s vs %s\n", sets[i]->name, sets[i]->name);
        compare_files_within_set(settings, sets[i]);
        for(j = 0; j < settings->num_sets; j++) {
            printf("COMPARING SETS: %s vs %s\n", sets[i]->name, sets[j]->name);
        }
    }
}

int main(int argc, char **argv) {
    struct commet_job *settings = get_settings(argc, argv);
    compare_all_sets(settings);
    return EXIT_SUCCESS;
}
