// Cartridge MBC banking and battery saves. Exit 0 on success (asserts abort).
#undef NDEBUG
#include "cartridge.h"
#include "memory.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>

// Each 16KB bank is filled with its bank number (low byte).
static std::vector<uint8_t> makeRom(uint8_t type, int banks, uint8_t ramCode) {
    std::vector<uint8_t> rom(banks * 0x4000);
    for (size_t i = 0; i < rom.size(); i++) rom[i] = (uint8_t)(i / 0x4000);
    rom[0x147] = type;
    rom[0x149] = ramCode;
    return rom;
}

static void romOnly() {
    Cartridge c;
    c.loadData(makeRom(0x00, 2, 0));
    assert(c.read(0x0000) == 0 && c.read(0x4000) == 1);
    c.write(0x2000, 5);                          // no MBC: ignored
    assert(c.read(0x4000) == 1);
    assert(c.read(0xA000) == 0xFF);              // no RAM
    c.write(0x0000, 0x0A); c.write(0xA000, 0x42);
    assert(c.read(0xA000) == 0xFF);
}

static void mbc1() {
    Cartridge c;
    c.loadData(makeRom(0x03, 128, 3));           // 2MB, 32KB RAM
    assert(c.read(0x4000) == 1);
    c.write(0x2000, 0);    assert(c.read(0x4000) == 1);    // 0 -> 1
    c.write(0x2000, 5);    assert(c.read(0x4000) == 5);
    c.write(0x2000, 0x3F); assert(c.read(0x4000) == 0x1F); // 5 bits only
    c.write(0x2000, 0x20); assert(c.read(0x4000) == 1);    // low 5 bits = 0 -> 1
    // Upper bits in mode 0 affect 4000-7FFF only.
    c.write(0x2000, 3); c.write(0x4000, 2);
    assert(c.read(0x4000) == 0x43 && c.read(0x0000) == 0);
    // Mode 1: 0000-3FFF maps bank upper<<5.
    c.write(0x6000, 1);
    assert(c.read(0x0000) == 0x40 && c.read(0x4000) == 0x43);
    c.write(0x6000, 0);
    assert(c.read(0x0000) == 0);
    // Bank number masked by ROM size (32 banks -> 0x23 & 0x1F = 3).
    Cartridge small;
    small.loadData(makeRom(0x01, 32, 0));
    small.write(0x2000, 3); small.write(0x4000, 1);
    assert(small.read(0x4000) == 3);

    // RAM: disabled reads 0xFF and ignores writes.
    c.write(0x4000, 0);
    assert(c.read(0xA000) == 0xFF);
    c.write(0xA000, 0x11); c.write(0x0000, 0x0A);
    assert(c.read(0xA000) == 0x00);
    c.write(0xA000, 0x11); assert(c.read(0xA000) == 0x11);
    // RAM banking only in mode 1.
    c.write(0x6000, 1); c.write(0x4000, 2);
    assert(c.read(0xA000) == 0x00);
    c.write(0xA000, 0x22); assert(c.read(0xA000) == 0x22);
    c.write(0x4000, 0); assert(c.read(0xA000) == 0x11);
    c.write(0x6000, 0); c.write(0x4000, 2); assert(c.read(0xA000) == 0x11);
    c.write(0x0000, 0x00); assert(c.read(0xA000) == 0xFF);
}

static void mbc5() {
    Cartridge c;
    c.loadData(makeRom(0x1B, 512, 4));           // 8MB, 128KB RAM
    assert(c.read(0x4000) == 1);
    c.write(0x2000, 0);    assert(c.read(0x4000) == 0);    // bank 0 valid
    c.write(0x2000, 0xFF); assert(c.read(0x4000) == 0xFF);
    c.write(0x3000, 1);                                     // bank 0x1FF
    assert(c.read(0x4000) == 0xFF);
    c.write(0x2000, 0x05);                                  // bank 0x105
    assert(c.read(0x4000) == 0x05 && c.read(0x0000) == 0);
    c.write(0x3000, 0); assert(c.read(0x4000) == 0x05);
    // RAM banks.
    c.write(0x0000, 0x0A);
    c.write(0x4000, 0);  c.write(0xA000, 0xAA);
    c.write(0x4000, 15); c.write(0xBFFF, 0xBB);
    assert(c.read(0xA000) == 0x00);
    c.write(0x4000, 0);  assert(c.read(0xA000) == 0xAA);
    c.write(0x4000, 15); assert(c.read(0xBFFF) == 0xBB);
    c.write(0x0000, 0);  assert(c.read(0xA000) == 0xFF);

    Cartridge small;                              // masked by bank count
    small.loadData(makeRom(0x19, 4, 0));
    small.write(0x2000, 6); assert(small.read(0x4000) == 2);
}

// 9-bit bank: bank 0x100 and 0x000 share the low byte, so check via a
// marker written into bank 0x100 of a synthetic image.
static void mbc5HighBit() {
    std::vector<uint8_t> rom = makeRom(0x19, 512, 0);
    rom[0x100 * 0x4000] = 0xEE;
    Cartridge c;
    c.loadData(rom);
    c.write(0x2000, 0); c.write(0x3000, 1);
    assert(c.read(0x4000) == 0xEE);
    c.write(0x3000, 0);
    assert(c.read(0x4000) == 0x00);
}

static void memoryRoutesExternalRam() {
    Cartridge c;
    c.loadData(makeRom(0x03, 4, 2));
    Memory m;
    m.cart = &c;
    assert(m.read(0xA000) == 0xFF);
    m.write(0x0000, 0x0A);
    m.write(0xA123, 0x5C);
    assert(m.read(0xA123) == 0x5C && c.read(0xA123) == 0x5C);
}

static void writeFile(const char* path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    f.write((const char*)data.data(), data.size());
}

static void battery() {
    const char* rom = "build/cart_test.gb";
    const char* sav = "build/cart_test.sav";
    std::remove(sav);
    writeFile(rom, makeRom(0x03, 4, 2));
    {
        Cartridge c;
        c.load(rom);
        c.write(0x0000, 0x0A);
        c.write(0xA000, 0x77); c.write(0xBFFF, 0x88);
        c.save();
    }
    {
        Cartridge c;
        c.load(rom);
        c.write(0x0000, 0x0A);
        assert(c.read(0xA000) == 0x77 && c.read(0xBFFF) == 0x88);
    }
    // Wrong-size save is ignored.
    writeFile(sav, std::vector<uint8_t>(10, 0x99));
    {
        Cartridge c;
        c.load(rom);
        c.write(0x0000, 0x0A);
        assert(c.read(0xA000) == 0x00);
    }
    // Non-battery carts never write a .sav.
    std::remove(sav);
    writeFile(rom, makeRom(0x02, 4, 2));
    {
        Cartridge c;
        c.load(rom);
        c.write(0x0000, 0x0A); c.write(0xA000, 1);
        c.save();
    }
    assert(!std::ifstream(sav).good());
}

int main() {
    romOnly();
    mbc1();
    mbc5();
    mbc5HighBit();
    memoryRoutesExternalRam();
    battery();
    return 0;
}
