// Boot ROM mapping (0x0000-0x00FF) and 0xFF50 unmap. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "cpu.h"
#include "memory.h"
#include "cartridge.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::string writeFile(const char* name, const std::vector<uint8_t>& data) {
    std::string path = std::string("build/") + name;
    std::ofstream(path, std::ios::binary).write((const char*)data.data(), data.size());
    return path;
}

int main() {
    // Cartridge: 32KB ROM-only, every byte 0xAA so it's distinguishable from boot bytes.
    std::vector<uint8_t> rom(0x8000, 0xAA);
    rom[0x147] = 0x00;
    rom[0x148] = 0x00;
    rom[0x149] = 0x00;
    Cartridge cart;
    cart.loadData(rom);

    // Without boot ROM: post-boot state, cartridge visible at 0x0000.
    {
        Memory memory;
        memory.cart = &cart;
        CPU cpu(memory);
        assert(cpu.PC == 0x0100 && cpu.SP == 0xFFFE && cpu.A == 0x01 && cpu.F == 0xB0);
        assert(memory.read(0x0000) == 0xAA);
        assert(memory.read(0xFF40) == 0x91);
    }

    // Synthetic boot image, laid out like the real one: JP 0x00FC ; ... ;
    // 0x00FC: LD A,1 ; 0x00FE: LDH (0x50),A -> execution falls through to 0x0100 in the cartridge.
    std::vector<uint8_t> boot(256, 0x00);
    const uint8_t jump[] = {0xC3, 0xFC, 0x00};
    const uint8_t tail[] = {0x3E, 0x01, 0xE0, 0x50};
    std::copy(std::begin(jump), std::end(jump), boot.begin());
    std::copy(std::begin(tail), std::end(tail), boot.begin() + 0xFC);

    Memory memory;
    memory.cart = &cart;
    memory.loadBootRom(writeFile("boot_test_boot.bin", boot));
    CPU cpu(memory);
    assert(cpu.PC == 0x0000 && cpu.SP == 0x0000 && !cpu.IME);
    assert(cpu.A == 0 && cpu.F == 0 && cpu.B == 0 && cpu.C == 0);
    assert(cpu.D == 0 && cpu.E == 0 && cpu.H == 0 && cpu.L == 0);
    assert(memory.read(0x0000) == 0xC3);
    assert(memory.read(0x00FF) == 0x50);
    assert(memory.read(0x0100) == 0xAA);   // above the boot ROM: cartridge
    assert(memory.read(0xFF40) == 0x00);   // LCD off at power-up

    cpu.step(); cpu.step();                 // JP 0x00FC ; LD A,1
    assert(memory.read(0x0000) == 0xC3);   // still mapped
    cpu.step();                             // LDH (0x50),A
    assert(cpu.PC == 0x0100);
    assert(memory.read(0x0000) == 0xAA);   // unmapped
    assert(memory.read(0x00FF) == 0xAA);

    memory.write(0xFF50, 0x00);             // no way to remap
    assert(memory.read(0x0000) == 0xAA);
    assert(memory.read(0xFF50) == 0xFF);

    // Writing 0 to 0xFF50 keeps the boot ROM mapped.
    Memory m2;
    m2.cart = &cart;
    m2.loadBootRom(writeFile("boot_test_boot.bin", boot));
    m2.write(0xFF50, 0x00);
    assert(m2.read(0x0000) == 0xC3);

    // Wrong size is rejected.
    bool threw = false;
    try { m2.loadBootRom(writeFile("boot_test_bad.bin", std::vector<uint8_t>(255))); }
    catch (const std::exception&) { threw = true; }
    assert(threw);

    std::puts("boot_test OK");
    return 0;
}
