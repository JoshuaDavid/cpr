#ifndef FILTER_READS_H
#define FILTER_READS_H

#include "shame.h"
#include "debug.h"
#include "commet_job.h"
#include "bit_vector.h"

float shannon_entropy(char *read) {
    float entropy = 0.0;
    float basefreqs[5] = {
        0
    };
    int i;
    int len = strlen(read);
    for(i = 0; i < len; i++) {
        switch(read[i]) {
            case 'a': case 'A': basefreqs[0] += 1.0; break;
            case 'c': case 'C': basefreqs[1] += 1.0; break;
            case 'g': case 'G': basefreqs[2] += 1.0; break;
            case 't': case 'T': basefreqs[3] += 1.0; break;
            default:            basefreqs[4] += 1.0; break;
        }
    }
    for(i = 0; i < 5; i++) {
        basefreqs[i] /= (float) len;
        if(basefreqs[i] > 0) {
            entropy += basefreqs[i] * log(basefreqs[i]) / log(2);
        }
    }
    return fabsf(entropy);
}

int num_ns(char *read) {
    int i;
    int len = strlen(read);
    int num_n = 0;
    for(i = 0; i < len; i++) {
        switch(read[i]) {
            case 'a': case 'A': case 'c': case 'C': break;
            case 'g': case 'G': case 't': case 'T': break;
            default: num_n++; break;
        }
    }
    return num_n;
}

BITVEC *filter_reads(CJOB *settings, char *fafname) {
    FILE *fp;
    fp = fopen(fafname, "r");
    char read[65536];
    uintmax_t num_lines = 0;
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else {
            num_lines++;
        }
    }
    fseek(fp, 0, SEEK_SET);
    BITVEC *bv = bv_create(num_lines);
    uintmax_t readnum = 0;
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else {
            if(strlen(read) < settings->min_length_of_read) {
                DBG(6) printf("READ TOO SHORT: %s\n", read);
                bv_set(bv, readnum, 0);
            } else if(num_ns(read) > settings->max_n_in_read) {
                DBG(6) printf("TOO MANY NS:    %s\n", read);
                bv_set(bv, readnum, 0);
            } else if(shannon_entropy(read) < settings->min_entropy ) {
                DBG(6) printf("LOW ENTROPY:    %s\n", read);
                bv_set(bv, readnum, 0);
            } else {
                DBG(6) printf("ACCEPTING:    %s\n", read);
                bv_set(bv, readnum, 1);
            }
            readnum++;
            if(DEBUG_LEVEL >= 3 && readnum % (num_lines / 100) == 0) {
                printf("Bit vector for %s is %f%% done.\n", fafname,
                        100.0 * (double) readnum / (double) num_lines);
            } else if(DEBUG_LEVEL >= 2 && readnum % (num_lines / 10) == 0) {
                printf("Bit vector for %s is %f%% done.\n", fafname,
                        100.0 * (double) readnum / (double) num_lines);
            }
        }
    }
    return bv;
}

char *get_bvfname_from_one_fafname(CJOB *settings, char *fafname) {
    DBG(3) printf("basenameof(\"%s\")\n", fafname);
    char _bvfname[4096];
    char *bname = basenameof(fafname);
    DBG(2) printf("basenameof(\"%s\") == \"%s\"\n", fafname, bname);
    sprintf(_bvfname, "%s/%s.bv", settings->output_directory, bname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    DBG(2) printf("bvfname == %s\n", bvfname);
    free(bname);
    return bvfname;
}

char *get_bvfname_of_index_and_search(CJOB *settings, char *ifafname, char *sfafname) {
    char _bvfname[4096];
    char *sfabfname = basenameof(sfafname);
    char *ifabfname = basenameof(ifafname);
    sprintf(_bvfname, "%s/%s_in_%s.bv", settings->output_directory, 
            basenameof(sfabfname), basenameof(ifabfname));
    free(sfabfname);
    free(ifabfname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    return bvfname;
}

char *get_bvfname_of_file_in_set(CJOB *settings, char *sfafname, READSET *set) {
    char _bvfname[4096];
    char *sfabfname = basenameof(sfafname);
    sprintf(_bvfname, "%s/%s_in_set_%s.bv", settings->output_directory, 
            basenameof(sfabfname), set->name);
    free(sfabfname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    return bvfname;
}

char *get_bvfname_of_file_not_in_set(CJOB *settings, char *sfafname, READSET *set) {
    char _bvfname[4096];
    char *sfabfname = basenameof(sfafname);
    sprintf(_bvfname, "%s/%s_not_in_set_%s.bv", settings->output_directory, 
            basenameof(sfabfname), set->name);
    free(sfabfname);
    char *bvfname = calloc(strlen(_bvfname) + 1, sizeof(char));
    strcpy(bvfname, _bvfname);
    return bvfname;
}

void create_all_filter_files_serial(CJOB *settings) {
    int i = 0, j = 0;
    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {
            char *fafname = set->filenames[j];
            char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
            BITVEC *bv = filter_reads(settings, fafname);
            bv_save_to_file(bv, bvfname);
        }
    }
}

// Implementation is identical to above serial implementation, except that
// this one is multiprocess. Ideally, I would move this to threads instead
// of processes, as processes are kind of heavy.
void create_all_filter_files_parallel(CJOB *settings) {
    int i = 0, j = 0;

    int num_files = 0, pid_counter = 0;
    for(i = 0; i < settings->num_sets; i++) {
        num_files += settings->sets[i]->num_files;
    }

    pid_t *pids = calloc(num_files, sizeof(pid_t));

    READSET **sets = settings->sets;
    for(i = 0; i < settings->num_sets; i++) {
        READSET *set = sets[i];
        for(j = 0; j < set->num_files; j++) {
            pid_t pid = fork();
            if(pid == -1) {
                perror("Fork failed!\n");
            } 
            if(pid == 0 || pid == -1) {
                char *fafname = set->filenames[j];
                char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
                BITVEC *bv = filter_reads(settings, fafname);
                bv_save_to_file(bv, bvfname);
                exit(EXIT_SUCCESS);
            } else {
                pids[pid_counter] = pid;
                pid_counter++;
            }
        }
    }

    for(i = 0; i < num_files; i++) {
        int status = 0;
        waitpid(pids[i], &status, 0);
        assert(status == 0);
    }
}

void create_all_filter_files(CJOB *settings, int parallel) {
    if(parallel >= 1) {
        create_all_filter_files_parallel(settings);
    } else {
        create_all_filter_files_serial(settings);
    }
}

#endif
