#pragma once
#include <cstdint>
#include <vector>

class Memory;

class PPU {
private:
    Memory& memory;
    int scanline_counter = 0;

    std::vector<uint8_t> vram; // 0x8000 - 0x9FFF (8KB)
    std::vector<uint8_t> oam;  // 0xFE00 - 0xFE9F (160 bytes)

    void updateSTAT();

public:
    uint32_t framebuffer[160 * 144] = {0}; 
    
    bool frame_ready = false;

    PPU(Memory& mem);

    // Methods to read/write VRAM and OAM
    uint8_t readVRAM(uint16_t addr);
    void writeVRAM(uint16_t addr, uint8_t value);
    uint8_t readOAM(uint16_t addr);
    void writeOAM(uint16_t addr, uint8_t value);

    // Methods to read/write PPU registers
    uint8_t readRegister(uint16_t addr);
    void writeRegister(uint16_t addr, uint8_t value);

    // PPU Core
    void tick(int cycles);
};