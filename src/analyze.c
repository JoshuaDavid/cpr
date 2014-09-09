#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "compare_sets.h"
#include "shame.h"

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    COUNTER ***shared = get_raw_comparison_matrix(settings);
    print_comparison_percents(settings, shared);
    return EXIT_SUCCESS;
}
