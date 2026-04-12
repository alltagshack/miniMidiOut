#include <chrono>
#include <thread>
#include "time_tools.h"

int tt_waiting (unsigned int ms) {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return 0;
    } catch (...) {
        return 1;
    }
}

unsigned int tt_now (void) {
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    auto ms = duration_cast<milliseconds>(now).count();
    /* (seconds * 1000) + (nanoseconds / 1 000 000) a 32‑Bit‑Overflow is ok */
    return static_cast<unsigned int>(ms);
}
