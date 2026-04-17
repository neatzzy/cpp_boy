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
        ly++;
        memory.write_direct(0xFF44, ly);

        // If we just entered VBlank (LY == 144), request a VBlank interrupt
        if (ly == 144) {
            memory.requestInterrupt(0); // Interrupt Bit 0 is VBlank
            frame_ready = true; // Signal that a new frame is ready to be rendered
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
    
    // Set LY=LYC flag (Bit 2) and request STAT interrupt if enabled
    uint8_t lyc = memory.read_direct(0xFF45);
    if (ly == lyc) {
        stat |= 0b00000100; // Sets LY=LYC flag
        // Throw STAT interrupt if enabled
        if (stat & 0b01000000) memory.requestInterrupt(1); 
    } else {
        stat &= ~0b00000100; // Clears LY=LYC flag
    }

    // Determine the current mode based on LY and scanline_counter
    if (ly >= 144) {
        // VBlank period
        stat = (stat & 0xFC) | 1;
    } else {
        // Visible scanlines
        if (scanline_counter < 80) {
            stat = (stat & 0xFC) | 2; // Mode 2 (OAM Search)
        } else if (scanline_counter < 80 + 172) {
            stat = (stat & 0xFC) | 3; // Mode 3 (Pixel Transfer)
        } else {
            stat = (stat & 0xFC) | 0; // Mode 0 (HBlank)
        }
    }

    memory.write_direct(0xFF41, stat);
}

// Implementations for reading/writing PPU registers
uint8_t PPU::readVRAM(uint16_t addr) { return vram[addr - 0x8000]; }
void PPU::writeVRAM(uint16_t addr, uint8_t value) { vram[addr - 0x8000] = value; }
uint8_t PPU::readOAM(uint16_t addr) { return oam[addr - 0xFE00]; }
void PPU::writeOAM(uint16_t addr, uint8_t value) { oam[addr - 0xFE00] = value; }