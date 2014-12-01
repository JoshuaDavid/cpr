#include "shame.h"
#include "bit_vector.h"
#include "commet_job.h"
#include "extract_reads.h"

// Take a fasta file and filter bit vector, write the reads to the file specified by
// ofafname
void extract_reads(char *ifafname, char *fbvfname, char *ofafname) {
    FILE *ifafp = fopen(ifafname, "r");
    FILE *ofafp = NULL;
    if(strcmp(ofafname, "stdout") == 0) {
        ofafp = stdout;
    } else {
        ofafp = fopen(ofafname, "w");
    }
    BITVEC *fbv = bv_read_from_file(fbvfname);
    char read[0x10000] = "";
    char last[0x10000] = "";
    uintmax_t readnum = 0;
    while(NULL != fgets(read, sizeof(read), ifafp)) {
        if(read[0] == '>') {
            strcpy(last, read);
        } else {
            if(bv_get(fbv, readnum)) {
                fwrite(last, sizeof(char), strlen(last), ofafp);
                fwrite(read, sizeof(char), strlen(read), ofafp);
            }
            readnum++;
        }
    }
}
