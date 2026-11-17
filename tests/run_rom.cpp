// Headless test runner for Blargg and mooneye-gb ROMs.
// Blargg tests print results as ASCII tiles on the background map, so the
// verdict is read straight from VRAM: "Passed" -> exit 0, "Failed" -> exit 1,
// no verdict within the frame limit -> exit 2.
// Mooneye tests execute LD B,B (0x40) when done: B,C,D,E,H,L = 3,5,8,13,21,34
// means pass, all 0x42 means fail.
#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "input.h"
#include "ppu.h"
#include "apu.h"
#include "cartridge.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::string screenText(PPU& ppu) {
    std::string text;
    for (int y = 0; y < 18; y++) {
        for (int x = 0; x < 20; x++) {
            uint8_t tile = ppu.readVRAM(0x9800 + y * 32 + x);
            text += (tile >= 32 && tile < 127) ? (char)tile : ' ';
        }
        text += '\n';
    }
    return text;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Use: ./run_rom <rom.gb> [max_frames]\n");
        return 2;
    }
    int maxFrames = argc > 2 ? std::atoi(argv[2]) : 6000; // combined cpu_instrs needs ~3300

    Cartridge cart;
    cart.load(argv[1]);
    Memory memory;
    memory.cart = &cart;
    PPU ppu(memory);
    Timer timer(memory);
    Joypad joypad(memory);
    APU apu;
    CPU cpu(memory);
    memory.ppu = &ppu;
    memory.timer = &timer;
    memory.joypad = &joypad;
    memory.apu = &apu;

    std::string text;
    try {
        for (int frame = 0; frame < maxFrames; frame++) {
            int cycles = 0;
            while (cycles < 70224) {
                if (!cpu.halted && memory.read(cpu.PC) == 0x40) {
                    uint8_t r[6] = {cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L};
                    static const uint8_t fib[6] = {3, 5, 8, 13, 21, 34};
                    if (std::equal(r, r + 6, fib)) return 0;
                    if (std::all_of(r, r + 6, [](uint8_t v) { return v == 0x42; })) {
                        std::printf("mooneye: Failed\n");
                        return 1;
                    }
                }
                cycles += cpu.step(); // step() ticks timer/PPU/APU itself
            }
            apu.samples.clear();
            text = screenText(ppu);
            if (text.find("Passed") != std::string::npos) return 0;
            if (text.find("Failed") != std::string::npos) break;
        }
    } catch (const std::string& e) {
        text += "[CRASH] " + e + "\n";
    } catch (const char* e) {
        text += std::string("[CRASH] ") + e + "\n";
    }

    std::printf("%s", text.c_str());
    return text.find("Failed") != std::string::npos ? 1 : 2;
}
