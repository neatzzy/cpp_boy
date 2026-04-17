#include "memory.h"
#include "cartridge.h"
#include "ppu.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

void Memory::loadBIOS(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open BIOS file: " + path);
    }
    file.read((char*)ram.data(), 256);
}

uint8_t Memory::read(uint16_t addr) {
    // If the address is in the cartridge ROM area, delegate to the cartridge
    if(addr < 0x8000){
        if (cart) return cart->read(addr); 
        return 0xFF; // If no cartridge is loaded, return 0xFF for unmapped reads
    }

    return ram[addr];
}

void Memory::write(uint16_t addr, uint8_t value) {
    if(addr == 0xFF04){
        ram[0xFF04] = 0; // Writing to DIV resets it to 0
        return;
    }

    // DEBUG
    if (addr == 0xFF02 && value == 0x81) {
        char c = (char)ram[0xFF01];   // Read the character from the SB register
        std::cout << c << std::flush; // Print immediately to the terminal
        // We don't return here. We let the write of 0x81 flow to RAM
        // because some tests read the value back to know the state.
    }

    if(addr < 0x8000){
        // Delegate writes to the cartridge if the address is in the ROM area. Most cartridges won't actually write to this area, but some might have special behavior (e.g., for bank switching).
        if (cart) cart->write(addr, value);
        return;
    }

    if (addr >= 0x8000 && addr <= 0x9FFF) { ppu->writeVRAM(addr, value); return; }
    if (addr >= 0xFE00 && addr <= 0xFE9F) { ppu->writeOAM(addr, value); return; }

    ram[addr] = value;
}

uint16_t Memory::read16(uint16_t addr) {
    return (read(addr + 1) << 8) | read(addr);
}

void Memory::write16(uint16_t addr, uint16_t value) {
    write(addr, value & 0xFF);
    write(addr + 1, (value >> 8) & 0xFF);
}

void Memory::push(uint16_t value, uint16_t& SP) {
    SP -= 2;
    write16(SP, value);
}

uint16_t Memory::pop(uint16_t& SP) {
    uint16_t value = read16(SP);
    SP += 2;
    return value;
}

void Memory::requestInterrupt(uint8_t interrupt_bit) {
    uint8_t req = read(0xFF0F);
    req |= (1 << interrupt_bit);
    write_direct(0xFF0F, req);
}

uint8_t Memory::read_direct(uint16_t addr) {
    return ram[addr];
}