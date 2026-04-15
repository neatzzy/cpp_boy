#include "cartridge.h"
#include <fstream>
#include <stdexcept>

void Cartridge::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open ROM file: " + path);
    }
    rom.clear();
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    rom.resize(size);
    file.read(reinterpret_cast<char*>(rom.data()), size);
}