#include "counter.h"
#include "bit_vector.h"
#include "commet_job.h"
#include "filter_reads.h"
#include "shame.h"
#include "compare_sets.h"
#include <math.h>  // for M_PI

BITVEC **get_filter_bvs(CJOB *settings, READSET *set) {
    BITVEC **bvs = calloc(set->num_files, sizeof(BITVEC *));
    uintmax_t i = 0;
    for(i = 0; i < set->num_files; i++) {
        char *fafname = set->filenames[i];
        char *bvfname = get_bvfname_from_one_fafname(settings, fafname);
        bvs[i] = bv_read_from_file(bvfname);
    }
    return bvs;
}

BITVEC ***get_comparison_bvs(CJOB *settings, READSET *set_a, READSET *set_b) {
    BITVEC ***comparisons = calloc(set_a->num_files, sizeof(BITVEC **));
    uintmax_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {
        comparisons[i] = calloc(set_b->num_files, sizeof(BITVEC *));
        for(j = 0; j < set_b->num_files; j++) {
            char *ifafname = set_a->filenames[i];
            char *sfafname = set_a->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            comparisons[i][j] = bv_read_from_file(bvfname);
        }
    }
    return comparisons;
}

void create_in_set_bvfiles_for_one_file(CJOB *settings, char *sfafname, READSET *set) {
    BITVEC **set_bvs = get_filter_bvs(settings, set);
    int i;
    char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
    BITVEC *sbv = bv_read_from_file(sbvfname);
    BITVEC *acc = bv_create(sbv->num_bits);
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        char *isbvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
        BITVEC *isbv = bv_read_from_file(isbvfname);

        acc = bv_or(acc, isbv);
    }

    char *insetbvfname = get_bvfname_of_file_in_set(settings, sfafname, set);
    char *notinsetbvfname = get_bvfname_of_file_not_in_set(settings, sfafname, set);
    DBG(1) printf("Saving %s\n", insetbvfname);
    DBG(1) printf("Saving %s\n", notinsetbvfname);
    BITVEC *notinsetbv = bv_not(acc);
    bv_save_to_file(acc, insetbvfname);
    bv_save_to_file(notinsetbv, notinsetbvfname);

    // clean up
    free(sbvfname);
    free(insetbvfname);
    free(notinsetbvfname);
    bv_destroy(sbv);
    bv_destroy(acc);
    for(i = 0; i < set->num_files; i++) {
        bv_destroy(set_bvs[i]);
    }
}

COUNTER *compare_two_sets(CJOB *settings, READSET *set_a, READSET *set_b) {
    if(DEBUG_LEVEL >= 2) printf("Comparing sets %s and %s.\n", set_a->name, set_b->name);
    BITVEC **bvs_a = get_filter_bvs(settings, set_a);
    BITVEC **bvs_b = get_filter_bvs(settings, set_b);
    VERIFY_NONEMPTY(bvs_a);
    uintmax_t i = 0, j = 0;
    COUNTER *shared = calloc(1, sizeof(COUNTER));
    for(i = 0; i < set_a->num_files; i++) {
        char *sfafname  = set_a->filenames[i];
        char *sbvfname  = get_bvfname_from_one_fafname(settings, sfafname);
        char *insetbvfname = get_bvfname_of_file_in_set(settings, sfafname, set_b);

        create_in_set_bvfiles_for_one_file(settings, sfafname, set_b);

        BITVEC *sbv  = bv_read_from_file(sbvfname);
        BITVEC *insetbv = bv_read_from_file(insetbvfname);

        uintmax_t insetcount = bv_count_bits(insetbv);
        uintmax_t scount = bv_count_bits(sbv);

        shared->t += insetcount;
        shared->f += scount - insetcount;

        // Clean up
        free(sbvfname);
        free(insetbvfname);
        bv_destroy(sbv);
        bv_destroy(insetbv);
    }
    if(DEBUG_LEVEL >= 1) {
        printf("%s     in %s: %ju\n", set_a->name, set_b->name, shared->t);
        printf("%s NOT in %s: %ju\n", set_a->name, set_b->name, shared->f);
        puts("");
    }
    return shared;
}

