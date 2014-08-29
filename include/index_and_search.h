#ifndef INDEX_AND_SEARCH_H
#define INDEX_AND_SEARCH_H

#include "hash.h"
#include "counter.h"
#include "filter_reads.h"
#include "shame.h"

uint64_t kmer_mask(int kmer_size) {
    uint64_t mask = (1ll << (uint64_t)kmer_size) - 1;
    return mask;
}

uint64_t get_num_kmers(CJOB *settings, char *fafname) {
    if(DEBUG_LEVEL >= 1) printf("Counting kmers in %s.\n", fafname);
    int kmer_size = settings->kmer_size;
    char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
    BITVEC *bv = bv_read_from_file(bvfname);
    
    FILE *fafp = fopen(fafname, "r");

    char read[4096];
    uint64_t current_read = 0;
    uint64_t num_kmers = 0;
    uint64_t num_lines = 0;
    // For some reason, vim folds wrong if I inline this curly brace :/
    while(NULL != fgets(read, sizeof(read), fafp)) 
    {
        if(read[0] != '>') {
            if(bv_get(bv, current_read)) {
                int len = strlen(read);
                if(len >= kmer_size) {
                    num_kmers += len - kmer_size + 1;
                }
            }
            current_read++;
        }
    }
    if(DEBUG_LEVEL >= 1) {
        printf("Done counting kmers in %s (%lli total in %lli lines).\n", fafname, num_kmers, current_read);
    }
    fclose(fafp);
    return num_kmers;
}

uint64_t hash_of_base(char b) {
    // Just some random numbers.
    switch(b) {
        case 'a': case 'A': return 0x000001d27d51ef41;
        case 'c': case 'C': return 0x000004bfa017c492;
        case 'g': case 'G': return 0x00001cf916ce3d1e;
        case 't': case 'T': return 0x00001994cb8816cd;
        default:            return 0x000015e5f7a5a001;
    }
}

uint64_t hash_of_kmer(char *read, int kmer_size) {
    uint64_t mask = kmer_mask(kmer_size);
    uint64_t value = 0;
    int i = 0;
    for(i = 0; i < kmer_size; i++) {
        char b = read[i];
        if(!b) break;
        value <<= 1;
        value ^= hash_of_base(b);
    }
    value &= mask;
    return value;
}

void index_read_into_hash(CJOB *settings, HASH *h, char* read) {
    int kmer_size = settings->kmer_size;
    uint64_t mask = kmer_mask(kmer_size);
    int len = strlen(read);
    int i;
    if(len < kmer_size) return;
    uint64_t value = hash_of_kmer(read, kmer_size);
    uint64_t bh;
    for(i = 1; i < len - kmer_size - 1; i++) {
        value <<= 1;
        value ^= hash_of_base(read[i + kmer_size - 1]);
        value &= mask;
        hash_insert(h, value);
    }
}

struct counter *search_read_in_hash(CJOB *settings, HASH *h, char* seq) {
    int kmer_size = settings->kmer_size;
    uint64_t mask = kmer_mask(kmer_size);
    int len = strlen(seq);
    int i;
    struct counter *same = calloc(1, sizeof(struct counter));
    if(same == NULL) {
        perror("search read in hash --> calloc");
        exit(EXIT_FAILURE);
    }
    if(len < kmer_size) return same;
    uint64_t value = hash_of_kmer(seq, kmer_size);
    for(i = 1; i < len - kmer_size - 1; i++) {
        value <<= 1;
        value ^= hash_of_base(seq[i + kmer_size - 1]);
        value &= mask;
        if(hash_lookup(h, value)) {
            same->t++;
        } else {
            same->f++;
        }
    }
    return same;
}

HASH *index_from_file(CJOB *settings, char *fafname) {
    uint64_t num_kmers = get_num_kmers(settings, fafname);
    char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
    HASH *h = hash_create(num_kmers);
    FILE *fafp;
    fafp = fopen(fafname, "r");
    char read[65536];
    int readnum = 0;
    struct bit_vector *bv = bv_read_from_file(bvfname);
    while(NULL != fgets(read, sizeof(read), fafp)) {
        if(read[0] != '>') {
            if(bv_get(bv, readnum)) {
                index_read_into_hash(settings, h, read);
            }
            readnum++;
        }
    }
    return h;
}

