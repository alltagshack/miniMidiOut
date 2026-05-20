#include <math.h>
#include <stdio.h>
#include "modus.h"
#include "voice.h"

volatile Modus modus;

void modus_switch (char m)
{
  if (m == '\0') {
    if (modus == SINUS) m = 'a'; /* -> sAw */
    if (modus == SAW) m = 'q'; /* -> sQuare */
    if (modus == SQUARE) m = 'r'; /* -> tRiangle */
    if (modus == TRIANGLE) m = 'o'; /* -> nOise */
    if (modus == NOISE) m = 'i'; /* -> sInus */
  }

  if (m == 'i') {
    printf("~~~~ sinus\n");
    modus = SINUS;
  } else if (m == 'a') {
    printf("//// saw\n");
    modus = SAW;
  } else if (m == 'q') {
    printf("_||_ square\n");
    modus = SQUARE;
  } else if (m == 'r') {
    printf("/\\/\\ triangle\n");
    modus = TRIANGLE;
  } else if (m == 'o') {
    printf("XXXX noise\n");
    modus = NOISE;
  }
}

float modus_nextSample (Voice *v)
{
  float sample = 0.0f;

  switch (modus) {
    case SAW:
      sample += v->volume * (2.0f * v->phase - 1.0f);
      break;
    case SQUARE:
      if (v->phase < 0.5f)
        sample -= v->volume;
      else
        sample += v->volume;
      break;
    case TRIANGLE: {
      float tri;
      if (v->phase < 0.5f)
        tri = 4.0f * v->phase - 1.0f;
      else
        tri = -4.0f * v->phase + 3.0f;
      sample += v->volume * tri;
      break;
    }
    case NOISE: {
      float filtered = noise_filter(v);
      sample += v->volume * filtered;
      break;
    }
    default:
      sample += v->volume * sinf(2.0f * M_PI * v->phase);
  }

  return sample;
}
