#pragma once
#include <vector>
#include <cstdint>
#include <string>

class Cartridge;
class PPU;
class Timer;
class Joypad;
class APU;
class Memory {
public:
    Cartridge* cart = nullptr;
    PPU* ppu = nullptr;
    Timer* timer = nullptr;
    Joypad* joypad = nullptr;
    APU* apu = nullptr;

    std::vector<uint8_t> ram = std::vector<uint8_t>(65536, 0);
    std::vector<uint8_t> bootRom; // mapped at 0x0000-0x00FF while non-empty; cleared by a non-zero write to 0xFF50

    Memory();
    void loadBootRom(const std::string& path); // throws unless the file is exactly 256 bytes

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    void write_direct(uint16_t addr, uint8_t value){ram[addr] = value;}
    void requestInterrupt(uint8_t interrupt_bit);
    uint8_t read_direct(uint16_t addr);
};
