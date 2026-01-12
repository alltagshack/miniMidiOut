#ifndef __NOISE_H
#define __NOISE_H 1

#define NOISE_BASS_BOOST_FREQ  220.0f
#define NOISE_BASS_BOOST_VALUE 8.0f

typedef struct {
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

#endif