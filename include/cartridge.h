#pragma once
#include <string>
#include <vector>
#include <cstdint>

// ROM-only, MBC1 and MBC5 cartridges with optional battery-backed RAM.
class Cartridge {
public:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;   // external RAM (0xA000-0xBFFF)

    void load(const std::string& filename);   // also loads <rom>.sav if battery
    void loadData(std::vector<uint8_t> data); // parses header, no file I/O
    void save();                              // writes <rom>.sav (battery carts only)
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);

private:
    enum class Mbc { None, Mbc1, Mbc5 };
    Mbc mbc = Mbc::None;
    bool battery = false;
    bool rumble = false;
    bool ramEnabled = false;
    bool mode = false;        // MBC1 banking mode
    uint16_t romBank = 1;     // MBC1: 5 bits, MBC5: 9 bits
    uint8_t upper = 0;        // MBC1: 2-bit upper/RAM bank, MBC5: RAM bank
    std::string savePath;

    uint8_t romAt(unsigned bank, uint16_t addr) const;
};
