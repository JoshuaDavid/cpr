#include "shame.h"
#include "index_and_search.h"
#include "counter.h"
#include "filter_reads.h"
#include "easy_pthreads.h"

int unique_collisions(CJOB *settings, HASH *h, char *read);
int total_collisions(CJOB *settings, HASH *h, char *read);
COUNTER *collisions_vs_not(CJOB *settings, HASH *h, int inumreads, char *sfafname);
pj_t *pj_create_collisions_vs_not(CJOB *settings, HASH *h, int inumreads, char *sfafname);
void pj_exec_collisions_vs_not(pj_t *job);
COUNTER *pj_join_collisions_vs_not(pj_t *job);

int unique_collisions(CJOB *settings, HASH *h, char *read) {
    int kmer_size = settings->kmer_size;
    uintmax_t mask = kmer_mask(kmer_size);
    int len = strlen(read);
    int i;
    int same = 0;
    uintmax_t value = hash_of_kmer(read, kmer_size);
    for(i = 1; i < len - kmer_size - 1; i++) {
        value <<= 1;
        value ^= hash_of_base(read[i + kmer_size - 1]);
        value &= mask;
        if(hash_lookup(h, value)) {
            same++;
        }
    }
    return same;
}

int total_collisions(CJOB *settings, HASH *h, char *read) {
    int kmer_size = settings->kmer_size;
    uintmax_t mask = kmer_mask(kmer_size);
    int len = strlen(read);
    int i;
    // Find the kmer which collided most with the one in the index file --
    // this one will be the one we use as the index hash.
    int max_collisions = 0;
    uintmax_t value = hash_of_kmer(read, kmer_size);
    for(i = 1; i < len - kmer_size - 1; i++) {
        value <<= 1;
        value ^= hash_of_base(read[i + kmer_size - 1]);
        value &= mask;
        int collisions = hash_lookup(h, value);
        if(collisions > max_collisions) {
            max_collisions = collisions;
        }
    }
    return max_collisions;
}

COUNTER *collisions_vs_not(CJOB *settings, HASH *h, int inumreads, char *sfafname) {
    // inumreads is the number of reads in the index file that were inserted
    // into HASH h. The number of opportunities for a collision is equal to
    // the product of the number of reads in the index and search files.

    // The name of the search bit_vector filename. (sbvf == search bit vector file)
    char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
    
    // sfafp == search fasta file pointer
    FILE *sfafp = fopen(sfafname, "r");

    // If your reads are longer than 65 kbp, this software is going to throw
    // out garbage results anyway.
    char read[65536];

    // Which read are we on (note that since we're filtering, that's not always
    // going to be the same as the line number)
    int readnum = 0;

    // The number of collisions. For each read in the search file, we determine
    // whether that read also occurs in the index file. Make sure kmer_length
    // is at least 10 + log2(inumreads * snumreads) for best results.
    int num_collisions = 0;

    // The product of the number of reads in the index and search files.
    int num_comparisons = 0;

    // This is our filter file.
    BITVEC *sbv = bv_read_from_file(sbvfname);
    while(NULL != fgets(read, sizeof(read), sfafp)) {
        // Ignore name lines
        if(read[0] != '>') {
            // Only lines that passed filtering step
            if(bv_get(sbv, readnum)) {
                if(unique_collisions(settings, h, read) >= settings->min_shared_kmers) {
                    num_collisions  += inumreads *  otal_collisions(settings, h, read);
                }
                num_comparisons += inumreads;
            }
            readnum++;
        }
    }
    COUNTER *collisions = calloc(1, sizeof(COUNTER));
    collisions->t = num_collisions;
    collisions->f = num_comparisons;

    fclose(sfafp);
    free(sbvfname);

    return collisions;
}

pj_t *pj_create_collisions_vs_not(CJOB *settings, HASH *h, int inumreads, char *sfafname) {
    // Allocate memory for argument and return pointers
    void **args = calloc(4, sizeof(void *));
    void *ret   = calloc(1, sizeof(COUNTER *));

    int *pinumreads = calloc(1, sizeof(int));
    *pinumreads = inumreads;

    args[0] = settings;
    args[1] = h;
    args[2] = pinumreads;
    args[4] = sfafname;
    pj_t *job = pj_create(args, ret, pj_exec_collisions_vs_not);
    return job;
}

void pj_exec_collisions_vs_not(pj_t *job) {
    CJOB *settings  = (CJOB *)job->args[0];
    HASH *h         = (HASH *)job->args[1];
    int inumreads   = *((int *)job->args[2]);
    char *sfafname  = (char *)job->args[3];

    COUNTER *ret = collisions_vs_not(settings, h, inumreads, sfafname);
    job->ret = (void *)ret;
}

COUNTER *pj_join_collisions_vs_not(pj_t *job) {
    pj_join(job);
    return ((struct counter *)job->ret);
}

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    if(DEBUG_LEVEL >= 1) print_commet_job(settings);
    create_all_filter_files(settings, settings->parallelism);
    char *ifafname = settings->sets[0]->filenames[0];
    char *sfafname = settings->sets[0]->filenames[0];
    HASH *h = index_from_file(settings, ifafname);
    int inumreads = (int) count_reads(settings, ifafname);
    int snumreads = (int) count_reads(settings, sfafname);
    printf("inumreads == %i\n", (int)inumreads);
    printf("snumreads == %i\n", (int)snumreads);
    COUNTER *c = collisions_vs_not(settings, h, inumreads, sfafname);
    printf("struct counter {\n\tint t = %i;\n\tint f = %i;\n};\n", (int)c->t, (int)c->f);
    cjob_destroy(settings);
    return EXIT_SUCCESS;
}
