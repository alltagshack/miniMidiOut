#include <Arduino.h>
#include <stdint.h>
#include "voice.hpp"
#include "sustain.hpp"
#include "pitchbend.hpp"
#include "modulation.hpp"
#include "octave.hpp"

Voice voices[VOICE_MAX];
volatile int voice_active_value;

Voice *voice_new () {
    Voice *v = &voices[0];
    bool isFound = false;
    for (uint8_t i = 0; i < VOICE_MAX; i++) {
        if (voices[i].state == VOICE_OFF) {
            v = &voices[i];
            isFound = true;
        } else {
            voices[i].volume = ((float)voices[i].volume) * 0.85f;
        }
    }
    /* no non active voice found. we use the 0 voice! */
    if (isFound) voice_active_value--;
    return v;
}

void voice_off (const uint8_t note) {
    int i;
    for (i = 0; i < VOICE_MAX; i++) {
        if ((voices[i].state != VOICE_OFF) && voices[i].note == note) {
            noInterrupts();
            voices[i].hold = 0;
            voices[i].release = sustain_on? VOICE_SUSTAIN_RELEASE : VOICE_FAST_RELEASE;
            interrupts();
        }
    }
}

void voice_init (Voice *v, uint8_t note, uint8_t velocity) {
    v->state = VOICE_OFF;
    v->freqX100 = voice_get_freq(note + octave_pitch);
    v->incr = pitchbend_incr(v->freqX100);
    v->note = note;
    v->phase = 0;
    v->cutoff = modulation_cutoff();
    v->hold = 4*SAMPLE_RATE;
    v->volume = ((uint32_t)velocity)<<10;
    v->release = VOICE_SUSTAIN_RELEASE;
}

uint32_t voice_get_freq (uint8_t note) {
    if (note > 132) note = 132;
    return pgm_read_dword(&voice_midiFreq[note]);
}