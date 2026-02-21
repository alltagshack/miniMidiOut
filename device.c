#include <stddef.h>
#include <stdio.h>
/* open */
#include <fcntl.h>
/* close */
#include <unistd.h>
#include <linux/input.h>
#include <portaudio.h>

#include <dirent.h>
#include <sys/ioctl.h>

#include <sys/epoll.h>

#include "device.h"


int device_open (Device *dev, char *name)
{
    if (dev == NULL) return -1;
    if (dev->open == NULL) {
        dev->_fd = -1;

        dev->_fd = open(name, O_RDONLY | O_NONBLOCK);
        if (dev->_fd < 0) {
            fprintf(stderr, "error open device %s\n", name);
            return -1;
        }
        return 0;
    }
    return dev->open(dev, name);
}

void device_close (Device *dev)
{
    if (dev == NULL) return;

    if (dev->close == NULL) {
        if (dev->_fd >= 0) {
            close(dev->_fd);
            return;
        }
    }
    dev->close(dev);
}

int device_add_poll (Device *dev, struct epoll_event *all, unsigned int id, int epoll_fd)
{
    if (dev == NULL) return -1;
    if (dev->add_poll == NULL) {
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
    return dev->add_poll(dev, all, id, epoll_fd);
}

int device_check_poll (Device *dev, struct epoll_event *all, unsigned int id)
{
    if (dev == NULL) return -1;
    if (dev->check_poll == NULL) return -1;
    
    return dev->check_poll(dev, all, id);
}