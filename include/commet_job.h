#ifndef COMMET_JOB_H
#define COMMET_JOB_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "debug.h"

#define READSET struct readset
#define CJOB struct commet_job

READSET {
    char *name;
    char **filenames;
    int num_files;
    uintmax_t *kmers_if_files;
};

CJOB {
    READSET **sets;
    int num_sets;
    char *output_directory;     /* -o */
    int kmer_size;              /* -k */
    float min_entropy;          /* -e */
    int min_shared_kmers;       /* -t */
    int parallelism; /* -r */
    int min_length_of_read;     /* -l */
    int max_n_in_read;          /* -n */
    uintmax_t max_reads_in_set;  /* -m */
};

READSET **read_sets_file(char *filename);
CJOB *default_commet_job(void);
void print_commet_job(CJOB *settings);
void print_usage(void);
CJOB *get_settings(int argc, char **argv);
void cjob_destroy(CJOB *settings);

#endif
