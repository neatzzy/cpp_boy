#include "cartridge.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

void Cartridge::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Falha critica: Cartucho nao encontrado! " + filename);
    }
    loadData(std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {}));

    if (!battery || ram.empty()) return;
    savePath = std::filesystem::path(filename).replace_extension(".sav").string();
    std::ifstream sav(savePath, std::ios::binary);
    std::vector<uint8_t> data(std::istreambuf_iterator<char>(sav), {});
    if (data.size() == ram.size()) ram = std::move(data);
}

void Cartridge::loadData(std::vector<uint8_t> data) {
    rom = std::move(data);
    uint8_t type = rom.size() > 0x149 ? rom[0x147] : 0;
    uint8_t ramCode = rom.size() > 0x149 ? rom[0x149] : 0;

    if (type >= 0x01 && type <= 0x03) mbc = Mbc::Mbc1;
    else if (type >= 0x19 && type <= 0x1E) mbc = Mbc::Mbc5;
    else if (type != 0x00)
        std::cerr << "Warning: unsupported cartridge type 0x" << std::hex << (int)type
                  << std::dec << ", running as ROM-only\n";
    battery = type == 0x03 || type == 0x1B || type == 0x1E;
    rumble = type >= 0x1C && type <= 0x1E;

    static const size_t ramSizes[] = {0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000};
    ram.assign(ramCode < 6 ? ramSizes[ramCode] : 0, 0);
}

void Cartridge::save() {
    if (!battery || ram.empty() || savePath.empty()) return;
    std::ofstream(savePath, std::ios::binary).write(reinterpret_cast<const char*>(ram.data()), ram.size());
}

uint8_t Cartridge::romAt(unsigned bank, uint16_t addr) const {
    size_t banks = rom.size() / 0x4000;
    if (banks) bank %= banks;
    size_t i = bank * 0x4000 + (addr & 0x3FFF);
    return i < rom.size() ? rom[i] : 0xFF;
}

uint8_t Cartridge::read(uint16_t addr) {
    if (addr < 0x4000) return romAt(mbc == Mbc::Mbc1 && mode ? upper << 5 : 0, addr);
    if (addr < 0x8000) return romAt(mbc == Mbc::Mbc1 ? (upper << 5) | romBank : romBank, addr);
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!ramEnabled || ram.empty()) return 0xFF;
        unsigned bank = mbc == Mbc::Mbc5 || mode ? upper : 0;
        return ram[(bank * 0x2000 + (addr - 0xA000)) % ram.size()];
    }
    return 0xFF;
}

void Cartridge::write(uint16_t addr, uint8_t value) {
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!ramEnabled || ram.empty()) return;
        unsigned bank = mbc == Mbc::Mbc5 || mode ? upper : 0;
        ram[(bank * 0x2000 + (addr - 0xA000)) % ram.size()] = value;
        return;
    }
    if (addr >= 0x8000) return;
    if (addr < 0x2000) { ramEnabled = (value & 0x0F) == 0x0A; return; }

    if (mbc == Mbc::Mbc1) {
        if (addr < 0x4000) romBank = (value & 0x1F) ? (value & 0x1F) : 1;
        else if (addr < 0x6000) upper = value & 0x03;
        else mode = value & 0x01;
    } else if (mbc == Mbc::Mbc5) {
        if (addr < 0x3000) romBank = (romBank & 0x100) | value;
        else if (addr < 0x4000) romBank = (romBank & 0xFF) | ((value & 0x01) << 8);
        else if (addr < 0x6000) upper = value & (rumble ? 0x07 : 0x0F);
    }
}
