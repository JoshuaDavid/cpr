#ifndef INDEX_AND_SEARCH_H
#define INDEX_AND_SEARCH_H

#include "hash.h"
// #include "counter.h"
#include "filter_reads.h"
// #include "shame.h"

uintmax_t kmer_mask(int kmer_size);
uintmax_t get_num_kmers(CJOB *settings, char *fafname);
uintmax_t hash_of_base(char b);
uintmax_t hash_of_kmer(char *read, int kmer_size);
void index_read_into_hash(CJOB *settings, HASH *h, char* read);
struct counter *search_read_in_hash(CJOB *settings, HASH *h, char* seq);
HASH *index_from_file(CJOB *settings, char *fafname);
BITVEC *search_file_in_hash(CJOB *settings, HASH *h, char *fafname);
void create_search_files_of_one_index_serial(
        CJOB *settings, char *ifafname);
void create_search_files_of_one_index_parallel(
        CJOB *settings, char *ifafname);
void create_search_files_of_one_index(
        CJOB *settings, char *ifafname, int parallel);
void create_all_search_files_serial(CJOB *settings, int parallel);
void create_all_search_files_parallel(CJOB *settings, int parallel);
void create_all_search_files(CJOB *settings, int parallel);

#endif
