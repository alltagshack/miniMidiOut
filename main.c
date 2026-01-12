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

#include "modus.h"
#include "noise.h"
#include "voice.h"
#include "pseudo_random.h"
#include "time_tools.h"
#include "keyboard.h"
#include "globals.h"

/* 48000 44100 16000 11025 */
#define DEFAULT_RATE           44100
#define DEFAULT_BUFFER_SIZE    16

#define FADING1                2
#define DEFAULT_FADING         10
#define FADING2                60

#define DEFAULT_ENVELOPE       16000
#define ENVELOPE_MAX           0.35f

#define SUSTAIN_MODESWITCH_MS  1500U
#define SUSTAIN_NEEDED         3U

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
            float n = pr_float();

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

    if (modus == NOISE)
    {
      float lowcut_freq = 0.5f * freq;
      userData->noise_detail.lowpass_alpha =
        (2.0f * M_PI * lowcut_freq / g_sampleRate) /
        (1.0f + 2.0f * M_PI * lowcut_freq / g_sampleRate);
      
      float c  = 1.0f / tanf(M_PI * freq / g_sampleRate);
      userData->noise_detail.bandmix = 1.0f / (1.0f + c / 0.7f + c * c);
      
      userData->noise_detail.lowpass_weight = fmaxf(0.0f, 1.0f - freq / 2093.0f);
      userData->noise_detail.bandpass_weight = fminf(1.0f, freq / 2093.0f);
      userData->noise_detail.highpass_weight = 1.0f - userData->noise_detail.lowpass_weight;

      /* bass boost */
      float shelf_fc = fminf(NOISE_BASS_BOOST_FREQ, lowcut_freq);
      float shelf_bw = 2.0f * M_PI * shelf_fc / g_sampleRate;
      userData->noise_detail.low_shelf_alpha = shelf_bw / (1.0f + shelf_bw);
      float boost_db = NOISE_BASS_BOOST_VALUE * fmaxf(0.0f, 1.0f - freq / 2093.0f);
      userData->noise_detail.low_shelf_gain = powf(10.0f, boost_db / 20.0f);
    }

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
  int kbd_fd;
  int midi_count;
  struct pollfd *pfds;
  struct pollfd *all;
  struct input_event ev;
  ssize_t n;

  kbd_fd = keyboard_open();
  if (kbd_fd < 0) {
    printf("missing this letter keyboard as event device: %s\n", keyboard_dev);
  }

  midi_count = snd_rawmidi_poll_descriptors_count((snd_rawmidi_t *)data);
  pfds = calloc(midi_count, sizeof(struct pollfd));
  snd_rawmidi_poll_descriptors((snd_rawmidi_t *)data, pfds, midi_count);

  all = calloc(midi_count + 1, sizeof(struct pollfd));

  /* copy midi pfds */
  for (i = 0; i < midi_count; ++i) {
    all[i] = pfds[i];
    all[i].events = POLLIN;
  }
  if (kbd_fd >= 0) {
    /* keyboard as last poll element */
    all[midi_count].fd = kbd_fd;
    all[midi_count].events = POLLIN;
  }

  err = Pa_Initialize();

  if (err != paNoError)
    goto error;

  if (g_outputDeviceId >= 0) {
    params.device = g_outputDeviceId;
    params.channelCount = 2;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(g_outputDeviceId)->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
      &stream,
      NULL,
      &params,
      g_sampleRate,
      g_bufferSize,
      paClipOff,
      audioCallback,
      NULL
    );
  } else {
    err = Pa_OpenDefaultStream(
      &stream, 0, 2, paFloat32, g_sampleRate,
      g_bufferSize, audioCallback, NULL
    );
  }
  if (err != paNoError)
    goto error;

  err = _theme(stream);
  if (err != paNoError)
    goto error;
  

  while (g_keepRunning)
  {
    if (voice_active < 1 && Pa_IsStreamActive(stream)) {
      voice_active = 0;
      err = Pa_StopStream(stream);
      if (err != paNoError) {
        goto error;
      }
    }

    int ret = poll(all, midi_count + 1, 200);
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "error poll\n");
      goto out;
    }


    if ((kbd_fd >= 0) && (all[midi_count].revents & POLLIN))
    {
      n = read(kbd_fd, &ev, sizeof(ev));
      if (n == sizeof(ev) && ev.type == EV_KEY)
      {
          if ((ev.code == KEY_KP1 || ev.code == KEY_1) && ev.value == 1) {
            modus_switch('i');
          } else if ((ev.code == KEY_KP2 || ev.code == KEY_2) && ev.value == 1) {
            modus_switch('a');
          } else if ((ev.code == KEY_KP3 || ev.code == KEY_3) && ev.value == 1) {
            modus_switch('q');
          } else if ((ev.code == KEY_KP4 || ev.code == KEY_4) && ev.value == 1) {
            modus_switch('r');
          } else if ((ev.code == KEY_KP5 || ev.code == KEY_5) && ev.value == 1) {
            modus_switch('o');

          } else if ((ev.code == KEY_KP6 || ev.code == KEY_6) && ev.value == 1) {
            g_fading = DEFAULT_FADING;
          } else if ((ev.code == KEY_KP7 || ev.code == KEY_7) && ev.value == 1) {
            g_fading = FADING1;
          } else if ((ev.code == KEY_KP8 || ev.code == KEY_8) && ev.value == 1) {
            g_fading = FADING2;
          } else if ((ev.code == KEY_KP9 || ev.code == KEY_9) && ev.value == 1) {
            g_autoFading = g_autoFading == 1? 0 : 1;

          } else if ((ev.code == KEY_KPMINUS || ev.code == KEY_MINUS) && ev.value == 1) {
            voice_pitch /= 2.0f;
          } else if ((ev.code == KEY_KPDOT || ev.code == KEY_DOT) && ev.value == 1) {
            voice_pitch *= 2.0f;

          } else if ((ev.code == KEY_KP0 || ev.code == KEY_0) && ev.value == 1) {
            g_sustain = 1;
          } else if ((ev.code == KEY_KP0 || ev.code == KEY_0) && ev.value == 0) {
            g_sustain = 0;

          } else if (ev.code == KEY_ESC && ev.value == 1) {
            g_keepRunning = 0;
            break;
          }

        }
    }

    for (i = 0; i < midi_count; ++i)
    {
      if (all[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        goto out;
      }

      if (all[i].revents & POLLIN) {

        n = snd_rawmidi_read((snd_rawmidi_t *)data, midi_buf, sizeof(midi_buf));

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
    if (err != paNoError)
      goto error;
  }

  err = Pa_CloseStream(stream);

error:
  if (err != paNoError) 
    fprintf(stderr, "error PortAudio: %s\n", Pa_GetErrorText(err));

  Pa_Terminate();
  if (kbd_fd >= 0) close(kbd_fd);
  free(all);
  free(pfds);
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
  keyboard_dev = KEYBOARD_EVENT;

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
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d %s [sampleRate]\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboard_dev);
    fprintf(stderr, "\t%s hw:2,0,0 saw -1 %d %d %d %s %d\n", argv[0], g_bufferSize, g_fading, g_envelopeSamples, keyboard_dev, g_sampleRate);
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
    keyboard_dev = argv[7];
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
    pthread_create(&midi_read_thread, NULL, midiReadThread, midi_in);
    pthread_join(midi_read_thread, NULL);
    
    snd_rawmidi_drop(midi_in);
    snd_rawmidi_close(midi_in);

    tt_waiting(200);
  }

  return 0;
}
