#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

struct commet_job {
    char *config_file;
    char *output_directory;     /* -o */
    int kmer_size;              /* -k */
    float min_entropy;          /* -e */
    int min_shared_kmers;       /* -t */
    int min_length_of_read;     /* -l */
    int max_n_in_read;          /* -n */
    uint64_t max_reads_in_set;  /* -m */
};

struct commet_job *default_commet_job(void) {
    struct commet_job *settings = calloc(1, sizeof(struct commet_job));
    settings->output_directory = "./output_commet";
    settings->kmer_size = 33;
    settings->min_entropy = 0.0;
    settings->min_shared_kmers = 2;
    settings->min_length_of_read = 2;
    settings->max_n_in_read = 2;
    settings->max_reads_in_set = 0xFFFFFFFFFFFFFFFF;
    return settings;
}

void print_settings(struct commet_job *settings) {
    printf("settings->output_directory   = \"%s\";\n",
            settings->output_directory);
    printf("settings->kmer_size          = %i;\n",
            settings->kmer_size);
    printf("settings->min_entropy        = %f;\n",
            settings->min_entropy);
    printf("settings->min_shared_kmers   = %i;\n",
            settings->min_shared_kmers);
    printf("settings->min_length_of_read = %i;\n",
            settings->min_length_of_read);
    printf("settings->max_n_in_read      = %i;\n",
            settings->max_n_in_read);
    printf("settings->max_reads_in_set   = %llu;\n",
            settings->max_reads_in_set);
}

struct commet_job *get_settings(int argc, char **argv) {
    struct commet_job *settings = default_commet_job();
    char c;
    while((c = getopt(argc, argv, "e:l:k:m:o:n:t:h")) != -1) {
        switch(c) {
            case 'e':
                sscanf(optarg, "%f", &(settings->min_entropy));
                break;
            case 'l':
                sscanf(optarg, "%i", &(settings->min_length_of_read));
                break;
        }
    }
    return settings;
}
