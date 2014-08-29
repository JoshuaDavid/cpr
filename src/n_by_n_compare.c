#include "shame.h"
#include "index_and_search.h"

int main(int argc, char **argv) {
    CJOB *settings = get_settings(argc, argv);
    if(DEBUG_LEVEL >= 1) print_commet_job(settings);
    create_all_filter_files(settings, 0);
    create_all_search_files_serial(settings);
    return 0;
}
