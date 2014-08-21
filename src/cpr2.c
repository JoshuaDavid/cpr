#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "commet_job.h"
#include "hash.h"
#include "counter.h"
#include "unistd.h"

float entropy_of_read(char *read) {
    // Todo: figure out how this works
    return 2.0;
}

int read_is_valid(struct commet_job *settings, char *read) {
    int len = strlen(read);
    int num_ns = 0;
    int i = 0;
    for(i = 0; i < len; i++) {
        if(read[i] == 'n' || read[i] == 'N') {
            num_ns++;
        }
    }
    float entropy = entropy_of_read(read);
    if(     len     < settings->min_length_of_read
        ||  num_ns  > settings->max_n_in_read
        ||  entropy < settings->min_entropy) {
        return 0;
    } else {
        return 1;
    }
}

uint64_t kmer_mask(int kmer_size) {
    uint64_t mask = (1ll << (uint64_t)kmer_size) - 1;
    return mask;
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

void index_file(struct commet_job *settings, struct hash *h, char *filename) {
    //printf("Indexing file %s\n", filename);
    struct stat st;
    stat(filename, &st);
    uint64_t file_size = st.st_size;
    uint64_t chars_read = 0;
    int tick = 0;
    int ticks = 10;
    FILE *fp;
    fp = fopen(filename, "r");
    char read[65536];
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of file, ignore
        } else if(read_is_valid(settings, read)) {
            index_read_in_hash(h, read, settings->kmer_size);
        }
        chars_read += strlen(read);
        if(chars_read * ticks >= file_size * tick) {
            tick++;
            //printf("Indexing of %s is %f%% done.\n", 
            //    filename,  (100.0 * (double) chars_read) / file_size);
        }
        
    }
    return;
}

struct hash *index_set_parallel(struct commet_job *settings, struct readset *set) {
    struct hash *h = hash_create();
    int i;
    int is_parent = 1;
    pid_t pids[256];
    printf("Indexing set \"%s\".\n", set->name);
    for(i = 0; i < set->num_files; i++) {
        if(is_parent) {
            pid_t pid = fork();
            if(pid == -1) {
                perror("fork failed\n");
            } else if(pid == 0) {
                // child process
                is_parent = 0;
                index_file(settings, h, set->filenames[i]);
                exit(EXIT_SUCCESS);
            } else {
                pids[i] = pid;
            }
        }
    }
    for(i = 0; i < set->num_files; i++) {
        int status = 0;
        waitpid(pids[i], &status, 0);
    }
    return h;
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

// I think this may have race conditions if += is not atomic
struct counter *search_file(struct commet_job *settings, struct hash *h, char *filename) {
    printf("Searching file %s\n", filename);
    FILE *fp;
    fp = fopen(filename, "r");
    struct counter *same = calloc(1, sizeof(struct counter));
    char read[65536];
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else if(read_is_valid(settings, read)) {
            struct counter *c = search_seq_in_hash(h, read, settings->kmer_size);
            if(c->t >= settings->min_shared_kmers) {
                same->t++;
            } else {
                same->f++;
            }
        }
        
    }
    printf("%s, %lld, %lld\n", filename, same->t, same->f);
    return same;
}

struct counter *search_set(struct commet_job *settings, struct hash *h, struct readset *set) {
    int i;
    struct counter *same = calloc(1, sizeof(struct counter));
    printf("Searching set \"%s\".\n", set->name);
    for(i = 0; i < set->num_files; i++) {
        struct counter *c = search_file(settings, h, set->filenames[i]);
        same->t += c->t;
        same->f += c->f;
    }
    return same;
}

int main(int argc, char **argv) {
    struct commet_job *settings = get_settings(argc, argv);
    int i;
    int j;
    for(i = 0; i < settings->num_sets; i++) {
        struct hash *h = index_set_parallel(settings, settings->sets[i]);
        for(j = 0; j < settings->num_sets; j++) {
            struct counter *same = search_set(settings, h, settings->sets[j]);
            printf("%s     in %s: %lld\n", settings->sets[i]->name,
                    settings->sets[j]->name, same->t);
            printf("%s NOT in %s: %lld\n", settings->sets[i]->name,
                    settings->sets[j]->name, same->f);
        }
        //hash_destroy(h);
    }
    return 0;
}
