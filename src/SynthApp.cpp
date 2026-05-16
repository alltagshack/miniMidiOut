#include <cmath>
#include <portaudio.h>

/* open */
#include <fcntl.h>
/* close */
#include <unistd.h>

#include <sys/epoll.h>

#include "Waveform.hpp"
#include "Voice.hpp"
#include "RecycleList.hpp"
#include "SynthApp.hpp"

const double SynthApp::_fade = 0.000001;
RecycleList<Voice> SynthApp::Voices;

void SynthApp::NoteOn (int noteNumber, int velocity)
{
    Voices.Add(noteNumber, velocity / 127.0f * 0.4f, _currentWaveform);
    _sustainCount = 0;
}

void SynthApp::NoteOff (int noteNumber)
{
    auto view = Voices.All([noteNumber](const std::unique_ptr<Voice> &v) {
        return v->_note == noteNumber;
    });
    
    for (const auto &v : view)
    {
        if (_sustainPedal)
        {
            v->_pendingNoteOff = true;
        }
        else
        {
            v->_isDeleted = true;
        }
    }
}

bool SynthApp::MidiMessageReceived ()
{
    bool isNewVoice = false;
    unsigned char buffer[3];
    
    int n = read(_selectedMidiIn, buffer, sizeof(buffer));

    if (n > 0) {
        unsigned char status   = buffer[0];
        unsigned char note     = buffer[1];
        unsigned char velocity = buffer[2];
        
        if (status == 0x90 && velocity > 0)
        {
            NoteOn(note, velocity);
            isNewVoice = true;
        }
        else if (status == 0x80 || (status == 0x90 && velocity == 0))
        {
            NoteOff(note);
        }
        else if (status == 0xB0 && note == 0x40)
        {
            bool newState = velocity >= 64;
            if (newState != _sustainPedal)
            {
                _sustainPedal = newState;

                if (_sustainPedal == false)
                {
                    auto view = Voices.All([](const std::unique_ptr<Voice> &v) {
                        return v->_pendingNoteOff == true;
                    });
                    for (const auto &v : view)
                    {
                        v->_isDeleted = true;
                        v->_pendingNoteOff = false;
                    }
                } else {
                    if (Voices.IsEmpty())
                    {
                        _sustainCount++;
                        if (_sustainCount > 2)
                        {
                            // on silence -> 3x sustain = cycles through waveforms
                            _currentWaveform++;
                            _sustainCount = 0;
                        }
                    } else {
                        _sustainCount = 0;
                    }
                }
            }
        }
        else if (status == 0xE0)
        {
            // relative -1..+1 (center -> 0)
            double rel = (((velocity << 7) | note) - 8192) / (double)8192.0;

            // semitone offset -4 .. +4
            double semitones = rel * 4.0;

            // Frequency factor and new frequency
            float factor = std::powf(2.0, semitones / 12.0);

            for (const auto &v : Voices)
            {
                v->_frequency = v->_orgFrequency * factor;
            }
        }
    }
    return isNewVoice;
}

SynthApp::SynthApp (char *midiDev, Waveform waveform)
{
    _sustainCount = 0;
    _sustainPedal = false;
    _currentWaveform = waveform;
    _selectedMidiIn = open(midiDev, O_RDONLY | O_NONBLOCK);
    
    Pa_Initialize();
    Pa_OpenDefaultStream(
        &_output,
        0,
        2,
        paFloat32,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        SynthApp::Read,
        NULL
    );
}

SynthApp::~SynthApp ()
{
    if (Pa_IsStreamActive(_output)) {
        Pa_StopStream(_output);
    }
    Pa_CloseStream(_output);
    Pa_Terminate();
    
    close(_selectedMidiIn);
}

void SynthApp::Run (bool &keepRunning)
{
    int epoll_fd;
    struct epoll_event *events;
    
    epoll_fd = epoll_create1(0);
    // 1 element = index 0
    events = (struct epoll_event *)calloc(1, sizeof(struct epoll_event));
    events[0].events = EPOLLIN;
    events[0].data.fd = _selectedMidiIn;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _selectedMidiIn, &events[0]);

    Pa_StartStream(_output);

    while (keepRunning)
    {
        epoll_wait(epoll_fd, events, 1, 200);
        MidiMessageReceived();
    }
    
    Pa_StopStream(_output);
    
    free(events);
    close(epoll_fd);
}

int SynthApp::Read (
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
){
    float *out = (float *)outputBuffer;
    
    for (long unsigned int j = 0; j < framesPerBuffer; ++j) {
        float mixedSample = 0.0f;

        for (const auto &v : Voices)
        {
            if (v->_isDeleted == false && v->_volume > 0.00001)
            {
                v->_volume -= _fade;
                mixedSample += v->NextSample();
            }
        }

        if (mixedSample > 1) mixedSample = 1.0f;
        else if (mixedSample < -1) mixedSample = -1.0f;

        *out++ = mixedSample; /* left */
        *out++ = mixedSample; /* right */
    }

    return paContinue;
}
