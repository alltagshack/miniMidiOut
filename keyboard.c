#include <stdio.h>
/* open */
#include <fcntl.h>
/* close */
#include <unistd.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include "keyboard.h"

#include "modus.h"
#include "voice.h"
#include "globals.h"

static int _fd;

void keyboard_open (char *dev)
{
    _fd = open(dev, O_RDONLY | O_NONBLOCK);
    if (_fd < 0) {
        fprintf(stderr, "missing this letter keyboard as event device: %s\n", dev);
    }
}

void keyboard_close (void)
{
    if (_fd >= 0) {
        close(_fd);
    }
}

void keyboard_add_poll (struct epoll_event *all, unsigned int id, int epoll_fd)
{
    if (_fd >= 0) {
        /* keyboard as last poll element */
        all[id].events = EPOLLIN; 
        all[id].data.fd = _fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd, &all[id]);
    }
}

int keyboard_check_event (struct epoll_event *all, unsigned int id)
{
   if ((_fd >= 0) && (all[id].events & EPOLLIN))
    {
        struct input_event ev;
        ssize_t n = read(all[id].data.fd, &ev, sizeof(ev));
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
                return -1;
            }

        }
    }
    return 0;
}