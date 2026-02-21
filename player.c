#include <portaudio.h>
#include <unistd.h>

#include "player.h"
#include "noise.h"
#include "modus.h"
#include "voice.h"
#include "time_tools.h"
#include "globals.h"

static PaStream *_stream;
static PaStreamParameters _params;


PaError player_stop (void) {
    return Pa_StopStream(_stream);
}

PaError player_is_active (void) {
    return Pa_IsStreamActive(_stream);
}

PaError player_close (void) {
    return Pa_CloseStream(_stream);
}

PaError player_open (
    int (*audio_callback)(
        const void *,
        void *,
        unsigned long,
        const PaStreamCallbackTimeInfo *,
        PaStreamCallbackFlags,
        void *
    )
)
{
    PaError err = paNoError;

    if (g_outputDeviceId >= 0) {
        _params.device = g_outputDeviceId;
        _params.channelCount = 2;
        _params.sampleFormat = paFloat32;
        _params.suggestedLatency = Pa_GetDeviceInfo(g_outputDeviceId)->defaultLowOutputLatency;
        _params.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
          &_stream, 
          NULL, 
          &_params, 
          g_sampleRate, 
          g_bufferSize, 
          paClipOff, 
          audio_callback, 
          NULL);
    } else {
        err = Pa_OpenDefaultStream(
          &_stream, 
          0, 
          2, 
          paFloat32, 
          g_sampleRate, 
          g_bufferSize, 
          audio_callback, 
          NULL);
    }
    return err;
}

PaError play (unsigned char status, unsigned char note, unsigned char vel)
{
  static unsigned int count    = 0;
  static unsigned int start_ms = 0;
  static unsigned int now_ms = 0;
  static int old_sustain = 0;
  float freq;
  Voice *userData;

  if ((status & 0xF0) == 0x90 && vel > 0) {
    freq = voice_midi2freq(&note);

    //printf("%.2f Hz (%d)\n", freq, vel);

    userData = voice_get();

    voice_active++;
    userData->note = note;
    userData->freq = freq;
    userData->active = (g_autoFading == 1)? 2 : 1;
    userData->phase = 0.0;
    userData->volume = 0.7f * ((float)vel / 127.0f);
    userData->envelope = g_envelopeSamples;

    if (modus == NOISE) noise_prepare(userData);

    if (Pa_IsStreamStopped(_stream)) {
      PaError err = Pa_StartStream(_stream);
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

    if (old_sustain == 1 && (g_sustain == 0 || g_sustain == 2) && Pa_IsStreamStopped(_stream)) {
      if (count == 0) start_ms = now_ms;
      count++;
    }

    if (count > 0 && ( (now_ms - start_ms ) > SUSTAIN_MODESWITCH_MS)) {
      count = 0;
    }

    //printf("old_sustain: 0x%X, sustain: 0x%X, count: %d, activeCount: %d\n", old_sustain, g_sustain, count, voice_active);

    if (count >= SUSTAIN_NEEDED)
    {
      count = 0;
      modus_switch('\0');
    }

    old_sustain = g_sustain;

  } else if ((status & 0xF0) == 0x80 ||
            ((status & 0xF0) == 0x90 && vel == 0)) {

    userData = voice_find_by_note(&note);
    if (userData) {
      userData->active = 2;
    }
  }

  return paNoError;
}
