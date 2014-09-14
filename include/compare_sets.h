#ifndef SIMILARITY_MATRIX_H
#define SIMILARITY_MATRIX_H

#include <math.h>
#include "counter.h"
#include "bit_vector.h"
#include "commet_job.h"
#include "filter_reads.h"
#include "shame.h"
#include "gnuplot_location.h"

#define VERIFY_NONEMPTY(arr) if(arr[0] == NULL) {\
    fprintf(stderr, "Array is empty\n");\
    exit(EXIT_FAILURE);\
}

BITVEC **get_filter_bvs(CJOB *settings, READSET *set);
BITVEC ***get_comparison_bvs(CJOB *settings, READSET *set_a, READSET *set_b);
void create_in_set_bvfiles_for_one_file(CJOB *settings, char *sfafname, READSET *set);
COUNTER *compare_two_sets(CJOB *settings, READSET *set_a, READSET *set_b);
void compare_files_within_set(CJOB *settings, READSET *set);
COUNTER ***get_raw_comparison_matrix(CJOB *settings);
void print_comparison_percents(CJOB *settings, COUNTER ***shared);
void draw_circle(FILE *gnuploth, float x, float y, float r, char *color);
float shared_area(float r_a, float r_b, float d);
float get_best_d(float r_a, float r_b, float area_shared);
void make_venn_diagram(CJOB * settings, COUNTER ***shared, int i, int j);
void make_venn_diagrams(CJOB * settings, COUNTER ***shared);
void print_comparison(CJOB *settings, READSET *set_a, READSET *set_b);

#endif
