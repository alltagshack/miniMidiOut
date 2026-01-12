#ifndef __VOICE_H
#define __VOICE_H 1

#include "NoiseDetail.h"

#define MAX_VOICES    16
#define DEFAULT_PITCH 440.0f

typedef struct {
  double freq;
  float volume;
  double phase;
  unsigned int envelope;
  int active;
  NoiseDetail noise_detail;
} Voice;

extern Voice voices[MAX_VOICES];
extern volatile int activeCount;
extern volatile float concertPitch;

Voice *getVoiceForNote ();
Voice *findVoiceByFreq (float *freq);
float midi2Freq (unsigned char *note);

#endif