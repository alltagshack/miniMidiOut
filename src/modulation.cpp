#include <Arduino.h>

#include <stdint.h>
#include "modulation.hpp"
#include "voice.hpp"

uint16_t modulation_value;
uint16_t modulation_value_old;

uint32_t modulation_cutoff (void)
{
    return 0x24000000UL + modulation_value * 0x00340000UL;
}


void modulation_refresh (void)
{
    uint32_t co = modulation_cutoff();
    for (uint8_t i = 0; i < VOICE_MAX; ++i) {
        if (voices[i].state != VOICE_OFF) {
            voices[i].cutoff = co;
        }
    }
}