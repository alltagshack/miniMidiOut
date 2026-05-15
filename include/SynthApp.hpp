#ifndef _SYNTH_APP_HPP
#define _SYNTH_APP_HPP 1

#include <alsa/asoundlib.h>
#include <portaudio.h>

#include "Waveform.hpp"
#include "Voice.hpp"
#include "RecycleList.hpp"

#define FRAMES_PER_BUFFER 128

class SynthApp
{

private:
    static const double _fade;
    static RecycleList<Voice> Voices;

    snd_rawmidi_t *_selectedMidiIn;
    PaStream *_output;
    
    Waveform _currentWaveform;
    bool _sustainPedal;

    void NoteOn (int noteNumber, int velocity);
    void NoteOff (int noteNumber);
    bool MidiMessageReceived ();

public:
    SynthApp (char *midiDev, Waveform waveform);
    virtual ~SynthApp ();
    
    void Run (bool &keepRunning);

    static int Read (
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );
};

#endif
