#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Use: ./emulator <rom.gb>\n";
        return 1;
    }

    bool running = true;

    try {
        Cartridge cart;
        cart.load(argv[1]);

        Memory memory;
        memory.cart = &cart;
        memory.loadBIOS("gb_bios.bin");

        CPU cpu(memory);

        while (running) {
            // Debug (optional)
            std::printf("instruction at PC: %02X\n", memory.read(cpu.PC));
            std::printf(
                "PC: %04X  A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X\n",
                cpu.PC, cpu.A, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L
            );

            cpu.step();

            // Stops the emulator if it detects a loop at the start of the program (common in some test ROMs)
            if (cpu.PC == 0x0000) {
                std::cout << "Loop detected, stopping.\n";
                break;
            }
        }
    } catch (const std::string& e) {
        std::cerr << "\n[CRASH] Error in CPU: " << e << "\n";
    } catch (const char* e) {
        std::cerr << "\n[CRASH] Fatal Error: " << e << "\n"; 
    } catch (const std::exception& e) {
        std::cerr << "\n[CRASH] Standard Exception: " << e.what() << "\n";
    }

    return 0;
}