#include <stdio.h>
#include <fcntl.h>
#include "keyboard.h"

char *keyboard_dev;

int keyboard_open (void)
{
    int fd = open(keyboard_dev, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "open keyboard");
        return -1;
    }
    return fd;
}
