#pragma once

#include <cstdint>
#include <vector>
#include <string>

class Cartridge {
public:
    std::vector<uint8_t> rom;

    void load(const std::string& path);
};