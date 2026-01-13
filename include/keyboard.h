#ifndef __KEYBOARD_H
#define __KEYBOARD_H 1

#include <poll.h>

#define KEYBOARD_EVENT "/dev/input/event0"

void keyboard_open (char *dev);
void keyboard_close (void);
void keyboard_add_poll (struct pollfd *all, unsigned int id);

/**
 * @brief check a poll for a keyboard event
 
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int keyboard_check_event (struct pollfd *all, unsigned int id);

#endif