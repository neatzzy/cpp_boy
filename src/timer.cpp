#include "timer.h"

Timer::Timer(Memory& mem) : memory(mem) {}

// Recomputes the TIMA clock signal; a 1 -> 0 transition increments TIMA.
void Timer::checkEdge() {
    static const int bits[4] = {9, 3, 5, 7};
    uint8_t tac = memory.read_direct(0xFF07);
    bool signal = (tac & 0b100) && ((counter >> bits[tac & 0b11]) & 1);

    if (last_signal && !signal) {
        uint8_t tima = memory.read_direct(0xFF05);
        if (tima == 0xFF) {
            memory.write_direct(0xFF05, memory.read_direct(0xFF06)); // reload from TMA
            memory.requestInterrupt(2);
        } else {
            memory.write_direct(0xFF05, tima + 1);
        }
    }
    last_signal = signal;
}

void Timer::tick(int cycles) {
    // One T-cycle at a time so every edge is seen (no skipped ticks, no lost remainder)
    for (int i = 0; i < cycles; i++) {
        counter++;
        checkEdge();
    }
    memory.write_direct(0xFF04, counter >> 8);
}

void Timer::resetDiv() {
    counter = 0;
    memory.write_direct(0xFF04, 0);
    checkEdge();
}

void Timer::tacChanged() {
    checkEdge();
}
