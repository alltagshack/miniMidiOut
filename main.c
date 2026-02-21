#define _GNU_SOURCE

#include <linux/input-event-codes.h>
#include <portaudio.h>
#include <math.h>
#include <string.h>

#include <pthread.h>

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/epoll.h>

#include <sched.h>
#include <sys/mman.h>

#include "modus.h"
#include "noise.h"
#include "voice.h"
#include "pseudo_random.h"
#include "time_tools.h"
#include "keyboard.h"
#include "midi.h"
#include "device.h"
#include "player.h"
#include "globals.h"

static int audioCallback (const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags, void *userData) {

  float *out = (float *)outputBuffer;
  unsigned int i, j;
  int cnt = voice_active;
  if (cnt < 1) {
    return paContinue;
  }
  for (i = 0; i < framesPerBuffer; ++i) {
    float sample = 0.0f;
    float env = 1.0f;
    for (j = 0; j < VOICE_MAX; ++j) {
      if (voices[j].active > 0) {

        if (voices[j].envelope != 0 && g_envelopeSamples != 0) {
          env += ENVELOPE_MAX * ((float)voices[j].envelope / g_envelopeSamples);
          voices[j].envelope--;
        }

        if (voices[j].active == 2) {
          if (g_sustain == 1) {
            voices[j].volume -= 0.000002f;
          } else if (g_sustain == 2) {
            /* on release sustain we force a faster silence */
            voices[j].volume = 0.0001f;
          } else {
            voices[j].volume -= (float)g_fading * 0.000005f;
          }

          if (g_autoFading == 2) {
            voices[j].freq *= 1.0001;
          } else if (g_autoFading == 3) {
            voices[j].freq *= 0.9999;
          }
        }

        if ((i == 0) && ((voices[j].volume < 0.001f) || voices[j].freq > 18000 || voices[j].freq < 1)) {
          voices[j].volume = 0.0f;
          voices[j].active = 0;
          voice_active--;
        }

        switch (modus) {
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
            float filtered = noise_filter(&voices[j]);
            sample += env * voices[j].volume * filtered;
            break;
          }
          default:
            sample += env * voices[j].volume * sinf(2.0f * M_PI * voices[j].phase);
        }

        voices[j].phase += (
          voices[j].freq * powf(2.0f, voice_pitchbend * VOICE_PITCHBEND_RANGE)
        ) / g_sampleRate;
        if (voices[j].phase >= 1.0f) {
          voices[j].phase -= 1.0f;
        }

      }
    }
    /* after all voices are faded out, the release of sustain ends */
    if (g_sustain == 2) g_sustain = 0;

    *out++ = sample; /* left */
    *out++ = sample; /* right */
  }

  return paContinue;
}

static PaError _theme (void)
{
  PaError err = paNoError;

  err = play(0x90, 0x30, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);
  err = play(0x90, 0x32, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);
  err = play(0x90, 0x34, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);

  err = play(0x90, 0x30, 0x0);
  if (err != paNoError) return err;
  err = play(0x90, 0x32, 0x0);
  if (err != paNoError) return err;
  err = play(0x90, 0x34, 0x0);

  return err;
}

void *midiReadThread (void *dummy)
{
    PaError err;
    int ret1, ret2;

    int epoll_fd;
    struct epoll_event *events;

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        fprintf(stderr, "error epoll create.\n");
        return NULL;
    }

    events = calloc(2, sizeof(struct epoll_event));
    
    if (device_add_poll(&dMidi, events, 0, epoll_fd) < 0) {
        fprintf(stderr, "error epoll add for midi.\n");
        free(events);
        close(epoll_fd);
        return NULL;
    }

    if (device_add_poll(&dKeyboard, events, 1, epoll_fd) < 0) {
      fprintf(stderr, "ignore keyboard\n");
    }
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    err = player_open(audioCallback);
    if (err != paNoError) goto error;

    err = _theme();
    if (err != paNoError) goto error;

    while (g_keepRunning) {
        if (voice_active < 1 && player_is_active()) {
            voice_active = 0;
            err = player_stop();
            if (err != paNoError) goto error;
        }

        ret1 = epoll_wait(epoll_fd, events, 2, 200);
        if (ret1 < 0) {
            fprintf(stderr, "error epoll wait.\n");
            goto out;
        }

        ret2 = device_check_poll(&dKeyboard, events, 1);
        if (ret2 < 0) break;


        /* midi stuff */
        if (events[0].events & (EPOLLERR | EPOLLHUP)) {
            fprintf(stderr, "error epoll.\n");
            goto out;
        }

        ret2 = device_check_poll(&dMidi, events, 0);
        if (ret2 < 0) goto error;
        if (ret2 == 1) goto out;

    } /* end while */

    printf("quitting...\n");

