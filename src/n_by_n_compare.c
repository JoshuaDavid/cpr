#include "shame.h"
#include "index_and_search.h"

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    if(DEBUG_LEVEL >= 1) print_commet_job(settings);
    create_all_filter_files(settings, settings->parallelism);
    create_all_search_files(settings, settings->parallelism);
    cjob_destroy(settings);
    return 0;
}
