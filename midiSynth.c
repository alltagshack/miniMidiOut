#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <portaudio.h>
#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 128
#define MAX_VOICES 16

typedef struct {
    double freq;
    float volume;
    double phase;
    int active;
} Voice;

typedef enum modes_s {
	SINUS,
	SAW,
	SQUARE
} Modus;

static volatile int keepRunning = 1;
static volatile int activeCount = 0;
static volatile Modus mode = SAW;

Voice voices[MAX_VOICES];

void handleSig (int sig) {
	keepRunning = 0;
}

float midi2Freq (unsigned char *note)
{
    return 440.0f * powf(2.0f, (*note - 69) / 12.0f);
}

Voice* getVoiceForNote ()
{
    int i;
    for (i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active == 0)
            return &voices[i];
    }
    return &voices[0];
}

Voice* findVoiceByFreq (float *freq) {
    int i;
    for (i = 0; i < MAX_VOICES; i++) {
        if ((voices[i].active == 1) && voices[i].freq == *freq)
            return &voices[i];
    }
    return NULL;
}

static int audioCallback (
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
){

    float *out = (float*)outputBuffer;
    unsigned int i,j;
    int cnt = activeCount;
    if (cnt < 1) {
        return paContinue;
    }
    for (i = 0; i < framesPerBuffer; ++i) {
        float sample = 0;
        for (j = 0; j < MAX_VOICES; ++j) {
            if (voices[j].active > 0) {
		if (voices[j].active == 2) {
			voices[j].volume -= 0.00001f;
			if (voices[j].volume < 0.01f) {
				voices[j].active = 0;
				activeCount--;
			}
		}
		switch (mode) {
		case SAW:
                	sample += voices[j].volume * (2.0f * voices[j].phase - 1.0f);
			break;
		case SQUARE:
			if (voices[j].phase < 0.5f)
                	    sample -= voices[j].volume;
			else
                	    sample += voices[j].volume;
			break;
		default:
                	sample += voices[j].volume * sinf(2.0f * M_PI * voices[j].phase);
		}
                voices[j].phase += voices[j].freq / SAMPLE_RATE;
		if (voices[j].phase >= 1.0f) {
		    voices[j].phase -= 1.0f;
		}
            }
        }

        *out++ = sample; // left
        *out++ = sample; // right
    }

    return paContinue;
}

void* midiReadThread (void *data)
{
    unsigned char buffer[3];
    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) goto error;
   
    err = Pa_OpenDefaultStream(&stream,
        0,
        2,
        paFloat32,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        audioCallback,
        NULL
    );
    if (err != paNoError) goto error;

    while (keepRunning) {

        if (activeCount < 1 && Pa_IsStreamActive(stream)) {
            activeCount = 0;
            err = Pa_StopStream(stream);
            if (err != paNoError) goto error;
        }

	int n = snd_rawmidi_read((snd_rawmidi_t *)data, buffer, sizeof(buffer));

	if (n > 0) {
            unsigned char status = buffer[0];
            unsigned char note   = buffer[1];
            unsigned char vel    = buffer[2];

            if ((status & 0xF0) == 0x90 && vel > 0) {
                float freq = midi2Freq(&note);
                printf("%.2f Hz (%d)\n", freq, vel);

                Voice *userData = getVoiceForNote();

                activeCount++;
                userData->active = 1;
                userData->phase = 0.0;
                userData->freq = freq;
		if (vel < 70) {
	                userData->volume = 0.2f * ( (float)vel / 70.0f);
		} else if (vel < 90) {
	                userData->volume = 0.2f + 0.5f * ((float)vel - 70.0f) / 20.0f;
		} else {
	                userData->volume = 0.7f + 0.3f * ((float)vel - 90.0f) / 37.0f;
		}

                if (Pa_IsStreamStopped(stream)) {
                    err = Pa_StartStream(stream);
                    if (err != paNoError) goto error;
                }

	    } else if ((status & 0xF0) == 0xB0 && note == 0x00) {

		switch (mode) {
		case SAW:
			mode = SQUARE;
			printf("_||_ SQUARE\n");
			break;
		case SQUARE:
			mode = SINUS;
			printf("~~~~ SINUS\n");
			break;
		default:
			mode = SAW;
			printf("//// SAW\n");
		}

            } else if ((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && vel == 0)) {
                float freq = midi2Freq(&note);
                Voice* v = findVoiceByFreq(&freq);
                if (v) {
                    v->active = 2;
                }
            }
        }

    }
    printf("quitting...\n");

    if (Pa_IsStreamActive(stream)) {
    	err = Pa_StopStream(stream);
   	if (err != paNoError) goto error;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;

    Pa_Terminate();
    return NULL;
error:
    Pa_Terminate();
    fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));
    return NULL;
}

int main (int argc, char * argv[])
{
    snd_rawmidi_t *midi_in;
    pthread_t midi_read_thread;

    if (argc < 3) {
        fprintf(stderr, "example usage: %s hw:2,0,0 [sin|sqr|saw]\n", argv[0]);
        return 1;
    }

    int err = snd_rawmidi_open(
        &midi_in,
        NULL, 
        argv[1],
        SND_RAWMIDI_NONBLOCK
    );
    if (err < 0) {
        fprintf(stderr, "error opening device: %s\n", snd_strerror(err));
        return 1;
    }

    if (argv[2][1] == 'a') {
	    printf("saw\n");
	    mode = SAW;
    } else if (argv[2][1] == 'i') {
	    printf("sinus\n");
	    mode = SINUS;
    } else if (argv[2][1] == 'q') {
	    printf("squere\n");
	    mode = SQUARE;
    }
    signal(SIGINT, handleSig);

    pthread_create(&midi_read_thread, NULL, midiReadThread, midi_in);
    pthread_join(midi_read_thread, NULL);

    snd_rawmidi_close(midi_in);

    return 0;
}
