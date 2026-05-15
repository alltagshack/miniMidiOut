#include <signal.h>
#include <cstdio>
#include "Waveform.hpp"
#include "SynthApp.hpp"

bool keepRunning = true;

void handleSig (int sig) {
    keepRunning = false;
}

int main (int argc, char * argv[])
{
    Waveform wf;
    
    if (argc < 3) {
        printf("usage: %s midiDevice waveform\n", argv[0]);
        printf(" - midiDevice example: /dev/midi2\n");
        printf(" - waveform: s=sin, a=saw, t=triangle, q=square\n");
        printf(" - CTRL+C to exit\n");
        return 1;
    }
    
    switch(argv[2][0])
    {
        case 's':
            wf = Waveform::Sin;
            break;
        case 'a':
            wf = Waveform::Saw;
            break;
        case 't':
            wf = Waveform::Triangle;
            break;
        case 'q':
        default:
            wf = Waveform::Square;
    }
    
    auto app = new SynthApp(argv[1], wf);
    signal(SIGINT, handleSig);

    app->Run(keepRunning);

    delete app;
    return 0;
}
