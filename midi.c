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
#include "midi.h"

#include "voice.h"
#include "player.h"
#include "globals.h"

static int _fd;
static unsigned char _midi_byte;
static unsigned char _running_status = 0;
static unsigned char _data1 = 0;
static int _expecting = 0;

static PaError parse_byte ()
{
    PaError err = paNoError;

    /* Status byte? */
    if (_midi_byte & 0x80) {
        _running_status = _midi_byte;
        _expecting = 1;
        return err;
    }

    /* Data byte */
    if (_expecting == 1) {
        _data1 = _midi_byte;
        _expecting = 2;
        return err;
    }

    if (_expecting == 2) {
        unsigned char status = _running_status & 0xF0;
        unsigned char channel = _running_status & 0x0F;
        unsigned char note = _data1;
        unsigned char vel = _midi_byte;

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
          int value = (vel << 7) | _data1;
          voice_pitchbend = ((float)value - 8192.0f) / 8192.0f;
        }
        _expecting = 1; /* ready for next event */
    }
    return err;
}


int midi_open (char *dev)
{
    _fd = -1;

    _fd = open(dev, O_RDONLY | O_NONBLOCK);
    if (_fd < 0) {
        fprintf(stderr, "error open device %s\n", dev);
        return -1;
    }
    return 0;
}

void midi_close (void)
{
    if (_fd >= 0) {
        close(_fd);
    }
}

int midi_add_poll (struct epoll_event *all, unsigned int id, int epoll_fd)
{
    if (_fd >= 0) {
        all[id].events = EPOLLIN;
        all[id].data.fd = _fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd, &all[id]) == -1) {
            fprintf(stderr, "error epoll add.\n");
            return -1;
        }
    }
    return 0;
}

int midi_check_event (struct epoll_event *all, unsigned int id)
{
    PaError err;
    ssize_t n;

    if ((_fd >= 0) && (all[id].events & EPOLLIN))
    {
        while (g_keepRunning) {
            n = read(_fd, &_midi_byte, 1);

            if (n > 0) {
                
                err = parse_byte();
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