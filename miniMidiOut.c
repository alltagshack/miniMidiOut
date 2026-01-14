#include <alsa/asoundlib.h>
#include <math.h>
#include <portaudio.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SAMPLE_RATE 44100
#define MAX_VOICES 16

#define WINDOW_MS 1500U
#define NEEDED    3U

typedef struct {
    double freq;
    float volume;
    double phase;
    int active;
} Voice;

typedef enum modes_s { SINUS, SAW, SQUARE, TRIANGLE } Modus;

static volatile int keepRunning = 1;
static volatile int activeCount = 0;
static volatile int sustain = 0;
int bufferSize = 16;
int outputDeviceId = -1;

static volatile Modus mode = SAW;

Voice voices[MAX_VOICES];

void handleSig (int sig) { keepRunning = 0; }

float midi2Freq (unsigned char * const note) {
    return 440.0f * powf(2.0f, (*note - 69) / 12.0f);
}

static void waiting (unsigned int ms)
{
    struct timespec req = {0};
    req.tv_sec  = 0;
    req.tv_nsec = ms * 1000000;
    nanosleep(&req, NULL);
}

static unsigned int now (void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    /* (seconds * 1000) + (nanoseconds / 1 000 000) a 32‑Bit‑Overflow is ok */
    return (unsigned int)(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);
}

Voice *getVoiceForNote () {
    int i;
    for (i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active == 0) return &voices[i];
    }
    /* no non active voice found. we use voice 0! */
    activeCount--;
    return &voices[0];
}

Voice *findVoiceByFreq (float * const freq) {
    int i;
    for (i = 0; i < MAX_VOICES; i++) {
        if ((voices[i].active == 1) && voices[i].freq == *freq)
            return &voices[i];
    }
    return NULL;
}

