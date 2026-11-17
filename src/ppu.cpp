#include "ppu.h"
#include "memory.h"

PPU::PPU(Memory& mem) : memory(mem), vram(0x2000, 0), oam(160, 0) {
    for (int i = 0; i < 160 * 144; i++) {
        framebuffer[i] = 0xFF000000;
    }
}

void PPU::tick(int cycles) {
    uint8_t lcdc = memory.read_direct(0xFF40);
    bool lcd_enable = (lcdc & 0x80) != 0; // LCD Control Bit 7 is the LCD Enable flag

    if (!lcd_enable) {
        // If LCD is disabled, reset the scanline counter and LY register, and set mode to HBlank (0)
        scanline_counter = 0;
        window_line = 0;
        stat_line = false;
        memory.write_direct(0xFF44, 0); // LY = 0
        uint8_t stat = memory.read_direct(0xFF41);
        memory.write_direct(0xFF41, (stat & 0xFC) | 0x00); // Modo 0
        return;
    }

    scanline_counter += cycles;

    // PPU takes 456 cycles to draw a scanline.
    if (scanline_counter >= 456) {
        scanline_counter -= 456;

        uint8_t ly = memory.read_direct(0xFF44);

        if (ly < 144) {
            renderScanline(); // Desenha a linha ANTES de a mudar
        }

        ly++;
        memory.write_direct(0xFF44, ly);
        if (ly == 144) window_line = 0; // new frame starts after VBlank; reset here, before next render

        // If we just entered VBlank (LY == 144), request a VBlank interrupt
        if (ly == 144) {
            memory.requestInterrupt(0); // Interrupt Bit 0 is VBlank
        }

        // After LY reaches 153, it wraps back to 0
        if (ly > 153) {
            memory.write_direct(0xFF44, 0);
        }
    }

    updateSTAT(); // Update the STAT register based on the current scanline and mode
}

void PPU::updateSTAT() {
    uint8_t stat = memory.read_direct(0xFF41);
    uint8_t ly = memory.read_direct(0xFF44);
    
    // Determine the current mode based on LY and scanline_counter
    uint8_t mode;
    if (ly >= 144) {
        mode = 1; // VBlank period
    } else if (scanline_counter < 80) {
        mode = 2; // OAM Search
    } else if (scanline_counter < 80 + 172) {
        mode = 3; // Pixel Transfer
    } else {
        mode = 0; // HBlank
    }
    stat = (stat & 0xFC) | mode;

    // LY=LYC flag (Bit 2)
    bool coincidence = ly == memory.read_direct(0xFF45);
    if (coincidence) stat |= 0b00000100;
    else stat &= ~0b00000100;

    memory.write_direct(0xFF41, stat);

    // STAT interrupt line: OR of all enabled sources, requested on a 0->1 edge only
    bool line = (coincidence && (stat & 0b01000000)) ||
                (mode == 0 && (stat & 0b00001000)) ||
                (mode == 1 && (stat & 0b00010000)) ||
                (mode == 2 && (stat & 0b00100000));
    if (line && !stat_line) memory.requestInterrupt(1);
    stat_line = line;
}

// Implementations for reading/writing PPU registers
uint8_t PPU::readVRAM(uint16_t addr) { return vram[addr - 0x8000]; }
void PPU::writeVRAM(uint16_t addr, uint8_t value) { vram[addr - 0x8000] = value; }
uint8_t PPU::readOAM(uint16_t addr) { return oam[addr - 0xFE00]; }
void PPU::writeOAM(uint16_t addr, uint8_t value) { oam[addr - 0xFE00] = value; }

// Implementations for reading/writing PPU registers
uint32_t PPU::getColor(uint8_t color_id, uint16_t palette_addr) {
    uint8_t palette = memory.read_direct(palette_addr);
    
    int shift = color_id * 2;
    uint8_t color = (palette >> shift) & 0b11;

    switch(color) {
        case 0: return 0xFFFFFFFF; // White
        case 1: return 0xFFAAAAAA; // Light Gray
        case 2: return 0xFF555555; // Dark Gray
        case 3: return 0xFF000000; // Black
    }
    return 0xFF000000;
}