out:

    if (player_is_active()) {
        err = player_stop();
        if (err != paNoError) goto error;
    }

    err = player_close();

error:
    if (err != paNoError) 
        fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));

    Pa_Terminate();
    free(events);
    close(epoll_fd);
    return NULL;
}

void handle_sig (int sig) {
    g_keepRunning = 0;
}

int main (int argc, char *argv[])
{
  pthread_t midi_read_thread;
  char m = '\0';
  struct sigaction sa;
  struct sched_param param;

  param.sched_priority = 50;
  sched_setscheduler(0, SCHED_FIFO, &param);

  mlockall(MCL_CURRENT | MCL_FUTURE);

  g_autoFading = 0;
  g_keepRunning = 1;
  g_sustain = 0;
  
  modus = SAW;
  char keyboardDevice[256];

  g_outputDeviceId = -1;
  g_bufferSize = DEFAULT_BUFFER_SIZE;
  g_fading = DEFAULT_FADING;
  g_envelopeSamples = DEFAULT_ENVELOPE;
  g_sampleRate = DEFAULT_RATE;

  sa.sa_handler = handle_sig;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  midi_init();
  keyboard_init();

  if (argc < 2) {
    fprintf(stderr, "example usage:\n");
    fprintf(stderr, "\t%s /dev/midi3 [sin|saw|sqr|tri|noi]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw [outputDeviceId]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 [bufferSize]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d [fade -20 to 100]\n", argv[0], g_bufferSize);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d [envelope]\n", argv[0], g_bufferSize, g_fading);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d [keypad]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d /dev/input/event0 [sampleRate]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d /dev/input/event0 %d\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, g_sampleRate);
    return 1;
  }
  if (argc >= 3) {
    m = argv[2][1];
    modus_switch(m);
  }
  if (argc >= 4) {
    g_outputDeviceId = atoi(argv[3]);
  }
  if (argc >= 5) {
    g_bufferSize = atoi(argv[4]);
  }
  if (argc >= 6) {
    g_fading = atoi(argv[5]);
    if (g_fading == 0) g_fading = 1;

    if (g_fading < 0) {
      g_autoFading = 1;
      g_fading *= -1;
    }
  }
  if (argc == 7) {
    g_envelopeSamples = atoi(argv[6]);
  }
  if (argc >= 8) {
    if (device_open(&dKeyboard, argv[7]) < 0) {
      fprintf(stderr, "ignore invalid keyboard\n");
    };
  } else {
    keyboard_search(&dKeyboard, keyboardDevice);
  }
  if (argc == 9) {
    g_sampleRate = atoi(argv[8]);
  }

  pr_seed((uint32_t)time(NULL));
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  while (g_keepRunning)
  {
    if (device_open(&dMidi, argv[1]) < 0)
    {
      fprintf(stderr, "wait connecting to %s\n", argv[1]);
      tt_waiting(500);
      continue;
    }
    printf("Connected to %s\n", argv[1]);

    pthread_create(&midi_read_thread, NULL, midiReadThread, NULL);
    pthread_join(midi_read_thread, NULL);

    tt_waiting(200);
    
    device_close(&dMidi);
  }

  device_close(&dKeyboard);
  return 0;
}
