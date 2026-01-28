#ifndef __MODUS_H
#define __MODUS_H 1

typedef enum modes_s { SINUS, SAW, SQUARE, TRIANGLE, NOISE } Modus;

extern volatile Modus modus;

void modus_switch (char m);

#endif