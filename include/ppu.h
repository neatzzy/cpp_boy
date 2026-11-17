#pragma once
#include <cstdint>
#include <vector>

class Memory;

class PPU {
private:
    Memory& memory;
    int scanline_counter = 0;
    bool stat_line = false; // previous STAT interrupt line state (for 0->1 edge detection)

    std::vector<uint8_t> vram; // 0x8000 - 0x9FFF (8KB)
    std::vector<uint8_t> oam;  // 0xFE00 - 0xFE9F (160 bytes)

    uint8_t window_line = 0;   // internal window line counter
    int bg_index[160] = {0};   // raw BG/window colour index of current line (for OBJ priority)

    void updateSTAT();
    int tilePixel(uint16_t tileMap, uint8_t x, uint8_t y);
    void renderWindow();
    void renderSprites();

public:
    uint32_t framebuffer[160 * 144] = {0}; 

    PPU(Memory& mem);

    // Methods to read/write VRAM and OAM
    uint8_t readVRAM(uint16_t addr);
    void writeVRAM(uint16_t addr, uint8_t value);
    uint8_t readOAM(uint16_t addr);
    void writeOAM(uint16_t addr, uint8_t value);

    // Method to render the current scanline
    void renderScanline();
    void renderBackground();
    uint32_t getColor(uint8_t color_id, uint16_t palette_addr);

    // PPU Core
    void tick(int cycles);
};