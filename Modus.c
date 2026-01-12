#include <stdio.h>
#include "Modus.h"

volatile Modus mode;

void switchMode (char m) {
  if (m == '\0') {
    if (mode == SINUS) m = 'a'; /* -> sAw */
    if (mode == SAW) m = 'q'; /* -> sQuare */
    if (mode == SQUARE) m = 'r'; /* -> tRiangle */
    if (mode == TRIANGLE) m = 'o'; /* -> nOise */
    if (mode == NOISE) m = 'i'; /* -> sInus */
  }

  if (m == 'i') {
    printf("~~~~ sinus\n");
    mode = SINUS;
  } else if (m == 'a') {
    printf("//// saw\n");
    mode = SAW;
  } else if (m == 'q') {
    printf("_||_ square\n");
    mode = SQUARE;
  } else if (m == 'r') {
    printf("/\\/\\ triangle\n");
    mode = TRIANGLE;
  } else if (m == 'o') {
    printf("XXXX noise\n");
    mode = NOISE;
  }
}
