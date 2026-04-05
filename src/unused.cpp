#include <Arduino.h>

#include "voice.hpp"
#include "pitchbend.hpp"


const char INPUT_PIN = 0;

void updateControl()
{
    //volume = analogRead(INPUT_PIN) >> 2;
    for (uint8_t i = 0; i < VOICE_MAX; ++i) {
        if (voices[i].state == VOICE_RELEASE) {
            //voices[i].osc.setFreq(pitchbend_apply(voices[i].freq));
            voices[i].volume -= voice_release_value;
            if (voices[i].volume < VOICE_SUSTAIN_RELEASE) {
                //voices[i].osc.setFreq(0);
                voices[i].state = VOICE_OFF;
                voice_active_value--;
            }
        }else if (voices[i].state == VOICE_ON) {
            //voices[i].osc.setFreq(pitchbend_apply(voices[i].freq));
        }
    }
}
