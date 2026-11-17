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
        // TIMA overflow: reads 0 for one M-cycle, then TMA is loaded and IF bit 2 set
        static constexpr int RELOAD_DELAY = 4;
        int reload_delay = 0;  // T-cycles until the pending reload
        int reload_window = 0; // T-cycles left in the M-cycle where TMA is loaded

        void checkEdge();

    public:
        Timer(Memory& mem);
        void tick(int cycles);
        void resetDiv();    // called on DIV write
        void tacChanged();  // called on TAC write
        void writeTIMA(uint8_t value);
        void writeTMA(uint8_t value);
};
