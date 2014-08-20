#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

struct hash *index_set_parallel(struct readset *set) {
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
                printf("index_file(%s)\n", set->filenames[i]);
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


int main(int argc, char **argv) {
    struct commet_job *settings = get_settings(argc, argv);
    print_commet_job(settings);
    index_set_parallel(settings->sets[0]);
}
