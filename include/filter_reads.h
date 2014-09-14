#ifndef FILTER_READS_H
#define FILTER_READS_H

#include "shame.h"
#include "debug.h"
#include "commet_job.h"
#include "bit_vector.h"

float shannon_entropy(char *read);
int num_ns(char *read);
BITVEC *filter_reads(CJOB *settings, char *fafname);
char *get_bvfname_from_one_fafname(CJOB *settings, char *fafname);
char *get_bvfname_of_index_and_search(CJOB *settings, char *ifafname, char *sfafname);
char *get_bvfname_of_file_in_set(CJOB *settings, char *sfafname, READSET *set);
char *get_bvfname_of_file_not_in_set(CJOB *settings, char *sfafname, READSET *set);
void create_all_filter_files_serial(CJOB *settings);

// Implementation is identical to above serial implementation, except that
// this one is multiprocess. Ideally, I would move this to threads instead
// of processes, as processes are kind of heavy.
void create_all_filter_files_parallel(CJOB *settings);
void create_all_filter_files(CJOB *settings, int parallel);

#endif
