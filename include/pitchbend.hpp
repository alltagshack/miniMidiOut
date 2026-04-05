#ifndef __PITCHBEND_HPP
#define __PITCHBEND_HPP 1

#include <stdint.h>

extern int16_t pitchbend;

uint32_t pitchbend_incr (uint32_t freqX100);

#endif