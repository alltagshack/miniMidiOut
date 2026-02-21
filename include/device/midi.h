#ifndef __MIDI_H
#define __MIDI_H 1

#include "device.h"

struct midi_parameters {
    unsigned char midi_byte;
    unsigned char running_status;
    unsigned char data;
    int expecting;
};

extern Device dMidi;

void midi_init();

#endif