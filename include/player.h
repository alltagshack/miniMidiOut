#ifndef __PLAYER_H
#define __PLAYER_H 1

#include <portaudio.h>


PaError player_stop (void);
PaError player_is_active (void);
PaError player_close (void);
PaError play (unsigned char status, unsigned char note, unsigned char vel);

/**
 * @brief open a stream for portaudio
 
 * @param audio_callback pointer to portaudio callback function
 *
 * @return paNoError on success
 */
PaError player_open (
    int (*audio_callback)(
        const void *,
        void *,
        unsigned long,
        const PaStreamCallbackTimeInfo *,
        PaStreamCallbackFlags,
        void *
    )
);

#endif