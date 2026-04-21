#ifndef __VOICE_HPP
#define __VOICE_HPP 1

#include <stdint.h>

#define VOICE_FAST_RELEASE         90u
#define VOICE_SUSTAIN_RELEASE       1u

#define VOICE_MAX                   8
#define SAMPLE_RATE              8000u

enum VoiceState {VOICE_OFF, VOICE_ON, VOICE_RELEASE};

typedef struct Voice_s
{
    VoiceState      state;
    uint8_t          note;
    uint8_t       release;
    /// if the value is not -1, how many samples this voice has to play the note
    int32_t          hold;
    uint32_t     freqX100;
    uint32_t       volume;
    uint32_t        phase;
    uint32_t         incr;
} Voice;

const uint32_t voice_midiFreq[133] PROGMEM = {
    817,     866,     917,     972,
   1030,    1091,    1156,    1224,
   1297,    1375,    1456,    1543,
   1635,    1732,    1835,    1944,
   2060,    2182,    2312,    2449,
   2595,    2750,    2913,    3086,
   3270,    3464,    3670,    3889,
   4120,    4365,    4624,    4899,
   5191,    5500,    5827,    6173,
   
   6540,    6929,    7341,    7778,
   8240,    8730,    9249,    9799,
  10382,   11000,   11654,   12347,
  13081,   13859,   14683,   15556,
  16481,   17461,   18499,   19599,
  20765,   22000,   23308,   24694,
  26162,   27718,   29366,   31112,
  32962,   34922,   36999,   39199,
  41530,   44000,   46616,   49388,
  52325,   55436,   58732,   62225,
  65925,   69845,   73998,   78399,
  83060,   88000,   93232,   98776,
 104650,  110873,  117465,  124450,
 131851,  139691,  147997,  156798,
 166121,  176000,  186465,  197553,
 209300,  221746,  234931,  248901,
   
 263702,  279382,  295995,  313596,
 332243,  352000,  372931,  395106,
 418600,  443492,  469863,  497803,
 527404,  558765,  591991,  627192,
 664487,  704000,  745862,  790213,
 837201,  886984,  939727,  995606,
1054808, 1117530, 1183982, 1254385,
1328975, 1408000, 1491724, 1580426,
1674403
};

void voice_init (Voice *v);
Voice *voice_get ();
void voice_off (const uint8_t note);
void voice_volume_refresh (Voice *v);
uint32_t voice_get_freq (uint8_t note);

extern Voice voices[VOICE_MAX];
extern volatile int voice_active_value;

#endif