#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hash.h"
#include "commet_job.h"
#include "counter.h"


uint64_t kmer_mask(int kmer_size) {
    uint64_t mask = (1ll << (uint64_t)kmer_size) - 1;
    return mask;
}

uint64_t hash_of_base(char b) {
    uint64_t ac  = 0x0000164b553dae05;
    uint64_t ag  = 0x00000e0de3e45789;
    uint64_t at  = 0x00001994cb8816cd;
    uint64_t cg  = 0x000012f4f52a6a97;
    uint64_t n   = 0x000015e5f7a5a001;
    uint64_t value = 0;
    if(b=='a'||b=='A'||b=='c'||b=='C') value ^= ac;
    if(b=='a'||b=='A'||b=='g'||b=='G') value ^= ag;
    if(b=='a'||b=='A'||b=='t'||b=='T') value ^= at;
    if(b=='g'||b=='G'||b=='c'||b=='C') value ^= cg;
    if(b=='n'||b=='N')                 value ^= n;
    return value;
}

uint64_t hash_of_kmer(char *read, int kmer_size) {
    uint64_t mask = kmer_mask(kmer_size);
    uint64_t value = 0;
    int i = 0;
    for(i = 0; i < kmer_size; i++) {
        char b = read[i];
        if(!b) break;
        uint64_t bh = hash_of_base(b);
        value <<= 1;
        value ^= bh;

    }
    value &= mask;
    return value;
}

void index_seq_in_hash(struct hash *h, char* seq, int kmer_size) {
    uint64_t mask = kmer_mask(kmer_size);
    int len = strlen(seq);
    int i;
    if(len < kmer_size) return;
    uint64_t value = hash_of_kmer(seq, kmer_size);
    uint64_t bh;
    for(i = 1; i < len - kmer_size - 1; i++) {
        bh = hash_of_base(seq[i + kmer_size - 1]);
        value <<= 1;
        value ^= bh;
        value &= mask;
        hash_insert(h, value);
    }
}

struct counter *search_seq_in_hash(struct hash *h, char* seq, int kmer_size) {
    uint64_t mask = kmer_mask(kmer_size);
    int len = strlen(seq);
    int i;
    struct counter *same = calloc(1, sizeof(struct counter));
    if(len < kmer_size) return same;
    uint64_t value = hash_of_kmer(seq, kmer_size);
    uint64_t bh;
    for(i = 1; i < len - kmer_size - 1; i++) {
        bh = hash_of_base(seq[i + kmer_size - 1]);
        value <<= 1;
        value ^= bh;
        value &= mask;
        if(hash_lookup(h, value)) {
            same->t++;
        } else {
            same->f++;
        }
    }
    return same;
}

void index_reads_from_file(struct hash *h, char *filename) {
    FILE *fp;
    char line[4096];

    fp = fopen(filename, "r");

    if(fp == NULL) {
        fprintf(stderr, "Could not open file \"%s\" for reading\n", filename);
        exit(EBADF);
    }

    while(fgets(line, sizeof(line), fp)) {
        int len = strlen(line);
        if(line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        index_seq_in_hash(h, line, 32);
    }
}

struct counter *search_reads_from_file(struct hash *h, char *filename) {
    FILE *fp;
    char line[4096];
    struct counter *same = calloc(1, sizeof(struct counter));

    fp = fopen(filename, "r");

    if(fp == NULL) {
        fprintf(stderr, "Could not open file \"%s\" for reading\n", filename);
        exit(EBADF);
    }

    while(fgets(line, sizeof(line), fp)) {
        int len = strlen(line);
        if(line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        struct counter *c = search_seq_in_hash(h, line, 32);
        same->t += c->t;
        same->f += c->f;
    }
    return same;
}

struct args {
    int max_Ns;
    int kmer_size;
};

int main(int argc, char *argv[]) {
    struct readset **sets;
    if(argc > 1) {
        sets = read_sets_file(argv[1]);
    }
    double **sim_mat;
    int i = 0;
    int j = 0;
    int k = 0;
    puts("");
    puts("");
    puts("");
    while(sets[++i]);
    int num_sets = i;
    for(i = 0; i < num_sets; i++) {
        struct hash *h = hash_create();
        struct readset *index_set = sets[i];
        char **index_filenames = index_set->filenames;
        k = 0;
        while(index_filenames[k]) {
            index_reads_from_file(h, index_filenames[k]);
            k++;
        }
        for(j = i + 1; j < num_sets; j++) {
            struct readset *search_set = sets[j];
            char **search_filenames = search_set->filenames;
            printf("Searching %s in %s\n", search_set->name, index_set->name);
            k = 0;
            struct counter *totals = calloc(1, sizeof(struct counter));
            while(search_filenames[k]) {
                struct counter *counts = search_reads_from_file(h, search_filenames[k]);
                totals->t += counts->t;
                totals->f += counts->f;
                k++;
            }
            double similarity = (double)totals->t /
                                (totals->t + totals->f);
            printf("Similarity: %f%%\n", 100.0 * similarity);
        }
        hash_destroy(h);
    }
    return 0;
}
