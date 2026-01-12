#ifndef __MODUS_H
#define __MODUS_H 1

typedef enum modes_s { SINUS, SAW, SQUARE, TRIANGLE, NOISE } Modus;

extern volatile Modus mode;

void switchMode (char m);

#endif