#ifndef __MODULATION_HPP
#define __MODULATION_HPP 1

#include <stdint.h>

#define MODULATION_POTI     A4

extern uint16_t modulation_value;
extern uint16_t modulation_value_old;

uint32_t modulation_cutoff (void);
void modulation_refresh (void);

#endif