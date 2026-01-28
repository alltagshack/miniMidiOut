#ifndef __NOISE_H
#define __NOISE_H 1

#define NOISE_BASS_BOOST_FREQ  220.0f
#define NOISE_BASS_BOOST_VALUE 8.0f

struct Voice_s;

typedef struct Noise_s {
  float lowpass_alpha;

  float lowpass_state;
  float bandpass_state;
  float highpass_state;

  float bandmix;

  float lowpass_weight;
  float bandpass_weight;
  float highpass_weight;

  float low_shelf_state;
  float low_shelf_alpha;
  float low_shelf_gain;
} Noise;

void noise_prepare (struct Voice_s * const userData);
float noise_filter (struct Voice_s * const voice);

#endif