static int audioCallback (const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags, void *userData) {

  float sample, tri;
  unsigned int i, j;
  float *out = (float *)outputBuffer;
  
  if (activeCount < 1) {
    return paContinue;
  }
  for (i = 0; i < framesPerBuffer; ++i) {
    sample = 0;
    for (j = 0; j < MAX_VOICES; ++j) {
      if (voices[j].active > 0) {
        if (voices[j].active == 2) {
          if (sustain == 1) {
            voices[j].volume -= 0.000003f;
          } else {
            voices[j].volume -= 0.00015f;
          }
          if (voices[j].volume < 0.001f) {
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
        case TRIANGLE:
          if (voices[j].phase < 0.5f)
            tri = 4.0f * voices[j].phase - 1.0f;
          else
            tri = -4.0f * voices[j].phase + 3.0f;
          sample += voices[j].volume * tri;
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

    *out++ = sample; /* left */
    *out++ = sample; /* right */
  }

  return paContinue;
}

void switchMode (char m) {
    if (m == '\0') {
        if (mode == SAW) m = 'q'; /* -> sQuare */
        if (mode == SQUARE) m = 'r'; /* -> tRiangle */
        if (mode == TRIANGLE) m = 'i'; /* -> sInus */
        if (mode == SINUS) m = 'a'; /* -> sAw */
    }

    if (m == 'i') {
        printf("~~~~ sinus\n");
        mode = SINUS;
    } else if (m == 'a') {
        printf("//// saw\n");
        mode = SAW;
    } else if (m == 'q') {
        printf("_||_ square\n");
        mode = SQUARE;
    } else if (m == 'r') {
        printf("/\\/\\ triangle\n");
        mode = TRIANGLE;
    }
}

void *midiReadThread (void *data) {
    unsigned char buffer[3];
    unsigned char status, note, vel;
    PaStream *stream;
    ssize_t n;
    unsigned int count    = 0;
    unsigned int start_ms = 0;
    int old_sustain = 0;
    float freq;
    PaStreamParameters params;
    PaError err;
    Voice *userData;

    err = Pa_Initialize();
    if (err != paNoError) goto error;

    if (outputDeviceId >= 0) {
        params.device = outputDeviceId;
        params.channelCount = 2;
        params.sampleFormat = paFloat32;
        params.suggestedLatency = Pa_GetDeviceInfo(outputDeviceId)->defaultLowOutputLatency;
        params.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
            &stream,
            NULL,
            &params,
            SAMPLE_RATE,
            bufferSize,
            paClipOff,
            audioCallback,
            NULL
        );
    } else {
        err = Pa_OpenDefaultStream(
          &stream, 0, 2, paFloat32, SAMPLE_RATE,
          bufferSize, audioCallback, NULL
        );
    }
    if (err != paNoError) goto error;

    while (keepRunning) {

        if (activeCount < 1 && Pa_IsStreamActive(stream)) {
            activeCount = 0;
            err = Pa_StopStream(stream);
            if (err != paNoError) goto error;
        }

        n = snd_rawmidi_read((snd_rawmidi_t *)data, buffer, 3);

        if (n > 0) {
            status = buffer[0];
            note = buffer[1];
            vel = buffer[2];
            status = status & 0xF0;

            switch (status) {
                case 0x90:
                    if (vel == 0) {
                        freq = midi2Freq(&note);
                        userData = findVoiceByFreq(&freq);
                        if (userData) userData->active = 2;
                    } else {
                        freq = midi2Freq(&note);
                        printf("%.2f Hz (%d)\n", freq, vel);
                        userData = getVoiceForNote();
                        activeCount++;
                        userData->active = 1;
                        userData->phase = 0.0;
                        userData->freq = freq;
                        userData->volume = 0.7f * ((float)vel / 127.0f);

                        if (Pa_IsStreamStopped(stream)) {
                            err = Pa_StartStream(stream);
                            if (err != paNoError) goto error;
                        }
                    }
                    break;
                case 0x80:
                    freq = midi2Freq(&note);
                    userData = findVoiceByFreq(&freq);
                    if (userData) {
                        userData->active = 2;
                    }
                    break;
                case 0xB0:
                    if (note == 0x40) {
                        if (vel == 0x7F) sustain = 1;
                        if (vel == 0x00) sustain = 0;

                        printf("sustain: 0x%X\n", vel);

                        if (old_sustain == 1 && sustain == 0 && Pa_IsStreamStopped(stream)) {
                            if (count == 0) start_ms = now();
                            count++;
                        }

                        if (count > 0 && ( (now() - start_ms ) > WINDOW_MS)) count = 0;

                        if (count >= NEEDED)
                        {
                            count = 0;
                            switchMode('\0');
                        }

                        old_sustain = sustain;
                    }
                    break;
                default:
                    break;
            }

        } else if (n == -ENODEV || n == -EIO || n == -EPIPE || n == -EBADFD) {
            goto out;
        }
    }
    printf("quitting...\n");

out:
    snd_rawmidi_drop((snd_rawmidi_t *)data);
    
    if (Pa_IsStreamActive(stream)) {
        err = Pa_StopStream(stream);
        if (err != paNoError) goto error;
    }

    err = Pa_CloseStream(stream);

error:
    Pa_Terminate();
    if (err != paNoError)
        fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));
    return NULL;
}

int main (int argc, char *argv[])
{
    snd_rawmidi_t *midi_in;
    pthread_t midi_read_thread;
    int err;

    if (argc < 3) {
        fprintf(stderr, "example usage:\n\t%s hw:2,0,0 sin|saw|sqr|tri [outputDeviceId] [bufferSize]\n", argv[0]);
        return 1;
    }
    if (argc >= 4) {
        outputDeviceId = atoi(argv[3]);
    }
    if (argc == 5) {
        bufferSize = atoi(argv[4]);
    }

    switchMode(argv[2][1]);
    signal(SIGINT, handleSig);
    
    while (keepRunning)
    {
        midi_in = NULL;
        err = snd_rawmidi_open(&midi_in, NULL, argv[1], SND_RAWMIDI_NONBLOCK);
        if (err < 0)
        {
            waiting(500);
            continue;
        }

        pthread_create(&midi_read_thread, NULL, midiReadThread, midi_in);
        pthread_join(midi_read_thread, NULL);

        snd_rawmidi_drop(midi_in);
        snd_rawmidi_close(midi_in);

        waiting(200);
    }
    
    return 0;
}
