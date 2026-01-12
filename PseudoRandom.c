#include <stdint.h>
#include "PseudoRandom.h"

static uint32_t rng_state = 1;

inline void rngSeed (uint32_t seed) {
  if (seed == 0) seed = 1;
  rng_state = seed;
}

inline float rngFloat (void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return (float)( (int32_t)x ) / (float)INT32_MAX;
}

