#include <cmath>
#include "Waveform.hpp"
#include "Voice.hpp"

double Voice::Increment ()
{
    return twoPi * _frequency / SAMPLE_RATE;
}

Voice::Voice (int note, float volume, Waveform waveform)
{
    _waveform = waveform;
    _note = note;
    _phase = 0.0;
    _volume = volume;
    _isDeleted = false;
    _pendingNoteOff = false;

    _frequency = MidiNoteToFrequency(note);
    _orgFrequency = _frequency;
}

float Voice::NextSample ()
{
    float sample = 0.0f;
    double phaseNorm = _phase / twoPi; // 0..1

    switch (_waveform)
    {
        case Waveform::Saw:
            sample = (2.0 * phaseNorm - 1.0) * _volume;
            break;

        case Waveform::Square:
            sample = (phaseNorm < 0.5) ? _volume : -_volume;
            break;

        case Waveform::Triangle:
            sample = (1.0 - 4.0 * std::abs(phaseNorm - 0.5)) * _volume;
            break;

        case Waveform::Sin:
        default:
            sample = std::sinf(_phase) * _volume;
            break;
    }

    _phase += Increment();
    if (_phase >= twoPi) _phase -= twoPi;

    return sample;
}

float Voice::MidiNoteToFrequency (int note)
{
    return 440.0f * std::powf(2.0, (note - 69) / 12.0);
}


