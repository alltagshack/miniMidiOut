#include <stdio.h>
#include "modus.h"

volatile Modus modus;

void modus_switch (char m) {
  if (m == '\0') {
    if (modus == SINUS) m = 'a'; /* -> sAw */
    if (modus == SAW) m = 'q'; /* -> sQuare */
    if (modus == SQUARE) m = 'r'; /* -> tRiangle */
    if (modus == TRIANGLE) m = 'o'; /* -> nOise */
    if (modus == NOISE) m = 'i'; /* -> sInus */
  }

  if (m == 'i') {
    printf("~~~~ sinus\n");
    modus = SINUS;
  } else if (m == 'a') {
    printf("//// saw\n");
    modus = SAW;
  } else if (m == 'q') {
    printf("_||_ square\n");
    modus = SQUARE;
  } else if (m == 'r') {
    printf("/\\/\\ triangle\n");
    modus = TRIANGLE;
  } else if (m == 'o') {
    printf("XXXX noise\n");
    modus = NOISE;
  }
}
