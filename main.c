#include <alsa/asoundlib.h>
#include <linux/input-event-codes.h>
#include <portaudio.h>
#include <math.h>

#include <pthread.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/epoll.h>

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

        voices[j].phase += voices[j].freq / g_sampleRate;
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

  if ((status & 0xF0) == 0x90 && vel > 0) {
    float freq = voice_midi2freq(&note);
    printf("%.2f Hz (%d)\n", freq, vel);

    Voice *userData = voice_get();

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

    float freq = voice_midi2freq(&note);
    Voice *v = voice_find_by_freq(&freq);
    if (v) {
      v->active = 2;
    }
  }

  return paNoError;
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
    unsigned char midi_buf[3];
    PaStream *stream;
    PaError err;
    PaStreamParameters params;
    int i;
    int ret1, ret2;

    int midi_count;
    int epoll_fd;
    struct epoll_event *events;

    midi_count = snd_rawmidi_poll_descriptors_count((snd_rawmidi_t *)data);
    
    events = calloc(midi_count + 1, sizeof(struct epoll_event));
    
    struct pollfd *pfds = calloc(midi_count, sizeof(struct pollfd));
    snd_rawmidi_poll_descriptors((snd_rawmidi_t *)data, pfds, midi_count);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create");
        free(pfds);
        free(events);
        return NULL;
    }

    // Registriere alle MIDI-FD beim epoll
    for (i = 0; i < midi_count; ++i) {
        events[i].events = EPOLLIN;
        events[i].data.fd = pfds[i].fd; // nur fd speichern
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfds[i].fd, &events[i]) == -1) {
            perror("epoll_ctl: add");
            free(pfds);
            free(events);
            close(epoll_fd);
            return NULL;
        }
    }

    keyboard_add_poll(events, midi_count, epoll_fd);
    
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

        ret1 = epoll_wait(epoll_fd, events, midi_count + 1, 200);
        if (ret1 < 0) {
            perror("epoll_wait");
            goto out;
        }

        ret2 = keyboard_check_event(events, midi_count);
        if (ret2 < 0) break;

        for (i = 0; i < ret1; ++i) {
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                goto out;
            }

            if (events[i].events & EPOLLIN) {
                ssize_t n = snd_rawmidi_read((snd_rawmidi_t *)data, midi_buf, sizeof(midi_buf));

                if (n > 0) {
                    err = play(stream, midi_buf[0], midi_buf[1], midi_buf[2]);
                    if (err) goto error;

                } else if (n == -ENODEV || n == -EIO || n == -EPIPE || n == -EBADFD) {
                    goto out;
                }
            }
        } /* end for midi polls */

    } /* end while */

    printf("quitting...\n");

out:
    snd_rawmidi_drop((snd_rawmidi_t *)data);

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
    free(pfds);
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
  snd_rawmidi_t *midi_in;
  int alsaerr;

  /* only if root start. only senseful on PREEMPT_RT linux.

  struct sched_param p = { .sched_priority = 98 };
  sched_setscheduler(0, SCHED_FIFO, &p);

  */

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
    fprintf(stderr, "\t%s hw:2,0,0 [sin|saw|sqr|tri|noi]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw [outputDeviceId]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 [bufferSize]\n", argv[0]);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d [fade -20 to 100]\n", argv[0], g_bufferSize);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d [envelope]\n", argv[0], g_bufferSize, g_fading);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d [keypad]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d %s [sampleRate]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboardDevice);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d %s %d\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboardDevice, g_sampleRate);
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
    midi_in = NULL;
    alsaerr = snd_rawmidi_open(&midi_in, NULL, argv[1], SND_RAWMIDI_NONBLOCK);
    if (alsaerr < 0)
    {
      tt_waiting(500);
      continue;
    }
    keyboard_open(keyboardDevice);

    pthread_create(&midi_read_thread, NULL, midiReadThread, midi_in);
    pthread_join(midi_read_thread, NULL);
    
    snd_rawmidi_drop(midi_in);
    snd_rawmidi_close(midi_in);

    tt_waiting(200);
    keyboard_close();
  }

  return 0;
}