void compare_files_within_set(CJOB *settings, READSET *set) {
    int i = 0, j = 0;
    // Bit vectors for each file;
    // Easily parallelizeable
    BITVEC **bvs = get_filter_bvs(settings, set);

    // Get comparison bit vectors
    // Easily parallelizeable
    BITVEC **comparisons[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        BITVEC **row = calloc(set->num_files, sizeof(BITVEC **));
        comparisons[i] = row;
        for(j = 0; j < set->num_files; j++) {
            char *ifafname = set->filenames[i];
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            comparisons[i][j] = bv_read_from_file(bvfname);
        }
    }

    // Calculate similarity between each pair of files
    // Easily parallelizeable
    float *similarity[set->num_files];
    for(i = 0; i < set->num_files; i++) {
        float *row = NULL;
        similarity[i] = calloc(set->num_files, sizeof(float));
        char *ifafname = set->filenames[i];
        uintmax_t num_reads_indexed = bv_count_bits(bvs[i]);
        printf("%s: %ju\n", ifafname, num_reads_indexed);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            // Segfaults here for some reason
            uintmax_t num_index_in_search = bv_count_bits(comparisons[i][j]);
            similarity[i][j] = (float) num_index_in_search ;// (float) num_reads_indexed;
        }
    }

    // Print similarity percents: 
    // Not parallelizeable (and why would you bother?)
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf(";%s", ifafname);
    }
    printf("\n");
    for(i = 0; i < set->num_files; i++) {
        char *ifafname = set->filenames[i];
        printf("%s", ifafname);
        uintmax_t num_reads_indexed = bv_count_bits(bvs[i]);
        for(j = 0; j < set->num_files; j++) {
            char *sfafname = set->filenames[j];
            printf(";%7.0f", similarity[i][j]);
        }
        printf("\n");
    }
}

COUNTER ***get_raw_comparison_matrix(CJOB *settings) {
    int i = 0, j = 0;
    READSET **sets = settings->sets;
    COUNTER ***shared = calloc(settings->num_sets, sizeof(COUNTER **));
    for(i = 0; i < settings->num_sets; i++) {
        shared[i] = calloc(settings->num_sets, sizeof(COUNTER *));
        for(j = 0; j < settings->num_sets; j++) {
            shared[i][j] = compare_two_sets(settings, sets[i], sets[j]);
        }
    }
    return shared;
}

void print_comparison_percents(CJOB *settings, COUNTER ***shared) {
    int i = 0, j = 0;

    if(DEBUG_LEVEL >= 1) {
        printf("Percentage of set on left which appears in set on top\n");
    }
    printf(" ");
    for(i = 0; i < settings->num_sets; i++) {
        printf(";%s", settings->sets[i]->name);
    }
    printf("\n");
    for(i = 0; i < settings->num_sets; i++) {
        printf("%s", settings->sets[i]->name);
        for(j = 0; j < settings->num_sets; j++) {
            printf(";%7f%%", 100.0 * (float) shared[i][j]->t / (float)(shared[i][j]->t + shared[i][j] ->f));
        }
        printf("\n");
    }
}

void draw_circle(FILE *gnuploth, float x, float y, float r, char *color) {
    static int i = 1;
    fprintf(gnuploth, "set angles degrees\n");
    fprintf(gnuploth, "set object %i circle at %f, %f  size %f fc rgb \"%s\"\n",
            i, x, y, r, color);
    fprintf(gnuploth, "set object %i fillstyle transparent solid 0.25 noborder\n",
            i);
    i++;
}

float shared_area(float r_a, float r_b, float d) {
    if(r_a + r_b < d) {
        d = r_a + r_b - 1e-10;
    } else if(d == 0) {
        d = 1e-10;
    }
    float ca = (d * d + r_b * r_b - r_a * r_a) / (2 * d * r_a);
    float cb = (d * d + r_a * r_a - r_b * r_b) / (2 * d * r_b);
    if(ca < -1) ca = -1;
    if(cb < -1) cb = -1;
    if(ca >  1) ca =  1;
    if(cb >  1) cb =  1;
    float aa = r_a * r_a * acosf(ca);
    float ab = r_b * r_b * acosf(cb);
    float k = 0.25 * (-d+r_a+r_b)*(d+r_a-r_b)*(d-r_a+r_b)*(d+r_a+r_b);
    if(k < 0) k = 0;
    float area_shared = aa + ab - sqrtf(k);
    return area_shared;
}

