#include <math.h>
#include <stddef.h>
#include "voice.h"
#include "globals.h"

Voice voices[VOICE_MAX];
volatile int voice_active = 0;
volatile float voice_pitch = VOICE_PITCH;
volatile float voice_pitchbend = 0.0f;

Voice *voice_get () {
  int i;
  for (i = 0; i < VOICE_MAX; i++) {
    if (voices[i].active == 0)
      return &voices[i];
  }
  /* no non active voice found. we use voice 0! */
  voice_active--;
  return &voices[0];
}

Voice *voice_find_by_note (const unsigned char *note) {
  int i;
  for (i = 0; i < VOICE_MAX; i++) {
    if ((voices[i].active == 1) && voices[i].note == *note)
      return &voices[i];
  }
  return NULL;
}

float voice_midi2freq (const unsigned char *note) {
  return voice_pitch * powf(2.0f, (*note - 69) / 12.0f);
}

void voice_increment (Voice * v) {
  v->phase += (
    v->freq * powf(2.0f, voice_pitchbend * VOICE_PITCHBEND_RANGE)
  ) / g_sampleRate;

  if (v->phase >= 1.0f)
  {
    v->phase -= 1.0f;
  }
}
