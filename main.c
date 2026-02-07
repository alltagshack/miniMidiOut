#define _GNU_SOURCE

#include <linux/input-event-codes.h>
#include <portaudio.h>
#include <math.h>

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

#include <errno.h>
#include <sched.h>
#include <sys/mman.h>

#include "modus.h"
#include "noise.h"
#include "voice.h"
#include "pseudo_random.h"
#include "time_tools.h"
#include "keyboard.h"
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
        }

        if ((i == 0) && (voices[j].volume < 0.001f)) {
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

PaError play (PaStream *stream, unsigned char status, unsigned char note, unsigned char vel)
{
  static unsigned int count    = 0;
  static unsigned int start_ms = 0;
  static unsigned int now_ms = 0;
  static int old_sustain = 0;
  float freq;
  Voice *userData;

  if ((status & 0xF0) == 0x90 && vel > 0) {
    freq = voice_midi2freq(&note);
    printf("%.2f Hz (%d)\n", freq, vel);

    userData = voice_get();

    voice_active++;
    userData->active = (g_autoFading == 1)? 2 : 1;
    userData->phase = 0.0;
    userData->freq = freq;
    userData->volume = 0.7f * ((float)vel / 127.0f);
    userData->envelope = g_envelopeSamples;

    if (modus == NOISE) noise_prepare(userData);

    if (Pa_IsStreamStopped(stream)) {
      PaError err = Pa_StartStream(stream);
      if (err != paNoError) return err;
    }

  } else if ((status & 0xF0) == 0xB0 && note == 0x40) {
    now_ms = tt_now();

    if (vel == 0x7F) g_sustain = 1;
    if (vel == 0x00) g_sustain = 2;

    if (old_sustain == 1 && g_sustain == 2) {
      /* release sustain */
      g_sustain = 0;
    }

    if (old_sustain == 1 && (g_sustain == 0 || g_sustain == 2) && Pa_IsStreamStopped(stream)) {
      if (count == 0) start_ms = now_ms;
      count++;
    }

    if (count > 0 && ( (now_ms - start_ms ) > SUSTAIN_MODESWITCH_MS)) {
      count = 0;
    }

    printf("old_sustain: 0x%X, sustain: 0x%X, count: %d, activeCount: %d\n",
      old_sustain, g_sustain, count, voice_active);

    if (count >= SUSTAIN_NEEDED)
    {
      count = 0;
      modus_switch('\0');
    }

    old_sustain = g_sustain;

  } else if ((status & 0xF0) == 0x80 ||
            ((status & 0xF0) == 0x90 && vel == 0)) {

    /* release a tone */

    freq = voice_midi2freq(&note);
    userData = voice_find_by_freq(&freq);
    if (userData) {
      userData->active = 2;
    }
  }

  return paNoError;
}

static unsigned char running_status = 0;
static unsigned char data1 = 0;
static int expecting = 0;

PaError midi_parse_byte (unsigned char b, PaStream *stream)
{
    PaError err = paNoError;

    /* Status byte? */
    if (b & 0x80) {
        running_status = b;
        expecting = 1;
        return err;
    }

    /* Data byte */
    if (expecting == 1) {
        data1 = b;
        expecting = 2;
        return err;
    }

    if (expecting == 2) {
        unsigned char status = running_status & 0xF0;
        unsigned char channel = running_status & 0x0F;
        unsigned char note = data1;
        unsigned char vel = b;

        /* NOTE ON */
        if (status == 0x90 && vel > 0) {
            err = play(stream, 0x90 | channel, note, vel);
        }

        /* NOTE OFF */
        else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            err = play(stream, 0x80 | channel, note, vel);
        }

        /* SUSTAIN PEDAL */
        else if (status == 0xB0 && note == 0x40) {
            err = play(stream, 0xB0 | channel, note, vel);
        }
        else if (status == 0xE0) {
          int value = (vel << 7) | data1;
          voice_pitchbend = ((float)value - 8192.0f) / 8192.0f;
        }
        expecting = 1; /* ready for next event */
    }
    return err;
}

static PaError _theme (PaStream *stream)
{
  PaError err = paNoError;

  err = play (stream, 0x90, 0x30, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);
  err = play (stream, 0x90, 0x32, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);
  err = play (stream, 0x90, 0x34, 0x42);
  if (err != paNoError) return err;
  tt_waiting(500);

  err = play (stream, 0x90, 0x30, 0x0);
  if (err != paNoError) return err;
  err = play (stream, 0x90, 0x32, 0x0);
  if (err != paNoError) return err;
  err = play (stream, 0x90, 0x34, 0x0);

  return err;
}