float get_best_d(float r_a, float r_b, float area_shared) {
    float lastdiff = 1e+18; // A very large number
    float d;
    float num_ticks = 500;
    float tick = (r_a + r_b) / num_ticks;
    for(d = num_ticks * tick; d >= 0; d -= tick) {
        float as = shared_area(r_a, r_b, d + tick * tick);
        float diff = fabsf(area_shared - as);
        DBG(3) printf("r_a: %f, r_b: %f, d: %f, as: %f, goal: %f, diff: %f\n",
                r_a, r_b, d, as, area_shared, diff);
        if(diff > lastdiff) {
            return d - tick;
        } else {
            lastdiff = diff;
        }
    }
    return d;
}

void make_venn_diagram(CJOB * settings, COUNTER ***shared, int i, int j) {
    char *colors[8] = {"yellow", "cyan", "orange", "blue", "red", "green", "pink", "gray"};
    FILE *pin = popen(GNUPLOT " ", "w");
    if(pin == NULL) {
        puts("An error occurred when attempting to open gnuplot.");
        puts("Perhaps you do not have gnuplot installed.");
        perror("popen");
    }
    READSET **sets = settings->sets;
    fprintf(pin, "set title \"\"\n",
            sets[i]->name, sets[j]->name);
    fprintf(pin, "unset border\n");
    fprintf(pin, "unset xtics\n");
    fprintf(pin, "unset ytics\n");

    fprintf(pin, "set size ratio -1\n");
    fprintf(pin, "set xrange [-4:4]\n");
    fprintf(pin, "set yrange [-4:4]\n");
    
    DBG(2) {
        printf("%20s: %ji reads total\n", sets[i]->name, shared[i][j]->t + shared[i][j]->f);
        printf("%20s: %ji reads total\n", sets[j]->name, shared[j][i]->t + shared[j][i]->f);
        printf("%20s     in %20s: %ji reads\n", sets[i]->name, sets[j]->name, shared[i][j]->t);
        printf("%20s not in %20s: %ji reads\n", sets[i]->name, sets[j]->name, shared[i][j]->f);
        printf("%20s     in %20s: %ji reads\n", sets[j]->name, sets[i]->name, shared[j][i]->t);
        printf("%20s not in %20s: %ji reads\n", sets[j]->name, sets[i]->name, shared[j][i]->f);
    }

    float shared_ab = (float)(shared[i][j]->t + shared[j][i]->t) / 2.0;
    float total_a =  (float)(shared[i][j]->t + shared[i][j]->f);
    float total_b =  (float)(shared[j][i]->t + shared[j][i]->f);
    float total_ab = total_a + total_b;
    float area_ab = 8.0 * M_PI;
    float area_a = total_a * area_ab / total_ab;
    float area_b = total_b * area_ab / total_ab;
    float r_a = sqrtf(area_a / M_PI);
    float r_b = sqrtf(area_b / M_PI);
    float area_shared = shared_ab * area_ab / total_ab;
    float d = get_best_d(r_a, r_b, area_shared);
    float x_a = -(d) / 2.0;
    float x_b = +(d) / 2.0;

    DBG(2) {
        printf("shared_ab = %f\n", shared_ab);
        printf("total_a = %f\n", total_a);
        printf("total_b = %f\n", total_b);
        printf("total_ab = %f\n", total_ab);
        printf("area_ab = %f\n", area_ab);
        printf("area_a = %f\n", area_a);
        printf("area_b = %f\n", area_b);
        printf("r_a = %f\n", r_a);
        printf("r_b = %f\n", r_b);
        printf("area_shared = %f\n", area_shared);
        printf("d = %f\n", d);
        printf("x_a = %f\n", x_a);
        printf("x_b = %f\n", x_b);
    }

    draw_circle(pin, x_a, 0.0, r_a, colors[i]);
    draw_circle(pin, x_b, 0.0, r_b, colors[j]);

    char *fmt = "set label \"%s\"     at -%f, %f center font \"Arial, 8\"\n";
    char *titlefmt = "set label \"Set %s and Set %s\" at -%f, %f center font \"Arial, 12\"\n";
    fprintf(pin, titlefmt, sets[i]->name, sets[j]->name, 0.0, 3.0);

    fprintf(pin, fmt, sets[i]->name, -2.0, 2.4);
    fprintf(pin, fmt, "not in",      -2.0, 2.2);
    fprintf(pin, fmt, sets[j]->name, -2.0, 2.0);
    char a_not_in_b[32] = {0};
    sprintf(a_not_in_b, "%ji reads", shared[i][j]->f);
    fprintf(pin, fmt, a_not_in_b,    -2.0, 1.6);

    fprintf(pin, fmt, sets[i]->name,  0.0, 2.4);
    fprintf(pin, fmt, "in",           0.0, 2.2);
    fprintf(pin, fmt, sets[j]->name,  0.0, 2.0);
    fprintf(pin, fmt, sets[j]->name,  0.0, 2.0);
    char nshared[32] = {0};
    sprintf(nshared, "%i reads", (int)shared_ab);
    fprintf(pin, fmt, nshared,  0.0, 1.6);

    fprintf(pin, fmt, sets[j]->name,  2.0, 2.4);
    fprintf(pin, fmt, "not in",       2.0, 2.2);
    fprintf(pin, fmt, sets[i]->name,  2.0, 2.0);
    char b_not_in_a[32] = {0};
    sprintf(b_not_in_a, "%ji reads", shared[j][i]->f);
    fprintf(pin, fmt, b_not_in_a,     2.0, 1.6);

    fprintf(pin, "set terminal png size 500,500 enhanced font \"Arial,8\"\n");
    fprintf(pin, "set output \"%s/venn_%s_vs_%s.png\"\n",
            settings->output_directory, sets[i]->name, sets[j]->name);
    fprintf(pin, "plot -4 notitle\n");
    DBG(1) {
        printf("made \"%s/venn_%s_vs_%s.png\"\n",
                settings->output_directory, sets[i]->name, sets[j]->name);
    }
    fflush(pin);
    fclose(pin);
}

