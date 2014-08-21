#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hash2.h"
#include "counter.h"
#include "filter_reads.h"


uint64_t kmer_mask(int kmer_size) {
    uint64_t mask = (1ll << (uint64_t)kmer_size) - 1;
    return mask;
}

int file_exists(char *fname) {
    struct stat st;
    return 0 == stat(fname, &st);
}

uint64_t get_num_kmers(char *fafname, char *bvfname, int kmer_size) {
    if(DEBUG_LEVEL >= 1) printf("Counting kmers in %s.\n", fafname);
    FILE *fafp; // Fasta file pointer
    fafp = fopen(fafname, "r");
    struct bit_vector *bv;
    bv = read_bit_vector(bvfname);
    
    char read[4096];
    uint64_t current_read = 0;
    uint64_t num_kmers = 0;
    while(NULL != (fgets(read, sizeof(read), fafp))) {
        if(read[0] == '>') {
            // This is the name of the read, and we don't care.
        } else {
            if(bv_get(bv, current_read)) {
                int len = strlen(read);
                if(len >= kmer_size) {
                    num_kmers += len - kmer_size + 1;
                }
            }
        }
    };
    if(DEBUG_LEVEL >= 1) printf("Done counting kmers in %s (%lli total).\n", fafname, num_kmers);
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
        uint64_t bh = hash_of_base(b);
        value <<= 1;
        value ^= bh;

    }
    value &= mask;
    return value;
}

void index_read_in_hash(struct hash *h, char* read, int kmer_size) {
    uint64_t mask = kmer_mask(kmer_size);
    int len = strlen(read);
    int i;
    if(len < kmer_size) return;
    uint64_t value = hash_of_kmer(read, kmer_size);
    uint64_t bh;
    for(i = 1; i < len - kmer_size - 1; i++) {
        bh = hash_of_base(read[i + kmer_size - 1]);
        value <<= 1;
        value ^= bh;
        value &= mask;
        hash_insert(h, value);
    }
}

void index_file(struct commet_job *settings, struct hash *h, 
        char *fafname, char * bvfname) {
    if(DEBUG_LEVEL >= 1) printf("Indexing reads in %s.\n", fafname);
    FILE *fp;
    fp = fopen(fafname, "r");
    char read[65536];
    int readnum = 0;
    struct bit_vector *bv = read_bit_vector(bvfname);
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of file, ignore
        } else {
            if(bv_get(bv, readnum)) {
                index_read_in_hash(h, read, settings->kmer_size);
            }
            readnum++;
        }
    }
    if(DEBUG_LEVEL >= 1) printf("Done indexing reads in %s.\n", fafname);
    return;
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

struct bit_vector *search_file(struct commet_job *settings, struct hash *h,
        char *fafname, char * bvfname) {
    if(DEBUG_LEVEL >= 1) printf("Searching reads in %s.\n", fafname);
    FILE *fp;
    fp = fopen(fafname, "r");
    char read[65536];
    uint64_t num_lines = 0;
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else {
            num_lines++;
        }
    }
    fseek(fp, 0, SEEK_SET);
    struct bit_vector *sbv = create_bit_vector(num_lines);
    int readnum = 0;
    struct bit_vector *bv = read_bit_vector(bvfname);
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else {
            if(bv_get(bv, readnum)) {
                struct counter *c = search_seq_in_hash(h, read, settings->kmer_size);
                if(c->t >= settings->min_shared_kmers) {
                    bv_set(sbv, readnum, 1);
                } else {
                    bv_set(sbv, readnum, 0);
                }
            }
            readnum++;
        }
    }
    if(DEBUG_LEVEL >= 1) printf("Done searching reads in %s.\n", fafname);
    return sbv;
}

