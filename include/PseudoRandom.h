#ifndef __PSEUDORANDOM_H
#define __PSEUDORANDOM_H 

#include <cstdint>
#include <climits>

class PseudoRandom {
private:
    static uint32_t _state;
    PseudoRandom (uint32_t seed = 1);
    ~PseudoRandom() = default;
    
public:
    static PseudoRandom & instance ();

    // no copy
    PseudoRandom (const PseudoRandom&) = delete;
    // no copy via =
    PseudoRandom& operator= (const PseudoRandom&) = delete;
    // same above, but for move (ptr)
    PseudoRandom (PseudoRandom&&) = delete;
    PseudoRandom& operator= (PseudoRandom&&) = delete;

    void setSeed (uint32_t seed);
    inline float floatRnd (void);
};

inline float PseudoRandom::floatRnd() {
    uint32_t x = _state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    _state = x;
    return static_cast<float>(static_cast<int32_t>(x)) / static_cast<float>(INT32_MAX);
}

#endif
