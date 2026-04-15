#include "memory.h"
#include <fstream>
#include <stdexcept>

void Memory::loadROM(const std::string& path) {
    std::ifstream file("/roms/" + path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open ROM file: " + path);
    }
    file.read((char*)ram, 0x8000);
}

void Memory::loadBIOS(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open BIOS file: " + path);
    }
    file.read((char*)ram, 256);
}

uint8_t Memory::read(uint16_t addr) {
    if(addr < 0x8000){
        return cart->rom[addr];
    }
    return ram[addr];
}

void Memory::write(uint16_t addr, uint8_t value) {
    ram[addr] = value;
}

uint16_t Memory::read16(uint16_t addr) {
    return (ram[addr + 1] << 8) | ram[addr];
}

void Memory::write16(uint16_t addr, uint16_t value) {
    ram[addr] = value & 0xFF;
    ram[addr + 1] = (value >> 8) & 0xFF;
}

void Memory::push(uint16_t value, uint16_t& SP) {
    // Push value onto stack (decrement SP and write value)
    SP -= 2;
    write16(SP, value);
}

uint16_t Memory::pop(uint16_t& SP) {
    // Pop value from stack (read value and increment SP)
    uint16_t value = read16(SP);
    SP += 2;
    return value;
}