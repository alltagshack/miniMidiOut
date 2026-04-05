#include <Arduino.h>

#include <stdint.h>
#include "pitchbend.hpp"
#include "voice.hpp"

int16_t pitchbend;

uint32_t pitchbend_incr (uint32_t freqX100)
{
    uint32_t t = 42949673u / SAMPLE_RATE;

    if (freqX100 == 0) return 0;
    if (pitchbend == 0) return freqX100 * t;


    // pitchbend is a value from -8192 to 8191.
    // to map it to -2 halftone till +2: factor range is 0.6674199 to 1.2599210
    // idea: map it from 6675 to 12599 and make all integer!
    // better than x10000: x8192

    if (pitchbend <= 0) {
        freqX100 *= map(pitchbend, -8192, 0, 5467, 8192);
    } else {
        freqX100 *= map(pitchbend, 0, 8191, 8192, 10321);
    }
    freqX100 >>= 13;

    return freqX100 * t;
}