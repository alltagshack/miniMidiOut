#ifndef __MIDI_H
#define __MIDI_H 1

#include <sys/epoll.h>

/**
 * @brief open a midi device
 
 * @param dev pointer to char array with "/dev/..."
 *
 * @return 0 on success, -1 on error
 */
int midi_open (char *dev);

void midi_close (void);

/**
 * @brief check a poll for a midi event
 
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int midi_add_poll (struct epoll_event *all, unsigned int id, int epoll_fd);

/**
 * @brief check a poll for a midi event
 
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int midi_check_event (struct epoll_event *all, unsigned int id);

#endif