int main(int argc, char **argv) {
    struct commet_job *settings = get_settings(argc, argv);
    if(DEBUG_LEVEL >= 1) print_commet_job(settings);
    int i = 0;
    int j = 0;
    int k = 0;
    int num_files = 0;
    for(i = 0; i < settings->num_sets; i++) {
        num_files += settings->sets[i]->num_files;
    }
    char **fafnames = calloc(num_files, sizeof(char *));
    char **bvfnames = calloc(num_files, sizeof(char *));
    for(i = 0; i < settings->num_sets; i++) {
        struct readset *set = settings->sets[i];
        for(j = 0; j < set->num_files; j++) {
            char *fafname = set->filenames[j];
            char bvfname[4096];
            sprintf(bvfname, "%s/%s.bv", settings->output_directory, fafname);
            fafnames[k] = fafname;
            bvfnames[k] = bvfname;
            if(DEBUG_LEVEL >= 2) printf("bvfnames[%i] = %s\n", k, bvfnames[k]);
            k++;
        }
    }
    pid_t *pids_i = calloc(num_files, sizeof(pid_t));
    for(i = 0; i < num_files; i++) {
        pid_t pid_i;
        pid_i = fork();
        if(pid_i == -1) {
            perror("Fork failed.");
            exit(1);
        } else if(pid_i == 0) {
            if(DEBUG_LEVEL >= 2) printf("bvfnames[%i] = %s\n", i, bvfnames[i]);
            if(file_exists(bvfnames[i])) {
                printf("Using existing file %s.\n", bvfnames[i]);
            } else {
                printf("Creating new file %s.\n", bvfnames[i]);
                struct bit_vector *bv = filter_reads(settings, fafnames[i]);
                bv_save_to_file(bv, bvfnames[i]);
                printf("Created  new file %s.\n", bvfnames[i]);
            }
            exit(EXIT_SUCCESS);
        } else {
            if(DEBUG_LEVEL >= 2) printf("I am the parent.\n");
            pids_i[i] = pid_i;
            int status;
            waitpid(pids_i[i], &status, 0);
        }
    }
    for(i = 0; i < num_files; i++) {
        int status;
        waitpid(pids_i[i], &status, 0);
    }
    exit(0xEDEAD);
    pids_i = calloc(num_files, sizeof(pid_t));
    for(i = 0; i < num_files; i++) {
        pid_t pid_i;
        pid_i = fork();
        if(pid_i == -1) {
            perror("Fork failed.");
            exit(1);
        } else if(pid_i == 0) {
            uint64_t num_kmers = get_num_kmers( fafnames[i], bvfnames[i], settings->kmer_size); 
            struct hash *h = hash_create(num_kmers);
            index_file(settings, h, fafnames[i], bvfnames[i]);
            pid_t *pids_j = calloc(num_files, sizeof(pid_t));
            for(j = 0; j < num_files; j++) {
                pid_t pid_j;
                pid_j = fork();
                if(pid_j == -1) {
                    perror("Fork failed.");
                    exit(1);
                } else if(pid_j == 0) {
                    // Is child process
                    struct bit_vector *sbv = search_file(settings, h, fafnames[j], bvfnames[j]);
                    char outfilename[4096];
                    sprintf(outfilename, "%s/%s_in_%s.bv", settings->output_directory, fafnames[j], fafnames[i]);
                    bv_save_to_file(sbv, outfilename);
                    if(DEBUG_LEVEL >= 1) printf("Wrote to %s\n", outfilename);
                    exit(EXIT_SUCCESS);
                } else {
                    pids_j[j] = pid_j;
                }
            }
            for(j = 0; j < num_files; j++) {
                int status;
                waitpid(pids_j[j], &status, 0);
            }
            hash_destroy(h);
            exit(EXIT_SUCCESS);
        } else {
            pids_i[i] = pid_i;
            if(!settings->ridiculous_parallelism) {
                // This option allows small datasets to run
                // quickly, but probably shouldn't be used
                // for large datasets.
                int status;
                waitpid(pid_i, &status, 0);
            }
        }
    }
    for(i = 0; i < num_files; i++) {
        int status;
        waitpid(pids_i[i], &status, 0);
    }
    return 0;
}
