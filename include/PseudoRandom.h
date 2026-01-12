#ifndef __PSEUDO_RANDOM_H
#define __PSEUDO_RANDOM_H 1

#include <stdint.h>

void rngSeed (uint32_t seed);
float rngFloat (void);

#endif