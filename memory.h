#pragma once

#include <cstdint>
#include <string>
#include "cartridge.h"

class Memory {
public:
    uint8_t ram[65536];

    Cartridge* cart;

    void loadROM(const std::string& path);
    void loadBIOS(const std::string& path);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    void push(uint16_t value, uint16_t& SP);
    uint16_t pop(uint16_t& SP);
};
