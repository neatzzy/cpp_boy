#pragma once
#include <vector>
#include <cstdint>
#include <string>

class Cartridge;
class PPU;
class Memory {
public:
    Cartridge* cart = nullptr;
    PPU* ppu = nullptr;

    std::vector<uint8_t> ram = std::vector<uint8_t>(65536, 0);

    Memory();

    void loadBIOS(const std::string& path);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    void push(uint16_t value, uint16_t& SP);
    uint16_t pop(uint16_t& SP);
    void write_direct(uint16_t addr, uint8_t value){ram[addr] = value;}
    void requestInterrupt(uint8_t interrupt_bit);
    uint8_t read_direct(uint16_t addr);
};
