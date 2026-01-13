#ifndef __GLOBALS_H
#define __GLOBALS_H 1

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

extern int g_bufferSize;
extern int g_fading;
extern int g_sampleRate;
extern int g_autoFading;
extern unsigned int g_envelopeSamples;
extern int g_outputDeviceId;

extern volatile int g_keepRunning;
extern volatile int g_sustain;

#endif