#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Cartridge {
public:
    std::vector<uint8_t> rom;

    void load(const std::string& filename);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
};