BITVEC *search_file_in_hash(CJOB *settings, HASH *h, char *fafname) {
    uint64_t num_kmers = get_num_kmers(settings, fafname);
    char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
    FILE *fafp = fopen(fafname, "r");
    char read[65536];
    int readnum = 0;
    struct bit_vector *bv = bv_read_from_file(bvfname);
    struct bit_vector *sbv = bv_create(bv->num_bits);
    while(NULL != fgets(read, sizeof(read), fafp)) {
        if(read[0] != '>') {
            if(bv_get(bv, readnum)) {
                COUNTER *c = search_read_in_hash(settings, h, read);
                if(c->t >= settings->min_shared_kmers) {
                    bv_set(sbv, readnum, 1);
                } else {
                    bv_set(sbv, readnum, 1);
                }
                free(c);
            }
            readnum++;
        }
    }
    return sbv;
}

void create_search_files_of_one_index_serial(CJOB *settings, char *ifafname) {
    char *ibvfname = get_bvfname_from_one_fafname(settings, ifafname);
    int i = 0, j = 0;
    HASH *h = index_from_file(settings, ifafname);
    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *bv = search_file_in_hash(settings, h, sfafname);
            bv_save_to_file(bv, bvfname);
        }
    }
    hash_destroy(h);
}

void create_search_files_of_one_index_parallel(CJOB *settings, char *ifafname) {
    char *ibvfname = get_bvfname_from_one_fafname(settings, ifafname);
    int i = 0, j = 0;

    // Begin forking code block
    int num_files = 0, pid_counter = 0;
    for(i = 0; i < settings->num_sets; i++) {
        num_files += settings->sets[i]->num_files;
    }
    pid_t *pids = calloc(num_files, sizeof(pid_t));
    // End forking code block

    HASH *h = index_from_file(settings, ifafname);
    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {

            // Begin forking code block
            pid_t pid = fork();
            if(pid == -1) {
                perror("Fork failed!\n");
                exit(EXIT_FAILURE);
            } else if(pid == 0) {
            // End forking code block

            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *bv = search_file_in_hash(settings, h, sfafname);
            bv_save_to_file(bv, bvfname);

            // Begin forking code block
            exit(EXIT_SUCCESS);
            } else {
                pids[pid_counter] = pid;
                pid_counter++;
            }
            // End forking code block

        }
    }

    // Begin forking code block
    for(i = 0; i < num_files; i++) {
        int status = 0;
        waitpid(pids[i], &status, 0);
        assert(status == 0);
    }
    // End forking code block

    hash_destroy(h);
}

void create_search_files_of_one_index(CJOB *settings, char *ifafname, int parallel) {
    if(parallel >= 1) {
        create_search_files_of_one_index_parallel(settings, ifafname);
    } else {
        create_search_files_of_one_index_serial(settings, ifafname);
    }
}

void create_all_search_files_serial(CJOB *settings, int parallel) {
    int i = 0, j = 0;
    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {
            char *ifafname = set->filenames[j];
            create_search_files_of_one_index(settings, ifafname, parallel);
        }
    }
}

void create_all_search_files_parallel(CJOB *settings, int parallel) {
    int i = 0, j = 0;

    // Begin forking code block
    int num_files = 0, pid_counter = 0;
    for(i = 0; i < settings->num_sets; i++) {
        num_files += settings->sets[i]->num_files;
    }
    pid_t *pids = calloc(num_files, sizeof(pid_t));
    // End forking code block

    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {

            // Begin forking code block
            pid_t pid = fork();
            if(pid == -1) {
                perror("Fork failed!\n");
                exit(EXIT_FAILURE);
            } else if(pid == 0) {
            // End forking code block

            char *ifafname = set->filenames[j];
            create_search_files_of_one_index(settings, ifafname, parallel);

            // Begin forking code block
            exit(EXIT_SUCCESS);
            } else {
                pids[pid_counter] = pid;
                pid_counter++;
            }
            // End forking code block
        }
    }

    // Begin forking code block
    for(i = 0; i < num_files; i++) {
        int status = 0;
        waitpid(pids[i], &status, 0);
        assert(status == 0);
    }
    // End forking code block
}

void create_all_search_files(CJOB *settings, int parallel) {
    if(parallel >= 2) {
        create_all_search_files_parallel(settings, parallel);
    } else {
        create_all_search_files_serial(settings, parallel);
    }
}

#endif
