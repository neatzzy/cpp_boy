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
            memory.write_direct(0xFF05, 0x00); // TIMA reads 0 until the delayed reload
            reload_delay = RELOAD_DELAY;
        } else {
            memory.write_direct(0xFF05, tima + 1);
        }
    }
    last_signal = signal;
}

void Timer::tick(int cycles) {
    // One T-cycle at a time so every edge is seen (no skipped ticks, no lost remainder)
    for (int i = 0; i < cycles; i++) {
        if (reload_window > 0) reload_window--;
        if (reload_delay > 0 && --reload_delay == 0) {
            memory.write_direct(0xFF05, memory.read_direct(0xFF06)); // reload from TMA
            memory.requestInterrupt(2);
            reload_window = 4;
        }
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

void Timer::writeTIMA(uint8_t value) {
    if (reload_window > 0) return;  // TMA is being loaded this cycle: write is lost
    reload_delay = 0;               // writing during the delay cancels the reload
    memory.write_direct(0xFF05, value);
}

void Timer::writeTMA(uint8_t value) {
    memory.write_direct(0xFF06, value);
    if (reload_window > 0) memory.write_direct(0xFF05, value); // reload cycle copies new TMA
}
