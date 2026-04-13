#include <linux/input-event-codes.h>
#include <portaudio.h>
#include <math.h>
#include <string.h>

#include <csignal>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/epoll.h>

#include <thread>

#include <sched.h>
#include <sys/mman.h>

#include <map>
#include <string>
#include <fstream>
#include <iostream>

#include "modus.h"
#include "noise.h"
#include "voice.h"
#include "pseudo_random.h"
#include "time_tools.h"
#include "device/keyboard.h"
#include "device/midi.h"
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

void midiReadThread (void)
{
    PaError err;
    int ret1, ret2;

    int epoll_fd;
    struct epoll_event *events;

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::clog << "error epoll create.\n";
        return;
    }

    events = (struct epoll_event *)calloc(2, sizeof(struct epoll_event));
    
    if (device_add_poll(&dMidi, events, 0, epoll_fd) < 0) {
        std::clog << "error epoll add for midi.\n";
        free(events);
        close(epoll_fd);
        return;
    }

    if (device_add_poll(&dKeyboard, events, 1, epoll_fd) < 0) {
      std::clog << "ignore keyboard\n";
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
            std::clog << "error epoll wait.\n";
            goto out;
        }

        ret2 = device_check_poll(&dKeyboard, events, 1);
        if (ret2 < 0) break;


        /* midi stuff */
        if (events[0].events & (EPOLLERR | EPOLLHUP)) {
            std::clog << "error epoll.\n";
            goto out;
        }

        ret2 = device_check_poll(&dMidi, events, 0);
        if (ret2 < 0) goto error;
        if (ret2 == 1) goto out;

    } /* end while */

    std::cout << "quitting...\n";

out:

    if (player_is_active()) {
        err = player_stop();
        if (err != paNoError) goto error;
    }

    err = player_close();

error:
    if (err != paNoError) {
        std::clog << "error PortAudio: " << Pa_GetErrorText(err) << "\n";
    }
    Pa_Terminate();
    free(events);
    close(epoll_fd);
    return;
}


std::map<std::string, std::string> parse_config (const std::string & path)
{
    std::map<std::string,std::string> cfg;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f,line))
    {
        auto pos = line.find('=');
        if (pos==std::string::npos) continue;
        std::string k = line.substr(0,pos);
        std::string v = line.substr(pos+1);
        // trim simple
        auto trim = [](std::string &s){
            while(!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
            while(!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
        };
        trim(k); trim(v);
        if (!k.empty()) cfg[k]=v;
    }
    return cfg;
}

void handle_sig (int sig) {
    g_keepRunning = 0;
}

int main (int argc, char *argv[])
{
    char m = '\0';
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

    midi_init();
    keyboard_init();

    if (argc < 2) {
        std::clog << "example usage:\n";
        std::clog << "\t" << argv[0] << " configFile\n";
        return 1;
    }

    auto cfg = parse_config(argv[1]);
    if (cfg.find("midiDevice") == cfg.end()) {
        std::clog << "Missing midiDevice in configFile " << argv[1] << "\n";
        return 1;
    }

    if (cfg.find("modus") != cfg.end()) {
        m = cfg["modus"][1];
        modus_switch(m);
    }
    if (cfg.find("outputDeviceId") != cfg.end()) {
        g_outputDeviceId = stoi(cfg["outputDeviceId"]);
    }
    if (cfg.find("bufferSize") != cfg.end()) {
        g_bufferSize = stoi(cfg["bufferSize"]);
    }
    if (cfg.find("fading") != cfg.end()) {
        g_fading = stoi(cfg["fading"]);
        if (g_fading == 0) g_fading = 1;

        if (g_fading < 0) {
            g_autoFading = 1;
            g_fading *= -1;
        }
    }
    if (cfg.find("envelopeSamples") != cfg.end()) {
        g_envelopeSamples = stoi(cfg["envelopeSamples"]);
    }
    if (cfg.find("keypadDevice") != cfg.end()) {
        if (device_open(&dKeyboard, cfg["keypadDevice"].data()) < 0) {
            std::clog << "ignore invalid keyboard\n";
        }
    } else {
        keyboard_search(&dKeyboard, keyboardDevice);
    }
    if (cfg.find("sampleRate") != cfg.end()) {
        g_sampleRate = stoi(cfg["sampleRate"]);
    }

    PseudoRandom::instance().setSeed((uint32_t)time(NULL));
    std::signal(SIGINT, handle_sig);
    std::signal(SIGTERM, handle_sig);

    while (g_keepRunning)
    {
        if (device_open(&dMidi, cfg["midiDevice"].data()) < 0)
        {
            std::clog << "wait connecting to " << cfg["midiDevice"] << "\n";
            tt_waiting(500);
            continue;
        }
        std::cout << "Connected to " << cfg["midiDevice"] << "\n";

        std::thread midiThread(midiReadThread);
        midiThread.join();

        tt_waiting(200);

        device_close(&dMidi);
    }

    device_close(&dKeyboard);
    return 0;
}
