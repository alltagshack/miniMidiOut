#ifndef _WAVEFORM_HPP
#define _WAVEFORM_HPP 1

#include <type_traits>

enum class Waveform { Sin, Saw, Square, Triangle, _COUNT };

inline Waveform& operator++ (Waveform& c) {
    using underlying = std::underlying_type_t<Waveform>;
    c = static_cast<Waveform>((static_cast<underlying>(c) + 1) % static_cast<underlying>(Waveform::_COUNT));
    return c;
}

inline Waveform operator++ (Waveform& c, int) {
    Waveform tmp = c;
    ++c;
    return tmp;
}

#endif
