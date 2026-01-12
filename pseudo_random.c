#include <stdint.h>
#include "pseudo_random.h"

static uint32_t _state = 1;

inline void pr_seed (uint32_t seed) {
  if (seed == 0) seed = 1;
  _state = seed;
}

inline float pr_float (void) {
  uint32_t x = _state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  _state = x;
  return (float)( (int32_t)x ) / (float)INT32_MAX;
}

