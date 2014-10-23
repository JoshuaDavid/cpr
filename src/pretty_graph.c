#include "shame.h"
#include "bit_vector.h"

struct distance_matrix {
    char **names;
    int size;
    float **distances;
};

#define DM struct distance_matrix;

DM *dm_read(char *input, char sepchar) {
    int num_rows = 0;
    int num_cols = 0;
    int i = 0;
    while(input[i] != '\0') {
        while(input[i] != '\n') {
            while(input[i] != sepchar) {
                i++;
            }
            i++;
        }
        i++;
    }
}

