#pragma once
#include "memory.h"
#include <cstdint>

// DIV/TIMA driven by one 16-bit system counter, as on hardware.
// DIV = high byte; TIMA ticks on the falling edge of the TAC-selected counter bit.
class Timer {
    private:
        Memory& memory;
        uint16_t counter = 0;
        bool last_signal = false;

        void checkEdge();

    public:
        Timer(Memory& mem);
        void tick(int cycles);
        void resetDiv();    // called on DIV write
        void tacChanged();  // called on TAC write
};
