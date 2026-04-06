#include <Arduino.h>
#include <stdint.h>
#include "voice.hpp"

Voice voices[VOICE_MAX];
volatile int voice_active_value;

Voice *voice_get () {
    for (uint8_t i = 0; i < VOICE_MAX; i++) {
        if (voices[i].state == VOICE_OFF) {
            return &voices[i];
        }    
    }
    /* no non active voice found. we use the 0 voice! */
    voice_active_value--;
    return &voices[0];
}

Voice *voice_find_by_note (const uint8_t note) {
    int i;
    for (i = 0; i < VOICE_MAX; i++) {
    if ((voices[i].state != VOICE_OFF) && voices[i].note == note)
        return &voices[i];
    }
    return nullptr;
}

void voice_init (Voice *v) {
    v->state = VOICE_OFF;
    v->freqX100 = 0;
    v->phase = 0;
    v->volume = 0;
    v->incr = 0;
    v->hold = -1;
    v->release = VOICE_SUSTAIN_RELEASE;
}

uint32_t voice_get_freq (uint8_t note) {
    if (note > 132) note = 132;
    return pgm_read_dword(&voice_midiFreq[note]);
}