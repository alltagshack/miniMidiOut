#ifndef __DEVICE_H
#define __DEVICE_H 1

#include <sys/epoll.h>

struct device_s;

typedef struct device_s Device;

struct device_s {
    int _fd;
    void *_parameters;

    int     (*open)(Device *dev, char *name);
    void    (*close)(Device *dev);
    int     (*add_poll)(Device *dev, struct epoll_event *all, unsigned int id, int epoll_fd);
    int     (*check_poll)(Device *dev, struct epoll_event *all, unsigned int id);
};

/**
 * @brief open a device
 *
 * @param dev pointer to the device
 * @param name pointer to char array with "/dev/..."
 *
 * @return 0 on success, -1 on error
 */
int device_open (Device *dev, char *name);

/**
 * @brief close a device
 *
 * @param dev pointer to the device
 *
 * @return 0 on success, -1 on error
 */
void device_close (Device *dev);

/**
 * @brief add a poll for an event
 *
 * @param dev pointer to the device
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int device_add_poll (Device *dev, struct epoll_event *all, unsigned int id, int epoll_fd);

/**
 * @brief check a poll for an event
 *
 * @param dev pointer to the device
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int device_check_poll (Device *dev, struct epoll_event *all, unsigned int id);

#endif