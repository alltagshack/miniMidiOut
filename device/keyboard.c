#include <stddef.h>
#include <stdio.h>
/* open */
#include <fcntl.h>
/* close */
#include <unistd.h>
#include <linux/input.h>


#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sys/epoll.h>
#include "device.h"
#include "device/keyboard.h"

#include "modus.h"
#include "voice.h"
#include "globals.h"

void keyboard_search (Device *dev, char *keyboardDevice) {
    DIR *dir;
    struct dirent *entry;

    if (dev == NULL)
    {
        fprintf(stderr, "missing pointer to device 'keyboard'\n");
        keyboardDevice[0] = '\0';
        return;
    }
    dev->_fd = -1;

    if ((dir = opendir(KEYBOARD_EVENTS)) == NULL)
    {
        fprintf(stderr, "problem to open %s\n", KEYBOARD_EVENTS);
        keyboardDevice[0] = '\0';
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "event"))
        {
            char name[256] = {0};
            size_t leng = strlen(KEYBOARD_EVENTS) + 1 + strlen(entry->d_name) + 1;

            snprintf(
                keyboardDevice, 
                leng, 
                "%s/%s",
                KEYBOARD_EVENTS,
                entry->d_name
            );

            dev->_fd = open(keyboardDevice, O_RDONLY | O_NONBLOCK);
            if (dev->_fd < 0) {
                fprintf(stderr, "problem to open %s\n", keyboardDevice);
                continue;
            }

            ioctl(dev->_fd, EVIOCGNAME(sizeof(name)), name);

            if (strstr(name, KEYBOARD_SEARCH_STR) != NULL)
            {
                /* keyboard found and return */
                printf("Found %s: %s\n", name, keyboardDevice);
                closedir(dir);
                return;
            }

            close(dev->_fd);
        }
    }
    /* nothing found */
    keyboardDevice[0] = '\0';
    closedir(dir);
}

static int check_poll (Device *dev, struct epoll_event *all, unsigned int id)
{
    if (dev == NULL) return -1;
    
    if ((dev->_fd >= 0) && (all[id].events & EPOLLIN))
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
                g_autoFading = (g_autoFading+1)%4;

            } else if ((ev.code == KEY_KPMINUS || ev.code == KEY_MINUS) && ev.value == 1) {
                voice_pitch /= 2.0f;
            } else if ((ev.code == KEY_KPDOT || ev.code == KEY_DOT) && ev.value == 1) {
                voice_pitch *= 2.0f;

            } else if ((ev.code == KEY_KP0 || ev.code == KEY_0) && ev.value == 1 && g_sustain == 0) {
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

Device dKeyboard;

void keyboard_init() {
    dKeyboard._fd = -1;
    dKeyboard.open = NULL;
    dKeyboard.close = NULL;
    dKeyboard.add_poll = NULL;
    dKeyboard.check_poll = check_poll;
}