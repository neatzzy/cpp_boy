#include "input.h"

uint8_t Joypad::nibble() const {
    uint8_t n = 0x0F;
    if (!(select & 0x10)) n &= dirs;
    if (!(select & 0x20)) n &= buttons;
    return n;
}

uint8_t Joypad::read() const { return 0xC0 | select | nibble(); }

void Joypad::write(uint8_t value) {
    uint8_t before = nibble();
    select = value & 0x30;
    if (before & ~nibble()) memory.requestInterrupt(4);
}

void Joypad::setButton(Button button, bool pressed) {
    uint8_t before = nibble();
    int b = static_cast<int>(button);
    uint8_t& group = b < 4 ? dirs : buttons;
    uint8_t mask = 1 << (b & 3);
    if (pressed) group &= ~mask; else group |= mask;
    if (before & ~nibble()) memory.requestInterrupt(4);
}
