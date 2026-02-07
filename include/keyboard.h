#ifndef __KEYBOARD_H
#define __KEYBOARD_H 1

#include <sys/epoll.h>

#define KEYBOARD_EVENTS "/dev/input"
#define KEYBOARD_SEARCH_STR "eyboard"

/**
 * @brief search input events for a keyboard
 
 * @param result is '\0' if no event path is found
 */
void keyboard_search (char *result);

void keyboard_open (char *dev);
void keyboard_close (void);
void keyboard_add_poll (struct epoll_event *all, unsigned int id, int epoll_fd);

/**
 * @brief check a poll for a keyboard event
 
 * @param all pointer to poll array
 * @param id  index in poll array
 *
 * @return 0 on success, -1 on error
 */
int keyboard_check_event (struct epoll_event *all, unsigned int id);

#endif