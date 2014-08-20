#include "borked.h"

int main(void) {
    struct earr *a = create_earr();
    int i;
    pid_t pid;
    int size = 0x10000;
    if(0 == (pid = fork())) {
        for(i = 0; i < size; i++) {
            earr_insert(a, i);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        for(i = 0; i < size; i++) {
            earr_lookup(a, i);
        }
    }
    return EXIT_SUCCESS;
}
