#ifndef __VOICE_H
#define __VOICE_H 1

#include "noise.h"

#define VOICE_MAX             16
#define VOICE_PITCH           440.0f
#define VOICE_PITCHBEND_RANGE (2.0f / 12.0f)

typedef struct Voice_s {
  double freq;
  float volume;
  double phase;
  unsigned int envelope;
  int active;
  Noise noise_detail;
} Voice;

extern Voice voices[VOICE_MAX];
extern volatile int voice_active;
extern volatile float voice_pitch;
extern volatile float voice_pitchbend;

Voice *voice_get ();
Voice *voice_find_by_freq (float *freq);
float voice_midi2freq (unsigned char *note);

#endif