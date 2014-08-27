#include <math.h>
#include <stdint.h>
#include "debug.h"
#include "commet_job.h"
#include "bit_vector.h"

float shannon_entropy(char *read) {
    float entropy = 0.0;
    float basefreqs[5] = {0};
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

struct bit_vector *filter_reads(struct commet_job *settings, char *fafname) {
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
    struct bit_vector *bv = bv_create(num_lines);
    uint64_t readnum = 0;
    while(NULL != fgets(read, sizeof(read), fp)) {
        if(read[0] == '>') {
            // Name of read, ignore
        } else {
            if(  strlen(read) < settings->min_length_of_read
              || num_ns(read) <= settings->max_n_in_read
              || shannon_entropy(read) < settings->min_entropy ) {
                bv_set(bv, readnum, 0);
            } else {
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
