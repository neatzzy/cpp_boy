#include "cpu.h"
#include "memory.h"

#include <iostream>

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Uso: ./emulator rom.gb\n";
        return 1;
    }

    // =========================
    // Criar componentes
    // =========================
    Memory memory;
    CPU cpu(memory);

    // =========================
    // Carregar BIOS e ROM
    // =========================
    memory.loadBIOS("gb_dmg0_rom.bin");
    memory.loadROM(argv[1]);

    // =========================
    // Loop principal
    // =========================
    bool running = true;

    while (running) {

        // Debug (opcional)
        std::printf("instruction at PC: %02X\n", memory.read(cpu.PC));
        std::printf(
            "PC: %04X  A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X\n",
            cpu.PC, cpu.A, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L
        );

        cpu.step();

        // Segurança para não travar infinito
        if (cpu.PC == 0x0000) {
            std::cout << "Loop detectado, parando.\n";
            break;
        }
    }

    return 0;
}