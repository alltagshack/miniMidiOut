#include <cstdint>
#include "PseudoRandom.h"

uint32_t PseudoRandom::_state = 1;

PseudoRandom::PseudoRandom (uint32_t seed)
{
    setSeed(seed);
}

void PseudoRandom::setSeed (uint32_t seed)
{
    if (seed == 0) seed = 1;
    _state = seed;
}

PseudoRandom & PseudoRandom::instance ()
{
    static PseudoRandom inst;
    return inst;
}
