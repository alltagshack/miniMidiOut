#include <math.h>
#include <stddef.h>
#include "Voice.h"

Voice voices[MAX_VOICES];
volatile int activeCount = 0;
volatile float concertPitch = DEFAULT_PITCH;

Voice *getVoiceForNote () {
  int i;
  for (i = 0; i < MAX_VOICES; i++) {
    if (voices[i].active == 0)
      return &voices[i];
  }
  /* no non active voice found. we use voice 0! */
  activeCount--;
  return &voices[0];
}

Voice *findVoiceByFreq (float *freq) {
  int i;
  for (i = 0; i < MAX_VOICES; i++) {
    if ((voices[i].active == 1) && voices[i].freq == *freq)
      return &voices[i];
  }
  return NULL;
}

float midi2Freq (unsigned char *note) {
  return concertPitch * powf(2.0f, (*note - 69) / 12.0f);
}