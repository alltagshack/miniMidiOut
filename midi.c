#include <stddef.h>
#include <stdio.h>
/* open */
#include <fcntl.h>
/* close */
#include <unistd.h>
#include <linux/input.h>
#include <errno.h>
#include <portaudio.h>

#include <dirent.h>
#include <sys/ioctl.h>

#include <sys/epoll.h>
#include "device.h"
#include "midi.h"

#include "voice.h"
#include "player.h"
#include "globals.h"

static PaError parse_byte (Device *dev)
{
    PaError err = paNoError;
    struct midi_parameters *p;

    if (dev == NULL) return -1;
    p = (struct midi_parameters *) dev->_parameters;

    /* Status byte? */
    if (p->midi_byte & 0x80) {
        p->running_status = p->midi_byte;
        p->expecting = 1;
        return err;
    }

    /* Data byte */
    if (p->expecting == 1) {
        p->data = p->midi_byte;
        p->expecting = 2;
        return err;
    }

    if (p->expecting == 2) {
        unsigned char status = p->running_status & 0xF0;
        unsigned char channel = p->running_status & 0x0F;
        unsigned char note = p->data;
        unsigned char vel = p->midi_byte;

        /* NOTE ON */
        if (status == 0x90 && vel > 0) {
            err = play(0x90 | channel, note, vel);
        }

        /* NOTE OFF */
        else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            err = play(0x80 | channel, note, vel);
        }

        /* SUSTAIN PEDAL */
        else if (status == 0xB0 && note == 0x40) {
            err = play(0xB0 | channel, note, vel);
        }
        /* BITCH BEND */
        else if (status == 0xE0) {
          int value = (vel << 7) | p->data;
          voice_pitchbend = ((float)value - 8192.0f) / 8192.0f;
        }
        p->expecting = 1; /* ready for next event */
    }
    return err;
}

static int add_poll (Device *dev, struct epoll_event *all, unsigned int id, int epoll_fd)
{
    if (dev == NULL) return -1;

    if (dev->_fd >= 0) {
        all[id].events = EPOLLIN;
        all[id].data.fd = dev->_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dev->_fd, &all[id]) == -1) {
            fprintf(stderr, "error epoll add.\n");
            return -1;
        }
    }
    return 0;
}

static int check_poll (Device *dev, struct epoll_event *all, unsigned int id)
{
    PaError err;
    ssize_t n;
    struct midi_parameters *p;

    if (dev == NULL) return -1;
    p = (struct midi_parameters *) dev->_parameters;

    if ((dev->_fd >= 0) && (all[id].events & EPOLLIN))
    {
        while (g_keepRunning) {
            n = read(dev->_fd, &p->midi_byte, 1);

            if (n > 0) {
                
                err = parse_byte(dev);
                if (err) return -1;
                
            } else if (n == 0) {
                break;
            } else {
                if (
                    errno == EAGAIN ||
                    errno == EWOULDBLOCK
                ) {
                    break;
                } if (
                    errno == ENODEV || 
                    errno == EIO || 
                    errno == EPIPE || 
                    errno == EBADFD
                ) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

Device dMidi;
static struct midi_parameters parameters = {
    .expecting = 0,
    .running_status = 0,
    .data = 0
};

void midi_init() {
    dMidi._fd = -1;
    dMidi._parameters = &parameters;
    dMidi.open = NULL;
    dMidi.close = NULL;
    dMidi.add_poll = add_poll;
    dMidi.check_poll = check_poll;
}