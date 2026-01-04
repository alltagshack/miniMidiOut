#include <alsa/asoundlib.h>
#include <math.h>
#include <portaudio.h>
#include <pthread.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/select.h>

#include "NoiseDetail.h"
#include "Voice.h"

#define SAMPLE_RATE         44100
#define MAX_VOICES          16
#define DEFAULT_BUFFER_SIZE 32
#define DEFAULT_FADING      10

#define DEFAULT_ENVELOPE    16000
#define ENVELOPE_MAX        0.35f

#define SUSTAIN_MODESWITCH_MS 1500U
#define SUSTAIN_NEEDED        3U

#define NOISE_BASS_BOOST_FREQ 220.0f
#define NOISE_BASS_BOOST_VALUE 8.0f

typedef enum modes_s { SINUS, SAW, SQUARE, TRIANGLE, NOISE } Modus;

static volatile Modus mode = SAW;
int bufferSize = DEFAULT_BUFFER_SIZE;
int fading = DEFAULT_FADING;
int autoFading = 0;
unsigned int envelopeSamples = DEFAULT_ENVELOPE;

static volatile int keepRunning = 1;
static volatile int activeCount = 0;
static volatile int sustain = 0;
int outputDeviceId = -1;

Voice voices[MAX_VOICES];

static uint32_t rng_state = 1;

static inline void rngSeed (uint32_t seed) {
  if (seed == 0) seed = 1;
  rng_state = seed;
}

static inline float rngFloat (void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return (float)( (int32_t)x ) / (float)INT32_MAX;
}

void handleSig (int sig) { keepRunning = 0; }

float midi2Freq (unsigned char *note) {
  return 440.0f * powf(2.0f, (*note - 69) / 12.0f);
}

static int openKeyboard (const char *devpath)
{
    int fd = open(devpath, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open keyboard");
        return -1;
    }
    return fd;
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
    if (voices[i].active == 0)
      return &voices[i];
  }
  // no non active voice found. we use voice 0!
  activeCount--;
  return &voices[0];
}

Voice *findVoiceByFreq (float *freq) {
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

  float *out = (float *)outputBuffer;
  unsigned int i, j;
  int cnt = activeCount;
  if (cnt < 1) {
    return paContinue;
  }
  for (i = 0; i < framesPerBuffer; ++i) {
    float sample = 0.0f;
    float env = 1.0f;
    for (j = 0; j < MAX_VOICES; ++j) {
      if (voices[j].active > 0) {

        if (voices[j].envelope != 0 && envelopeSamples != 0) {
          env += ENVELOPE_MAX * ((float)voices[j].envelope / envelopeSamples);
          voices[j].envelope--;
        }

        if (voices[j].active == 2) {
          if (sustain == 1) {
            voices[j].volume -= 0.000002f;
          } else if (sustain == 2) {
            // on release sustain we force a faster silence
            voices[j].volume = 0.0001f;
          } else {
            voices[j].volume -= (float)fading * 0.000005f;
          }
        }

        if (voices[j].volume < 0.001f) {
          voices[j].active = 0;
          activeCount--;
          continue;
        }

        switch (mode) {
          case SAW:
            sample += env * voices[j].volume * (2.0f * voices[j].phase - 1.0f);
            break;
          case SQUARE:
            if (voices[j].phase < 0.5f)
              sample -= env * voices[j].volume;
            else
              sample += env * voices[j].volume;
            break;
          case TRIANGLE: {
            float tri;
            if (voices[j].phase < 0.5f)
              tri = 4.0f * voices[j].phase - 1.0f;
            else
              tri = -4.0f * voices[j].phase + 3.0f;
            sample += env * voices[j].volume * tri;
            break;
          }
          case NOISE: {
            float n = rngFloat();

            voices[j].noise_detail.lowpass_state +=
              voices[j].noise_detail.lowpass_alpha * (n - voices[j].noise_detail.lowpass_state);

            voices[j].noise_detail.highpass_state =
              n - voices[j].noise_detail.lowpass_state;

            voices[j].noise_detail.bandpass_state = voices[j].noise_detail.bandmix * (
              n + 2.0f * voices[j].noise_detail.bandpass_state - voices[j].noise_detail.highpass_state
            );

            voices[j].noise_detail.low_shelf_state += voices[j].noise_detail.low_shelf_alpha * (
              voices[j].noise_detail.low_shelf_gain * voices[j].noise_detail.lowpass_state -
              voices[j].noise_detail.low_shelf_state
            );

            float filtered =
              voices[j].noise_detail.lowpass_weight * voices[j].noise_detail.lowpass_state +
              voices[j].noise_detail.bandpass_weight * voices[j].noise_detail.bandpass_state +
              voices[j].noise_detail.highpass_weight * voices[j].noise_detail.highpass_state + (
                1.0f - (
                  voices[j].noise_detail.lowpass_weight +
                  voices[j].noise_detail.bandpass_weight +
                  voices[j].noise_detail.highpass_weight
                )
              ) * voices[j].noise_detail.low_shelf_state;
            
            sample += env * voices[j].volume * filtered;
            break;
          }
          default:
            sample += env * voices[j].volume * sinf(2.0f * M_PI * voices[j].phase);
        }

        voices[j].phase += voices[j].freq / SAMPLE_RATE;
        if (voices[j].phase >= 1.0f) {
          voices[j].phase -= 1.0f;
        }

      }
    }
    // after all voices are faded out, the release of sustain ends
    if (sustain == 2) sustain = 0;

    *out++ = sample; // left
    *out++ = sample; // right
  }

  return paContinue;
}

