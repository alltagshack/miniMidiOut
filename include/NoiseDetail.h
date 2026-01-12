#ifndef __NOISE_DETAIL_H
#define __NOISE_DETAIL_H 1

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
} NoiseDetail;

#endif