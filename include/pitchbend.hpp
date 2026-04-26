#ifndef __PITCHBEND_HPP
#define __PITCHBEND_HPP 1

#include <stdint.h>

#define PITCH_POTI          A0
#define PITCH_LED           A1

extern int16_t pitchbend;
extern int16_t poti_pitch;
extern int16_t poti_pitch_old;

uint32_t pitchbend_incr (uint32_t freqX100);
void pitchbend_refresh (void);

#endif