// Colour index (0-3) of the pixel at (x, y) of a 256x256 tile map.
int PPU::tilePixel(uint16_t tileMap, uint8_t x, uint8_t y) {
    uint8_t lcdc = memory.read_direct(0xFF40);
    bool unsig = (lcdc & 0b00010000) != 0;
    uint16_t tileData = unsig ? 0x8000 : 0x8800;

    uint16_t tileAddress = tileMap + (y / 8) * 32 + (x / 8);
    int16_t tileNum = 0;
    if (tileAddress >= 0x8000 && tileAddress <= 0x9FFF) {
        uint8_t raw = readVRAM(tileAddress);
        tileNum = unsig ? raw : (int8_t)raw;
    }

    uint16_t tileLocation = tileData + (unsig ? tileNum : tileNum + 128) * 16;
    uint8_t line = (y % 8) * 2;

    uint8_t data1 = 0, data2 = 0;
    if (tileLocation >= 0x8000 && tileLocation + line + 1 <= 0x9FFF) {
        data1 = readVRAM(tileLocation + line);
        data2 = readVRAM(tileLocation + line + 1);
    }
    int bit = 7 - (x % 8);
    return (((data2 >> bit) & 1) << 1) | ((data1 >> bit) & 1);
}

void PPU::renderScanline() {
    uint8_t lcdc = memory.read_direct(0xFF40);
    uint8_t ly = memory.read_direct(0xFF44);
    if (ly >= 144) return;

    renderBackground(); // also fills bg_index (all 0 when BG is off)
    if (lcdc & 0b00000001) renderWindow();
    if (lcdc & 0b00000010) renderSprites();
}

void PPU::renderBackground() {
    uint8_t lcdc = memory.read_direct(0xFF40);
    uint8_t scrollY = memory.read_direct(0xFF42);
    uint8_t scrollX = memory.read_direct(0xFF43);
    uint8_t ly = memory.read_direct(0xFF44);
    uint16_t tileMap = (lcdc & 0b00001000) ? 0x9C00 : 0x9800;
    bool bgOn = lcdc & 0b00000001;

    for (int pixel = 0; pixel < 160; pixel++) {
        int c = bgOn ? tilePixel(tileMap, (uint8_t)(pixel + scrollX), (uint8_t)(scrollY + ly)) : 0;
        bg_index[pixel] = c;
        framebuffer[ly * 160 + pixel] = getColor(c, 0xFF47);
    }
}

void PPU::renderWindow() {
    uint8_t lcdc = memory.read_direct(0xFF40);
    uint8_t ly = memory.read_direct(0xFF44);
    int wx = memory.read_direct(0xFF4B) - 7;
    if (!(lcdc & 0b00100000) || ly < memory.read_direct(0xFF4A) || wx >= 160) return;

    uint16_t tileMap = (lcdc & 0b01000000) ? 0x9C00 : 0x9800;
    for (int pixel = wx < 0 ? 0 : wx; pixel < 160; pixel++) {
        int c = tilePixel(tileMap, (uint8_t)(pixel - wx), window_line);
        bg_index[pixel] = c;
        framebuffer[ly * 160 + pixel] = getColor(c, 0xFF47);
    }
    window_line++;
}

void PPU::renderSprites() {
    uint8_t lcdc = memory.read_direct(0xFF40);
    int ly = memory.read_direct(0xFF44);
    int height = (lcdc & 0b00000100) ? 16 : 8;

    // First 10 sprites in OAM order that cover this line.
    int hit[10], n = 0;
    for (int i = 0; i < 40 && n < 10; i++) {
        int top = oam[i * 4] - 16;
        if (ly >= top && ly < top + height) hit[n++] = i;
    }

    // Drawn pixel owner per x: lower X wins, ties by lower OAM index.
    int owner_x[160];
    for (int& v : owner_x) v = 1000;
    for (int k = 0; k < n; k++) {
        int i = hit[k];
        int sx = oam[i * 4 + 1] - 8;
        uint8_t tile = oam[i * 4 + 2], attr = oam[i * 4 + 3];
        int row = ly - (oam[i * 4] - 16);
        if (attr & 0x40) row = height - 1 - row;
        if (height == 16) tile = (tile & 0xFE) | (row >= 8);
        uint16_t addr = 0x8000 + tile * 16 + (row % 8) * 2;
        uint8_t lo = readVRAM(addr), hi = readVRAM(addr + 1);

        for (int px = 0; px < 8; px++) {
            int x = sx + px;
            if (x < 0 || x >= 160 || sx >= owner_x[x]) continue;
            int bit = (attr & 0x20) ? px : 7 - px;
            int c = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
            if (c == 0) continue;
            owner_x[x] = sx;
            if ((attr & 0x80) && bg_index[x] != 0) continue;
            framebuffer[ly * 160 + x] = getColor(c, (attr & 0x10) ? 0xFF49 : 0xFF48);
        }
    }
}