void make_venn_diagrams(CJOB * settings, COUNTER ***shared) {
    int i = 0, j = 0;
    for(i = 0; i < settings->num_sets; i++) {
        for(j = 0; j < settings->num_sets; j++) {
            make_venn_diagram(settings, shared, i, j);
        }
    }
}

void print_comparison(CJOB *settings, READSET *set_a, READSET *set_b) {
    BITVEC ***comparisons = get_comparison_bvs(settings, set_a, set_b);
    BITVEC **bvs_a = get_filter_bvs(settings, set_a);
    BITVEC **bvs_b = get_filter_bvs(settings, set_b);
    uintmax_t i = 0, j = 0;
    for(i = 0; i < set_a->num_files; i++) {
        char *sfafname = set_a->filenames[i];
        char *sbvfname = get_bvfname_from_one_fafname(settings, sfafname);
        BITVEC *sbv = bv_read_from_file(sbvfname);
        printf("Counting reads in \"%s\" that occur in any of [\n", sfafname);
        BITVEC *acc = bv_read_from_file(sfafname);
        uintmax_t sbv_count = bv_count_bits(sbv);
        BITVEC *next;
        for(j = 0; j < set_b->num_files; j++) {
            char *ifafname = set_a->filenames[j];
            char *ibvfname = get_bvfname_from_one_fafname(settings, ifafname);
            BITVEC *ibv = bv_read_from_file(ibvfname);
            uintmax_t ibv_count = bv_count_bits(ibv);
            char *bvfname = get_bvfname_of_index_and_search(settings, ifafname, sfafname);
            BITVEC *intersection = bv_read_from_file(bvfname);
            next = bv_or(acc, intersection);
            printf("\n\t\"%s\", ", sfafname);
            fflush(NULL);
            uintmax_t acc_count = bv_count_bits(acc);
            uintmax_t int_count = bv_count_bits(intersection);
            uintmax_t next_count = bv_count_bits(next);
            acc = next;
            bv_destroy(acc); // Fix memory leak
            printf("\n%s(%ju) IN %s(%ju): %ju\n", ibvfname, ibv_count, sbvfname, sbv_count, int_count);
        }
        printf("\n]\n");
    }
}