void switchMode (char m) {
  if (m == '\0') {
    if (mode == SINUS) m = 'a'; // -> sAw
    if (mode == SAW) m = 'q'; // -> sQuare
    if (mode == SQUARE) m = 'r'; // -> tRiangle
    if (mode == TRIANGLE) m = 'o'; // -> nOise
    if (mode == NOISE) m = 'i'; // -> sInus
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
  } else if (m == 'o') {
    printf("XXXX noise\n");
    mode = NOISE;
  }

}

void *midiReadThread (void *data) {
  unsigned char midi_buf[3];
  PaStream *stream;
  PaError err;
  unsigned int count    = 0;
  unsigned int start_ms = 0;
  unsigned int now_ms = 0;
  int old_sustain = 0;
  PaStreamParameters params;

  float c, lowcut_freq, quality, shelf_fc, shelf_bw, boost_db;

  // @todo keyboard input optional and hotplug!

  const char *kbd_dev = "/dev/input/event0";
  int kbd_fd = openKeyboard(kbd_dev);
  if (kbd_fd < 0) {
    fprintf(stderr, "error opening (USB) keyboard: %s\n", kbd_dev);
    return NULL;
  }

  err = Pa_Initialize();

  if (err != paNoError)
    goto error;

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
  if (err != paNoError)
    goto error;

  while (keepRunning) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(kbd_fd, &rfds);

    int midi_fd = snd_rawmidi_poll_descriptors((snd_rawmidi_t *)data, NULL, 0);
    if (midi_fd >= 0) FD_SET(midi_fd, &rfds);

    int maxfd = (kbd_fd > midi_fd) ? kbd_fd : midi_fd;
    int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
    if (ready < 0) {
      if (errno == EINTR) continue;
      perror("select");
      break;
    }

    if (activeCount < 1 && Pa_IsStreamActive(stream)) {
      activeCount = 0;
      err = Pa_StopStream(stream);
      if (err != paNoError)
        goto error;
    }

    if (FD_ISSET(kbd_fd, &rfds))
    {
      struct input_event ev;
      ssize_t n = read(kbd_fd, &ev, sizeof(ev));
      if (n == sizeof(ev) && ev.type == EV_KEY && ev.value == 1)
      {
          printf("Key pressed: %u\n", ev.code);
          if (ev.code == KEY_I) {
              switchMode('i');
          } else if (ev.code == KEY_A) {
              switchMode('a');
          } else if (ev.code == KEY_Q) {
              switchMode('q');
          } else if (ev.code == KEY_R) {
              switchMode('r');
          } else if (ev.code == KEY_O) {
              switchMode('o');
          }
      }
    }


    if (midi_fd >= 0 && FD_ISSET(midi_fd, &rfds))
    {
      ssize_t n = snd_rawmidi_read((snd_rawmidi_t *)data, midi_buf, sizeof(midi_buf));

      if (n > 0) {
        unsigned char status = midi_buf[0];
        unsigned char note = midi_buf[1];
        unsigned char vel = midi_buf[2];

        //printf("Debug: %x %x %x\n", status, note, vel);

        if ((status & 0xF0) == 0x90 && vel > 0) {
          float freq = midi2Freq(&note);
          printf("%.2f Hz (%d)\n", freq, vel);

          Voice *userData = getVoiceForNote();

          activeCount++;
          userData->active = (autoFading == 1)? 2 : 1;
          userData->phase = 0.0;
          userData->freq = freq;
          userData->volume = 0.7f * ((float)vel / 127.0f);
          userData->envelope = envelopeSamples;

          if (mode == NOISE) {
            lowcut_freq = 0.5f * freq;
            userData->noise_detail.lowpass_alpha =
              (2.0f * M_PI * lowcut_freq / SAMPLE_RATE) /
              (1.0f + 2.0f * M_PI * lowcut_freq / SAMPLE_RATE);
            
            quality = 0.7f;
            c  = 1.0f / tanf(M_PI * freq / SAMPLE_RATE);
            userData->noise_detail.bandmix = 1.0f / (1.0f + c / quality + c * c);
            
            userData->noise_detail.lowpass_weight = fmaxf(0.0f, 1.0f - freq / 2093.0f);
            userData->noise_detail.bandpass_weight = fminf(1.0f, freq / 2093.0f);
            userData->noise_detail.highpass_weight = 1.0f - userData->noise_detail.lowpass_weight;

            // bass boost
            shelf_fc = fminf(NOISE_BASS_BOOST_FREQ, lowcut_freq);
            shelf_bw = 2.0f * M_PI * shelf_fc / SAMPLE_RATE;
            userData->noise_detail.low_shelf_alpha = shelf_bw / (1.0f + shelf_bw);
            boost_db = NOISE_BASS_BOOST_VALUE * fmaxf(0.0f, 1.0f - freq / 2093.0f);
            userData->noise_detail.low_shelf_gain = powf(10.0f, boost_db / 20.0f);
          }

          if (Pa_IsStreamStopped(stream)) {
            err = Pa_StartStream(stream);
            if (err != paNoError)
              goto error;
          }

        } else if ((status & 0xF0) == 0xB0 && note == 0x40) {
          now_ms = now();

          if (vel == 0x7F) sustain = 1;
          if (vel == 0x00) sustain = 2;

          if (old_sustain == 1 && sustain == 2) {
            // release
            sustain = 0;
          }

          if (old_sustain == 1 && (sustain == 0 || sustain == 2) && Pa_IsStreamStopped(stream)) {
            if (count == 0) start_ms = now_ms;
            count++;
          }

          if (count > 0 && ( (now_ms - start_ms ) > SUSTAIN_MODESWITCH_MS)) {
            count = 0;
          }

          printf("old_sustain: 0x%X, sustain: 0x%X, count: %d, activeCount: %d\n",
            old_sustain, sustain, count, activeCount);

          if (count >= SUSTAIN_NEEDED)
          {
            count = 0;
            switchMode('\0');
          }

          old_sustain = sustain;

        } else if ((status & 0xF0) == 0xB0 && note == 0x00) {

          switchMode('\0');

        } else if ((status & 0xF0) == 0x80 ||
                  ((status & 0xF0) == 0x90 && vel == 0)) {
          float freq = midi2Freq(&note);
          Voice *v = findVoiceByFreq(&freq);
          if (v) {
            v->active = 2;
          }
        }
      } else if (n == -ENODEV) {
        break;
      }
    }
  }

  printf("quitting...\n");

  if (Pa_IsStreamActive(stream)) {
    err = Pa_StopStream(stream);
    if (err != paNoError)
      goto error;
  }

  err = Pa_CloseStream(stream);
  if (err != paNoError)
    goto error;

  Pa_Terminate();
  close(kbd_fd);
  return NULL;

