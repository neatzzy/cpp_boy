#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "ppu.h"
#include "cartridge.h"
#include <iostream>
#include <SDL2/SDL.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Use: ./emulator <rom.gb>\n";
        return 1;
    }

    try {
        Cartridge cart;
        cart.load(argv[1]);

        Memory memory;
        memory.cart = &cart; 

        PPU ppu(memory);
        Timer timer(memory);
        CPU cpu(memory);

        memory.ppu = &ppu;

        memory.loadBIOS("gb_bios.bin");

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "Error initializing SDL: " << SDL_GetError() << "\n";
            return 1;
        }

        SDL_Window* window = SDL_CreateWindow("CPP Boy", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            160 * 4, 144 * 4, SDL_WINDOW_SHOWN);

        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        SDL_Texture* texture = SDL_CreateTexture(renderer, 
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);       
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_Event event;
        bool running = true;

        std::cout << "Emulator started! Press ESC to exit.\n";

        const int MAX_CYCLES_PER_FRAME = 70224;

        while (running) {
            int cycles_this_frame = 0;

            while (cycles_this_frame < MAX_CYCLES_PER_FRAME) {
                int cycles = cpu.step();
                timer.tick(cycles);
                ppu.tick(cycles);
                
                cycles_this_frame += cycles;
            }

            printf("Instruction: %02X | A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: %04X\n", 
                memory.read(cpu.PC - 1), cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP, cpu.PC);

            ppu.frame_ready = false; 

            SDL_UpdateTexture(texture, nullptr, ppu.framebuffer, 160 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } 
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                }
            }
        }
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    } catch (const std::string& e) {
        std::cerr << "\n[CRASH] Error in CPU: " << e << "\n";
    } catch (const char* e) {
        std::cerr << "\n[CRASH] Fatal Error: " << e << "\n"; 
    } catch (const std::exception& e) {
        std::cerr << "\n[CRASH] Standard Exception: " << e.what() << "\n";
    }


    return 0;
}