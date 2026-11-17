#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "input.h"
#include "ppu.h"
#include "apu.h"
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
        Joypad joypad(memory);
        APU apu;
        CPU cpu(memory);

        memory.ppu = &ppu;
        memory.timer = &timer;
        memory.joypad = &joypad;
        memory.apu = &apu;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            std::cerr << "Error initializing SDL: " << SDL_GetError() << "\n";
            return 1;
        }

        SDL_Window* window = SDL_CreateWindow("CPP Boy", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            160 * 4, 144 * 4, SDL_WINDOW_SHOWN);

        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_Texture* texture = SDL_CreateTexture(renderer, 
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);       
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_AudioSpec want{}, have{};
        want.freq = 44100;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 1024;
        SDL_AudioDeviceID audio = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (audio) SDL_PauseAudioDevice(audio, 0);
        else std::cerr << "Warning: no audio device (" << SDL_GetError() << "), running silent\n";
        // Cap the queue at ~3 frames so audio latency can't drift; vsync paces the loop.
        const Uint32 MAX_QUEUED_BYTES = 3 * 738 * 2 * sizeof(float);

        SDL_Event event;
        bool running = true;

        std::cout << "Emulator started! Press ESC to exit.\n";

        const int MAX_CYCLES_PER_FRAME = 70224;

        while (running) {
            int cycles_this_frame = 0;

            while (cycles_this_frame < MAX_CYCLES_PER_FRAME) {
                cycles_this_frame += cpu.step(); // step() ticks timer/PPU/APU itself
            }

            if (audio && SDL_GetQueuedAudioSize(audio) < MAX_QUEUED_BYTES)
                SDL_QueueAudio(audio, apu.samples.data(), apu.samples.size() * sizeof(float));
            apu.samples.clear();

            SDL_UpdateTexture(texture, nullptr, ppu.framebuffer, 160 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } 
                else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                    bool down = event.type == SDL_KEYDOWN;
                    SDL_Keycode key = event.key.keysym.sym;
                    if (down && key == SDLK_ESCAPE) running = false;
                    switch (key) {
                        case SDLK_RIGHT:     joypad.setButton(Button::Right, down); break;
                        case SDLK_LEFT:      joypad.setButton(Button::Left, down); break;
                        case SDLK_UP:        joypad.setButton(Button::Up, down); break;
                        case SDLK_DOWN:      joypad.setButton(Button::Down, down); break;
                        case SDLK_z:         joypad.setButton(Button::A, down); break;
                        case SDLK_x:         joypad.setButton(Button::B, down); break;
                        case SDLK_RETURN:    joypad.setButton(Button::Start, down); break;
                        case SDLK_RSHIFT:
                        case SDLK_BACKSPACE: joypad.setButton(Button::Select, down); break;
                        default: break;
                    }
                    if (down && key != SDLK_ESCAPE) cpu.stopped = false; // any key wakes STOP
                }
            }
        }
        cart.save();
        if (audio) SDL_CloseAudioDevice(audio);
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