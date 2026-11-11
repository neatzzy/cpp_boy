// Joypad register (0xFF00) behaviour. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "input.h"
#include "memory.h"
#include <cassert>

int main() {
    Memory memory;
    Joypad joypad(memory);
    memory.joypad = &joypad;
    auto irq = [&] { return (memory.read_direct(0xFF0F) & 0x10) != 0; };

    // Nothing selected, nothing pressed: upper bits 1, low nibble 0xF.
    memory.write(0xFF00, 0x30);
    assert(memory.read(0xFF00) == 0xFF);

    // Select directions (bit 4 = 0): Right pressed clears bit 0.
    memory.write(0xFF00, 0x20);
    joypad.setButton(Button::Right, true);
    assert(memory.read(0xFF00) == 0xEE);
    assert(irq());
    joypad.setButton(Button::Right, false);
    assert(memory.read(0xFF00) == 0xEF);

    // Select buttons (bit 5 = 0): directions are invisible, Start is bit 3.
    memory.write(0xFF00, 0x10);
    joypad.setButton(Button::Down, true);
    assert(memory.read(0xFF00) == 0xDF);
    joypad.setButton(Button::Start, true);
    assert(memory.read(0xFF00) == 0xD7);
    joypad.setButton(Button::Down, false);
    joypad.setButton(Button::Start, false);

    // Both groups selected: AND of both.
    memory.write(0xFF00, 0x00);
    joypad.setButton(Button::Left, true);   // dir bit 1
    joypad.setButton(Button::A, true);      // btn bit 0
    assert(memory.read(0xFF00) == 0xCC);
    joypad.setButton(Button::Left, false);
    joypad.setButton(Button::A, false);

    // Low nibble is read-only; only bits 4-5 are writable.
    memory.write(0xFF00, 0xFF);
    assert(memory.read(0xFF00) == 0xFF);

    // No interrupt if the pressed button is not in a selected group.
    memory.write_direct(0xFF0F, 0);
    memory.write(0xFF00, 0x20);             // directions only
    joypad.setButton(Button::B, true);
    assert(!irq());
    joypad.setButton(Button::B, false);

    // No interrupt on release, nor on a repeated press.
    joypad.setButton(Button::Up, true);
    assert(irq());
    memory.write_direct(0xFF0F, 0);
    joypad.setButton(Button::Up, true);
    joypad.setButton(Button::Up, false);
    assert(!irq());

    // Selecting a group with a held button pulls the line low: interrupt.
    memory.write(0xFF00, 0x30);
    joypad.setButton(Button::Select, true);
    assert(!irq());
    memory.write(0xFF00, 0x10);
    assert(irq());
    return 0;
}
