#ifndef COUNTER_H
#define COUNTER_H

#include <stdint.h>

#define COUNTER struct counter

struct counter {
    uintmax_t t;
    uintmax_t f;
};

#endif // COUNTER_H
