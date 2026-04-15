#include <atomic>
#include "globals.h"
#include "Modus.h"

int g_bufferSize;
int g_fading;
int g_sampleRate;
int g_autoFading;
unsigned int g_envelopeSamples;
int g_outputDeviceId;

std::atomic<int> g_keepRunning;
std::atomic<int> g_sustain;

Modus g_modus(Modus::Value::SAW);
