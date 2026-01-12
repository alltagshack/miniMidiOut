#ifndef __KEYBOARD_H
#define __KEYBOARD_H 1

#define KEYBOARD_EVENT "/dev/input/event0"

extern char *keyboard_dev;

int keyboard_open (void);

#endif