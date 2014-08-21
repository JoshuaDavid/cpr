#include "commet_job.h"
#include "bit_vector.h"


uint64_t get_num_kmers(char *fafname, char *bvfname, int kmer_size) {
    FILE *fafp; // Fasta file pointer
    fafp = fopen(fafname, "r");
    int fafd = fileno(fafp);
    struct stat st_fa;
    // Die on error.
    assert(0 == fstat(fafd, &st_fa));
    off_t size_fa = st_fa.st_size;
    struct bit_vector *bv = read_bit_vector(bvfname);
    
    char read[4096];
    uint64_t current_read = 0;
    uint64_t num_kmers = 0;
    while(NULL != (fgets(read, sizeof(read), fafp))) {
        if(read[0] == '>') {
            // This is the name of the read, and we don't care.
        } else {
            if(bv_get(bv, current_read)) {
                int len = strlen(read);
                if(len >= kmer_size) {
                    num_kmers += len - kmer_size + 1;
                }
            }
        }
    };
    return num_kmers;
}

int main(int argc, char **argv) {
    struct commet_job *settings = get_settings(argc, argv);
    char *fafname = "1427.1.1377.10k.fasta";
    char bvfname[4096];
    sprintf(bvfname, "output_commet/%s.bv", fafname);
    uint64_t num_kmers = get_num_kmers(settings, fafname, bvfname);
    printf("%lli\n", num_kmers);
}
