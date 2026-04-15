#include "cartridge.h"
#include <fstream>
#include <stdexcept>

void Cartridge::load(const std::string& filename) {
    // Open the file in binary mode and move the cursor to the end to get the size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Falha critica: Cartucho nao encontrado! " + filename);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg); // Returns the cursor to the beginning of the file

    rom.resize(size); // Resize the ROM vector to fit the entire file
    file.read(reinterpret_cast<char*>(rom.data()), size);
}

uint8_t Cartridge::read(uint16_t addr) {
    if (addr < rom.size()) {
        return rom[addr];
    }
    return 0xFF; // Standard return value for unmapped memory reads, can be adjusted based on specific cartridge behavior
}

void Cartridge::write(uint16_t addr, uint8_t value) {
    // Most cartridges are read-only, but some may have writable areas (e.g., for save data).
    // This function can be expanded to handle specific cartridge types that support writing.
}