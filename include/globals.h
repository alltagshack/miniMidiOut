#ifndef __GLOBALS_H
#define __GLOBALS_H 1

int g_bufferSize;
int g_fading;
int g_sampleRate;
int g_autoFading;
unsigned int g_envelopeSamples;
int g_outputDeviceId;

volatile int g_keepRunning;
volatile int g_sustain;

#endif