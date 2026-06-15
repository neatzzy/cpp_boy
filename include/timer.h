#pragma once
#include "memory.h"
#include <cstdint>

class Timer {
    private:
        Memory& memory;
        int div_counter = 0;
        int tima_counter = 1024; // Standard inital frequency

        void setClockFreq();

    public:
        Timer(Memory& mem);
        void tick(int cycles);
};