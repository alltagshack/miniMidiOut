#ifndef _VOICE_H
#define _VOICE_H 1

#include "NoiseDetail.h"

typedef struct {
  double freq;
  float volume;
  double phase;
  unsigned int envelope;
  int active;
  NoiseDetail noise_detail;
} Voice;

#endif