error:
  Pa_Terminate();
  close(kbd_fd);
  fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));
  return NULL;
}


int main (int argc, char *argv[])
{
  snd_rawmidi_t *midi_in;
  pthread_t midi_read_thread;
  char m = '\0';
  struct sigaction sa;

  struct timespec req = {0};
  req.tv_sec  = 0;
  req.tv_nsec = 500000000;

  sa.sa_handler = handleSig;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (argc < 2) {
    fprintf(stderr, "example usage:\n");
    fprintf(stderr, "\t%s hw:2,0,0 [sin|saw|sqr|tri|noi]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw [outputDeviceId]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 [bufferSize]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d [fade -20 to 100]\n", argv[0], bufferSize);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d [envelope]\n", argv[0], bufferSize, fading);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d\n", argv[0], bufferSize, fading, envelopeSamples);
    return 1;
  }
  if (argc >= 3) {
    m = argv[2][1];
    switchMode(m);
  }
  if (argc >= 4) {
    outputDeviceId = atoi(argv[3]);
  }
  if (argc >= 5) {
    bufferSize = atoi(argv[4]);
  }
  if (argc == 6) {
    fading = atoi(argv[5]);
    if (fading == 0) fading = 1;

    if (fading < 0) {
      autoFading = 1;
      fading *= -1;
    }
  }

  rngSeed((uint32_t)time(NULL));
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL); 

  while (keepRunning)
  {
    int err = snd_rawmidi_open(&midi_in, NULL, argv[1], SND_RAWMIDI_NONBLOCK);
    if (err < 0)
    {
      if (nanosleep(&req, NULL) != 0) {
        fprintf(stderr, "error during wait for opening a midi device: %s\n", snd_strerror(err));
        return 1;
      }
    }
    else
    {
      pthread_create(&midi_read_thread, NULL, midiReadThread, midi_in);
      pthread_join(midi_read_thread, NULL);

      snd_rawmidi_close(midi_in);
    }
  }

  return 0;
}
