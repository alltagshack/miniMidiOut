#include <math.h>
#include "noise.h"
#include "voice.h"
#include "pseudo_random.h"
#include "globals.h"

void noise_prepare (Voice * const userData)
{
    float freq = userData->freq;
    float lowcut_freq = 0.5f * freq;
    userData->noise_detail.lowpass_alpha =
        (2.0f * M_PI * lowcut_freq / g_sampleRate) /
        (1.0f + 2.0f * M_PI * lowcut_freq / g_sampleRate);
    
    float c  = 1.0f / tanf(M_PI * freq / g_sampleRate);
    userData->noise_detail.bandmix = 1.0f / (1.0f + c / 0.7f + c * c);
    
    userData->noise_detail.lowpass_weight = fmaxf(0.0f, 1.0f - freq / 2093.0f);
    userData->noise_detail.bandpass_weight = fminf(1.0f, freq / 2093.0f);
    userData->noise_detail.highpass_weight = 1.0f - userData->noise_detail.lowpass_weight;

    /* bass boost */
    float shelf_fc = fminf(NOISE_BASS_BOOST_FREQ, lowcut_freq);
    float shelf_bw = 2.0f * M_PI * shelf_fc / g_sampleRate;
    userData->noise_detail.low_shelf_alpha = shelf_bw / (1.0f + shelf_bw);
    float boost_db = NOISE_BASS_BOOST_VALUE * fmaxf(0.0f, 1.0f - freq / 2093.0f);
    userData->noise_detail.low_shelf_gain = powf(10.0f, boost_db / 20.0f);
}

float noise_filter (Voice * const voice)
{
    float n = pr_float();

    voice->noise_detail.lowpass_state +=
        voice->noise_detail.lowpass_alpha * (n - voice->noise_detail.lowpass_state);

    voice->noise_detail.highpass_state =
        n - voice->noise_detail.lowpass_state;

    voice->noise_detail.bandpass_state = voice->noise_detail.bandmix * (
        n + 2.0f * voice->noise_detail.bandpass_state - voice->noise_detail.highpass_state
    );

    voice->noise_detail.low_shelf_state += voice->noise_detail.low_shelf_alpha * (
        voice->noise_detail.low_shelf_gain * voice->noise_detail.lowpass_state -
        voice->noise_detail.low_shelf_state
    );

    return voice->noise_detail.lowpass_weight * voice->noise_detail.lowpass_state +
        voice->noise_detail.bandpass_weight * voice->noise_detail.bandpass_state +
        voice->noise_detail.highpass_weight * voice->noise_detail.highpass_state + (
        1.0f - (
            voice->noise_detail.lowpass_weight +
            voice->noise_detail.bandpass_weight +
            voice->noise_detail.highpass_weight
        )
        ) * voice->noise_detail.low_shelf_state;
}