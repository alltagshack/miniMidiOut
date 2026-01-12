#include <time.h>
#include "Timetools.h"

int waiting (unsigned int ms)
{
  struct timespec req = {0};
  req.tv_sec  = 0;
  req.tv_nsec = ms * 1000000;
  if (nanosleep(&req, NULL) != 0) {
    return 1;
  }
  return 0;
}

unsigned int now (void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    /* (seconds * 1000) + (nanoseconds / 1 000 000) a 32‑Bit‑Overflow is ok */
    return (unsigned int)(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);
}
