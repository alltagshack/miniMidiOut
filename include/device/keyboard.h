#ifndef __KEYBOARD_H
#define __KEYBOARD_H 1

#include "device.h"

#include <sys/epoll.h>

#define KEYBOARD_EVENTS "/dev/input"
#define KEYBOARD_SEARCH_STR "eyboard"

extern Device dKeyboard;

void keyboard_init();

/**
 * @brief search input events for a keyboard
 *
 * @param dev pointer to the device
 * @param result is '\0' if no event path is found
 */
void keyboard_search (Device *dev, char *result);

#endif