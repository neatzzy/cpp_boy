#include "timer.h"

Timer::Timer(Memory& mem) : memory(mem) {}

void Timer::setClockFreq() {
    uint8_t tac = memory.read(0xFF07);
    uint8_t freq = tac & 0b11; // Read last 2 bits
    
    // Defines the number of cycles for each frequency setting
    switch (freq) {
        case 0b00: tima_counter = 1024; break; // 4096 Hz
        case 0b01: tima_counter = 16;   break; // 262144 Hz
        case 0b10: tima_counter = 64;   break; // 65536 Hz
        case 0b11: tima_counter = 256;  break; // 16384 Hz
    }
}

void Timer::tick(int cycles) {
    // 1. Update the DIV register (0xFF04)
    // The DIV register increments at 16384Hz, which is every 256 T-cycles (4194304 / 16384)
    div_counter += cycles;
    if (div_counter >= 256) {
        div_counter -= 256;
        uint8_t div = memory.read(0xFF04);
        memory.write_direct(0xFF04, div + 1);
    }

    // 2. Update the TIMA register (0xFF05) if enabled
    uint8_t tac = memory.read(0xFF07);
    bool is_timer_enabled = (tac & 0b100) != 0; // The Bit 2 enables/disables the Timer

    if (is_timer_enabled) {
        tima_counter -= cycles;

        if (tima_counter <= 0) {
            setClockFreq(); // Reset the counter based on the current frequency setting

            uint8_t tima = memory.read(0xFF05);
            if (tima == 0xFF) {
                // OVERFLOW
                // 1. TIMA receives the value from TMA (0xFF06)
                uint8_t tma = memory.read(0xFF06);
                memory.write_direct(0xFF05, tma);
                
                // 2. Request a Timer interrupt (Bit 2 of the IF register at 0xFF0F)
                memory.requestInterrupt(2);
            } else {
                // No overflow, just increment TIMA
                memory.write_direct(0xFF05, tima + 1);
            }
        }
    }
}