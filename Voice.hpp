#ifndef _VOICE_HPP
#define _VOICE_HPP 1

#include <numbers> 
#include "Waveform.hpp"

#define SAMPLE_RATE 22050

class Voice
{
private:
    const double twoPi = 2.0 * std::numbers::pi;
    Waveform _waveform;

    double Increment ();

public:
    int _note;
    float _orgFrequency;
    float _frequency;
    double _volume;
    double _phase;
    bool _isDeleted;
    bool _pendingNoteOff;


    Voice (int note, float volume, Waveform waveform = Waveform::Sin);
    float NextSample ();
    float MidiNoteToFrequency (int note);
};

#endif
