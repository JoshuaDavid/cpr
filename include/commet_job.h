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

struct readset {
    char *name;
    char **filenames;
    int num_files;
    uint64_t *kmers_if_files;
};

READSET **read_sets_file(char *filename) {
    READSET **sets = calloc(256, sizeof(READSET));
    FILE *fp;
    char line[4096];
    fp = fopen(filename, "r");
    if(fp == NULL) {
        fprintf(stderr, "Could not open file \"%s\" for reading\n", filename);
        exit(EBADF);
    }
    int i = 0;
    int j = 0;
    while(fgets(line, sizeof(line), fp)) {
        // Extract the set name and filenames -- format is as follows
        // set_name: file_1; file_2; file_3; file_4
        // This method is a bit of a beast, but it works.
        j = 0;
        while(line[++j] != ':');
        char *name = calloc(j, sizeof(char));
        strncpy(name, line, j + 1);
        name[j] = '\0';
        READSET *set = calloc(1, sizeof(READSET));
        set->name = name;
        char **filenames = calloc(256, sizeof(char*));
        while(line[j] == ' ' || line[j] == ':') j++;
        int k = j;
        while(line[++k] != '\0');
        char *p = calloc(k - j, sizeof(char));
        strncpy(p, line + j, k - j + 1);
        p[k - j] = '\0';
        int f = 0;
        int newfname = 1;
        int len = strlen(p);
        char *filename = p;
        while(*p) {
            if(*p == ' ' || *p == '\n' || *p == ';') {
                *p = '\0';
            }
            p++;
        }
        newfname = 1;
        for(j = 0; j < len; j++) {
            if(filename[j] == '\0') {
                newfname = 1;
            } else if(newfname) {
                filenames[f] = filename + j;
                f++;
                newfname = 0;
            }
        }
        set->num_files = f;
        set->filenames = filenames;
        sets[i] = set;
        i++;
    }
    int num_sets = i;
    sets[num_sets] = NULL;
    return sets;
}

struct commet_job {
    READSET **sets;
    int num_sets;
    char *output_directory;     /* -o */
    int kmer_size;              /* -k */
    float min_entropy;          /* -e */
    int min_shared_kmers;       /* -t */
    int ridiculous_parallelism; /* -r */
    int min_length_of_read;     /* -l */
    int max_n_in_read;          /* -n */
    uint64_t max_reads_in_set;  /* -m */
};

CJOB *default_commet_job(void) {
    CJOB *settings = calloc(1, sizeof(CJOB));
    settings->output_directory = "./output_commet";
    settings->kmer_size = 33;
    settings->min_entropy = 0.0;
    settings->min_shared_kmers = 2;
    settings->min_length_of_read = 2;
    settings->max_n_in_read = 2;
    settings->max_reads_in_set = 0xFFFFFFFFFFFFFFFF;
    return settings;
}

void print_commet_job(CJOB *settings) {
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
    printf("settings->ridiculous_parallelism = %i;\n",
            settings->ridiculous_parallelism);
    printf("settings->sets               = {\n");
    int i = 0;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = settings->sets[i];
        printf("\t\"%s\": [\n", set->name);
        int j = 0;
        for(j = 0; j < set->num_files; j++) {
            printf("\t\t\"%s\",\n", set->filenames[j]);
        }
        printf("\t],\n");
    }
    printf("}\n");
}

void print_usage(void) {
    printf("./cpr config.txt  ");
    printf("[-o output_directory] ");
    printf("[-k kmer_size] ");
    printf("[-e min_entropy] ");
    printf("[-t min_shared_kmers] ");
    printf("[-l min_length_of_read] ");
    printf("[-n max_n_in_read] ");
    printf("[-m max_reads_in_set]\n");
}

CJOB *get_settings(int argc, char **argv) {
    CJOB *settings = default_commet_job();
    char c;
    while((c = getopt(argc, argv, "m:e:l:k:n:o:rt:h")) != -1) {
        switch(c) {
            case 'e':
                sscanf(optarg, "%f", &(settings->min_entropy));
                break;
            case 'l':
                sscanf(optarg, "%i", &(settings->min_length_of_read));
                break;
            case 'k':
                sscanf(optarg, "%i", &(settings->kmer_size));
                break;
            case 'm':
                sscanf(optarg, "%lli", &(settings->max_reads_in_set));
                break;
            case 'o':
                sscanf(optarg, "%s", settings->output_directory);
                break;
            case 'n':
                sscanf(optarg, "%i", &(settings->max_n_in_read));
                break;
            case 'r':
                settings->ridiculous_parallelism = 1;
                break;
            case 't':
                sscanf(optarg, "%i", &(settings->min_shared_kmers));
                break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            case '?':
                printf("%s", optarg);
                break;
            default:
                printf("%s", optarg);
                break;
        }
    }
    if(argc >= optind) {
        char *config_filename = argv[optind];
        READSET **sets = read_sets_file(config_filename);
        int num_sets = 0;
        // Count the sets
        while(sets[num_sets++]);
        num_sets--;
        settings->sets = sets;
        settings->num_sets = num_sets;
    }

    return settings;
}

void cjob_destroy(CJOB *settings) {
    free(settings);
    settings = NULL;
}

#endif
