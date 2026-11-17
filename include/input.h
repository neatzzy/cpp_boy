#pragma once
#include "memory.h"
#include <cstdint>

enum class Button { Right, Left, Up, Down, A, B, Select, Start };

// Joypad register 0xFF00. Bits 4/5 select directions/buttons (0 = selected);
// low nibble is read-only, 0 = pressed.
class Joypad {
    private:
        Memory& memory;
        uint8_t select = 0x30;   // bits 4-5 as written
        uint8_t dirs = 0x0F;     // 0 = pressed
        uint8_t buttons = 0x0F;
        uint8_t nibble() const;

    public:
        Joypad(Memory& mem) : memory(mem) {}
        void write(uint8_t value);
        uint8_t read() const;
        void setButton(Button button, bool pressed);
};