void *midiReadThread (void *data)
{
    unsigned char midi_byte;
    PaStream *stream;
    PaError err;
    PaStreamParameters params;
    int ret1, ret2;

    int midi_fd = *((int *)data);
    int epoll_fd;
    struct epoll_event *events;
    ssize_t n;

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create");
        return NULL;
    }

    events = calloc(2, sizeof(struct epoll_event));
    
    events[0].events = EPOLLIN;
    events[0].data.fd = midi_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, midi_fd, &events[0]) == -1) {
        perror("epoll_ctl: add");
        free(events);
        close(epoll_fd);
        return NULL;
    }

    keyboard_add_poll(events, 1, epoll_fd);
    
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    if (g_outputDeviceId >= 0) {
        params.device = g_outputDeviceId;
        params.channelCount = 2;
        params.sampleFormat = paFloat32;
        params.suggestedLatency = Pa_GetDeviceInfo(g_outputDeviceId)->defaultLowOutputLatency;
        params.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(&stream, NULL, &params, g_sampleRate, g_bufferSize, paClipOff, audioCallback, NULL);
    } else {
        err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, g_sampleRate, g_bufferSize, audioCallback, NULL);
    }
    
    if (err != paNoError) goto error;

    err = _theme(stream);
    if (err != paNoError) goto error;

    while (g_keepRunning) {
        if (voice_active < 1 && Pa_IsStreamActive(stream)) {
            voice_active = 0;
            err = Pa_StopStream(stream);
            if (err != paNoError) goto error;
        }

        ret1 = epoll_wait(epoll_fd, events, 2, 200);
        if (ret1 < 0) {
            perror("epoll_wait");
            goto out;
        }

        ret2 = keyboard_check_event(events, 1);
        if (ret2 < 0) break;


        /* midi stuff */
        if (events[0].events & (EPOLLERR | EPOLLHUP)) {
            goto out;
        }

        if (events[0].events & EPOLLIN) {
            
            while (g_keepRunning) {
                n = read(midi_fd, &midi_byte, 1);

                if (n > 0) {
                  
                  err = midi_parse_byte(midi_byte, stream);
                  if (err) goto error;
                  
                } else if (n == 0) {
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } if (errno == ENODEV || errno == EIO || errno == EPIPE || errno == EBADFD) {
                        goto out;
                    }
                    break;
                }
            }
        }

    } /* end while */

    printf("quitting...\n");

out:

    if (Pa_IsStreamActive(stream)) {
        err = Pa_StopStream(stream);
        if (err != paNoError) goto error;
    }

    err = Pa_CloseStream(stream);

error:
    if (err != paNoError) 
        fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));

    Pa_Terminate();
    free(events);
    close(epoll_fd); // Schließe epoll FD
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
  int midi_fd;
  struct sched_param param;

  param.sched_priority = 50;
  sched_setscheduler(0, SCHED_FIFO, &param);

  mlockall(MCL_CURRENT | MCL_FUTURE);

  g_autoFading = 0;
  g_keepRunning = 1;
  g_sustain = 0;
  
  modus = SAW;
  char *keyboardDevice = KEYBOARD_EVENT;

  g_outputDeviceId = -1;
  g_bufferSize = DEFAULT_BUFFER_SIZE;
  g_fading = DEFAULT_FADING;
  g_envelopeSamples = DEFAULT_ENVELOPE;
  g_sampleRate = DEFAULT_RATE;

  sa.sa_handler = handle_sig;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (argc < 2) {
    fprintf(stderr, "example usage:\n");
    fprintf(stderr, "\t%s /dev/midi3 [sin|saw|sqr|tri|noi]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw [outputDeviceId]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 [bufferSize]\n", argv[0]);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d [fade -20 to 100]\n", argv[0], g_bufferSize);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d [envelope]\n", argv[0], g_bufferSize, g_fading);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d [keypad]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d %s [sampleRate]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboardDevice);
    fprintf(stderr, "\t%s /dev/midi3 saw -1 %d %d %d %s %d\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboardDevice, g_sampleRate);
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
    keyboardDevice = argv[7];
  }
  if (argc == 9) {
    g_sampleRate = atoi(argv[8]);
  }

  pr_seed((uint32_t)time(NULL));
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL); 

  while (g_keepRunning)
  {
    midi_fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (midi_fd < 0)
    {
      tt_waiting(500);
      continue;
    }
    
    keyboard_open(keyboardDevice);

    pthread_create(&midi_read_thread, NULL, midiReadThread, &midi_fd);
    pthread_join(midi_read_thread, NULL);

    tt_waiting(200);
    keyboard_close();
    
    if (midi_fd >= 0) close(midi_fd);
  }

  return 0;
}
